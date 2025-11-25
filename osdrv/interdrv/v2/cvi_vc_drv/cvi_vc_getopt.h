#ifndef __CVI_VC_GET_OPTION_H__
#define __CVI_VC_GET_OPTION_H__

#include "vdi_osal.h"
struct option {
	const char *name;
	/* has_arg can't be an enum because some compilers complain about
	   type mismatches in all the code that assumes it is an int.  */
	int has_arg;
	int *flag;
	int val;
};

extern char *optarg;

int getopt_long(int argc, char **argv, const char *options,
	    const struct option *long_options, int *opt_index);

char *cvi_strtok_r(char *s, const char *delim, char **save_ptr);
char *cvi_strtok(char *str, const char *delim);
void getopt_init(void);
int atoi(const char *str);
int cvi_isdigit(int c);
//double atof(const char* str);

#endif /* __CVI_VC_GET_OPTION_H__ */