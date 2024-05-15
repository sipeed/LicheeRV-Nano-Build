#include "text-render.h"
#include "bitmap.h"
#include <string.h>
#include <assert.h>

void text_render_test(const wchar_t* text)
{
	struct text_parameter_t param;
#if 1 || defined(OS_WINDOWS)
	//param.font = "Î¢ÈíÑÅºÚ";
	param.font = "C:\\WINDOWS\\Fonts\\msyh.ttc";
#else
	param.font = "/usr/share/fonts/truetype/arial.ttf";
#endif
	param.size = 32;
	struct text_render_t* api = text_render_freetype();
	void* render = api->create(&param);

	int w, h, pitch;
	const void* rgba = api->draw(render, text, &w, &h, &pitch);

	BITMAPINFOHEADER bi;
	memset(&bi, 0, sizeof(bi));
	bi.biSize = sizeof(bi);
	bi.biWidth = w;
	bi.biHeight = -h;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bitmap_save("a.bmp", &bi, rgba);

	api->destroy(render);
}
