#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/queue.h>
#include <pthread.h>
#include <stdatomic.h>
#include <inttypes.h>
#include <sys/mman.h>

#include <fcntl.h>		/* low-level i/o */
#include <unistd.h>
#include <sys/stat.h>
#include <linux/ion_cvitek.h>
#include <sys/ioctl.h>

#include "devmem.h"
#include "cvi_base.h"
#include "cvi_sys.h"
#include "hashmap.h"
#include <linux/cvi_base.h>
#include <linux/cvi_tpu_ioctl.h>
#include "sys_ioctl.h"


#define TPUDEVNAME "/dev/cvi-tpu0"

struct bind_t {
	TAILQ_ENTRY(bind_t) tailq;
	BIND_NODE_S *node;
};

TAILQ_HEAD(bind_head, bind_t) binds;
pthread_rwlock_t bind_lock;

static int devm_fd = -1, devm_cached_fd = -1;
static int ionFd = -1;
static Hashmap *ionHashmap = NULL;
static void *shared_mem;
static MMF_VERSION_S *mmf_version;
CVI_S32 *log_levels;
CVI_CHAR const *log_name[8] = {
	(CVI_CHAR *)"EMG", (CVI_CHAR *)"ALT", (CVI_CHAR *)"CRI", (CVI_CHAR *)"ERR",
	(CVI_CHAR *)"WRN", (CVI_CHAR *)"NOT", (CVI_CHAR *)"INF", (CVI_CHAR *)"DBG"
};

pthread_mutex_t tdma_pio_seq_lock = PTHREAD_MUTEX_INITIALIZER;
static uint32_t tdma_pio_seq;
static pthread_once_t lp_off_once = PTHREAD_ONCE_INIT;

static void _sys_low_power_off(void)
{
	// disable fab lowpower/slowdown
	// sys_ctrl clklp
	system("devmem 0x03002A50 32 0xFFFFFFFF");
	system("devmem 0x03002A60 32 0xFFFFFFFF");
	system("devmem 0x03002A00 32 0x003F5500");
	system("devmem 0x03002A04 32 0x003F5500");
	system("devmem 0x03002A0C 32 0x003F5500");
	system("devmem 0x03002A10 32 0x003F5500");
	system("devmem 0x03002A14 32 0x003F5500");
	system("devmem 0x03002A18 32 0x003F5500");
	system("devmem 0x03002A1C 32 0x003F5500");
	system("devmem 0x03002A20 32 0x003F5500");
	system("devmem 0x03002A24 32 0x003F5500");
	system("devmem 0x03002A28 32 0x003F5500");
	system("devmem 0x03002A2C 32 0x003F5500");
	system("devmem 0x03002A30 32 0x003F5500");
	// ddrsys clklp
	system("devmem 0x0800A014 32 0x00000FFF");
	// vipsys clklp
	system("devmem 0X0A0C8020 32 0x00000C01");
	system("devmem 0x0A0C8024 32 0x00000C01");
	system("devmem 0x0A0C8028 32 0x00000C01");
	system("devmem 0x0A0C802C 32 0x0C010C01");
	system("devmem 0x0A0C8038 32 0x001F0500");
	system("devmem 0x0A0C803C 32 0x001F0500");
	system("devmem 0x0A0C8040 32 0x001F0500");
	system("devmem 0x0A0C8044 32 0x001F0500");
	system("devmem 0x0A0C8048 32 0x001F0500");
	system("devmem 0x0A0C80C4 32 0x001F0500");
	system("devmem 0x0A0C80C8 32 0x001F0500");
	system("devmem 0x0A0C80CC 32 0x001F0500");
	// vc fab
	// system("devmem 0x0B030028 32 0x07");

	CVI_TRACE_SYS(CVI_DBG_INFO, "low_power_off\n");
}

static int _ion_hash_key(void *key)
{
	return (((uintptr_t)key) >> 10);
}

static bool _ion_hash_equals(void *keyA, void *keyB)
{
	return (keyA == keyB);
}

static CVI_S32 _SYS_MMAP(void)
{
	if (shared_mem != NULL) {
		CVI_TRACE_SYS(CVI_DBG_INFO, "already done mmap\n");
		return CVI_SUCCESS;
	}

	shared_mem = base_get_shm();
	if (shared_mem == NULL) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "base_get_shm failed!\n");
		return CVI_ERR_SYS_NOMEM;
	}

	log_levels = (CVI_S32 *)(shared_mem + BASE_LOG_LEVEL_OFFSET);

	mmf_version = (MMF_VERSION_S *)(shared_mem + BASE_VERSION_INFO_OFFSET);
	memset(mmf_version, 0, VERSION_INFO_RSV_SIZE);
	CVI_SYS_GetVersion(mmf_version);

	return CVI_SUCCESS;
}

