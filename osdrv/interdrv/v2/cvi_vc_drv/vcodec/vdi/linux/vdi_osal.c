//------------------------------------------------------------------------------
// File: vdi_osal.c
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------
#if defined(linux) || defined(__linux) || defined(ANDROID)

#include "vpuconfig.h"
#include <linux/slab.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include "../vdi_osal.h"

#ifdef REDUNDENT_CODE
static struct termios initial_settings, new_settings;
static int peek_character = -1;
#endif

#if !defined(USE_KERNEL_MODE) || defined(REDUNDENT_CODE)
static int log_colors[MAX_LOG_LEVEL] = {
	0,
	TERM_COLOR_R | TERM_COLOR_G | TERM_COLOR_B | TERM_COLOR_BRIGHT, // INFO
	TERM_COLOR_R | TERM_COLOR_B | TERM_COLOR_BRIGHT, // WARN
	TERM_COLOR_R | TERM_COLOR_BRIGHT, // ERR
	TERM_COLOR_R | TERM_COLOR_G | TERM_COLOR_B // TRACE
};

static unsigned int log_decor = LOG_HAS_TIME | LOG_HAS_FILE | LOG_HAS_MICRO_SEC |
			    LOG_HAS_NEWLINE | LOG_HAS_SPACE | LOG_HAS_COLOR;
#endif
static int max_log_level = NONE;
#ifdef ENABLE_LOG
static FILE *fpLog;
#endif

struct cvi_osal_file {
	struct file *filep;
	mm_segment_t old_fs;
};


#ifdef ENABLE_LOG
int InitLog(void)
{
	fpLog = osal_fopen("ErrorLog.txt", "w");

	return 1;
}

void DeInitLog(void)
{
	if (fpLog) {
		osal_fclose(fpLog);
		fpLog = NULL;
	}
}
#endif

#ifdef REDUNDENT_CODE
void SetLogColor(int level, int color)
{
	log_colors[level] = color;
}

int GetLogColor(int level)
{
	return log_colors[level];
}

void SetLogDecor(int decor)
{
	log_decor = decor;
}

int GetLogDecor(void)
{
	return log_decor;
}
#endif

void SetMaxLogLevel(int level)
{
	max_log_level = level;
}
int GetMaxLogLevel(void)
{
	return max_log_level;
}

void LogMsg(int level, const char *format, ...)
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
void osal_init_keyboard(void)
{
	tcgetattr(0, &initial_settings);
	new_settings = initial_settings;
	new_settings.c_lflag &= ~ICANON;
	new_settings.c_lflag &= ~ECHO;
	// new_settings.c_lflag &= ~ISIG;
	new_settings.c_cc[VMIN] = 1;
	new_settings.c_cc[VTIME] = 0;
	tcsetattr(0, TCSANOW, &new_settings);
	peek_character = -1;
}

void osal_close_keyboard(void)
{
	tcsetattr(0, TCSANOW, &initial_settings);
}

int osal_kbhit(void)
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

int osal_getch(void)
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

int osal_flush_ch(void)
{
	return 1;
}
#endif

int osal_rand(void)
{
	return 0;
}

void *osal_memcpy(void *dst, const void *src, int count)
{
	return memcpy(dst, src, count);
}

int osal_memcmp(const void *src, const void *dst, int size)
{
	return memcmp(src, dst, size);
}

void *osal_memset(void *dst, int val, int count)
{
	return memset(dst, val, count);
}

void *osal_malloc(int size)
{
	return vzalloc(size);
}

#ifdef REDUNDENT_CODE
void *osal_realloc(void *ptr, int size)
{
	return realloc(ptr, size);
}
#endif

void osal_free(void *p)
{
	vfree(p);
}

int osal_feof(osal_file_t fp)
{
	return 0;
}

