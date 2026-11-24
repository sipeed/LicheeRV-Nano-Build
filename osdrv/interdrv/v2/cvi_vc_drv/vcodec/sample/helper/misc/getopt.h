#ifndef __GETOPT_H__
#define __GETOPT_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int opterr; /* if error message should be printed */
extern int optind; /* index into parent argv vector */
extern int optopt; /* character checked for validity */
extern int optreset; /* reset getopt */
extern char *optarg; /* argument associated with option */

struct option {
	const char *name;
	int has_arg;
	int *flag;
	int val;
};

#define no_argument 0
#define required_argument 1
#define optional_argument 2

int getopt(int nargc, char *const nargv[], const char *ostr);

#if defined(PLATFORM_QNX)
#else
int getopt_long(int nargc, char **nargv, char *options,
		struct option *long_options, int *index);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __GETOPT_H__ */