static CVI_S32 _SYS_UNMMAP(void)
{
	if (shared_mem == NULL) {
		CVI_TRACE_SYS(CVI_DBG_INFO, "No need to unmap\n");
		return CVI_SUCCESS;
	}

	base_release_shm();
	shared_mem = NULL;
	log_levels = NULL;
	mmf_version = NULL;

	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_DevMem_Open(void)
{
	if (devm_fd < 0)
		devm_fd = devm_open();

	if (devm_cached_fd < 0)
		devm_cached_fd = devm_open_cached();

	if (devm_fd < 0 || devm_cached_fd < 0) {
		perror("devmem open failed\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_DevMem_Close(void)
{
	if (devm_fd < 0 || devm_cached_fd < 0)
		return CVI_SUCCESS;

	devm_close(devm_fd);
	devm_fd = -1;
	devm_close(devm_cached_fd);
	devm_cached_fd = -1;
	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_VI_Open(void)
{
	CVI_S32 s32ret = CVI_SUCCESS;

	s32ret = vi_dev_open();
	if (s32ret != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "base_vi_open failed\n");
		return CVI_ERR_SYS_NOTREADY;
	}

	return s32ret;
}

CVI_S32 CVI_SYS_VI_Close(void)
{
	CVI_S32 s32ret = CVI_SUCCESS;

	s32ret = vi_dev_close();
	if (s32ret != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "base_vi_close failed\n");
		return CVI_ERR_SYS_NOTREADY;
	}

	return s32ret;
}

CVI_S32 CVI_SYS_Init(void)
{
	CVI_S32 s32ret = CVI_SUCCESS, _sys_fd = -1;
	CVI_U32 sys_init = 0;
	VI_VPSS_MODE_S stVIVPSSMode;
	VPSS_MODE_S stVPSSMode;

	pthread_once(&lp_off_once, _sys_low_power_off);

	s32ret = base_dev_open();
	if (s32ret != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "base_dev_open failed\n");
		return CVI_ERR_SYS_NOTREADY;
	}

	s32ret = sys_dev_open();
	if (s32ret != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "sys_dev_open failed\n");
		return CVI_ERR_SYS_NOTREADY;
	}

	CVI_TRACE_SYS(CVI_DBG_INFO, "+\n");

	if (CVI_SYS_DevMem_Open() != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "devmem open failed.\n");
		return CVI_ERR_SYS_NOTREADY;
	}

	if (_SYS_MMAP() != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "_SYS_MMAP failed.\n");
		return CVI_ERR_SYS_NOMEM;
	}

	s32ret = vi_dev_open();
	if (s32ret != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_DEBUG, "base_vi_open failed\n");
		s32ret = CVI_SUCCESS;
		//If ko is not insmod, means no need this function.
		//return CVI_ERR_SYS_NOTREADY;
	}

	s32ret = vpss_dev_open();
	if (s32ret != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_DEBUG, "base_vpss_open failed\n");
		s32ret = CVI_SUCCESS;
		//If ko is not insmod, means no need this function.
		//return CVI_ERR_SYS_NOTREADY;
	}

#ifndef __SOC_PHOBOS__
	s32ret = vo_dev_open();
	if (s32ret != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_DEBUG, "base_vo_open failed\n");
		s32ret = CVI_SUCCESS;
		//If ko is not insmod, means no need this function.
		//return CVI_ERR_SYS_NOTREADY;
	}
#endif

	s32ret = dwa_dev_open();
	if (s32ret != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_DEBUG, "base_dwa_open failed\n");
		//If ko is not insmod, means no need this function.
		//return CVI_ERR_SYS_NOTREADY;
		s32ret = CVI_SUCCESS;
	}

	s32ret = rgn_dev_open();
	if (s32ret != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_DEBUG, "base_rgn_open failed\n");
		//If ko is not insmod, means no need this function.
		//return CVI_ERR_SYS_NOTREADY;
		s32ret = CVI_SUCCESS;
	}

