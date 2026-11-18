#ifndef _UTIL_FLOAT_
#define _UTIL_FLOAT_

#define OPT_FLOAT

#ifdef OPT_FLOAT
typedef struct {
	int16_t sig; // significant
	int16_t exp; // exponent
} FLOAT_T;
#else
typedef double FLOAT_T;
#endif

#ifdef __cplusplus
extern "C" {
#endif

FLOAT_T FINIT(int32_t sig, int32_t exp);
FLOAT_T FADD(FLOAT_T x, FLOAT_T y); // max error 0.006%
FLOAT_T FSUB(FLOAT_T x, FLOAT_T y); // max error 0.006%
FLOAT_T FMUL(FLOAT_T x, FLOAT_T y); // max error 0.006%
FLOAT_T FDIV(FLOAT_T x, FLOAT_T y); // max error 0.006%
FLOAT_T FLOG2(FLOAT_T x); // max error 0.6%
FLOAT_T FPOW2(FLOAT_T x); // max error 0.02%
int32_t FINT(FLOAT_T x); // max error 0.5
FLOAT_T FMIN(FLOAT_T x, FLOAT_T y);
FLOAT_T FMAX(FLOAT_T x, FLOAT_T y);
FLOAT_T FMINMAX(FLOAT_T min, FLOAT_T max, FLOAT_T x);

#ifdef __cplusplus
}
#endif

#endif
