#include <assert.h>
#include <math.h>
#include "colorspace.h"

#define PI	3.1415926535897932384626433832795

static inline unsigned char CLIP(double v)
{
	if(v < 0)	return 0;
	if(v > 255) return 255;
	return (unsigned char)v;
}

static inline void hue_lookup(int hue/*-180,180*/, double* sine, double* cosine)
{
	int i = 0;
	static int s_init = 0;
	static double hue_sin[361] = {0};
	static double hue_cos[361] = {0};
	if(0 == s_init)
	{
		s_init = 1;
		for(i=-180; i<181; i++)
		{
			hue_sin[i+180] = sin(i*PI/180.0);
			hue_cos[i+180] = cos(i*PI/180.0);
		}
	}
	*sine = hue_sin[hue+180];
	*cosine = hue_cos[hue+180];
}

static void yuv2rgb_lookup(unsigned char y, unsigned char u, unsigned char v, unsigned char *r, unsigned char *g, unsigned char *b)
{
	int i, j;
	static unsigned char rtable[256][256] = {0};
	static unsigned char btable[256][256] = {0};
	static int ytable[256] = {0};
	static int utable[256] = {0};
	static int vtable[256] = {0};
	//static int yvtable[256] = {0};
	//static int yutable[256] = {0};
	static int init = 0;

	if(0 == init)
	{
		for(i=0; i<256; i++)
		{
			ytable[i] = 298*(i-16);
			utable[i] = 100*(i-128);
			vtable[i] = 208*(i-128);
			//yvtable[i] = 409*(i-128);
			//yutable[i] = 516*(i-128);

			for(j=0; j<256; j++)
			{
				rtable[i][j] = CLIP((ytable[i] + 409*(j-128) + 128) >> 8);
				btable[i][j] = CLIP((ytable[i] + 516*(j-128) + 128) >> 8);
			}
		}
		init = 1;
	}

	*r = rtable[y][v];
	*b = btable[y][u];
	//*r = CLIP((ytable[y] + yvtable[v] + 128) >> 8);
	//*b = CLIP((ytable[y] + yutable[u] + 128) >> 8);
	*g = CLIP((ytable[y] - utable[u] - vtable[v] + 128) >> 8);
}

static inline void ccir601_rgb2yuv(double r, double g, double b, double* y, double* u, double* v)
{
	/*
		CCIR 601 defined the relationship between YCbCr and RGB Values:
			Ey =  Y = 0.299R + 0.587G + 0.114B
			Ecr = V = 0.713(R-Ey) = -0.500R - 0.419G - 0.081B
			Ecb = U = 0.564(B-Ey) = -0.169R - 0.331G + 0.500B

		where Ey, R, G and B are in the range [0, 1] and Ecr and Ecb are in the range [-0.5, 0.5]

		ref. http://www.fourcc.org/fccyuvrgb.php
	*/
	*y = 0.299*r + 0.587*g + 0.114*b;
	*v = (r-*y)*0.713;
	*u = (b-*y)*0.564;
}

static inline void rec601_rgb2yuv(unsigned char r, unsigned char g, unsigned char b, unsigned char* y, char* u, char* v)
{
	/*
	CCIR Rec. 601-1 spec used by TIFF & JPEG (from David):
		Y  = 0.2989R  + 0.5866G + 0.1145B
		Cr = -0.5000R - 0.4183G - 0.0816B
		Cb = -0.1687R - 0.3312G + 0.5000B		

		R = Y + 0.0000Cb + 1.4022Cr
		G = Y - 0.2456Cb - 0.7145Cr
		B = Y + 1.7710Cb + 0.0000Cr

	where R, G and B in [0, 255]; Y in [0, 255]; and Cb, Cr in [-128, 127]
	*/
	*y =   (unsigned char)(0.2989*r + 0.5866*g + 0.1145*b);
	*v =			(char)(-0.5000*r - 0.4183*g - 0.0816*b);
	*u =			(char)(-0.1687*r - 0.3312*g + 0.5000*b);
}