#if 0 //TODO: need remove to vo module
	// check fb parameters to see if fb on vpss or vo.
	memset(&vo_ctx, 0, sizeof(vo_ctx));
	file = fopen("/sys/module/cvi_fb/parameters/option", "r");
	if (file == NULL)
		vo_ctx.fb_on_vpss = CVI_FALSE;
	else {
		CVI_S32 option = 0;

		fscanf(file, "%d", &option);
		vo_ctx.fb_on_vpss = option & BIT(1);
		fclose(file);
	}
#endif
	_sys_fd = get_sys_fd();
	sys_get_sys_init(_sys_fd, &sys_init);
	if (sys_init == 0) {
		for (CVI_U8 i = 0; i < VI_MAX_PIPE_NUM; ++i)
			stVIVPSSMode.aenMode[i] = VI_OFFLINE_VPSS_OFFLINE;
		CVI_SYS_SetVIVPSSMode(&stVIVPSSMode);
		stVPSSMode.enMode = VPSS_MODE_SINGLE;
		for (CVI_U8 i = 0; i < VPSS_IP_NUM; ++i)
			stVPSSMode.aenInput[i] = VPSS_INPUT_MEM;
		CVI_SYS_SetVPSSModeEx(&stVPSSMode);

		CVI_SYS_StartThermalThread();
	}

	sys_set_sys_init(_sys_fd);

	CVI_TRACE_SYS(CVI_DBG_INFO, "-\n");

	return s32ret;
}

CVI_S32 CVI_SYS_Exit(void)
{
	CVI_S32 s32ret = CVI_SUCCESS, _sys_fd = -1;
	CVI_U32 sys_init = 0;

	CVI_TRACE_SYS(CVI_DBG_INFO, "+\n");

	_sys_fd = get_sys_fd();
	sys_get_sys_init(_sys_fd, &sys_init);
	if (sys_init == 1) {
		CVI_SYS_StopThermalThread();
	}

	s32ret = vi_dev_close();
	if (s32ret != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "base_vi_close failed\n");
		return CVI_ERR_SYS_NOTREADY;
	}

	s32ret = vpss_dev_close();
	if (s32ret != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "base_vpss_close failed\n");
		return CVI_ERR_SYS_NOTREADY;
	}

#ifndef __SOC_PHOBOS__
	s32ret = vo_dev_close();
	if (s32ret != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "base_vo_close failed\n");
		return CVI_ERR_SYS_NOTREADY;
	}
#endif

	s32ret = dwa_dev_close();
	if (s32ret != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "base_dwa_close failed\n");
		return CVI_ERR_SYS_NOTREADY;
	}

	s32ret = rgn_dev_close();
	if (s32ret != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "base_rgn_close failed\n");
		return CVI_ERR_SYS_NOTREADY;
	}

	if (ionFd > 0) {
		close(ionFd);
		ionFd = -1;
	}

	_SYS_UNMMAP();
	s32ret = CVI_SYS_DevMem_Close();
	if (s32ret != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "devmem close failed\n");
		return CVI_ERR_SYS_NOTREADY;
	}
	sys_dev_close();
	base_dev_close();

	CVI_TRACE_SYS(CVI_DBG_INFO, "-\n");

	return s32ret;
}

CVI_S32 _CVI_SYS_BindIOCtl(const MMF_CHN_S *pstSrcChn, const MMF_CHN_S *pstDestChn, CVI_U8 is_bind)
{
	CVI_S32 fd = 0;
	CVI_S32 ret = 0;
	struct sys_bind_cfg bind_cfg;

	if ((fd = get_sys_fd()) == -1)
		return CVI_ERR_SYS_NOTREADY;

	memset(&bind_cfg, 0, sizeof(struct sys_bind_cfg));
	bind_cfg.is_bind = is_bind;
	bind_cfg.mmf_chn_src = *pstSrcChn;
	bind_cfg.mmf_chn_dst = *pstDestChn;

	ret = ioctl(fd, SYS_SET_BINDCFG, &bind_cfg);

	if (ret)
		CVI_TRACE_SYS(CVI_DBG_ERR, "_CVI_SYS_BindIOCtl()failed\n");

	return ret;
}

CVI_S32 CVI_SYS_Bind(const MMF_CHN_S *pstSrcChn, const MMF_CHN_S *pstDestChn)
{
#ifdef __SOC_PHOBOS__
	if (pstDestChn && (pstDestChn->enModId == CVI_ID_VO)) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "No vo device, vo cannot be bind!\n");
		return CVI_ERR_SYS_ILLEGAL_PARAM;
	}
