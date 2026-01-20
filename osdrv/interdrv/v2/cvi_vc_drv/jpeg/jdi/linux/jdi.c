#if defined(linux) || defined(__linux) || defined(ANDROID)
#ifdef __arm__
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#undef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#define MMAP mmap64
#else
#define MMAP mmap
#endif

#include <linux/delay.h>
#include <linux/dma-buf.h>
#include <asm/cacheflush.h>

#include "../../include/jpulog.h"
#include "../../jpuapi/jpuapifunc.h"
#include "../jdi.h"
#include "driver/jpu.h"

#ifdef CLI_DEBUG_SUPPORT
#include "tcli.h"
#endif

#ifdef TRY_SEM_MUTEX
#include <semaphore.h>
typedef sem_t MUTEX_HANDLE;
#define JPU_SEM_NAME "/jpu_sem_core"
#define OPEN_FLAG O_CREAT
#define OPEN_MODE 0666
#define INIT_V 1
#else
typedef struct mutex MUTEX_HANDLE;
extern int jpu_wait_interrupt(int timeout);
extern int jpu_set_clock_gate(int *pEnable);
extern int jpu_get_instance_pool(jpudrv_buffer_t *p_jdb);
extern int jpu_open_instance(unsigned long *pInstIdx);
extern int jpu_close_instance(unsigned long *pInstIdx);
extern int jpu_get_instance_num(int *pInstNum);
extern int jpu_get_register_info(jpudrv_buffer_t *p_jdb_register);
extern int jpu_reset(int InstIdx);
#endif
#ifdef JPU_FPGA_PLATFORM
#define HPI_BUS_LEN 8
#define HPI_BUS_LEN_ALIGN 7

/*------------------------------------------------------------------------
HPI register definitions
------------------------------------------------------------------------*/
#define HPI_CHECK_STATUS 1
#define HPI_WAIT_TIME 0x100000
#define HPI_BASE 0x20030000
#define HPI_ADDR_CMD (0x00 << 2)
#define HPI_ADDR_STATUS (0x01 << 2)
#define HPI_ADDR_ADDR_H (0x02 << 2)
#define HPI_ADDR_ADDR_L (0x03 << 2)
#define HPI_ADDR_ADDR_M (0x06 << 2)
#define HPI_ADDR_DATA (0x80 << 2)

#define HPI_CMD_WRITE_VALUE ((8 << 4) + 2)
#define HPI_CMD_READ_VALUE ((8 << 4) + 1)

#define HPI_MAX_PKSIZE 256

// clock generator in FPGA

#define DEVICE0_ADDR_COMMAND 0x75
#define DEVICE0_ADDR_PARAM0 0x76
#define DEVICE0_ADDR_PARAM1 0x77
#define DEVICE1_ADDR_COMMAND 0x78
#define DEVICE1_ADDR_PARAM0 0x79
#define DEVICE1_ADDR_PARAM1 0x7a
#define DEVICE_ADDR_SW_RESET 0x7b

#define ACLK_MAX 30
#define ACLK_MIN 16
#define CCLK_MAX 30
#define CCLK_MIN 16

static void *hpi_init(unsigned long core_idx, unsigned long dram_base);
static void hpi_release(unsigned long core_idx);
static void hpi_write_register(unsigned long core_idx, void *base,
			       unsigned int addr, unsigned int data,
			       pthread_mutex_t io_mutex);
static unsigned int hpi_read_register(unsigned long core_idx, void *base,
				      unsigned int addr,
				      pthread_mutex_t io_mutex);
static int hpi_write_memory(unsigned long core_idx, void *base,
			    unsigned int addr, unsigned char *data, int len,
			    int endian, pthread_mutex_t io_mutex);
static int hpi_read_memory(unsigned long core_idx, void *base,
			   unsigned int addr, unsigned char *data, int len,
			   int endian, pthread_mutex_t io_mutex);
static int hpi_hw_reset(void *base);

static unsigned int pci_read_reg(unsigned int addr);
static void pci_write_reg(unsigned int addr, unsigned int data);
static void pci_write_memory(unsigned int addr, unsigned char *buf, int size);
static void pci_read_memory(unsigned int addr, unsigned char *buf, int size);

static int hpi_set_timing_opt(unsigned long core_idx, void *base,
			      pthread_mutex_t io_mutex);
static int ics307m_set_clock_freg(void *base, int Device, int OutFreqMHz,
				  int InFreqMHz);

static int jpu_swap_endian(unsigned char *data, int len, int endian);

#endif // JPU_FPGA_PLATFORM

#define JPU_BIT_REG_SIZE 0x300
#define JPU_BIT_REG_BASE (0x50470000)
#define JDI_DRAM_PHYSICAL_BASE 0x1F8000000
#define JDI_DRAM_PHYSICAL_SIZE (128 * 1024 * 1024)

#ifdef JPU_FPGA_PLATFORM
// #define SUPPORT_ALLOCATE_MEMORY_FROM_DRIVER
#define SUPPORT_INTERRUPT
#define JDI_SYSTEM_ENDIAN JDI_BIG_ENDIAN
#else
#define SUPPORT_ALLOCATE_MEMORY_FROM_DRIVER
#define SUPPORT_INTERRUPT
#define JDI_SYSTEM_ENDIAN JDI_LITTLE_ENDIAN
#endif

#define JPU_DEVICE_NAME "/dev/jpu"
#define TRY_OPEN_SEM_TIMEOUT 3

typedef struct jpu_buffer_pool_t {
	jpudrv_buffer_t jdb;
	int inuse;
	int is_jpe;
} jpu_buffer_pool_t;

typedef struct {
	int jpu_fd;
	jpu_instance_pool_t *pjip;
	int jip_size;
	int task_num;
	int bSingleEsBuf;
	int single_es_buf_size;
	int clock_state;
#ifndef SUPPORT_ALLOCATE_MEMORY_FROM_DRIVER
	jpudrv_buffer_t jdb_video_memory;
#endif
	jpudrv_buffer_t jdb_register;
	jpu_buffer_pool_t jpu_buffer_pool[MAX_JPU_BUFFER_POOL];
	int jpu_buffer_pool_count;
#ifdef TRY_SEM_MUTEX
	void *jpu_mutex;
#endif

#ifdef JPU_FPGA_PLATFORM
	MUTEX_HANDLE io_mutex;
#endif
	jpu_buffer_t vbStream;
	int enc_task_num;
} jdi_info_t;

static jdi_info_t s_jdi_info[MAX_NUM_JPU_CORE] = { 0 };

static int jpu_swap_endian(unsigned char *data, int len, int endian);

int jdi_probe(void)
{
	int ret;
	ret = jdi_init();
#ifndef JPU_FPGA_PLATFORM
	jdi_release();
#endif
	return ret;
}

int jdi_get_task_num(void)
{
	jdi_info_t *jdi = &s_jdi_info[0];
	return jdi->task_num;
}

int jdi_use_single_es_buffer(void)
{
	jdi_info_t *jdi = &s_jdi_info[0];
	return jdi->bSingleEsBuf;
}

int jdi_set_enc_task(int bSingleEsBuf, int *singleEsBufSize)
{
	jdi_info_t *jdi = &s_jdi_info[0];

	if (jdi->enc_task_num == 0) {
		jdi->bSingleEsBuf = bSingleEsBuf;
		jdi->single_es_buf_size = *singleEsBufSize;
	} else {
		if (jdi->bSingleEsBuf != bSingleEsBuf) {
			JLOG(ERR,
			     "[JDI]all instance use single es buffer flag must be the same\n");
			return -1;
		}
	}

	if (jdi->bSingleEsBuf) {
		if (jdi->single_es_buf_size != *singleEsBufSize) {
			JLOG(WARN, "different single es buf size %d ignored\n",
			     *singleEsBufSize);
			JLOG(WARN, "current single es buf size %d\n",
			     jdi->single_es_buf_size);
			*singleEsBufSize = jdi->single_es_buf_size;
		}
	}

	jdi->enc_task_num++;

	return 0;
}

int jdi_delete_enc_task(void)
{
	jdi_info_t *jdi = &s_jdi_info[0];

	if (jdi->enc_task_num == 0) {
		JLOG(ERR, "[JDI]jdi_delete_enc_task == 0\n");
		return -1;
	}
	jdi->enc_task_num--;
	return 0;
}