static inline void rec601_yuv2rgb(unsigned char y, unsigned char u, unsigned char v, unsigned char* r, unsigned char* g, unsigned char* b)
{
	char u2 = u - 128;
	char v2 = v - 128;
	*r = (unsigned char)(y + 0.0000*u2 + 1.4022*v2);
	*g = (unsigned char)(y - 0.2456*u2 - 0.7145*v2);
	*b = (unsigned char)(y + 1.7710*u2 + 0.0000*v2);
}

static inline void jfif601_rgb2yuv(unsigned char r, unsigned char g, unsigned char b, unsigned char* y, unsigned char* u, unsigned char* v)
{
	/*
	JFIF-Y'CbCr (601) from "digital 8-bit R'G'B'"
		Y' =       0.299    * R + 0.587    * G + 0.114    * B
		Cb = 128 - 0.168736 * R - 0.331264 * G + 0.5      * B
		Cr = 128 + 0.5      * R - 0.418688 * G - 0.081312 * B

	where R, G and B in [0, 255]; Y', Cb, Cr in [0, 255]
	*/

	*y = (unsigned char)(      0.299*r + 0.587*g + 0.114*b);
	*v = (unsigned char)(128 - 0.168736*r - 0.331264*g + 0.5*b);
	*u = (unsigned char)(128 + 0.5*r - 0.418688*g + 0.081312*b);
}

static inline void fourcc_rgb2yuv(unsigned char r, unsigned char g, unsigned char b, unsigned char* y, unsigned char* u, unsigned char* v)
{
	/*
		http://www.fourcc.org/fccyvrgb.php
	*/
	*y = (unsigned char)((0.257 * r) + (0.504 * g) + (0.098 * b) + 16);
	*v = (unsigned char)((0.439 * r) - (0.368 * g) - (0.071 * b) + 128);
	*u = (unsigned char)(-(0.148* r) - (0.291 * g) + (0.439 * b) + 128);
}

static inline void fourcc_yuv2rgb(unsigned char y, unsigned char u, unsigned char v, unsigned char* r, unsigned char* g, unsigned char* b)
{
	*b = CLIP(1.164*(y-16)				    + 2.018*(u-128));
	*g = CLIP(1.164*(y-16) - 0.813*(v-128) - 0.391*(u-128));
	*r = CLIP(1.164*(y-16) + 1.596*(v-128));
}

static inline void microsoft_rgb2yuv(unsigned char r, unsigned char g, unsigned char b, unsigned char* y, unsigned char* u, unsigned char* v)
{
	/*
		msdn: Convert Between YUV and RGB

		http://msdn.microsoft.com/en-us/library/ms893078
	*/

	*y = ( ( 66*r + 129*g +  25*b + 128) >> 8) + 16;
	*u = ( (-38*r +  74*g + 112*b + 128) >> 8) + 128;
	*v = ( (112*r -  94*g -  18*b + 128) >> 8) + 128;
}

static inline void microsoft_yuv2rgb(unsigned char y, unsigned char u, unsigned char v, unsigned char* r, unsigned char* g, unsigned char* b)
{
	*r = CLIP((298*(y-16)               + 409*(v-128) + 128) >> 8);
	*g = CLIP((298*(y-16) - 100*(u-128) - 208*(v-128) + 128) >> 8);
	*b = CLIP((298*(y-16) + 516*(u-128)               + 128) >> 8);
}

static inline void ppm_yuv2rgb(unsigned char y, unsigned char u, unsigned char v, unsigned char* r, unsigned char* g, unsigned char* b)
{
	/*
		http://www.fourcc.org/yuv2ppm.c
	*/
	*r = CLIP(y + 1.370705*(v-128));
	*g = CLIP(y - 0.698001*(v-128) - 0.337633*(u-128));
	*b = CLIP(y + 1.732446*(u-128));
}