#endif
	return _CVI_SYS_BindIOCtl(pstSrcChn, pstDestChn, 1);
}

CVI_S32 CVI_SYS_UnBind(const MMF_CHN_S *pstSrcChn, const MMF_CHN_S *pstDestChn)
{
#ifdef __SOC_PHOBOS__
	if (pstDestChn && (pstDestChn->enModId == CVI_ID_VO)) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "No vo device, cannot unbind vo!\n");
		return CVI_ERR_SYS_ILLEGAL_PARAM;
	}
#endif
	return _CVI_SYS_BindIOCtl(pstSrcChn, pstDestChn, 0);
}

CVI_S32 CVI_SYS_GetBindbyDest(const MMF_CHN_S *pstDestChn, MMF_CHN_S *pstSrcChn)
{
	CVI_S32 fd = 0;
	CVI_S32 ret = 0;
	struct sys_bind_cfg bind_cfg;

	if ((fd = get_sys_fd()) == -1)
		return CVI_ERR_SYS_NOTREADY;

	memset(&bind_cfg, 0, sizeof(struct sys_bind_cfg));
	bind_cfg.get_by_src = 0;
	bind_cfg.mmf_chn_dst = *pstDestChn;

	ret = ioctl(fd, SYS_GET_BINDCFG, &bind_cfg);

	if (ret) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "CVI_SYS_GetBindbyDest() failed\n");
		return ret;
	}

	memcpy(pstSrcChn, &bind_cfg.mmf_chn_src, sizeof(MMF_CHN_S));
	return CVI_SUCCESS;

}

CVI_S32 CVI_SYS_GetBindbySrc(const MMF_CHN_S *pstSrcChn, MMF_BIND_DEST_S *pstBindDest)
{
	CVI_S32 fd = 0;
	CVI_S32 ret = 0;
	struct sys_bind_cfg bind_cfg;

	if ((fd = get_sys_fd()) == -1)
		return CVI_ERR_SYS_NOTREADY;

	memset(&bind_cfg, 0, sizeof(struct sys_bind_cfg));
	bind_cfg.get_by_src = 1;
	bind_cfg.mmf_chn_src = *pstSrcChn;

	ret = ioctl(fd, SYS_GET_BINDCFG, &bind_cfg);

	if (ret) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "CVI_SYS_GetBindbySrc() failed\n");
		return ret;
	}

	memcpy(pstBindDest, &bind_cfg.bind_dst, sizeof(MMF_BIND_DEST_S));
	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_GetVersion(MMF_VERSION_S *pstVersion)
{
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pstVersion);

#ifndef MMF_VERSION
#define MMF_VERSION  (CVI_CHIP_NAME MMF_VER_PRIX MK_VERSION(VER_X, VER_Y, VER_Z) VER_D)
#endif
	snprintf(pstVersion->version, VERSION_NAME_MAXLEN, "%s-%s", MMF_VERSION, SDK_VER);
	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_GetChipId(CVI_U32 *pu32ChipId)
{
	static CVI_U32 id = 0xffffffff;
	int fd;

	if (id == 0xffffffff) {
		CVI_U32 tmp = 0;

		fd = get_base_fd();
		if (fd == -1) {
			CVI_TRACE_SYS(CVI_DBG_ERR, "Can't open device, cvi-base.\n");
			return CVI_ERR_SYS_NOTREADY;
		}

		if (ioctl(fd, IOCTL_READ_CHIP_ID, &tmp) < 0) {
			CVI_TRACE_SYS(CVI_DBG_ERR, "ioctl IOCTL_READ_CHIP_ID failed\n");
			return CVI_FAILURE;
		}

		id = tmp;
	}

	*pu32ChipId = id;
	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_GetPowerOnReason(CVI_U32 *pu32PowerOnReason)
{
	int fd;
	CVI_U32 ret_val = 0x0;
	CVI_U32 reason = 0x0;

	fd = get_base_fd();
	if (fd == -1) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "Can't open device, cvi-base.\n");
		return CVI_ERR_SYS_NOTREADY;
	}

	if (ioctl(fd, IOCTL_READ_CHIP_PWR_ON_REASON, &reason) < 0) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "IOCTL_READ_CHIP_PWR_ON_REASON failed\n");
		return CVI_FAILURE;
	}

	switch (reason) {
	case E_CHIP_PWR_ON_COLDBOOT:
		ret_val = CVI_COLDBOOT;
	break;
	case E_CHIP_PWR_ON_WDT:
		ret_val = CVI_WDTBOOT;
	break;
	case E_CHIP_PWR_ON_SUSPEND:
		ret_val = CVI_SUSPENDBOOT;
	break;
	case E_CHIP_PWR_ON_WARM_RST:
		ret_val = CVI_WARMBOOT;
	break;
	default:
		CVI_TRACE_SYS(CVI_DBG_ERR, "unknown reason (%#x)\n", reason);
		return CVI_ERR_SYS_NOT_PERM;
	break;
	}

	*pu32PowerOnReason = ret_val;
	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_GetChipVersion(CVI_U32 *pu32ChipVersion)
{
	static CVI_U32 version = 0xffffffff;
	int fd;

	if (version == 0xffffffff) {
		CVI_U32 tmp = 0;

		fd = get_base_fd();
		if (fd == -1) {
			CVI_TRACE_SYS(CVI_DBG_ERR, "Can't open device, cvi-base.\n");
			return CVI_ERR_SYS_NOTREADY;
		}

		if (ioctl(fd, IOCTL_READ_CHIP_VERSION, &tmp) < 0) {
			CVI_TRACE_SYS(CVI_DBG_ERR, "ioctl IOCTL_READ_CHIP_VERSION failed\n");
			return CVI_FAILURE;
		}

		switch (tmp) {
		case E_CHIPVERSION_U01:
			version = CVIU01;
		break;
		case E_CHIPVERSION_U02:
			version = CVIU02;
		break;
		default:
			CVI_TRACE_SYS(CVI_DBG_ERR, "unknown version(%#x)\n", tmp);
			return CVI_ERR_SYS_NOT_PERM;
		break;
		}
	}

	*pu32ChipVersion = version;
	return CVI_SUCCESS;
}

