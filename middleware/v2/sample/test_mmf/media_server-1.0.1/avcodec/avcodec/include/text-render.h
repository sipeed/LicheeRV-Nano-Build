#ifndef _text_render_h_
#define _text_render_h_

#include <stddef.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif
	
struct text_parameter_t
{
	const char* font;
	int size; // font size
	int color;
};

struct text_render_t
{
	void* (*create)(const struct text_parameter_t* param);

	void (*destroy)(void* render);

	/// @param[in] txt unicode string
	/// @param[out] w bitmap width
	/// @param[out] h bitmap height
	/// @param[out] pitch bitmap line bytes(>=w)
	/// @return NULL-failed, other-bitmap data
	const void* (*draw)(void* render, const wchar_t* txt, int *w, int *h, int* pitch);

	int (*config)(void* render, const struct text_parameter_t* param);
};

struct text_render_t* text_render_gdi(void);
struct text_render_t* text_render_freetype(void);

#ifdef __cplusplus
}
#endif
#endif /* !_text_render_h_ */