int jdi_get_enc_task_num(void)
{
	jdi_info_t *jdi = &s_jdi_info[0];

	return jdi->enc_task_num;
}

int jdi_init(void)
{
	jdi_info_t *jdi = &s_jdi_info[0];
#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
	unsigned long page_size = VMEM_PAGE_SIZE;
	unsigned long page_mask = (~(page_size - 1));
#endif
	PhysicalAddress pgoff;
	int ret;

	if (jdi->jpu_fd != -1 && jdi->jpu_fd != 0x00) {
		jdi->task_num++;
		return 0;
	}
	memset(jdi, 0, sizeof(jdi_info_t));

#ifdef ANDROID
	system("/system/lib/modules/load_android.sh");
	printf("s_jpu_fd\n");
#endif
	jdi->jpu_fd = 'j'; // TODO: refine this!

	memset((void *)&jdi->jpu_buffer_pool, 0x00,
	       sizeof(jpu_buffer_pool_t) * MAX_JPU_BUFFER_POOL);
	jdi->jpu_buffer_pool_count = 0;

	// printf("jdi_get_instance_pool\n");
	jdi->pjip = jdi_get_instance_pool();
	if (!(jdi->pjip)) {
		JLOG(ERR,
		     "[JDI] fail to create instance pool for saving context\n");
		goto ERR_JDI_INIT;
	}

#ifdef TRY_SEM_MUTEX
	// it's not error if semaphore existing
	jdi->jpu_mutex = sem_open(JPU_SEM_NAME, OPEN_FLAG, OPEN_MODE, INIT_V);
	if (jdi->jpu_mutex == SEM_FAILED) {
		JLOG(ERR, "sem_open failed...\n");
		return -1;
	}
#endif

	if (!jdi->pjip->instance_pool_inited) {
#if 0
		pthread_mutexattr_t mutexattr;

		pthread_mutexattr_init(&mutexattr);
		pthread_mutexattr_setpshared(&mutexattr,
					     PTHREAD_PROCESS_SHARED);

		pthread_mutex_init((pthread_mutex_t *)jdi->pjip->jpu_mutex,
				   &mutexattr);
		pthread_mutexattr_destroy(&mutexattr);
#elif defined(LIBCVIJPULITE)
		jdi->pjip->jpu_mutex = 0;
#endif
	}

#if defined(LIBCVIJPULITE)
	ret = jdi_lock(100);
#else
	mutex_init(&jdi->pjip->jpu_mutex);

	ret = jdi_lock();
#endif
	if (ret < 0) {
		JLOG(ERR, "[JDI] fail to pthread_mutex_t lock function\n");
		goto ERR_JDI_INIT;
	}
	// printf("JDI_IOCTL_GET_RESERVED_VIDEO_MEMORY_INFO\n");

#ifndef SUPPORT_ALLOCATE_MEMORY_FROM_DRIVER
	if (ioctl(jdi->jpu_fd, JDI_IOCTL_GET_RESERVED_VIDEO_MEMORY_INFO,
		  &jdi->jdb_video_memory) < 0) {
		JLOG(ERR, "[JDI] fail to get video memory information\n");
		goto ERR_JDI_INIT;
		return -1;
	}
	printf("jdi->jdb_video_memory: phy: %p, size:%d\n",
	       jdi->jdb_video_memory.phys_addr, jdi->jdb_video_memory.size);
#ifdef JPU_FPGA_PLATFORM
	jdi->jdb_video_memory.phys_addr = JDI_DRAM_PHYSICAL_BASE;
	jdi->jdb_video_memory.size = JDI_DRAM_PHYSICAL_SIZE;
#endif

	if (!jdi->pjip->instance_pool_inited)
		memset(&jdi->pjip->vmem, 0x00, sizeof(jpeg_mm_t));

	if (jmem_init(&jdi->pjip->vmem,
		      (unsigned long)jdi->jdb_video_memory.phys_addr,
		      jdi->jdb_video_memory.size) < 0) {
		JLOG(ERR, "[JDI] fail to init jpu memory management logic\n");
		goto ERR_JDI_INIT;
	}
#endif

#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
	if (jpu_get_register_info(&jdi->jdb_register) < 0) {
		JLOG(ERR, "[JDI] fail to get host interface register\n");
		goto ERR_JDI_INIT;
	}

	pgoff = jdi->jdb_register.phys_addr & page_mask;
#else
	jdi->jdb_register.size = JPU_BIT_REG_SIZE;
	pgoff = 0;
#endif

#ifdef JPU_FPGA_PLATFORM
	pthread_mutex_init(&jdi->io_mutex, NULL);
	hpi_init(0, JDI_DRAM_PHYSICAL_BASE);
#endif

	jdi->task_num++;
	jdi_unlock();

	jdi_set_clock_gate(1);

	// JLOG(INFO, "[JDI] success to init driver\n");
	return jdi->jpu_fd;

ERR_JDI_INIT:
	jdi_unlock();
	jdi_release();
	return -1;
}

int jdi_release(void)
{
	jdi_info_t *jdi = &s_jdi_info[0];
	int ret;
	int i;
	BOOL bCloseIonFd = TRUE;

	if (jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00)
		return 0;

#if defined(LIBCVIJPULITE)
	ret = jdi_lock(100);
#else
	ret = jdi_lock();
#endif
	if (ret < 0) {
		CVI_JPG_DBG_ERR(
			"[JDI] fail to pthread_mutex_t lock function\n");
		return -1;
	}

	jdi->task_num--;
	// means that the opened instance remains
	if (jdi->task_num > 0) {
		CVI_JPG_DBG_INFO("s_task_num =%d\n", jdi->task_num);
		jdi_unlock();
		return 0;
	}

	jdi_set_clock_gate(0);

	memset(&jdi->jdb_register, 0x00, sizeof(jpudrv_buffer_t));

	jdi_unlock();

	jdi->pjip = 0x00;

	jdi->jpu_fd = -1;

#ifdef TRY_SEM_MUTEX
	if (sem_close((MUTEX_HANDLE *)jdi->jpu_mutex) < 0) {
		JLOG(ERR, "[JDI]  sem_close thread id = %lu\n", pthread_self());
	}
#endif

#ifdef JPU_FPGA_PLATFORM
	pthread_mutex_destroy(&jdi->io_mutex);
	hpi_release(0);
#endif

	memset(jdi, 0, sizeof(jdi_info_t));

	for (i = 0; i < MAX_NUM_JPU_CORE; i++) {
		jdi = &s_jdi_info[i];
		if (jdi->jpu_fd > 0) {
			bCloseIonFd = FALSE;
			break;
		}
	}
	return 0;
}

jpu_instance_pool_t *jdi_get_instance_pool()
{
	jpudrv_buffer_t jdb = { 0 };
	jdi_info_t *jdi = &s_jdi_info[0];

	if (jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00)
		return NULL;

	if (sizeof(JpgInst) > MAX_INST_HANDLE_SIZE) {
		CVI_JPG_DBG_ERR("JpgInst = %d, MAX_INST_HANDLE_SIZE = %d\n",
				(int)sizeof(JpgInst), MAX_INST_HANDLE_SIZE);
	}

	if (jdi->pjip)
		return (jpu_instance_pool_t *)jdi->pjip;

	jdi->jip_size = sizeof(jpu_instance_pool_t);
#if 0
	jdi->jip_size += sizeof(MUTEX_HANDLE);
#endif

	jdb.size = jdi->jip_size;

	CVI_JPG_DBG_MEM("JDI_IOCTL_GET_INSTANCE_POOL, jdb.size = 0x%x\n",
			jdb.size);

	if (jpu_get_instance_pool(&jdb) < 0) {
		JLOG(ERR, "[JDI] fail to get instance pool. size=%d\n",
		     jdb.size);
		return NULL;
	}

	do {
		jdb.virt_addr =
			(__u8 *)(uintptr_t)(jdb.phys_addr); // instance pool was allocated by vmalloc
		jdi->pjip = (jpu_instance_pool_t *)jdb.virt_addr;
	} while (0);

#if 0
	//change the pointer of jpu_mutex to at end pointer of jpu_instance_pool_t to assign at allocated position.
	jdi->pjip->jpu_mutex = (void *)((unsigned long)jdi->pjip +
					sizeof(jpu_instance_pool_t));
#endif

	return (jpu_instance_pool_t *)jdi->pjip;
}