void *CVI_SYS_Mmap(CVI_U64 u64PhyAddr, CVI_U32 u32Size)
{
	CVI_SYS_DevMem_Open();

	return devm_map(devm_fd, u64PhyAddr, u32Size);
}

/* CVI_SYS_MmapCache - mmap the physical address to cached virtual-address
 *
 * @param pu64PhyAddr: the phy-address of the buffer
 * @param u32Size: the length of the buffer
 * @return virtual-address if success; 0 if fail.
 */
void *CVI_SYS_MmapCache(CVI_U64 u64PhyAddr, CVI_U32 u32Size)
{
	CVI_SYS_DevMem_Open();

	void *addr = devm_map(devm_cached_fd, u64PhyAddr, u32Size);

	if (addr)
		CVI_SYS_IonInvalidateCache(u64PhyAddr, addr, u32Size);
	return addr;
}

CVI_S32 CVI_SYS_Munmap(void *pVirAddr, CVI_U32 u32Size)
{
	devm_unmap(pVirAddr, u32Size);
	return CVI_SUCCESS;
}

int ionMalloc(int devFd, struct sys_ion_data *para, bool isCache)
{
	CVI_S32 ret;

	para->cached = isCache;
	ret = ioctl(devFd, SYS_ION_ALLOC, para);
	if (ret < 0) {
		printf("ioctl SYS_ION_ALLOC failed\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

int ionFree(struct sys_ion_data *para)
{
	CVI_S32 fd = -1;
	CVI_S32 ret = CVI_SUCCESS;

	if ((fd = get_sys_fd()) == -1)
		return CVI_ERR_SYS_NOTREADY;

	ret = ioctl(fd, SYS_ION_FREE, para);
	if (ret < 0) {
		printf("ioctl SYS_ION_ALLOC failed\n");
	}

	return CVI_SUCCESS;
}

static CVI_S32 _SYS_IonAlloc(CVI_U64 *pu64PhyAddr, CVI_VOID **ppVirAddr,
			     CVI_U32 u32Len, CVI_BOOL cached, const CVI_CHAR *name)
{
	CVI_S32 fd = -1;
	struct sys_ion_data *ion_data;

	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pu64PhyAddr);

	if ((fd = get_sys_fd()) == -1)
		return CVI_ERR_SYS_NOTREADY;

	if (!ionHashmap)
		ionHashmap = hashmapCreate(20, _ion_hash_key, _ion_hash_equals);

	ion_data = malloc(sizeof(*ion_data));
	ion_data->size = u32Len;
	// Set buffer as "anonymous" when user is passing null pointer.
	if (name)
		strncpy((char *)(ion_data->name), name, MAX_ION_BUFFER_NAME);
	else
		strncpy((char *)(ion_data->name), "anonymous", MAX_ION_BUFFER_NAME);

	if (ionMalloc(fd, ion_data, cached) != CVI_SUCCESS) {
		free(ion_data);
		CVI_TRACE_SYS(CVI_DBG_ERR, "alloc failed.\n");
		return CVI_ERR_SYS_NOMEM;
	}

	*pu64PhyAddr = ion_data->addr_p;
	hashmapPut(ionHashmap, (void *)(uintptr_t)*pu64PhyAddr, ion_data);

	if (ppVirAddr) {
		if (cached)
			*ppVirAddr = CVI_SYS_MmapCache(*pu64PhyAddr, u32Len);
		else
			*ppVirAddr = CVI_SYS_Mmap(*pu64PhyAddr, u32Len);
		if (*ppVirAddr == NULL) {
			hashmapGet(ionHashmap, (void *)(uintptr_t)*pu64PhyAddr);
			ionFree(ion_data);
			free(ion_data);
			CVI_TRACE_SYS(CVI_DBG_ERR, "mmap failed. (%s)\n", strerror(errno));
			return CVI_ERR_SYS_REMAPPING;
		}
	}
	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_IonAlloc(CVI_U64 *pu64PhyAddr, CVI_VOID **ppVirAddr, const CVI_CHAR *strName, CVI_U32 u32Len)
{
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pu64PhyAddr);

	return _SYS_IonAlloc(pu64PhyAddr, ppVirAddr, u32Len, CVI_FALSE, strName);
}

/* CVI_SYS_IonAlloc_Cached - acquire buffer of u32Len from ion
 *
 * @param pu64PhyAddr: the phy-address of the buffer
 * @param ppVirAddr: the cached vir-address of the buffer
 * @param strName: the name of the buffer
 * @param u32Len: the length of the buffer acquire
 * @return CVI_SUCCES if ok
 */
CVI_S32 CVI_SYS_IonAlloc_Cached(CVI_U64 *pu64PhyAddr, CVI_VOID **ppVirAddr,
				 const CVI_CHAR *strName, CVI_U32 u32Len)
{
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pu64PhyAddr);
	return _SYS_IonAlloc(pu64PhyAddr, ppVirAddr, u32Len, CVI_TRUE, strName);
}

CVI_S32 CVI_SYS_IonFree(CVI_U64 u64PhyAddr, CVI_VOID *pVirAddr)
{
	struct sys_ion_data *ion_data;

	if (ionHashmap == NULL) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "ion not alloc before.\n");
		return CVI_ERR_SYS_NOTREADY;
	}

	ion_data = hashmapGet(ionHashmap, (void *)(uintptr_t)u64PhyAddr);
	if (ion_data == NULL) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "u64PhyAddr(0x%"PRIx64") not found in ion.\n", u64PhyAddr);
		return CVI_ERR_SYS_ILLEGAL_PARAM;
	}
	hashmapRemove(ionHashmap, (void *)(uintptr_t)u64PhyAddr);
	if (pVirAddr)
		devm_unmap(pVirAddr, ion_data->size);
	ionFree(ion_data);
	free(ion_data);

	if (hashmapSize(ionHashmap) == 0) {
		hashmapFree(ionHashmap);
		ionHashmap = NULL;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_IonFlushCache(CVI_U64 u64PhyAddr, CVI_VOID *pVirAddr, CVI_U32 u32Len)
{
	CVI_S32 fd = -1;
	CVI_S32 ret = CVI_SUCCESS;
	struct sys_cache_op cache_cfg;

	if ((fd = get_sys_fd()) == -1)
		return CVI_ERR_SYS_NOTREADY;

	if (pVirAddr == NULL) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "pVirAddr Null.\n");
		return CVI_ERR_SYS_NULL_PTR;
	}

	cache_cfg.addr_p = u64PhyAddr;
	cache_cfg.addr_v = pVirAddr;
	cache_cfg.size = u32Len;

	ret = ioctl(fd, SYS_CACHE_FLUSH, &cache_cfg);
	if (ret < 0) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "ion flush err.\n");
		ret = CVI_ERR_SYS_NOTREADY;
	}
	return ret;
}

