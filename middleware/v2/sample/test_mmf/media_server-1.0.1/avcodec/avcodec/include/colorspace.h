#ifndef _colorspace_h_
#define _colorspace_h_

#ifdef __cplusplus
extern "C"
{
#endif


// Convert Between YUV and RGB
void rgb2yuv(unsigned char r, unsigned char g, unsigned char b, unsigned char* y, unsigned char* u, unsigned char* v);
void yuv2rgb(unsigned char y, unsigned char u, unsigned char v, unsigned char* r, unsigned char* g, unsigned char* b);

//void rgb24_yuv444(const unsigned char* rgb, int w, int h, int stride, unsigned char* y, unsigned char* u, unsigned char* v);
//void yuv444_rgb24(const unsigned char* y, const unsigned char* u, const unsigned char* v, int stride_y, int stride_uv, int w, int h, unsigned char* rgb);

// Convert Between YV12(Planar, top-down) and RGB(byte order 'R'/'G'/'B'/'A')
void rgb24_yv12(const unsigned char* rgb, int w, int h, int stride, unsigned char* y, unsigned char* u, unsigned char* v);
void rgb32_yv12(const unsigned char* rgb, int w, int h, int stride, unsigned char* y, unsigned char* u, unsigned char* v);
void yv12_rgb24(const unsigned char* y, const unsigned char* u, const unsigned char* v, int stride_y, int stride_uv, int w, int h, unsigned char* rgb);
void yv12_rgb32(const unsigned char* y, const unsigned char* u, const unsigned char* v, int stride_y, int stride_uv, int w, int h, unsigned char* rgb);
void nv12_rgb24(const unsigned char* y, const unsigned char* v, int stride_y, int stride_uv, int w, int h, unsigned char* rgb);

// contrast [0.0, 10.0] default: 1.0f - 0.01f[increment]
// brightness [-100.0, 100.0] default: 0.0f - 0.1f
// hue [-180.0, 180.0] default: 0.0f - 0.1f
// saturation [0.0, 10.0] default: 1.0f - 0.01f
void yuv_adjust(unsigned char* y, unsigned char* u, unsigned char* v, double contrast, double hue, double saturation, double brightness);

void yv12_adjust(unsigned char* y, unsigned char* u, unsigned char* v, int stride_y, int stride_uv, int w, int h, double contrast, double hue, double saturation, double brightness);
void nv12_adjust(unsigned char* y, unsigned char* v, int stride_y, int stride_uv, int w, int h, double contrast, double hue, double saturation, double brightness);


#ifdef __cplusplus
}
#endif

#endif /* !_colorspace_h_ */