int jdi_open_instance(unsigned long instIdx)
{
	int inst_num;
	jdi_info_t *jdi;

	jdi = &s_jdi_info[0];
	if (jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00)
		return -1;

	if (jpu_open_instance(&instIdx) < 0) {
		JLOG(ERR,
		     "[JDI] fail to deliver open instance num instIdx=%d\n",
		     (int)instIdx);
		return -1;
	}

	if (jpu_get_instance_num(&inst_num) < 0) {
		JLOG(ERR,
		     "[JDI] fail to deliver open instance num instIdx=%d\n",
		     (int)instIdx);
		return -1;
	}

	jdi->pjip->jpu_instance_num = inst_num;

	return 0;
}

int jdi_close_instance(unsigned long instIdx)
{
	int inst_num;
	jdi_info_t *jdi;

	jdi = &s_jdi_info[0];
	if (jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00)
		return -1;

	if (jpu_close_instance(&instIdx) < 0) {
		JLOG(ERR,
		     "[JDI] fail to deliver open instance num instIdx=%d\n",
		     (int)instIdx);
		return -1;
	}

	if (jpu_get_instance_num(&inst_num) < 0) {
		JLOG(ERR,
		     "[JDI] fail to deliver open instance num instIdx=%d\n",
		     (int)instIdx);
		return -1;
	}

	jdi->pjip->jpu_instance_num = inst_num;

	return 0;
}

int jdi_get_instance_num(void)
{
	jdi_info_t *jdi;

	jdi = &s_jdi_info[0];
	if (jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00)
		return -1;

	return jdi->pjip->jpu_instance_num;
}

int jdi_hw_reset(void)
{
	jdi_info_t *jdi = &s_jdi_info[0];

	if (jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00) {
		return 0;
	}

#ifdef JPU_FPGA_PLATFORM
	hpi_hw_reset((void *)jdi->jdb_register.virt_addr);
	return 1;
#else
	return jpu_reset(0);
#endif
}

#ifdef CLI_DEBUG_SUPPORT
void cli_show_jdi_info(void)
{
	jdi_info_t *jdi;

	jdi = &s_jdi_info[0];

	tcli_print("----------jdi info--------------\n");
	tcli_print("jdi_jpu_fd:%d\n", jdi->jpu_fd);
	tcli_print("memory jdi_jip_size:%d\n", jdi->jip_size);
	tcli_print("jdi_task_num:%d\n", jdi->task_num);
	tcli_print("jdi_bSingleEsBuf:%d\n", jdi->bSingleEsBuf);
	tcli_print("jdi_single_es_buf_size:%d\n", jdi->single_es_buf_size);
	tcli_print("jdi_clock_state:%d\n", jdi->clock_state);
	tcli_print("jdi_enc_task_num:%d\n", jdi->enc_task_num);
	tcli_print("ion jdi_vbStream.size:%d\n", jdi->vbStream.size);
	tcli_print("jdi_jdb_register.size:%d\n", jdi->jdb_register.size);
#ifndef SUPPORT_ALLOCATE_MEMORY_FROM_DRIVER
	tcli_print("jdi_jdb_video_memory.size:%d\n",
		   jdi->jdb_video_memory.size);
#endif
	tcli_print("jdi_jpu_buffer_pool_count:%d\n",
		   jdi->jpu_buffer_pool_count);

	for (int i = 0; i < jdi->jpu_buffer_pool_count; i++) {
		jpu_buffer_pool_t *p = &jdi->jpu_buffer_pool[i];

		if (p && p->inuse) {
			tcli_print(" jdi_jpu_buffer_pool[%d].jdb_size:%d\n", i,
				   p->jdb.size);
			tcli_print(" jdi_jpu_buffer_pool[%d].is_jpe:%d\n", i,
				   p->is_jpe);
		}
	}
}
#endif

#if defined(LIBCVIJPULITE)
int jdi_lock(int sleep_us)
#else
int jdi_lock(void)
#endif
{
#if 0
	jdi_info_t *jdi = &s_jdi_info[0];

	const int MUTEX_TIMEOUT = 0x7fffffff; // ms

	while (1) {
		int _ret;
		int i;

		for (i = 0;
		     (_ret = pthread_mutex_trylock(
			      (pthread_mutex_t *)jdi->pjip->jpu_mutex)) != 0 &&
		     i < MUTEX_TIMEOUT;
		     i++) {
			if (i == 0)
				JLOG(ERR,
				     "jdi_lock: mutex is already locked - try again : ret = %d,  pthread id =%lu\n",
				     _ret, pthread_self());
#ifdef _KERNEL_
			udelay(1 * 1000);
#else
			usleep(1 * 1000);
#endif // _KERNEL_
			JLOG(ERR,
			     "jdi_lock: mutex tiemout for %dms, i=%d,num=%d,jdi->pjip=%p, jpu_mutex=%p, pthread id =%lu\n",
			     JPU_INTERRUPT_TIMEOUT_MS, i,
			     jdi->pjip->jpu_instance_num, jdi->pjip,
			     jdi->pjip->jpu_mutex, pthread_self());
			if (i > JPU_INTERRUPT_TIMEOUT_MS *
					(jdi->pjip->jpu_instance_num)) {
				JLOG(ERR,
				     "jdi_lock: mutex tiemout for %dms, i=%d, jpu_instance_num=%d\n",
				     JPU_INTERRUPT_TIMEOUT_MS, i,
				     jdi->pjip->jpu_instance_num);
				break;
			}
		}

		if (_ret == 0)
			break;

		JLOG(ERR,
		     "jdi_lock: can't get lock - force to unlock. [%d:%s], pthread id =%lu\n",
		     _ret, strerror(_ret), pthread_self());
		jdi_unlock();
		jdi->pjip->pendingInst = NULL;
	}
	JLOG(INFO, "jdi->pjip=%p, jpu_mutex=%p. id=%lu\n", jdi->pjip,
	     jdi->pjip->jpu_mutex, pthread_self());

#elif defined(LIBCVIJPULITE)

	jdi_info_t *jdi = &s_jdi_info[0];
	struct timespec req = { 0, sleep_us * 1000 };

	while (!__sync_bool_compare_and_swap(&(jdi->pjip->jpu_mutex), 0, 1)) {
		int ret = nanosleep(&req, NULL);
		/* interrupted by a signal handler */
		if (ret < 0) {
			jdi->pjip->jpu_mutex = 0;
			return -1;
		}
	}
#endif

	return 0;
}

void jdi_unlock(void)
{
#if 0
	jdi_info_t *jdi = &s_jdi_info[0];
	JLOG(INFO, "id=%lu\n", pthread_self());
	pthread_mutex_unlock((pthread_mutex_t *)jdi->pjip->jpu_mutex);
#elif defined(LIBCVIJPULITE)
	jdi_info_t *jdi = &s_jdi_info[0];
	jdi->pjip->jpu_mutex = 0;
#endif
}

void jdi_write_register(unsigned int addr, unsigned int data)
{
	unsigned int *reg_addr;
	jdi_info_t *jdi;
	jdi = &s_jdi_info[0];

	if (!jdi->pjip || jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00)
		return;
#ifdef JPU_FPGA_PLATFORM
	hpi_write_register(0, (void *)jdi->jdb_register.virt_addr,
			   JPU_BIT_REG_BASE + addr, data, jdi->io_mutex);
#else
	reg_addr = (unsigned int *)(addr + jdi->jdb_register.virt_addr);

	writel(data, reg_addr);
#endif
}

unsigned int jdi_read_register(unsigned int addr)
{
	unsigned int *reg_addr;
	jdi_info_t *jdi;

	jdi = &s_jdi_info[0];
#ifdef JPU_FPGA_PLATFORM
	return hpi_read_register(0, (void *)jdi->jdb_register.virt_addr,
				 JPU_BIT_REG_BASE + addr, jdi->io_mutex);
#else
	reg_addr = (unsigned int *)(addr + jdi->jdb_register.virt_addr);

	return readl(reg_addr);
#endif
}

// It could be removed in the future
void *jdi_osal_memcpy(void *dst, const void *src, int count)
{
	// return memcpy(dst, src, count);
	int i;
	unsigned char *dst_c = (unsigned char *)dst;
	unsigned char *src_c = (unsigned char *)src;
	unsigned char tmp;
	for (i = 0; i < count; i++) {
		tmp = *src_c;
		*dst_c = tmp;
		src_c++;
		dst_c++;
	}

	return dst;
}