CVI_S32 CVI_SYS_IonInvalidateCache(CVI_U64 u64PhyAddr, CVI_VOID *pVirAddr, CVI_U32 u32Len)
{
	CVI_S32 fd = -1;
	CVI_S32 ret = CVI_SUCCESS;
	struct sys_cache_op cache_cfg;

	if ((fd = get_sys_fd()) == -1)
		return CVI_ERR_SYS_NOTREADY;

	if (pVirAddr == NULL) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "pVirAddr Null.\n");
		return CVI_ERR_SYS_NULL_PTR;
	}

	cache_cfg.addr_p = u64PhyAddr;
	cache_cfg.addr_v = pVirAddr;
	cache_cfg.size = u32Len;

	ret = ioctl(fd, SYS_CACHE_INVLD, &cache_cfg);
	if (ret < 0) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "ion flush err.\n");
		ret = CVI_ERR_SYS_NOTREADY;
	}
	return ret;
}

CVI_S32 CVI_SYS_IonGetFd(CVI_VOID)
{
	if (ionFd < 0) {
		ionFd = open("/dev/ion", O_RDWR | O_DSYNC);
		if (ionFd < 0) {
			CVI_TRACE_SYS(CVI_DBG_ERR, "ion fd open failed.\n");
			return -1;
		}
		ionHashmap = hashmapCreate(20, _ion_hash_key, _ion_hash_equals);
	}
	return ionFd;
}

