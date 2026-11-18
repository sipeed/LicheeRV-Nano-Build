#if defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) ||                \
	defined(WIN32) || defined(__MINGW32__)
#include <windows.h>
#endif

#include "../jpuapi/jpuconfig.h"
#include <linux/slab.h>
#include <linux/printk.h>
#include "../include/jpulog.h"

#ifdef REDUNDENT_CODE
#if defined(linux) || defined(__linux) || defined(ANDROID)
#include <time.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h> // for read()

static struct termios initial_settings, new_settings;
static int peek_character = -1;
#endif

static int log_colors[MAX_LOG_LEVEL] = {
	0,
	TERM_COLOR_R | TERM_COLOR_BRIGHT, // ERR
	TERM_COLOR_R | TERM_COLOR_B | TERM_COLOR_BRIGHT, // WARN
	TERM_COLOR_R | TERM_COLOR_G | TERM_COLOR_B | TERM_COLOR_BRIGHT, // INFO
	TERM_COLOR_R | TERM_COLOR_G | TERM_COLOR_B // TRACE
};

static unsigned int log_decor = (LOG_HAS_TIME | LOG_HAS_FILE | LOG_HAS_MICRO_SEC |
			     LOG_HAS_NEWLINE | LOG_HAS_SPACE | LOG_HAS_COLOR);
static FILE *fpLog;
#endif	// #ifdef REDUNDENT_CODE
static int max_log_level = ERR;

#ifdef REDUNDENT_CODE
int jdi_InitLog(void)
{
#if 0
	fpLog = fopen("c:\\out.log", "w");
	if (fpLog == NULL) {
		return -1;
	}
#endif

	return 1;
}

void jdi_DeInitLog(void)
{
	if (fpLog) {
		fclose(fpLog);
		fpLog = NULL;
	}
}

void jdi_SetLogColor(int level, int color)
{
	log_colors[level] = color;
}

int jdi_GetLogColor(int level)
{
	return log_colors[level];
}

void jdi_SetLogDecor(int decor)
{
	log_decor = decor;
}

int jdi_GetLogDecor(void)
{
	return log_decor;
}
#endif

void jdi_SetMaxLogLevel(int level)
{
	max_log_level = level;
}

int jdi_GetMaxLogLevel(void)
{
	return max_log_level;
}

void jdi_LogMsg(int level, const char *format, ...)
{
	va_list ptr;
	char logBuf[MAX_PRINT_LENGTH] = { 0 };

	if (level > max_log_level)
		return;

	va_start(ptr, format);
	vsnprintf(logBuf, MAX_PRINT_LENGTH, format, ptr);
	va_end(ptr);

	pr_err("%s", logBuf);
}

#ifdef REDUNDENT_CODE

#if defined(_MSC_VER)
static LARGE_INTEGER initial_;
static LARGE_INTEGER frequency_;
static LARGE_INTEGER counter_;
#endif

#if defined(linux) || defined(__linux) || defined(ANDROID)

void init_keyboard(void)
{
	tcgetattr(0, &initial_settings);
	new_settings = initial_settings;
	new_settings.c_lflag &= ~ICANON;
	new_settings.c_lflag &= ~ECHO;
	new_settings.c_lflag &= ~ISIG;
	new_settings.c_cc[VMIN] = 1;
	new_settings.c_cc[VTIME] = 0;
	tcsetattr(0, TCSANOW, &new_settings);
	peek_character = -1;
}

void close_keyboard(void)
{
	tcsetattr(0, TCSANOW, &initial_settings);
	fflush(stdout);
}

int kbhit(void)
{
	unsigned char ch;
	int nread;

	if (peek_character != -1)
		return 1;
	new_settings.c_cc[VMIN] = 0;
	tcsetattr(0, TCSANOW, &new_settings);
	nread = read(0, &ch, 1);
	new_settings.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &new_settings);
	if (nread == 1) {
		peek_character = (int)ch;
		return 1;
	}
	return 0;
}

int getch(void)
{
	int val;
	char ch;
	if (peek_character != -1) {
		val = peek_character;
		peek_character = -1;
		return val;
	}
	read(0, &ch, 1);
	return ch;
}
#endif //#if defined(linux) || defined(__linux) || defined(ANDROID)
#endif