int jdi_write_memory(unsigned long addr, unsigned char *data, int len,
		     int endian)
{
	jpudrv_buffer_t jdb;
	unsigned long offset;
	int i;

	jdi_info_t *jdi;

	jdi = &s_jdi_info[0];
	if (!jdi->pjip || jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00)
		return -1;

	memset(&jdb, 0x00, sizeof(jpudrv_buffer_t));

	for (i = 0; i < MAX_JPU_BUFFER_POOL; i++) {
		if (jdi->jpu_buffer_pool[i].inuse == 1) {
			jdb = jdi->jpu_buffer_pool[i].jdb;
			if (addr >= jdb.phys_addr &&
			    addr < (jdb.phys_addr + jdb.size))
				break;
		}
	}

	if (!jdb.size) {
		JLOG(ERR, "address 0x%08x is not mapped address!!!\n",
		     (int)addr);
		return -1;
	}

	offset = addr - (unsigned long)jdb.phys_addr;

#ifdef JPU_FPGA_PLATFORM
	hpi_write_memory(0, (void *)jdi->jdb_register.virt_addr, addr, data,
			 len, endian, jdi->io_mutex);
	memcpy((BYTE *)jdb.virt_addr + offset, data, len);
#else
	jpu_swap_endian(data, len, endian);
	memcpy((void *)((unsigned long)jdb.virt_addr + offset), data, len);
	//  take place with  jdi_osal_memcpy  by mingyi.dong
	// jdi_osal_memcpy((void *)(virtoffset), (void *)data, len);
#endif

	return len;
}

int jdi_read_memory(unsigned long addr, unsigned char *data, int len,
		    int endian)
{
	jpudrv_buffer_t jdb;
	unsigned long offset;
	int i;
	jdi_info_t *jdi;

	jdi = &s_jdi_info[0];
	if (!jdi->pjip || jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00)
		return -1;

	memset(&jdb, 0x00, sizeof(jpudrv_buffer_t));

	for (i = 0; i < MAX_JPU_BUFFER_POOL; i++) {
		if (jdi->jpu_buffer_pool[i].inuse == 1) {
			jdb = jdi->jpu_buffer_pool[i].jdb;
			if (addr >= jdb.phys_addr &&
			    addr < (jdb.phys_addr + jdb.size))
				break;
		}
	}

	if (!jdb.size)
		return -1;

	offset = addr - (unsigned long)jdb.phys_addr;

#ifdef JPU_FPGA_PLATFORM
	hpi_read_memory(0, (void *)jdi->jdb_register.virt_addr, addr, data, len,
			endian, jdi->io_mutex);
#else
	memcpy(data, (const void *)((unsigned long)jdb.virt_addr + offset),
	       len);
	jpu_swap_endian(data, len, endian);
#endif

	return len;
}

int jdi_get_allocated_memory(jpu_buffer_t *vb, int is_jpe)
{
	jdi_info_t *jdi = &s_jdi_info[0];
	jpudrv_buffer_t *jdb;
	int i;
	int isFind = 0;

	if (!jdi->pjip) {
		JLOG(ERR, "Invalid handle! jdi->pjip is NULL!\n");
		return -1;
	}

	if (jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00) {
		JLOG(ERR, "Invalid handle! jdi->jpu_fd=%d\n", jdi->jpu_fd);
		return -1;
	}

	for (i = 0; i < MAX_JPU_BUFFER_POOL; i++) {
		if (jdi->jpu_buffer_pool[i].inuse == 1 &&
		    jdi->jpu_buffer_pool[i].is_jpe == is_jpe) {
			jdb = &jdi->jpu_buffer_pool[i].jdb;
			if (vb->size == jdb->size) {
				vb->size = jdb->size;
				vb->base = jdb->base;
				vb->phys_addr = jdb->phys_addr;
				vb->virt_addr = jdb->virt_addr;
				isFind = 1;
				break;
			}
		}
	}
	if (isFind)
		return 0;
	else
		return -1;
}

#ifndef CVI_JPG_USE_ION_MEM
int jdi_allocate_dma_memory(jpu_buffer_t *vb, int is_jpe)
{
	jdi_info_t *jdi = &s_jdi_info[0];
	jpudrv_buffer_t jdb = { 0 };
	int i;
	int ret = 0;

	if (!jdi->pjip) {
		JLOG(ERR, "Invalid handle! jdi->pjip is NULL! id=%lu",
		     pthread_self());
		return -1;
	}

	if (jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00) {
		JLOG(ERR, "Invalid handle! jdi->jpu_fd=%d. id=%lu", jdi->jpu_fd,
		     pthread_self());
		return -1;
	}

#if defined(LIBCVIJPULITE)
	jdi_lock(100);
#else
	jdi_lock();
#endif

	memset(&jdb, 0x00, sizeof(jpudrv_buffer_t));

	jdb.size = vb->size;

	CVI_JPG_DBG_MEM("jdb.size = 0x%x\n", jdb.size);

#ifdef SUPPORT_ALLOCATE_MEMORY_FROM_DRIVER
	if (ioctl(jdi->jpu_fd, JDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY, &jdb) < 0) {
		JLOG(ERR, "fail to jdi_allocate_dma_memory. size=%d, id=%lu\n",
		     vb->size, pthread_self());
		ret = -1;
		goto fail;
	}
#else
	jdb.phys_addr = (unsigned long)jmem_alloc(&jdi->pjip->vmem, jdb.size);
	if (jdb.phys_addr == (unsigned long)-1) {
		JLOG(ERR, "Not enough memory. size=%d, id=%lu\n", vb->size,
		     pthread_self());
		ret = -1;
		goto fail;
	}

	unsigned long offset = (unsigned long)(jdb.phys_addr -
					       jdi->jdb_video_memory.phys_addr);
	jdb.base = jdi->jdb_video_memory.base + offset;
#endif
	vb->phys_addr = jdb.phys_addr;
	vb->base = jdb.base;

#ifdef JPU_FPGA_PLATFORM
	jdb.virt_addr = (unsigned long)malloc(jdb.size);
#else
	/* Map physical address to virtual address in user space */
	PhysicalAddress page_size = sysconf(_SC_PAGE_SIZE);
	PhysicalAddress page_mask = (~(page_size - 1));
	PhysicalAddress pgoff = jdb.phys_addr & page_mask;

	CVI_JPG_DBG_TRACE("mmap\n");

	jdb.virt_addr = MMAP(NULL, jdb.size, PROT_READ | PROT_WRITE, MAP_SHARED,
			     jdi->jpu_fd, pgoff);
	if ((void *)jdb.virt_addr == MAP_FAILED) {
		JLOG(ERR, "fail to map jdb.phys_addr=0x%lx, size=%d. id=%lu\n",
		     jdb.phys_addr, (int)jdb.size, pthread_self());
		memset(vb, 0x00, sizeof(jpu_buffer_t));
		ret = -1;
		goto fail;
	}
#endif
	vb->virt_addr = jdb.virt_addr;

	for (i = 0; i < MAX_JPU_BUFFER_POOL; i++) {
		if (jdi->jpu_buffer_pool[i].inuse == 0) {
			jdi->jpu_buffer_pool[i].jdb = jdb;
			jdi->jpu_buffer_pool_count++;
			jdi->jpu_buffer_pool[i].inuse = 1;
			jdi->jpu_buffer_pool[i].is_jpe = is_jpe;
			break;
		}
	}

	if (i >= MAX_JPU_BUFFER_POOL) {
		JLOG(ERR, "fail to find an unused buffer in pool! id=%lu\n",
		     pthread_self());
		memset(vb, 0x00, sizeof(jpu_buffer_t));
		ret = -1;
		goto fail;
	}

#if 0
	JLOG(INFO, "size=%d, addr=%p, i=%d, pool_count=%d. id=%lu", jdb.size,
	     jdb.phys_addr, i, jdi->jpu_buffer_pool_count, pthread_self());
#endif

fail:
	jdi_unlock();

	return ret;
}