#define yuv2rgb_imp yuv2rgb_lookup
//#define yuv2rgb_imp	microsoft_yuv2rgb
#define rgb2yuv_imp	microsoft_rgb2yuv

void rgb2yuv(unsigned char r, unsigned char g, unsigned char b, unsigned char* y, unsigned char* u, unsigned char* v)
{
#if COLORSPACE == MICROSOFT
	microsoft_rgb2yuv(r, g, b, y, u, v);
#elif COLORSPACE == FOURCC
	fourcc_rgb2yuv(r, g, b, y, u, v);
#elif COLORSPACE == JFIF601
	jfif601_rgb2yuv(r, g, b, y, u, v);
#elif COLORSPACE == REC601
	rec601_rgb2yuv(r, g, b, y, u, v);;
#elif COLORSPACE == CCIR601
	ccir601_rgb2yuv(r, g, b, y, u, v);
#else
	fourcc_rgb2yuv(r, g, b, y, u, v);
#endif
}

void yuv2rgb(unsigned char y, unsigned char u, unsigned char v, unsigned char* r, unsigned char* g, unsigned char* b)
{
#if COLORSPACE == MICROSOFT
	microsoft_yuv2rgb(y, u, v, r, g, b);
#elif COLORSPACE == FOURCEE
	fourcc_yuv2rgb(y, u, v, r, g, b);
#elif COLORSPACE == REC601
	rec601_yuv2rgb(y, u, v, r, g, b);
#elif COLORSPACE == PPM
	ppm_yuv2rgb(y, u, v, r, g, b);
#else
	yuv2rgb_lookup(y, u, v, r, g, b);
#endif
}

/*
RGB888 -> YV12

I420        | Horizontal  Vertical
-----------------|----------------------
Y Samples Period |     1          1
V Samples Period |     2          2
U Samples Period |     2          2
*/
void rgb24_yv12(const unsigned char* rgb, int w, int h, int stride, unsigned char* y, unsigned char* u, unsigned char* v)
{
	int i,j;
	const unsigned char* p;

	assert(stride >= 3*w);
	for (i = 0; i < h; i++)
	{
		for (j = 0; j < w / 2; j++)
		{
			p = rgb + i * stride + 2 * j * 3;

			if (0 == i % 2)
			{
				// even horizontal, even vertical				
				rgb2yuv(p[2], p[1], p[0], y, u, v);
				++y;
				++u;
				++v;

				*y++ = ((66 * p[5] + 129 * p[4] + 25 * p[3] + 128) >> 8) + 16;
			}
			else
			{
				*y++ = ((66 * p[2] + 129 * p[1] + 25 * p[0] + 128) >> 8) + 16;
				*y++ = ((66 * p[5] + 129 * p[4] + 25 * p[3] + 128) >> 8) + 16;
			}
		}
	}
}

void yv12_rgb24(const unsigned char* y, const unsigned char* u, const unsigned char* v, int stride_y, int stride_uv, int w, int h, unsigned char* rgb)
{
	int i, j;
	unsigned char r, g, b;
	const unsigned char *py, *pu, *pv;

	assert(stride_y >= w);
	assert(stride_uv*2 >= w);
	for (i = 0; i < h; i++)
	{
		py = y + stride_y * i;
		pu = u + stride_uv * (i / 2);
		pv = v + stride_uv * (i / 2);
		for (j = 0; j < w / 2; j++, ++pu, ++pv)
		{
			// Y[0], U[0], V[0] even horizontal, even vertical
			yuv2rgb(*py++, *pu, *pv, &r, &g, &b);
			*rgb++ = b; // rgb[i][j]
			*rgb++ = g;
			*rgb++ = r;

			// Y[1], U[0], V[0] even horizontal, odd vertical
			yuv2rgb(*py++, *pu, *pv, &r, &g, &b);
			*rgb++ = b; // rgb[i][j+1]
			*rgb++ = g;
			*rgb++ = r;
		}
	}
}