CVI_S32 CVI_SYS_SetVIVPSSMode(const VI_VPSS_MODE_S *pstVIVPSSMode)
{
	CVI_S32 fd = 0;

	if ((fd = get_sys_fd()) == -1)
		return CVI_ERR_SYS_NOTREADY;

	return sys_set_vivpssmode(fd, pstVIVPSSMode);
}

CVI_S32 CVI_SYS_GetVIVPSSMode(VI_VPSS_MODE_S *pstVIVPSSMode)
{
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pstVIVPSSMode);
	CVI_S32 fd = 0;

	if ((fd = get_sys_fd()) == -1)
		return CVI_ERR_SYS_NOTREADY;

	return sys_get_vivpssmode(fd, pstVIVPSSMode);
}

CVI_S32 CVI_SYS_SetVPSSMode(VPSS_MODE_E enVPSSMode)
{
	CVI_S32 fd = 0;

	if ((fd = get_sys_fd()) == -1)
		return CVI_ERR_SYS_NOTREADY;

	return sys_set_vpssmode(fd, enVPSSMode);
}

VPSS_MODE_E CVI_SYS_GetVPSSMode(void)
{
	CVI_S32 fd = 0;
	VPSS_MODE_E enMode;

	if ((fd = get_sys_fd()) == -1)
		return CVI_ERR_SYS_NOTREADY;

	sys_get_vpssmode(fd, &enMode);

	return enMode;
}

CVI_S32 CVI_SYS_SetVPSSModeEx(const VPSS_MODE_S *pstVPSSMode)
{
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pstVPSSMode);
	CVI_S32 fd = 0;

	if ((fd = get_sys_fd()) == -1)
		return CVI_ERR_SYS_NOTREADY;

	return sys_set_vpssmodeex(fd, pstVPSSMode);
}

CVI_S32 CVI_SYS_GetVPSSModeEx(VPSS_MODE_S *pstVPSSMode)
{
	CVI_S32 fd = 0;
	VPSS_MODE_S vpss_mode;

	if ((fd = get_sys_fd()) == -1)
		return CVI_ERR_SYS_NOTREADY;

	sys_get_vpssmodeex(fd, &vpss_mode);

	*pstVPSSMode = vpss_mode;

	return CVI_SUCCESS;
}

const CVI_CHAR *CVI_SYS_GetModName(MOD_ID_E id)
{
	return CVI_GET_MOD_NAME(id);
}

CVI_S32 CVI_LOG_SetLevelConf(LOG_LEVEL_CONF_S *pstConf)
{
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pstConf);

	if (pstConf->enModId >= CVI_ID_BUTT) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "Invalid ModId(%d)\n", pstConf->enModId);
		return CVI_ERR_SYS_ILLEGAL_PARAM;
	}

	if (_SYS_MMAP() != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "_SYS_MMAP failed.\n");
		return CVI_FAILURE;
	}

	log_levels[pstConf->enModId] = pstConf->s32Level;
	return CVI_SUCCESS;
}

CVI_S32 CVI_LOG_GetLevelConf(LOG_LEVEL_CONF_S *pstConf)
{
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pstConf);

	if (pstConf->enModId >= CVI_ID_BUTT) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "Invalid ModId(%d)\n", pstConf->enModId);
		return CVI_ERR_SYS_ILLEGAL_PARAM;
	}

	if (_SYS_MMAP() != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "_SYS_MMAP failed.\n");
		return CVI_FAILURE;
	}

	pstConf->s32Level = log_levels[pstConf->enModId];
	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_GetCurPTS(CVI_U64 *pu64CurPTS)
{
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pu64CurPTS);

	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	*pu64CurPTS = ts.tv_sec*1000000 + ts.tv_nsec/1000;

	return CVI_SUCCESS;

}