void jdi_free_dma_memory(jpu_buffer_t *vb)
{
	jdi_info_t *jdi = &s_jdi_info[0];
	jpudrv_buffer_t jdb;
	int i;
	int ret;

	if (!jdi->pjip) {
		JLOG(ERR, "Invalid handle! jdi->pjip is NULL! id=%lu",
		     pthread_self());
		return;
	}

	if (jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00) {
		JLOG(ERR, "Invalid handle! jdi->jpu_fd=%d. id=%lu", jdi->jpu_fd,
		     pthread_self());
		return;
	}

	if (vb->size == 0) {
		JLOG(ERR, "addr=%p, id=%lu\n", vb->phys_addr, pthread_self());
		return;
	}

#if defined(LIBCVIJPULITE)
	jdi_lock(100);
#else
	jdi_lock();
#endif

	memset(&jdb, 0x00, sizeof(jpudrv_buffer_t));

	for (i = 0; i < MAX_JPU_BUFFER_POOL; i++) {
		if (jdi->jpu_buffer_pool[i].jdb.phys_addr == vb->phys_addr) {
			jdi->jpu_buffer_pool[i].inuse = 0;
			jdi->jpu_buffer_pool[i].is_jpe = 0;
			jdi->jpu_buffer_pool_count--;
			jdb = jdi->jpu_buffer_pool[i].jdb;
			break;
		}
	}

	if (!jdb.size) {
		JLOG(ERR, "Invalid buffer to free! address=0x%lx. id=%lu\n",
		     jdb.virt_addr, pthread_self());
		jdi_unlock();
		return;
	}

#ifdef SUPPORT_ALLOCATE_MEMORY_FROM_DRIVER
#if 0
	JLOG(INFO, "size=%d, addr=%p, i=%d, pool_count=%d. id=%lu", jdb.size,
	     jdb.phys_addr, i, jdi->jpu_buffer_pool_count, pthread_self());
#endif
	ret = ioctl(jdi->jpu_fd, JDI_IOCTL_FREE_PHYSICAL_MEMORY, &jdb);
	if (ret < 0) {
		JLOG(ERR, "ioctl: free physical memory. id=%lu\n",
		     pthread_self());
	}
#else
	jmem_free(&jdi->pjip->vmem, jdb.phys_addr, 0);
#endif

#ifdef JPU_FPGA_PLATFORM
	free((void *)jdb.virt_addr);
#else
	if (munmap((void *)jdb.virt_addr, jdb.size) != 0) {
		JLOG(ERR, "fail to unmap virtual address(0x%lx)! id=%lu\n",
		     jdb.virt_addr, pthread_self());
	}
#endif

	memset(vb, 0, sizeof(jpu_buffer_t));

	jdi_unlock();
}

#else //#ifndef CVI_JPG_USE_ION_MEM

int jdi_allocate_ion_memory(jpu_buffer_t *vb, int is_jpe, int is_cached)
{
	int ret = 0;
	int i = 0;
	jdi_info_t *jdi = &s_jdi_info[0];
	jpudrv_buffer_t jdb = { 0 };

	if (!jdi->pjip) {
		JLOG(ERR, "Invalid handle! jdi->pjip is NULL!\n");
		return -1;
	}

	if (jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00) {
		JLOG(ERR, "Invalid handle! jdi->jpu_fd=%d\n", jdi->jpu_fd);
		return -1;
	}

#if defined(LIBCVIJPULITE)
	jdi_lock(100);
#else
	jdi_lock();
#endif

	memset(&jdb, 0x00, sizeof(jpudrv_buffer_t));

	jdb.size = vb->size;

	if (sys_ion_alloc_nofd((uint64_t *)&vb->phys_addr, (void **)&vb->virt_addr,
			  (uint8_t *)"jpeg_ion", vb->size, is_cached) != 0) {
		JLOG(ERR, "fail to allocate ion memory. size=%d\n", vb->size);
		jdi_unlock();
		return ret;
	}
	jdb.phys_addr = vb->phys_addr;
	jdb.virt_addr = vb->virt_addr;

	for (i = 0; i < MAX_JPU_BUFFER_POOL; i++) {
		if (jdi->jpu_buffer_pool[i].inuse == 0) {
			jdi->jpu_buffer_pool_count++;
			memcpy(&jdi->jpu_buffer_pool[i].jdb, &jdb,
			       sizeof(jpudrv_buffer_t));
			jdi->jpu_buffer_pool[i].is_jpe = is_jpe;
			jdi->jpu_buffer_pool[i].inuse = 1;
			break;
		}
	}
	if (i >= MAX_JPU_BUFFER_POOL) {
		JLOG(ERR, "fail to find an unused buffer in pool!\n");
		memset(vb, 0x00, sizeof(jpu_buffer_t));
		ret = -1;
		goto fail;
	}

	JLOG(INFO,
	     "phys_addr %llx, virt_addr %p, size 0x%x, cached=%d\n",
	     vb->phys_addr, vb->virt_addr, vb->size, is_cached);
fail:
	jdi_unlock();
	return ret;
}

int jdi_free_ion_memory(jpu_buffer_t *vb)
{
	jdi_info_t *jdi = &s_jdi_info[0];
	jpudrv_buffer_t *p_jdb = NULL;
	int ret = 0;
	int i = 0;

	if (!jdi->pjip) {
		JLOG(ERR, "Invalid handle! jdi->pjip is NULL!\n");
		return -1;
	}

	if (jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00) {
		JLOG(ERR, "Invalid handle! jdi->jpu_fd=%d\n", jdi->jpu_fd);
		return -1;
	}

	if (vb->size == 0) {
		JLOG(ERR, "addr=%p\n", vb->phys_addr);
		return -1;
	}

#if defined(LIBCVIJPULITE)
	jdi_lock(100);
#else
	jdi_lock();
#endif

	for (i = 0; i < MAX_JPU_BUFFER_POOL; i++) {
		if (jdi->jpu_buffer_pool[i].jdb.phys_addr == vb->phys_addr) {
			jdi->jpu_buffer_pool[i].inuse = 0;
			jdi->jpu_buffer_pool[i].is_jpe = 0;
			jdi->jpu_buffer_pool_count--;
			p_jdb = &jdi->jpu_buffer_pool[i].jdb;
			break;
		}
	}

	if (!p_jdb->size) {
		JLOG(ERR, "Invalid buffer to free! address=0x%lx\n",
		     p_jdb->virt_addr);
		jdi_unlock();
		return -1;
	}

	JLOG(INFO, "phys_addr %llx, virt_addr %p, fd %d\n", p_jdb->phys_addr,
	     p_jdb->virt_addr, p_jdb->base);
	ret = sys_ion_free_nofd((uint64_t)vb->phys_addr);
	if (ret != 0) {
		JLOG(ERR, "fail to free ion phys_addr = 0x%llx\n",
		     vb->phys_addr);
		jdi_unlock();
		return ret;
	}

	memset(p_jdb, 0x0, sizeof(jpudrv_buffer_t));
	memset(vb, 0, sizeof(jpu_buffer_t));

	jdi_unlock();

	return ret;
}

int jdi_invalidate_ion_cache(uint64_t u64PhyAddr, void *pVirAddr,
			     uint32_t u32Len)
{
	return sys_cache_invalidate(u64PhyAddr, phys_to_virt(u64PhyAddr), u32Len);
}

int jdi_flush_ion_cache(uint64_t u64PhyAddr, void *pVirAddr, uint32_t u32Len)
{
	return sys_cache_flush(u64PhyAddr, phys_to_virt(u64PhyAddr), u32Len);
}
#endif

int jdi_set_clock_gate(int enable)
{
	jdi_info_t *jdi = &s_jdi_info[0];
	JLOG(INFO, "jdi set clk %d\n", enable);

	if (jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00)
		return 0;

	jdi->clock_state = enable;

	return jpu_set_clock_gate(&enable);
}

int jdi_get_clock_gate(void)
{
	jdi_info_t *jdi;

	jdi = &s_jdi_info[0];
	return jdi->clock_state;
}

int jdi_wait_interrupt(int timeout)
{
#if 0//def SUPPORT_INTERRUPT
	int ret;
	jdi_info_t *jdi;

	jdi = &s_jdi_info[0];
	ret = jpu_wait_interrupt(timeout);
	if (ret != 0)
		ret = -1;

	// printf("receive a interrupt ......, ret = %d\n", ret);

	return ret;
#else
	Int64 elapse, cur;
	struct timespec64 tv;

	ktime_get_ts64(&tv);
	elapse = tv.tv_sec * 1000 + tv.tv_nsec / 1000000;

	while (1) {
#ifdef CVIDEBUG_V
		unsigned int reg;
		reg = jdi_read_register(MJPEG_PIC_STATUS_REG);
		JLOG(INFO, "The JPU status reg 0x%08x\n", reg);
		if (reg)
			break;
#else
		if (jdi_read_register(MJPEG_PIC_STATUS_REG))
			break;
#endif /* CVIDEBUG_V */

		ktime_get_ts64(&tv);
		cur = tv.tv_sec * 1000 + tv.tv_nsec / 1000000;

		if ((cur - elapse) > timeout) {
			return -1;
		}
		usleep_range(800, 1200);
	}
	return 0;
#endif
}