void rgb32_yv12(const unsigned char* rgb, int w, int h, int stride, unsigned char* y, unsigned char* u, unsigned char* v)
{
	int i,j;
	const unsigned char* p;

#define R 2
#define G 1
#define B 0

	assert(stride >= 4*w);
	for (i = 0; i < h; i++)
	{
		for (j = 0; j < w / 2; j++)
		{
			p = rgb + i * stride + 2 * j * 4;

			if (0 == i % 2)
			{
				// even horizontal, even vertical				
				rgb2yuv(p[R], p[G], p[B], y, u, v);
				++y;
				++u;
				++v;

				*y++ = ((66 * p[R+4] + 129 * p[G+4] + 25 * p[B+4] + 128) >> 8) + 16;
			}
			else
			{
				*y++ = ((66 * p[R] + 129 * p[G] + 25 * p[B] + 128) >> 8) + 16;
				*y++ = ((66 * p[R+4] + 129 * p[G+4] + 25 * p[B+4] + 128) >> 8) + 16;
			}
		}
	}
#undef R
#undef G
#undef B
}

void yv12_rgb32(const unsigned char* y, const unsigned char* u, const unsigned char* v, int stride_y, int stride_uv, int w, int h, unsigned char* rgb)
{
	int i, j;
	unsigned char r, g, b;
	const unsigned char *py, *pu, *pv;

	assert(stride_y >= w);
	assert(stride_uv*2 >= w);
	for (i = 0; i < h; i++)
	{
		py = y + stride_y * i;
		pu = u + stride_uv * (i / 2);
		pv = v + stride_uv * (i / 2);
		for (j = 0; j < w / 2; j++, ++pu, ++pv)
		{
			// Y[0], U[0], V[0] even horizontal, even vertical
			yuv2rgb(*py++, *pu, *pv, &r, &g, &b);
			*rgb++ = b; // rgb[i][j]
			*rgb++ = g;
			*rgb++ = r;
			*rgb++ = 0;

			// Y[1], U[0], V[0] even horizontal, odd vertical
			yuv2rgb(*py++, *pu, *pv, &r, &g, &b);
			*rgb++ = b; // rgb[i][j+1]
			*rgb++ = g;
			*rgb++ = r;
			*rgb++ = 0;
		}
	}
}

void nv12_rgb24(const unsigned char* y, const unsigned char* v, int stride_y, int stride_uv, int w, int h, unsigned char* rgb)
{
	int i, j;
	unsigned char r, g, b;
	const unsigned char *py, *pv;

	assert(stride_y >= w);
	assert(stride_uv*2 >= w);
	for(i=0; i<h; i++)
	{
		py = y + stride_y*i;
		pv = v + stride_y*(i/2);
		for(j=0; j<w/2; j++, pv+=2)
		{
			// Y[0], U[0], V[0] even horizontal, even vertical
			yuv2rgb(*py++, pv[0], pv[1], &r, &g, &b);
			*rgb++ = b; // rgb[i][j]
			*rgb++ = g;
			*rgb++ = r;

			// Y[1], U[0], V[0] even horizontal, odd vertical
			yuv2rgb(*py++, pv[0], pv[1], &r, &g, &b);
			*rgb++ = b; // rgb[i][j+1]
			*rgb++ = g;
			*rgb++ = r;
		}
	}
}

void yuv_adjust(unsigned char* y, unsigned char* u, unsigned char* v, double contrast, double hue, double saturation, double brightness)
{
	/*
		msdn: Processing in the 8-bit YUV Color Space

		http://msdn.microsoft.com/en-us/library/ff569191(v=VS.85).aspx
	*/

	double sine, cosine;
	hue_lookup((int)hue, &sine, &cosine);

	*y = CLIP(((*y-16) * contrast) + brightness + 16);
	*u = CLIP((((*u-128) * cosine + (*v-128) * sine) * contrast * saturation) + 128);
	*v = CLIP((((*v-128) * cosine - (*u-128) * sine) * contrast * saturation) + 128);
}

