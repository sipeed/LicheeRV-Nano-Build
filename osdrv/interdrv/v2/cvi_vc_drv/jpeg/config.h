

#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) ||                \
	defined(WIN32) || defined(__MINGW32__)
#define PLATFORM_WIN32
#elif defined(linux) || defined(__linux) || defined(ANDROID)
#if defined(USE_KERNEL_MODE)
#define PLATFORM_LINUX_KERNEL
#else
#define PLATFORM_LINUX
#endif
#else
#define PLATFORM_NON_OS
#endif

#if defined(_MSC_VER)
#include <windows.h>
#include <conio.h>
#define inline _inline
#define JPU_DELAY_MS(X) Sleep(X)
#define JPU_DELAY_US(X) Sleep(X)
		// should change to delay function which can be delay a
		// microsecond unut.
#define kbhit _kbhit
#define getch _getch
#elif defined(__GNUC__)
#ifdef _KERNEL_
#define JPU_DELAY_MS(X) udelay(X * 1000)
#define JPU_DELAY_US(X) udelay(X)
#else
#define JPU_DELAY_MS(X) usleep(X * 1000)
#define JPU_DELAY_US(X) usleep(X)
#endif
#elif defined(__ARMCC__)
#else
#error "Unknown compiler."
#endif

#define PROJECT_ROOT "..\\..\\..\\"

#if defined(JPU_FPGA_PLATFORM)
#if defined(ANDROID) || defined(linux)
#else
#define SUPPORT_CONF_TEST
#endif
#endif

#define API_VERSION 165

//#define MJPEG_ERROR_CONCEAL

#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_H__ */