void jdi_log(int cmd, int step)
{
	int i;

	switch (cmd) {
	case JDI_LOG_CMD_PICRUN:
		if (step == 1) //
			JLOG(INFO, "\n**PIC_RUN start\n");
		else
			JLOG(INFO, "\n**PIC_RUN end\n");
		break;
	}

	JLOG(INFO, "\nClock Status=%d\n", jdi_get_clock_gate());

	for (i = 0; i <= 0x238; i = i + 16) {
		JLOG(INFO, "0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", i,
		     jdi_read_register(i), jdi_read_register(i + 4),
		     jdi_read_register(i + 8), jdi_read_register(i + 0xc));
	}
}

int jpu_swap_endian(unsigned char *data, int len, int endian)
{
	unsigned long *p;
	unsigned long v1, v2, v3;
	int i;
	int swap = 0;
	p = (unsigned long *)data;

	if (endian == JDI_SYSTEM_ENDIAN)
		swap = 0;
	else
		swap = 1;

	if (swap) {
		if (endian == JDI_LITTLE_ENDIAN || endian == JDI_BIG_ENDIAN) {
			for (i = 0; i < (len >> 2); i += 2) {
				v1 = p[i];
				v2 = (v1 >> 24) & 0xFF;
				v2 |= ((v1 >> 16) & 0xFF) << 8;
				v2 |= ((v1 >> 8) & 0xFF) << 16;
				v2 |= ((v1 >> 0) & 0xFF) << 24;
				v3 = v2;
				v1 = p[i + 1];
				v2 = (v1 >> 24) & 0xFF;
				v2 |= ((v1 >> 16) & 0xFF) << 8;
				v2 |= ((v1 >> 8) & 0xFF) << 16;
				v2 |= ((v1 >> 0) & 0xFF) << 24;
				p[i] = v2;
				p[i + 1] = v3;
			}
		} else {
			int sys_endian = JDI_SYSTEM_ENDIAN;
			int swap4byte = 0;
			swap = 0;
			if (endian == JDI_32BIT_LITTLE_ENDIAN) {
				if (sys_endian == JDI_BIG_ENDIAN) {
					swap = 1;
				}
			} else {
				if (sys_endian == JDI_BIG_ENDIAN) {
					swap4byte = 1;
				} else if (sys_endian == JDI_LITTLE_ENDIAN) {
					swap4byte = 1;
					swap = 1;
				} else {
					swap = 1;
				}
			}
			if (swap) {
				for (i = 0; i < (len >> 2); i++) {
					v1 = p[i];
					v2 = (v1 >> 24) & 0xFF;
					v2 |= ((v1 >> 16) & 0xFF) << 8;
					v2 |= ((v1 >> 8) & 0xFF) << 16;
					v2 |= ((v1 >> 0) & 0xFF) << 24;
					p[i] = v2;
				}
			}
			if (swap4byte) {
				for (i = 0; i < (len >> 2); i += 2) {
					v1 = p[i];
					v2 = p[i + 1];
					p[i] = v2;
					p[i + 1] = v1;
				}
			}
		}
	}
	return swap;
}
PhysicalAddress jdi_get_memory_addr_high(PhysicalAddress addr)
{
	PhysicalAddress ret = 1;

	return (ret << 32) | addr;
}

#ifdef JPU_FPGA_PLATFORM

static void *s_hpi_base;
static unsigned long s_dram_base;
void *hpi_init(unsigned long core_idx, unsigned long dram_base)
{
#define MAX_NUM_JPU_CORE 2
	if (core_idx > MAX_NUM_JPU_CORE)
		return (void *)-1;
	s_dram_base = dram_base;

	return (void *)1;
}

void hpi_release(unsigned long core_idx)
{
}

void hpi_write_register(unsigned long core_idx, void *base, unsigned int addr,
			unsigned int data, pthread_mutex_t io_mutex)
{
	int status;
	jdi_info_t *jdi;

	jdi = &s_jdi_info[0];
	if (pthread_mutex_lock(&jdi->io_mutex) < 0)
		return;

	s_hpi_base = base;

	pci_write_reg(HPI_ADDR_ADDR_H, (addr >> 16));
	pci_write_reg(HPI_ADDR_ADDR_L, (addr & 0xffff));

	pci_write_reg(HPI_ADDR_DATA, ((data >> 16) & 0xFFFF));
	pci_write_reg(HPI_ADDR_DATA + 4, (data & 0xFFFF));

	pci_write_reg(HPI_ADDR_CMD, HPI_CMD_WRITE_VALUE);

	do {
		status = pci_read_reg(HPI_ADDR_STATUS);
		status = (status >> 1) & 1;
	} while (status == 0);

	pthread_mutex_unlock(&jdi->io_mutex);
}

unsigned int hpi_read_register(unsigned long core_idx, void *base,
			       unsigned int addr, pthread_mutex_t io_mutex)
{
	int status;
	unsigned int data;
	jdi_info_t *jdi;

	jdi = &s_jdi_info[0];
	if (pthread_mutex_lock(&jdi->io_mutex) < 0)
		return -1;

	s_hpi_base = base;

	pci_write_reg(HPI_ADDR_ADDR_H, ((addr >> 16) & 0xffff));
	pci_write_reg(HPI_ADDR_ADDR_L, (addr & 0xffff));

	pci_write_reg(HPI_ADDR_CMD, HPI_CMD_READ_VALUE);

	do {
		status = pci_read_reg(HPI_ADDR_STATUS);
		status = status & 1;
	} while (status == 0);

	data = pci_read_reg(HPI_ADDR_DATA) << 16;
	data |= pci_read_reg(HPI_ADDR_DATA + 4);

	pthread_mutex_unlock(&jdi->io_mutex);

	return data;
}

int hpi_write_memory(unsigned long core_idx, void *base, unsigned int addr,
		     unsigned char *data, int len, int endian,
		     pthread_mutex_t io_mutex)
{
	unsigned char *pBuf;
	unsigned char lsBuf[HPI_BUS_LEN];
	int lsOffset;
	jdi_info_t *jdi;

	jdi = &s_jdi_info[0];

	if (addr < s_dram_base) {
		fprintf(stderr, "[HPI] invalid address base address is 0x%lx\n",
			s_dram_base);
		return 0;
	}

	if (pthread_mutex_lock(&jdi->io_mutex) < 0)
		return 0;

	if (len == 0) {
		pthread_mutex_unlock(&jdi->io_mutex);
		return 0;
	}

	addr = addr - s_dram_base;
	s_hpi_base = base;

	lsOffset = addr - (addr / HPI_BUS_LEN) * HPI_BUS_LEN;
	if (lsOffset) {
		pci_read_memory((addr / HPI_BUS_LEN) * HPI_BUS_LEN, lsBuf,
				HPI_BUS_LEN);
		pBuf = (unsigned char *)malloc(
			(len + lsOffset + HPI_BUS_LEN_ALIGN) &
			~HPI_BUS_LEN_ALIGN);
		if (pBuf) {
			memset(pBuf, 0x00,
			       (len + lsOffset + HPI_BUS_LEN_ALIGN) &
				       ~HPI_BUS_LEN_ALIGN);
			memcpy(pBuf, lsBuf, HPI_BUS_LEN);
			memcpy(pBuf + lsOffset, data, len);
			jpu_swap_endian(pBuf,
					((len + lsOffset + HPI_BUS_LEN_ALIGN) &
					 ~HPI_BUS_LEN_ALIGN),
					endian);
			pci_write_memory((addr / HPI_BUS_LEN) * HPI_BUS_LEN,
					 (unsigned char *)pBuf,
					 ((len + lsOffset + HPI_BUS_LEN_ALIGN) &
					  ~HPI_BUS_LEN_ALIGN));
			free(pBuf);
		}
	} else {
		pBuf = (unsigned char *)malloc((len + HPI_BUS_LEN_ALIGN) &
					       ~HPI_BUS_LEN_ALIGN);
		if (pBuf) {
			memset(pBuf, 0x00,
			       (len + HPI_BUS_LEN_ALIGN) & ~HPI_BUS_LEN_ALIGN);
			memcpy(pBuf, data, len);
			jpu_swap_endian(pBuf,
					(len + HPI_BUS_LEN_ALIGN) &
						~HPI_BUS_LEN_ALIGN,
					endian);
			pci_write_memory(addr, (unsigned char *)pBuf,
					 (len + HPI_BUS_LEN_ALIGN) &
						 ~HPI_BUS_LEN_ALIGN);
			free(pBuf);
		}
	}

	pthread_mutex_unlock(&jdi->io_mutex);

	return len;
}