CVI_S32 CVI_SYS_TDMACopy(CVI_U64 u64PhyDst, CVI_U64 u64PhySrc, CVI_U32 u32Len)
{
#define TDMA2D_LEN_LIMIT 0xFFFFFFFF

	static int tpu_fd = -1;
	struct cvi_tdma_copy_arg tdma_ioctl;
	struct cvi_tdma_wait_arg wait_ioctl;

	if (u32Len >= TDMA2D_LEN_LIMIT) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "CVI_SYS_TDMACopy() input param can't be supported\n");
		return CVI_ERR_SYS_NOT_SUPPORT;
	}

	tpu_fd = open(TPUDEVNAME, O_RDWR | O_DSYNC);
	if (tpu_fd < 0) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "tpu fd open failed.\n");
		return CVI_ERR_SYS_NOTREADY;
	}

	memset(&tdma_ioctl, 0, sizeof(struct cvi_tdma_copy_arg));

	pthread_mutex_lock(&tdma_pio_seq_lock);
	tdma_pio_seq++;
	tdma_ioctl.seq_no = tdma_pio_seq;
	pthread_mutex_unlock(&tdma_pio_seq_lock);

	tdma_ioctl.paddr_src = u64PhySrc;
	tdma_ioctl.paddr_dst = u64PhyDst;
	tdma_ioctl.leng_bytes = u32Len;
	ioctl(tpu_fd, CVITPU_SUBMIT_PIO, &tdma_ioctl);

	//wait finished
	wait_ioctl.seq_no = tdma_ioctl.seq_no;
	ioctl(tpu_fd, CVITPU_WAIT_PIO, &wait_ioctl);

	if (wait_ioctl.ret)
		CVI_TRACE_SYS(CVI_DBG_ERR, "CVI_SYS_TDMACopy wait failed\n");

	close(tpu_fd);
	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_TDMACopy2D(CVI_TDMA_2D_S *param)
{
#define TDMA2D_W_LIMIT 0x10000
#define TDMA2D_H_LIMIT 0x10000

	static int tpu_fd = -1;
	struct cvi_tdma_copy_arg tdma_ioctl;
	struct cvi_tdma_wait_arg wait_ioctl;

	if (param->stride_bytes_src < param->w_bytes ||
			param->stride_bytes_dst < param->w_bytes ||
			param->w_bytes >= TDMA2D_W_LIMIT ||
			param->h >= TDMA2D_H_LIMIT) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "CVI_SYS_TDMACopy2D() input param can't be supported\n");
		return CVI_ERR_SYS_NOT_SUPPORT;
	}

	tpu_fd = open(TPUDEVNAME, O_RDWR | O_DSYNC);
	if (tpu_fd < 0) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "tpu fd open failed.\n");
		return CVI_ERR_SYS_NOTREADY;
	}

	memset(&tdma_ioctl, 0, sizeof(struct cvi_tdma_copy_arg));

	pthread_mutex_lock(&tdma_pio_seq_lock);
	tdma_pio_seq++;
	tdma_ioctl.seq_no = tdma_pio_seq;
	pthread_mutex_unlock(&tdma_pio_seq_lock);

	tdma_ioctl.enable_2d = 1;
	tdma_ioctl.paddr_src = param->paddr_src;
	tdma_ioctl.paddr_dst = param->paddr_dst;
	tdma_ioctl.h = param->h;
	tdma_ioctl.w_bytes = param->w_bytes;
	tdma_ioctl.stride_bytes_src = param->stride_bytes_src;
	tdma_ioctl.stride_bytes_dst = param->stride_bytes_dst;
	ioctl(tpu_fd, CVITPU_SUBMIT_PIO, &tdma_ioctl);

	//wait finished
	wait_ioctl.seq_no = tdma_ioctl.seq_no;
	ioctl(tpu_fd, CVITPU_WAIT_PIO, &wait_ioctl);

	if (wait_ioctl.ret)
		CVI_TRACE_SYS(CVI_DBG_ERR, "CVI_SYS_TDMACopy2D wait failed\n");

	close(tpu_fd);
	return CVI_SUCCESS;
}