osal_file_t osal_fopen(const char *osal_file_tname, const char *mode)
{
	struct cvi_osal_file *cvi_fp = (struct cvi_osal_file *)
				       vmalloc(sizeof(struct cvi_osal_file));

	if (!cvi_fp)
		return NULL;

	if (!strncmp(mode, "rb", 2)) {
		cvi_fp->filep = filp_open(osal_file_tname, O_RDONLY/*|O_NONBLOCK*/, 0644);
	} else if (!strncmp(mode, "wb", 2)) {
		cvi_fp->filep = filp_open(osal_file_tname, O_RDWR | O_CREAT, 0644);
	}

	if (IS_ERR(cvi_fp->filep)) {
		vfree(cvi_fp);
		return NULL;
	}

	cvi_fp->old_fs = get_fs();
	return cvi_fp;
}
size_t osal_fwrite(const void *p, int size, int count, osal_file_t fp)
{
	struct cvi_osal_file *cvi_fp = (struct cvi_osal_file *)fp;
	struct file *filep = cvi_fp->filep;

	return kernel_write(filep, p, size * count, &filep->f_pos);
}
size_t osal_fread(void *p, int size, int count, osal_file_t fp)
{
	struct cvi_osal_file *cvi_fp = (struct cvi_osal_file *)fp;
	struct file *filep = cvi_fp->filep;

	return kernel_read(filep, p, size * count, &filep->f_pos);
}
long osal_ftell(osal_file_t fp)
{
	struct cvi_osal_file *cvi_fp = (struct cvi_osal_file *)fp;
	struct file *filep = cvi_fp->filep;

	return filep->f_pos;
	return 0;
}

int osal_fseek(osal_file_t fp, long offset, int origin)
{
	struct cvi_osal_file *cvi_fp = (struct cvi_osal_file *)fp;
	struct file *filep = cvi_fp->filep;

	return default_llseek(filep, offset, origin);

	return 0;
}
int osal_fclose(osal_file_t fp)
{
	struct cvi_osal_file *cvi_fp = (struct cvi_osal_file *)fp;
	struct file *filep;

	if (!fp)
		return -1;

	filep = cvi_fp->filep;
	filp_close(filep, 0);
	set_fs(cvi_fp->old_fs);
	vfree(cvi_fp);
	return 0;
}

#ifdef REDUNDENT_CODE
int osal_fscanf(osal_file_t fp, const char *_Format, ...)
{
	int ret;
	return ret;
}

int osal_fprintf(osal_file_t fp, const char *_Format, ...)
{
	int ret;
	return ret;
}
#endif

void *osal_kmalloc(int size)
{
	return kmalloc(size, GFP_KERNEL);
}

void osal_kfree(void *p)
{
	kfree(p);
}

void *osal_ion_alloc(int size)
{
	PhysicalAddress u64PhysAddr;
	void *pVirtAddr;

	if (sys_ion_alloc_nofd((uint64_t *)&u64PhysAddr, (void **)&pVirtAddr,
			  "vcodec_bistream_ion", size, true) != 0) {
		CVI_VC_ERR("[VDI] fail to allocate ion memory. size=%d\n",
			   size);
		return NULL;
	}
	CVI_VC_MEM("physaddr=%llx, virtaddr=%p, size=0x%x\n",
		u64PhysAddr, (void *)pVirtAddr, size);
	return (void *)phys_to_virt(u64PhysAddr);
}

void osal_ion_free(void *p)
{
	PhysicalAddress u64PhysAddr = (PhysicalAddress)virt_to_phys(p);

	CVI_VC_MEM("physaddr=%llx, virtaddr=%p\n",
		u64PhysAddr, (void *)p);
	sys_ion_free_nofd((uint64_t)u64PhysAddr);
}
//------------------------------------------------------------------------------
// math related api
//------------------------------------------------------------------------------
#ifndef I64
typedef long long I64;
#endif

// 32 bit / 16 bit ==> 32-n bit remainder, n bit quotient
static int fixDivRq(int a, int b, int n)
{
	I64 c;
	I64 a_36bit;
	I64 mask, signBit, signExt;
	int i;

	// DIVS emulation for BPU accumulator size
	// For SunOS build
	mask = 0x0F;
	mask <<= 32;
	mask |= 0x00FFFFFFFF; // mask = 0x0FFFFFFFFF;
	signBit = 0x08;
	signBit <<= 32; // signBit = 0x0800000000;
	signExt = 0xFFFFFFF0;
	signExt <<= 32; // signExt = 0xFFFFFFF000000000;

	a_36bit = (I64)a;

	for (i = 0; i < n; i++) {
		c = a_36bit - (b << 15);
		if (c >= 0)
			a_36bit = (c << 1) + 1;
		else
			a_36bit = a_36bit << 1;

		a_36bit = a_36bit & mask;
		if (a_36bit & signBit)
			a_36bit |= signExt;
	}

	a = (int)a_36bit;
	return a; // R = [31:n], Q = [n-1:0]
}

int math_div(int number, int denom)
{
	int c;
	c = fixDivRq(number, denom, 17); // R = [31:17], Q = [16:0]
	c = c & 0xFFFF;
	c = (c + 1) >> 1; // round
	return (c & 0xFFFF);
}

#endif //#if defined(linux) || defined(__linux) || defined(ANDROID)