int hpi_read_memory(unsigned long core_idx, void *base, unsigned int addr,
		    unsigned char *data, int len, int endian,
		    pthread_mutex_t io_mutex)
{
	unsigned char *pBuf;
	unsigned char lsBuf[HPI_BUS_LEN];
	int lsOffset;
	jdi_info_t *jdi;

	jdi = &s_jdi_info[0];
	if (addr < s_dram_base) {
		fprintf(stderr, "[HPI] invalid address base address is 0x%lx\n",
			s_dram_base);
		return 0;
	}

	if (pthread_mutex_lock(&jdi->io_mutex) < 0)
		return 0;

	if (len == 0) {
		pthread_mutex_unlock(&jdi->io_mutex);
		return 0;
	}

	addr = addr - s_dram_base;
	s_hpi_base = base;

	lsOffset = addr - (addr / HPI_BUS_LEN) * HPI_BUS_LEN;
	if (lsOffset) {
		pci_read_memory((addr / HPI_BUS_LEN) * HPI_BUS_LEN, lsBuf,
				HPI_BUS_LEN);
		jpu_swap_endian(lsBuf, HPI_BUS_LEN, endian);
		len = len - (HPI_BUS_LEN - lsOffset);
		pBuf = (unsigned char *)malloc(
			((len + HPI_BUS_LEN_ALIGN) & ~HPI_BUS_LEN_ALIGN));
		if (pBuf) {
			memset(pBuf, 0x00,
			       ((len + HPI_BUS_LEN_ALIGN) &
				~HPI_BUS_LEN_ALIGN));
			pci_read_memory((addr + HPI_BUS_LEN_ALIGN) &
						~HPI_BUS_LEN_ALIGN,
					pBuf,
					((len + HPI_BUS_LEN_ALIGN) &
					 ~HPI_BUS_LEN_ALIGN));
			jpu_swap_endian(pBuf,
					((len + HPI_BUS_LEN_ALIGN) &
					 ~HPI_BUS_LEN_ALIGN),
					endian);

			memcpy(data, lsBuf + lsOffset, HPI_BUS_LEN - lsOffset);
			memcpy(data + HPI_BUS_LEN - lsOffset, pBuf, len);

			free(pBuf);
		}
	} else {
		pBuf = (unsigned char *)malloc((len + HPI_BUS_LEN_ALIGN) &
					       ~HPI_BUS_LEN_ALIGN);
		if (pBuf) {
			memset(pBuf, 0x00,
			       (len + HPI_BUS_LEN_ALIGN) & ~HPI_BUS_LEN_ALIGN);
			pci_read_memory(addr, pBuf,
					(len + HPI_BUS_LEN_ALIGN) &
						~HPI_BUS_LEN_ALIGN);
			jpu_swap_endian(pBuf,
					(len + HPI_BUS_LEN_ALIGN) &
						~HPI_BUS_LEN_ALIGN,
					endian);
			memcpy(data, pBuf, len);
			free(pBuf);
		}
	}

	pthread_mutex_unlock(&jdi->io_mutex);

	return len;
}

int hpi_hw_reset(void *base)
{
	s_hpi_base = base;
	pci_write_reg(DEVICE_ADDR_SW_RESET << 2, 1); // write data 1
	return 0;
}

int hpi_write_reg_limit(unsigned long core_idx, unsigned int addr,
			unsigned int data, pthread_mutex_t io_mutex)
{
	int status;
	int i;
	jdi_info_t *jdi;

	jdi = &s_jdi_info[0];
	if (pthread_mutex_lock(&jdi->io_mutex) < 0)
		return 0;

	pci_write_reg(HPI_ADDR_ADDR_H, (addr >> 16));
	pci_write_reg(HPI_ADDR_ADDR_L, (addr & 0xffff));

	pci_write_reg(HPI_ADDR_DATA, ((data >> 16) & 0xFFFF));
	pci_write_reg(HPI_ADDR_DATA + 4, (data & 0xFFFF));

	pci_write_reg(HPI_ADDR_CMD, HPI_CMD_WRITE_VALUE);

	i = 0;
	do {
		status = pci_read_reg(HPI_ADDR_STATUS);
		status = (status >> 1) & 1;
		if (i++ > 10000) {
			pthread_mutex_unlock(&jdi->io_mutex);
			return 0;
		}
	} while (status == 0);

	pthread_mutex_unlock(&jdi->io_mutex);

	return 1;
}

int hpi_read_reg_limit(unsigned long core_idx, unsigned int addr,
		       unsigned int *data, pthread_mutex_t io_mutex)
{
	int status;
	int i;
	jdi_info_t *jdi;

	jdi = &s_jdi_info[0];
	if (pthread_mutex_lock(&jdi->io_mutex) < 0)
		return 0;

	pci_write_reg(HPI_ADDR_ADDR_H, ((addr >> 16) & 0xffff));
	pci_write_reg(HPI_ADDR_ADDR_L, (addr & 0xffff));

	pci_write_reg(HPI_ADDR_CMD, HPI_CMD_READ_VALUE);

	i = 0;
	do {
		status = pci_read_reg(HPI_ADDR_STATUS);
		status = status & 1;
		if (i++ > 10000) {
			pthread_mutex_unlock(&jdi->io_mutex);
			return 0;
		}
	} while (status == 0);

	*data = pci_read_reg(HPI_ADDR_DATA) << 16;
	*data |= pci_read_reg(HPI_ADDR_DATA + 4);

	pthread_mutex_unlock(&jdi->io_mutex);

	return 1;
}

/*------------------------------------------------------------------------
 *  Usage : used to program output frequency of ICS307M
 *  Artument :
 *      Device      : first device selected if 0, second device if 1.
 *      OutFreqMHz  : Target output frequency in MHz.
 *      InFreqMHz   : Input frequency applied to device in MHz
 *                    this must be 10 here.
 *  Return : TRUE if success, FALSE if invalid OutFreqMHz.
 *----------------------------------------------------------------------*/

