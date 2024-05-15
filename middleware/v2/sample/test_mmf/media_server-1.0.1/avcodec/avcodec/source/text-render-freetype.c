#include "text-render.h"
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if defined(_AVCODEC_FREETYPE_)
#include <ft2build.h> 
#include FT_FREETYPE_H
#include FT_GLYPH_H
//#include FT_STROKER_H

// https://stuff.mit.edu/afs/athena/astaff/source/src-9.0/third/freetype/docs/tutorial/step1.html
// http://freetype.sourceforge.net/freetype2/docs/tutorial/step1.html

#define BYTES 4
#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) > (b) ? (a) : (b))
#define ALIGN(a, n) (((a) + (n) - 1) / (n) * (n))

struct ft2_context_t
{
	FT_Library  library;
	FT_Face     face;

	struct
	{
		unsigned char r;
		unsigned char g;
		unsigned char b;
	} color;

	int pitch;
	int width;
	int height;
	void *bitmap;
};

struct text_glyph_t
{
	FT_Glyph glyph;
	FT_BBox bbox;
	int x;
};

static int free_type_init(struct ft2_context_t* s)
{
	int r;
	r = FT_Init_FreeType(&s->library);
	if (0 != r)
	{
		printf("%s Could not load FreeType: %d\n", __FUNCTION__, r);
		return r;
	}
	return r;
}

static int text_render_config(void* p, const struct text_parameter_t* param)
{
	int r;
	struct ft2_context_t* render = (struct ft2_context_t*)p;
	if (param->font && *param->font)
	{
		FT_Face face = NULL;
		r = FT_New_Face(render->library, param->font, 0, &face);
		if (0 != r)
		{
			// FT_Err_Unknown_File_Format
			printf("%s Could not load font \"%s\": %d\n", __FUNCTION__, param->font, r);
			return r;
		}

		if (render->face)
			FT_Done_Face(render->face);
		render->face = face;
	}

	if (!render->face)
		return -1;

	r = FT_Set_Pixel_Sizes(render->face, 0, param->size);
	if (0 != r)
	{
		printf("%s Could not set font size to %d pixels: %d\n", __FUNCTION__, param->size, r);
		return r;
	}

	render->color.r = 0xFF & (param->color >> 16);
	render->color.g = 0xFF & (param->color >> 8);
	render->color.b = 0xFF & (param->color >> 0);
	return 0;
}

static void text_render_destroy(void* p)
{
	struct ft2_context_t* render = (struct ft2_context_t*)p;
	if (render->face)
	{
		FT_Done_Face(render->face);
		render->face = NULL;
	}

	if (render->library)
	{
		FT_Done_FreeType(render->library);
		render->library = NULL;
	}

	if (render->bitmap)
	{
		free(render->bitmap);
		render->bitmap = NULL;
	}

	free(render);
}

static void* text_render_create(const struct text_parameter_t* param)
{
	struct ft2_context_t* render;
	render = malloc(sizeof(struct ft2_context_t));
	if (NULL == render)
		return NULL;

	memset(render, 0, sizeof(struct ft2_context_t));
	if (0 != free_type_init(render))
	{
		text_render_destroy(render);
		return NULL;
	}

	if (0 != text_render_config(render, param))
	{
		text_render_destroy(render);
		return NULL;
	}
	return render;
}

static int text_render_bitmap(struct ft2_context_t* render, const FT_BitmapGlyph bmpGlyph, const struct text_glyph_t* txt, FT_BBox *bbox)
{
	int r, c;
	//FT_Bitmap bitmap = render->face->glyph->bitmap;
	FT_Bitmap* bitmap = &bmpGlyph->bitmap;
	unsigned char* s;
	unsigned char* d;

	for (r = 0; r < bitmap->rows; r++)
	{
		s = (unsigned char*)bitmap->buffer + r * bitmap->pitch;
		d = (unsigned char*)render->bitmap + ((bbox->yMax - txt->bbox.yMax + r) * render->pitch + txt->x + txt->bbox.xMin) * BYTES;
		for (c = 0; c < bitmap->width; c++, s++)
		{
			assert(4 == BYTES);
			*d++ = (*s * render->color.r) / 255;	// r
			*d++ = (*s * render->color.g) / 255;	// g
			*d++ = (*s * render->color.b) / 255;	// b
			*d++ = *s;	// a
		}
	}

	return 0;
}

