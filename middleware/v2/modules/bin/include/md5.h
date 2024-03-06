#ifndef _MD5_H_
#define _MD5_H_

#include <stdint.h>

#define MD5_STRING_LEN (32 + 1)

typedef int BOOL;
typedef unsigned char Uint8;
typedef unsigned short Uint16;
typedef unsigned int Uint32;
typedef char Int8;
typedef short Int16;
typedef int Int32;

#define osal_memset(ptr, val, size) memset((ptr), (val), (size))
#define osal_memcpy(dst, src, size) memcpy((dst), (src), (size))

#ifdef PLATFORM_QNX
#include <sys/stat.h>
#endif

#define MATCH_OR_MISMATCH(_expected, _value, _ret)                             \
	((_ret = (_expected == _value)) ? "MATCH" : "MISMATCH")

#if defined(WIN32) || defined(WIN64)
/*
 *	( _MSC_VER => 1200 )     6.0     vs6
 *	( _MSC_VER => 1310 )     7.1     vs2003
 *	( _MSC_VER => 1400 )     8.0     vs2005
 *	( _MSC_VER => 1500 )     9.0     vs2008
 *	( _MSC_VER => 1600 )    10.0     vs2010
 */
#if (_MSC_VER == 1200)
#define strcasecmp stricmp
#define strncasecmp strnicmp
#else
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif
#define inline _inline
#if (_MSC_VER == 1600)
#define strdup _strdup
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MD5_DIGEST_LENGTH 16

typedef struct MD5state_st {
	Uint32 A, B, C, D;
	Uint32 Nl, Nh;
	Uint32 data[16];
	Uint32 num;
} MD5_CTX;

void calcute_md5_value(const uint8_t *src, size_t len, Uint8 *result);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _MD5_ */