int ics307m_set_clock_freg(void *base, int Device, int OutFreqMHz,
			   int InFreqMHz)
{
	int VDW, RDW, OD, SDW, tmp;
	int min_clk;
	int max_clk;

	s_hpi_base = base;
	if (Device == 0) {
		min_clk = ACLK_MIN;
		max_clk = ACLK_MAX;
	} else {
		min_clk = CCLK_MIN;
		max_clk = CCLK_MAX;
	}

	if (OutFreqMHz < min_clk || OutFreqMHz > max_clk) {
		// printf ("Target Frequency should be from %2d to %2d !!!\n",
		// min_clk, max_clk);
		return 0;
	}

	if (OutFreqMHz >= min_clk && OutFreqMHz < 14) {
		switch (OutFreqMHz) {
		case 6:
			VDW = 4;
			RDW = 2;
			OD = 10;
			break;
		case 7:
			VDW = 6;
			RDW = 2;
			OD = 10;
			break;
		case 8:
			VDW = 8;
			RDW = 2;
			OD = 10;
			break;
		case 9:
			VDW = 10;
			RDW = 2;
			OD = 10;
			break;
		case 10:
			VDW = 12;
			RDW = 2;
			OD = 10;
			break;
		case 11:
			VDW = 14;
			RDW = 2;
			OD = 10;
			break;
		case 12:
			VDW = 16;
			RDW = 2;
			OD = 10;
			break;
		case 13:
			VDW = 18;
			RDW = 2;
			OD = 10;
			break;
		}
	} else {
		VDW = OutFreqMHz - 8; // VDW
		RDW = 3; // RDW
		OD = 4; // OD
	}

	switch (OD) { // change OD to SDW: s2:s1:s0
	case 0:
		SDW = 0;
		break;
	case 1:
		SDW = 0;
		break;
	case 2:
		SDW = 1;
		break;
	case 3:
		SDW = 6;
		break;
	case 4:
		SDW = 3;
		break;
	case 5:
		SDW = 4;
		break;
	case 6:
		SDW = 7;
		break;
	case 7:
		SDW = 4;
		break;
	case 8:
		SDW = 2;
		break;
	case 9:
		SDW = 0;
		break;
	case 10:
		SDW = 0;
		break;
	default:
		SDW = 0;
		break;
	}

	if (Device == 0) { // select device 1
		tmp = 0x20 | SDW;
		pci_write_reg((DEVICE0_ADDR_PARAM0) << 2, tmp); // write data 0
		tmp = (VDW << 7) & 0xff80 | RDW;
		pci_write_reg((DEVICE0_ADDR_PARAM1) << 2, tmp); // write data 1
		tmp = 1;
		pci_write_reg((DEVICE0_ADDR_COMMAND) << 2,
			      tmp); // write command set
		tmp = 0;
		pci_write_reg((DEVICE0_ADDR_COMMAND) << 2,
			      tmp); // write command reset
	} else { // select device 2
		tmp = 0x20 | SDW;
		pci_write_reg((DEVICE1_ADDR_PARAM0) << 2, tmp); // write data 0
		tmp = (VDW << 7) & 0xff80 | RDW;
		pci_write_reg((DEVICE1_ADDR_PARAM1) << 2, tmp); // write data 1
		tmp = 1;
		pci_write_reg((DEVICE1_ADDR_COMMAND) << 2,
			      tmp); // write command set
		tmp = 0;
		pci_write_reg((DEVICE1_ADDR_COMMAND) << 2,
			      tmp); // write command reset
	}
	return 1;
}

int hpi_set_timing_opt(unsigned long core_idx, void *base,
		       pthread_mutex_t io_mutex)
{
	int i;
	unsigned int iAddr;
	unsigned int uData;
	unsigned int uuData;
	int iTemp;
	int testFail;
#define MIX_L1_Y_SADDR (0x11000000 + 0x0138)
#define MIX_L1_CR_SADDR (0x11000000 + 0x0140)

	s_hpi_base = base;

	i = 2;
	// find HPI maximum timing register value
	do {
		i++;
		// iAddr = BIT_BASE + 0x100;
		iAddr = MIX_L1_Y_SADDR;
		uData = 0x12345678;
		testFail = 0;
		printf("HPI Tw, Tr value: %d\r", i);

		pci_write_reg(0x70 << 2, i);
		pci_write_reg(0x71 << 2, i);
		if (i < 15)
			pci_write_reg(0x72 << 2, 0);
		else
			pci_write_reg(0x72 << 2, i * 20 / 100);

		for (iTemp = 0; iTemp < 10000; iTemp++) {
			if (hpi_write_reg_limit(core_idx, iAddr, uData,
						io_mutex) == FALSE) {
				testFail = 1;
				break;
			}
			if (hpi_read_reg_limit(core_idx, iAddr, &uuData,
					       io_mutex) == FALSE) {
				testFail = 1;
				break;
			}
			if (uuData == uData) {
				if (hpi_write_reg_limit(core_idx, iAddr, 0,
							io_mutex) == FALSE) {
					testFail = 1;
					break;
				}
			} else {
				testFail = 1;
				break;
			}

			iAddr += 4;
			/*
			* if (iAddr == BIT_BASE + 0x200)
			* iAddr = BIT_BASE + 0x100;
			*/
			if (iAddr == MIX_L1_CR_SADDR)
				iAddr = MIX_L1_Y_SADDR;
			uData++;
		}
	} while (testFail && i < HPI_SET_TIMING_MAX);

	pci_write_reg(0x70 << 2, i);
	pci_write_reg(0x71 << 2, i + i * 40 / 100);
	pci_write_reg(0x72 << 2, i * 20 / 100);

	printf("\nOptimized HPI Tw value : %d\n", pci_read_reg(0x70 << 2));
	printf("Optimized HPI Tr value : %d\n", pci_read_reg(0x71 << 2));
	printf("Optimized HPI Te value : %d\n", pci_read_reg(0x72 << 2));

	return i;
}

void pci_write_reg(unsigned int addr, unsigned int data)
{
	unsigned long *reg_addr =
		(unsigned long *)(addr + (unsigned long)s_hpi_base);

	writel(data, reg_addr);
}

unsigned int pci_read_reg(unsigned int addr)
{
	unsigned long *reg_addr =
		(unsigned long *)(addr + (unsigned long)s_hpi_base);

	return readl(reg_addr);
}

void pci_read_memory(unsigned int addr, unsigned char *buf, int size)
{
	int status;
	int i, j, k;
	int data = 0;

	i = j = k = 0;

	for (i = 0; i < size / HPI_MAX_PKSIZE; i++) {
		pci_write_reg(HPI_ADDR_ADDR_H, (addr >> 16));
		pci_write_reg(HPI_ADDR_ADDR_L, (addr & 0xffff));

		pci_write_reg(HPI_ADDR_CMD, (((HPI_MAX_PKSIZE) << 4) + 1));

		do {
			status = 0;
			status = pci_read_reg(HPI_ADDR_STATUS);
			status = status & 1;
		} while (status == 0);

		for (j = 0; j < (HPI_MAX_PKSIZE >> 1); j++) {
			data = pci_read_reg(HPI_ADDR_DATA + j * 4);
			buf[k] = (data >> 8) & 0xFF;
			buf[k + 1] = data & 0xFF;
			k = k + 2;
		}

		addr += HPI_MAX_PKSIZE;
	}

	size = size % HPI_MAX_PKSIZE;

	if (((addr + size) & 0xFFFFFF00) != (addr & 0xFFFFFF00))
		size = size;

	if (size) {
		pci_write_reg(HPI_ADDR_ADDR_H, (addr >> 16));
		pci_write_reg(HPI_ADDR_ADDR_L, (addr & 0xffff));

		pci_write_reg(HPI_ADDR_CMD, (((size) << 4) + 1));

		do {
			status = 0;
			status = pci_read_reg(HPI_ADDR_STATUS);
			status = status & 1;

		} while (status == 0);

		for (j = 0; j < (size >> 1); j++) {
			data = pci_read_reg(HPI_ADDR_DATA + j * 4);
			buf[k] = (data >> 8) & 0xFF;
			buf[k + 1] = data & 0xFF;
			k = k + 2;
		}
	}
}

void pci_write_memory(unsigned int addr, unsigned char *buf, int size)
{
	int status;
	int i, j, k;
	int data = 0;

	i = j = k = 0;

	for (i = 0; i < size / HPI_MAX_PKSIZE; i++) {
		pci_write_reg(HPI_ADDR_ADDR_H, (addr >> 16));
		pci_write_reg(HPI_ADDR_ADDR_L, (addr & 0xffff));

		for (j = 0; j < (HPI_MAX_PKSIZE >> 1); j++) {
			data = (buf[k] << 8) | buf[k + 1];
			pci_write_reg(HPI_ADDR_DATA + j * 4, data);
			k = k + 2;
		}

		pci_write_reg(HPI_ADDR_CMD, (((HPI_MAX_PKSIZE) << 4) + 2));

		do {
			status = 0;
			status = pci_read_reg(HPI_ADDR_STATUS);
			status = (status >> 1) & 1;
		} while (status == 0);

		addr += HPI_MAX_PKSIZE;
	}

	size = size % HPI_MAX_PKSIZE;

	if (size) {
		pci_write_reg(HPI_ADDR_ADDR_H, (addr >> 16));
		pci_write_reg(HPI_ADDR_ADDR_L, (addr & 0xffff));

		for (j = 0; j < (size >> 1); j++) {
			data = (buf[k] << 8) | buf[k + 1];
			pci_write_reg(HPI_ADDR_DATA + j * 4, data);
			k = k + 2;
		}

		pci_write_reg(HPI_ADDR_CMD, (((size) << 4) + 2));

		do {
			status = 0;
			status = pci_read_reg(HPI_ADDR_STATUS);
			status = (status >> 1) & 1;

		} while (status == 0);
	}
}

int jdi_set_timing_opt(void)
{
	return 0;
}

int jdi_set_clock_freg(int Device, int OutFreqMHz, int InFreqMHz)
{
	return 0;
}
#endif

#endif
