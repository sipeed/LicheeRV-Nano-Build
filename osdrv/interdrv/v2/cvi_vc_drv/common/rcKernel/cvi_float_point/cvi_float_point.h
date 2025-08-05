
#ifndef __CVI_FLOAT_POINT__
#define __CVI_FLOAT_POINT__

#include <linux/types.h>

#define SOFT_FLOAT 1

#if SOFT_FLOAT
#include "cvi_soft_float.h"
typedef sw_float float32;
#else
#include <math.h>
typedef float float32;
#endif

float32 FRAC_INT_TO_CVI_FLOAT(int32_t a, int frac_bit);
int32_t CVI_FLOAT_TO_FRAC_INT(float32 a, int frac_bit);

float32 INT_TO_CVI_FLOAT(int32_t a);
int32_t CVI_FLOAT_TO_INT(float32 a);

float32 CVI_FLOAT_ADD(float32 a, float32 b);
float32 CVI_FLOAT_SUB(float32 a, float32 b);
float32 CVI_FLOAT_MUL(float32 a, float32 b);
float32 CVI_FLOAT_EXP(float32 a);
float32 CVI_FLOAT_LOG(float32 a);
float32 CVI_FLOAT_POW(float32 a, float32 b);
float32 CVI_FLOAT_DIV(float32 a, float32 b);

uint32_t CVI_FLOAT_EQ(float32 a, float32 b);
uint32_t CVI_FLOAT_LT(float32 a, float32 b);
uint32_t CVI_FLOAT_LE(float32 a, float32 b);
uint32_t CVI_FLOAT_GT(float32 a, float32 b);
uint32_t CVI_FLOAT_GE(float32 a, float32 b);

float32 CVI_FLOAT_MIN(float32 a, float32 b);
float32 CVI_FLOAT_MAX(float32 a, float32 b);
float32 CVI_FLOAT_CLIP(float32 low, float32 high, float32 a);
#endif