void yv12_adjust(unsigned char* y, unsigned char* u, unsigned char* v, int stride_y, int stride_uv, int w, int h, double contrast, double hue, double saturation, double brightness)
{
	int i,j;
	unsigned char *py, *pu, *pv;

	assert(stride_y >= w);
	assert(stride_uv*2 >= w);
	assert(contrast>=0 && contrast<=10);
	assert(brightness>=-100 && brightness<=100);
	assert(hue>=-180 && hue<=180);
	assert(saturation>=0 && saturation<=10);

	for(i=0; i<h; i++)
	{
		py = y + stride_y*i;
		pu = u + stride_uv*(i/2);
		pv = v + stride_uv*(i/2);
		for(j=0; j<w/2; j++, ++pu, ++pv)
		{
			if(0 == i%2)
			{
				// even horizontal, even vertical				
				yuv_adjust(py++, pu, pv, contrast, hue, saturation, brightness);
				//yuv_adjust(y+(i*stride_y+j*2), u+((i/2)*stride_uv+j), v+((i/2)*stride_uv+j), contrast, hue, saturation, brightness);
		
				*py = CLIP(((*py-16) * contrast) + brightness + 16); py++;
				//y[i*stride_y+j*2+1] = CLIP(((y[i*stride_y+j*2+1]-16) * contrast) + brightness + 16);
			}
			else
			{
				*py = CLIP(((*py-16) * contrast) + brightness + 16); py++;
				*py = CLIP(((*py-16) * contrast) + brightness + 16); py++;
				//y[i*stride_y+j*2] = CLIP(((y[i*stride_y+j*2]-16) * contrast) + brightness + 16);
				//y[i*stride_y+j*2+1] = CLIP(((y[i*stride_y+j*2+1]-16) * contrast) + brightness + 16);
			}
		}
	}
}

void nv12_adjust(unsigned char* y, unsigned char* v, int stride_y, int stride_uv, int w, int h, double contrast, double hue, double saturation, double brightness)
{
	int i,j;
	unsigned char *py, *pv;

	assert(stride_y >= w);
	assert(stride_uv*2 >= w);
	assert(contrast>=0.0 && contrast<=10.0);
	assert(brightness>=-100.0 && brightness<=100.0);
	assert(hue>=-180.0 && hue<=180.0);
	assert(saturation>=0.0 && saturation<=10.0);

	for(i=0; i<h; i++)
	{
		py = y + stride_y*i;
		pv = v + stride_y*(i/2);
		for(j=0; j<w/2; j++, pv+=2)
		{
			if(0 == i%2)
			{
				// even horizontal, even vertical				
				yuv_adjust(py++, pv, pv+1, contrast, hue, saturation, brightness);
				//yuv_adjust(y+(i*stride_y+j*2), u+((i/2)*stride_uv+j), v+((i/2)*stride_uv+j), contrast, hue, saturation, brightness);

				*py = CLIP(((*py-16) * contrast) + brightness + 16); py++;
				//y[i*stride_y+j*2+1] = CLIP(((y[i*stride_y+j*2+1]-16) * contrast) + brightness + 16);
			}
			else
			{
				*py = CLIP(((*py-16) * contrast) + brightness + 16); py++;
				*py = CLIP(((*py-16) * contrast) + brightness + 16); py++;
				//y[i*stride_y+j*2] = CLIP(((y[i*stride_y+j*2]-16) * contrast) + brightness + 16);
				//y[i*stride_y+j*2+1] = CLIP(((y[i*stride_y+j*2+1]-16) * contrast) + brightness + 16);
			}
		}
	}
}