static const void* text_render_draw(void* p, const wchar_t* txt, int *w, int *h, int* pitch)
{
	FT_UInt glyph_index;
	FT_Bool use_kerning;
	FT_UInt previous;
	FT_BBox bbox;
	struct text_glyph_t *vec;
	//FT_BitmapGlyph glyph_bitmap;
	size_t i, j, count;

	int r;
	struct ft2_context_t* render = (struct ft2_context_t*)p;

	count = txt ? wcslen(txt) : 0;
	if (0 == txt) return NULL;
	vec = (struct text_glyph_t*)malloc(sizeof(struct text_glyph_t) * count);
	if (NULL == vec) return NULL;
	memset(vec, 0, sizeof(struct text_glyph_t) * count);
	memset(&bbox, 0, sizeof(bbox));

	use_kerning = FT_HAS_KERNING(render->face);
	previous = 0;
	render->width = 0;
	for (i = 0, j = 0; i < count; i++)
	{
		/* convert character code to glyph index */
		glyph_index = FT_Get_Char_Index(render->face, txt[i]);

		/* retrieve kerning distance and move pen position */
		if (use_kerning && previous && glyph_index)
		{
			FT_Vector delta;
			FT_Get_Kerning(render->face, previous, glyph_index, FT_KERNING_DEFAULT, &delta);
			vec[j].x = (int)(delta.x >> 6); // 1/64
		}

		/* load glyph image into the slot (erase previous one) */
		r = FT_Load_Glyph(render->face, glyph_index, FT_LOAD_DEFAULT);
		if (r)
		{
			printf("%s load char(%x) => %d\n", __FUNCTION__, (unsigned int)txt[i], r);
			continue; /* ignore errors */
		}

		/* load glyph into s->face->glyph */
		// FT_Load_Char(FT_LOAD_RENDER) == FT_Get_Char_Index + FT_Load_Glyph(FT_LOAD_DEFAULT) + FT_Render_Glyph(FT_RENDER_MODE_NORMAL)
		//r = FT_Load_Char(render->face, *s, FT_LOAD_RENDER);
		//if (0 != r)
		//{
		//	printf("%s load char(%x) => %d\n", __FUNCTION__, (unsigned int)*s, r);
		//	return r;
		//}

		r = FT_Get_Glyph(render->face->glyph, &vec[j].glyph);
		if (r)
		{
			printf("%s FT_Get_Glyph(%x) => %d\n", __FUNCTION__, (unsigned int)txt[i], r);
			continue; /* ignore errors */
		}

		FT_Glyph_Get_CBox(vec[j].glyph, ft_glyph_bbox_pixels, &vec[j].bbox);
		bbox.yMin = MIN(bbox.yMin, vec[j].bbox.yMin);
		bbox.yMax = MAX(bbox.yMax, vec[j].bbox.yMax);

		/* increment pen position */
		vec[j].x += render->width;
		render->width = (int)(vec[j].x + (render->face->glyph->advance.x >> 6)); // 1/64
		++j;

		/* record current glyph index */
		previous = glyph_index;
	}

	render->height = (int)(bbox.yMax - bbox.yMin);
	render->height = ALIGN(render->height, 2);
	render->pitch = ALIGN(render->width, 4);
	render->bitmap = malloc(render->pitch * render->height * BYTES);
	memset(render->bitmap, 0, render->pitch * render->height * BYTES);

	for (i = 0; i < j; i++)
	{
		if (render->bitmap)
		{
			if (vec[i].glyph->format != FT_GLYPH_FORMAT_BITMAP)
			{
				r = FT_Glyph_To_Bitmap(&vec[i].glyph, FT_RENDER_MODE_NORMAL, NULL, 0);
				if (r)
				{
					printf("%s FT_Glyph_To_Bitmap(%x) => %d\n", __FUNCTION__, (unsigned int)txt[i], r);
					continue; /* ignore errors */
				}
			}

			//glyph_bitmap = (FT_BitmapGlyph)vec[i].glyph;
			text_render_bitmap(render, (FT_BitmapGlyph)vec[i].glyph, &vec[i], &bbox);

			//printf("x/y: %d/%d, box: %d/%d/%d/%d, left/top: %d/%d, bitmap row/width/pitch: %d/%d/%d\n",
			//	pen_x, pen_y, 
			//	bbox.xMin, bbox.yMin, bbox.xMax, bbox.yMax,
			//	glyph_bitmap->left, glyph_bitmap->top,
			//	glyph_bitmap->bitmap.rows, glyph_bitmap->bitmap.width, glyph_bitmap->bitmap.pitch);
		}

		FT_Done_Glyph(vec[i].glyph);
	}

	free(vec);
	*w = render->pitch;
	*h = render->height;
	*pitch = render->pitch * BYTES;
	return render->bitmap;
}

struct text_render_t* text_render_freetype(void)
{
	static struct text_render_t render = {
		text_render_create,
		text_render_destroy,
		text_render_draw,
		text_render_config,
	};
	return &render;
}

#endif /* _AVCODEC_FREETYPE_ */
