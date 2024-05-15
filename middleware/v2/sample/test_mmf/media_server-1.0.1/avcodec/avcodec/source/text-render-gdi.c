#include "text-render.h"
#if defined(OS_WINDOWS)
#include <Windows.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define BYTES 3
#define ALIGN(a, n) (((a) + (n) - 1) / (n) * (n))

struct gdi_context_t
{
	HDC hDC;
	HFONT hFont;
	void *bitmap;

	int pitch;
	int width;
	int height;
};

static int text_render_config(void* p, const struct text_parameter_t* param)
{
	HFONT hFont;
	struct gdi_context_t* render = (struct gdi_context_t*)p;

	hFont = CreateFontA(param->size, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, VARIABLE_PITCH, param->font);
	if (NULL == hFont)
		return -1;

	if (render->hFont)
		DeleteObject(render->hFont);
	render->hFont = hFont;
	return 0;
}

static void text_render_destroy(void* p)
{
	struct gdi_context_t* render = (struct gdi_context_t*)p;

	if (render->hFont)
	{
		DeleteObject(render->hFont);
		render->hFont = NULL;
	}

	if (render->hDC)
	{
		ReleaseDC(NULL, render->hDC);
		DeleteDC(render->hDC);
		render->hDC = NULL;
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
	struct gdi_context_t* render;
	render = malloc(sizeof(struct gdi_context_t));
	if (NULL == render)
		return NULL;

	memset(render, 0, sizeof(struct gdi_context_t));
	render->hDC = CreateCompatibleDC(NULL); // screen DC
	if (NULL == render->hDC)
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

// https://msdn.microsoft.com/en-us/library/windows/desktop/dd183402(v=vs.85).aspx
// Capturing an Image
static int text_render_bitmap(struct gdi_context_t* render, HBITMAP hBitmap)
{
	void* p;
	BITMAP bmp;
	BITMAPINFOHEADER bi;

	// Get the BITMAP from the HBITMAP
	memset(&bmp, 0, sizeof(bmp));
	GetObject(hBitmap, sizeof(BITMAP), &bmp);
	assert(bmp.bmWidth == render->pitch && bmp.bmHeight == render->height);

	memset(&bi, 0, sizeof(bi));
	bi.biSize = sizeof(bi);
	bi.biWidth = bmp.bmWidth;
	bi.biHeight = -bmp.bmHeight; // GetDIBits:  A bottom-up DIB is specified by setting the height to a positive number, while a top-down DIB is specified by setting the height to a negative number. 
	bi.biPlanes = 1;
	bi.biBitCount = BYTES * 8;
	bi.biCompression = BI_RGB;

	p = realloc(render->bitmap, bmp.bmHeight * bmp.bmWidth * BYTES);
	if (!p)
		return -ENOMEM;

	render->bitmap = p;
	GetDIBits(render->hDC, hBitmap, 0, bmp.bmHeight, p, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
	return 0;
}

static int text_render_draw_text(struct gdi_context_t* render, const wchar_t* txt)
{
//	SetBkColor(render->hDC, RGB(0x0, 0x0, 0xFF));
//	SetTextColor(render->hDC, RGB(0xFF, 0xFF, 0));
	TextOutW(render->hDC, 0, 0, txt, wcslen(txt));
	//RECT rc;
	//rc.left = x;
	//rc.top = y;
	//rc.right = render->width;
	//rc.bottom = render->height;

	//DrawTextA(render->hDC, txt, -1, &rc, DT_VCENTER | DT_LEFT);
	return 0;
}

static int text_render_getsize(struct gdi_context_t* render, const wchar_t* txt)
{
	RECT rc;
	memset(&rc, 0, sizeof(rc));

	// 1. GetTextExtent
	// 2. GetTextExtentPoint32
	// 3. DrawText with DT_CALCRECT
	if (0 == DrawTextW(render->hDC, txt, -1, &rc, DT_CALCRECT | DT_LEFT | DT_VCENTER))
		return -1;

	render->width = rc.right - rc.left;
	render->height = rc.bottom - rc.top;
	render->height = ALIGN(render->height, 2);
	render->pitch = ALIGN(render->width, 4);
	return 0;
}

static const void* text_render_draw(void* p, const wchar_t* txt, int *w, int *h, int* pitch)
{
	HDC hdc;
	HBITMAP hBitmap, old;
	struct gdi_context_t* render = (struct gdi_context_t*)p;

	if (render->hFont)
		SelectObject(render->hDC, render->hFont);

	if (0 != text_render_getsize(render, txt))
		return NULL;

	hdc = GetDC(NULL);
	hBitmap = CreateCompatibleBitmap(hdc, render->pitch, render->height);
	old = SelectObject(render->hDC, hBitmap);

	text_render_draw_text(render, txt);
	text_render_bitmap(render, hBitmap);

	SelectObject(render->hDC, old);
	DeleteObject(hBitmap);
	ReleaseDC(NULL, hdc);

	*w = render->pitch;
	*h = render->height;
	*pitch = render->pitch * BYTES;
	return render->bitmap;
}

struct text_render_t* text_render_gdi()
{
	static struct text_render_t render = {
		text_render_create,
		text_render_destroy,
		text_render_draw,
		text_render_config,
	};
	return &render;
}

#endif /* OS_WINDOWS */
