//#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>
//#include <errno.h>
#include "xil_types.h"
#include "FreeRTOS_POSIX.h"
#include <linux/errno.h>
#include <FreeRTOS_POSIX/errno.h>
//#include <sys/queue.h>
#include <linux/queue.h>
//#include <pthread.h>
#include "FreeRTOS_POSIX/pthread.h"

#include <stdatomic.h>
//#include <inttypes.h>
//#include <sys/mman.h>
#include <linux/mman.h>

//#include <fcntl.h>		/* low-level i/o */
#include <linux/fcntl.h>		/* low-level i/o */
//#include <unistd.h>
//#include <sys/stat.h>
//#include <sys/ioctl.h>
#include <linux/ioctl.h>
//#include <linux/ion_cvitek.h>


#include "devmem.h"
#include "cvi_base.h"
#include "cvi_sys.h"
//#include "ioctl_vio.h"
#include "hashmap.h"
#include <cvi_base.h>
#include "cvi_rpc.h"


#define IS_AUDIO_BINDINGS(x) ((x == CVI_ID_AIO) || (x == CVI_ID_AI) || (x == CVI_ID_AO)                  \
	|| (x == CVI_ID_AENC) || (x == CVI_ID_ADEC))
#define IS_VIDEO_BINDINGS(x) ((x == CVI_ID_VI) || (x == CVI_ID_VO) || (x == CVI_ID_VPSS)                 \
	|| (x == CVI_ID_VENC) || (x == CVI_ID_VDEC))

struct bind_t {
	TAILQ_ENTRY(bind_t) tailq;
	BIND_NODE_S *node;
};

TAILQ_HEAD(bind_head, bind_t) binds;
//poshiun
//pthread_rwlock_t bind_lock;

static atomic_bool sys_inited = ATOMIC_VAR_INIT(false);
static int devm_fd = -1, devm_cached_fd = -1;
static int ionFd = -1;
static Hashmap *ionHashmap;
static void *shared_mem;
static BIND_NODE_S *bind_nodes;
static MMF_VERSION_S *mmf_version;

VI_VPSS_MODE_S stVIVPSSMode;
VPSS_MODE_S stVPSSMode;
CVI_S32 *log_levels;
CVI_CHAR *log_name[8] = {"EMG", "ALT", "CRI", "ERR", "WRN", "NOT", "INF", "DBG"};

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
		return CVI_ERR_VB_NOMEM;
	}

	log_levels = (CVI_S32 *)(shared_mem + BASE_LOG_LEVEL_OFFSET);

	bind_nodes = (BIND_NODE_S *)(shared_mem + BASE_BIND_INFO_OFFSET);
	memset(bind_nodes, 0, BIND_INFO_RSV_SIZE);

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
	bind_nodes = NULL;
	mmf_version = NULL;

	return CVI_SUCCESS;
}


CVI_S32 CVI_SYS_VI_Open(void)
{
	CVI_S32 s32ret = CVI_SUCCESS;

	s32ret = v4l2_isp_open();

	return s32ret;
}

CVI_S32 CVI_SYS_VI_Close(void)
{
	CVI_S32 s32ret = CVI_SUCCESS;

	s32ret = v4l2_isp_close();

	return s32ret;
}

CVI_S32 CVI_SYS_Init(void)
{
	bool expect = false;

	// Only init once until exit.
	if (!atomic_compare_exchange_strong(&sys_inited, &expect, true))
		return CVI_SUCCESS;

	TAILQ_INIT(&binds);
	//poshiun
	//pthread_rwlock_init(&bind_lock, NULL);

	devm_fd = devm_open();
	if (devm_fd < 0)
		return -1;
	devm_cached_fd = devm_open_cached();
	if (devm_cached_fd < 0)
		return -1;

	if (_SYS_MMAP() != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "_SYS_MMAP failed.\n");
		return -1;
	}

	if (v4l2_dev_open() != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "dev open failed\n");
		return -EIO;
	}

	if (init_vpss_device() != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "init vpss failed.\n");
		return -EIO;
	}

	if (init_disp_device() != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "init disp failed.\n");
		return -EIO;
	}

	if (init_dwa_device() != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "init dwa failed.\n");
		return -EIO;
	}

	if (init_isp_device() != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "init isp failed.\n");
		return -EIO;
	}

	memset(&vo_ctx, 0, sizeof(vo_ctx));

	for (CVI_U8 i = 0; i < VI_MAX_PIPE_NUM; ++i)
		stVIVPSSMode.aenMode[i] = VI_OFFLINE_VPSS_OFFLINE;
	CVI_SYS_SetVIVPSSMode(&stVIVPSSMode);
	stVPSSMode.enMode = VPSS_MODE_SINGLE;
	for (CVI_U8 i = 0; i < VPSS_IP_NUM; ++i)
		stVPSSMode.aenInput[i] = VPSS_INPUT_MEM;
	CVI_SYS_SetVPSSModeEx(&stVPSSMode);

	CVI_SYS_StartThermalThread();

	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_Exit(void)
{
	struct bind_t *item, *item_tmp;
	bool expect = true;

	// Only exit once.
	if (!atomic_compare_exchange_strong(&sys_inited, &expect, false))
		return CVI_SUCCESS;

	CVI_SYS_StopThermalThread();

	v4l2_dev_close();
	//poshiun
	//pthread_rwlock_wrlock(&bind_lock);
	TAILQ_FOREACH_SAFE(item, &binds, tailq, item_tmp) {
		TAILQ_REMOVE(&binds, item, tailq);
		free(item);
	}
	memset(bind_nodes, 0, BIND_INFO_RSV_SIZE);
	//poshiun
	//pthread_rwlock_unlock(&bind_lock);
	//pthread_rwlock_destroy(&bind_lock);


	if (ionFd > 0) {
		close(ionFd);
		ionFd = -1;
	}

	_SYS_UNMMAP();
	devm_close(devm_fd);
	devm_close(devm_cached_fd);
	return 0;
}

static CVI_S32 _checkVcBind(MOD_ID_E srcId, MOD_ID_E dstId)
{
	if (srcId == CVI_ID_VENC ||
		dstId == CVI_ID_VDEC) {
		if (srcId == CVI_ID_VENC) {
			CVI_TRACE_SYS(CVI_DBG_ERR, "VENC does not support bind-mode source.\n");
		} else {
			CVI_TRACE_SYS(CVI_DBG_ERR, "VDEC does not support bind-mode destination.\n");
		}

		return -1;
	}

	return 0;
}

CVI_S32 CVI_SYS_Bind(const MMF_CHN_S *pstSrcChn, const MMF_CHN_S *pstDestChn)
{
//poshiun
#if 0
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pstSrcChn);
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pstDestChn);
#ifdef RPC_MULTI_PROCESS
	if (!isMaster()) {
		return rpc_client_sys_bind(pstSrcChn, pstDestChn);
	}
#endif
	struct bind_t *item, *item_tmp;
	CVI_S32 ret;

	if (pstSrcChn->enModId == CVI_ID_VI) {
		struct vdev *d = get_dev_info(VDEV_TYPE_ISP, 0);

		if (d->is_online) {
			CVI_TRACE_SYS(CVI_DBG_WARN, "Opeartion doesn't support if online\n");
			return CVI_SUCCESS;
		}
	}

	ret = _checkVcBind(pstSrcChn->enModId, pstDestChn->enModId);
	if (ret < 0) {
		return CVI_ERR_SYS_NOT_SUPPORT;
	}

	pthread_rwlock_wrlock(&bind_lock);
	TAILQ_FOREACH_SAFE(item, &binds, tailq, item_tmp) {
		if (!CHN_MATCH(&item->node->src, pstSrcChn))
			continue;

		// check if dst already bind to src
		for (CVI_U8 i = 0; i < item->node->dsts.u32Num; ++i) {
			if (CHN_MATCH(&item->node->dsts.astMmfChn[i], pstDestChn)) {
				CVI_TRACE_SYS(CVI_DBG_ERR, "Duplicate Dst(%d-%d-%d) to Src(%d-%d-%d)\n",
					pstDestChn->enModId, pstDestChn->s32DevId, pstDestChn->s32ChnId,
					pstSrcChn->enModId, pstSrcChn->s32DevId, pstSrcChn->s32ChnId);
				ret = CVI_ERR_SYS_ILLEGAL_PARAM;
				goto BIND_EXIT;
			}
		}
		// check if dsts have enough space for one more bind
		if (item->node->dsts.u32Num >= BIND_DEST_MAXNUM) {
			CVI_TRACE_SYS(CVI_DBG_ERR, "Over max bind Dst number\n");
			ret = CVI_ERR_SYS_NOMEM;
			goto BIND_EXIT;
		}
		item->node->dsts.astMmfChn[item->node->dsts.u32Num++] = *pstDestChn;

		goto BIND_SUCCESS;
	}

	int i;
	// if src not found
	for (i = 0; i < BIND_NODE_MAXNUM; ++i) {
		if (!bind_nodes[i].bUsed) {
			memset(&bind_nodes[i], 0, sizeof(bind_nodes[i]));
			bind_nodes[i].bUsed = true;
			bind_nodes[i].src = *pstSrcChn;
			bind_nodes[i].dsts.u32Num = 1;
			bind_nodes[i].dsts.astMmfChn[0] = *pstDestChn;
			break;
		}
	}

	if (i == BIND_NODE_MAXNUM) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "No free bind node\n");
		ret = CVI_ERR_SYS_NOMEM;
		goto BIND_EXIT;
	}

	item = malloc(sizeof(*item));
	if (item == NULL) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "malloc for new bind failed\n");
		memset(&bind_nodes[i], 0, sizeof(bind_nodes[i]));
		ret = CVI_ERR_SYS_NOMEM;
		goto BIND_EXIT;
	}

	item->node = &bind_nodes[i];
	TAILQ_INSERT_TAIL(&binds, item, tailq);

BIND_SUCCESS:
	ret = CVI_SUCCESS;

	if (pstDestChn->enModId == CVI_ID_VENC)
		venc_vb_ctx[pstDestChn->s32ChnId].enable_bind_mode = CVI_TRUE;
	else if (pstSrcChn->enModId == CVI_ID_VDEC)
		vdec_vb_ctx[pstSrcChn->s32ChnId].enable_bind_mode = CVI_TRUE;

BIND_EXIT:
	pthread_rwlock_unlock(&bind_lock);
	return ret;
#endif
}

CVI_S32 CVI_SYS_UnBind(const MMF_CHN_S *pstSrcChn, const MMF_CHN_S *pstDestChn)
{
//poshiun
#if 0
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pstSrcChn);
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pstDestChn);
#ifdef RPC_MULTI_PROCESS
	if (!isMaster()) {
		return rpc_client_sys_unbind(pstSrcChn, pstDestChn);
	}
#endif
	struct bind_t *item, *item_tmp;
	CVI_S32 ret;

	ret = _checkVcBind(pstSrcChn->enModId, pstDestChn->enModId);
	if (ret < 0) {
		return CVI_ERR_SYS_NOT_SUPPORT;
	}

	pthread_rwlock_wrlock(&bind_lock);
	TAILQ_FOREACH_SAFE(item, &binds, tailq, item_tmp) {
		if (!CHN_MATCH(&item->node->src, pstSrcChn))
			continue;

		for (CVI_U8 i = 0; i < item->node->dsts.u32Num; ++i) {
			if (CHN_MATCH(&item->node->dsts.astMmfChn[i], pstDestChn)) {
				if (--item->node->dsts.u32Num) {
					for (; i < item->node->dsts.u32Num; i++)
						item->node->dsts.astMmfChn[i] = item->node->dsts.astMmfChn[i + 1];
				}

				if (pstDestChn->enModId == CVI_ID_VENC)
					venc_vb_ctx[pstDestChn->s32ChnId].enable_bind_mode = CVI_FALSE;
				else if (pstSrcChn->enModId == CVI_ID_VDEC)
					vdec_vb_ctx[pstSrcChn->s32ChnId].enable_bind_mode = CVI_FALSE;

				pthread_rwlock_unlock(&bind_lock);
				return CVI_SUCCESS;
			}
		}
	}
	pthread_rwlock_unlock(&bind_lock);
	return CVI_SUCCESS;
#endif
}

CVI_S32 CVI_SYS_GetBindbyDest(const MMF_CHN_S *pstDestChn, MMF_CHN_S *pstSrcChn)
{
//poshiun
#if 0
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pstSrcChn);
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pstDestChn);

	struct bind_t *item, *item_tmp;

	pthread_rwlock_rdlock(&bind_lock);
	TAILQ_FOREACH_SAFE(item, &binds, tailq, item_tmp) {
		for (CVI_U8 i = 0; i < item->node->dsts.u32Num; ++i) {
			if (CHN_MATCH(&item->node->dsts.astMmfChn[i], pstDestChn)) {
				*pstSrcChn = item->node->src;
				pthread_rwlock_unlock(&bind_lock);
				return CVI_SUCCESS;
			}
		}
	}
	pthread_rwlock_unlock(&bind_lock);
#endif
	return CVI_FAILURE;
}

CVI_S32 CVI_SYS_GetBindbySrc(const MMF_CHN_S *pstSrcChn, MMF_BIND_DEST_S *pstBindDest)
{
//poshiun
#if 0
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pstSrcChn);
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pstBindDest);

	struct bind_t *item, *item_tmp;

	pthread_rwlock_rdlock(&bind_lock);
	TAILQ_FOREACH_SAFE(item, &binds, tailq, item_tmp) {
		for (CVI_U8 i = 0; i < item->node->dsts.u32Num; ++i) {
			if (CHN_MATCH(&item->node->src, pstSrcChn)) {
				*pstBindDest = item->node->dsts;
				pthread_rwlock_unlock(&bind_lock);
				return CVI_SUCCESS;
			}
		}
	}
	pthread_rwlock_unlock(&bind_lock);
#endif
	return CVI_FAILURE;
}

CVI_S32 CVI_SYS_GetVersion(MMF_VERSION_S *pstVersion)
{
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pstVersion);

#ifndef MMF_VERSION
#define MMF_VERSION  (CVI_CHIP_NAME MMF_VER_PRIX MK_VERSION(VER_X, VER_Y, VER_Z) VER_D)
#endif
	//poshiun
	//snprintf(pstVersion->version, VERSION_NAME_MAXLEN, "%s-%s", MMF_VERSION, SDK_VER);
	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_GetChipId(CVI_U32 *pu32ChipId)
{
	static CVI_U32 id = 0xffffffff;
	int fd;

	if (id == 0xffffffff) {
		CVI_U32 tmp = 0;

		fd = open("/dev/cvi-base", O_RDWR | O_SYNC);
		if (fd == -1) {
			CVI_TRACE_SYS(CVI_DBG_ERR, "Can't open device, cvi-base.\n");
			abort();
			return CVI_ERR_SYS_NOTREADY;
		}

		if (ioctl(fd, IOCTL_READ_CHIP_ID, &tmp) < 0) {
			CVI_TRACE_SYS(CVI_DBG_ERR, "ioctl IOCTL_READ_CHIP_ID failed\n");
			abort();
			return CVI_FAILURE;
		}

		switch (tmp) {
		case E_CHIPID_CV1822:
			id = CVI1822;
		break;
		case E_CHIPID_CV1832:
			id = CVI1832;
		break;
		case E_CHIPID_CV1835:
			id = CVI1835;
		break;
		case E_CHIPID_CV1838:
			id = CVI1838;
		break;
		case E_CHIPID_CV1829:
			id = CVI1829;
		break;
		//case E_CHIPID_CV1826:
		//	id = CVI1826;
		break;
		default:
			CVI_TRACE_SYS(CVI_DBG_ERR, "unknown id(%#x)\n", tmp);
			return CVI_FAILURE;
		break;
		}

		close(fd);
	}

	*pu32ChipId = id;
	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_GetChipVersion(CVI_U32 *pu32ChipVersion)
{
	static CVI_U32 version = 0xffffffff;
	int fd;

	if (version == 0xffffffff) {
		CVI_U32 tmp = 0;

		fd = open("/dev/cvi-base", O_RDWR | O_SYNC);
		if (fd == -1) {
			CVI_TRACE_SYS(CVI_DBG_ERR, "Can't open device, cvi-base.\n");
			abort();
			return CVI_ERR_SYS_NOTREADY;
		}

		if (ioctl(fd, IOCTL_READ_CHIP_VERSION, &tmp) < 0) {
			CVI_TRACE_SYS(CVI_DBG_ERR, "ioctl IOCTL_READ_CHIP_VERSION failed\n");
			abort();
			return CVI_FAILURE;
		}
//poshiun
#if 0
		switch (tmp) {
		case E_CHIPVERSION_U01:
			version = CVIU01;
		break;
		case E_CHIPVERSION_U02:
			version = CVIU02;
		default:
			CVI_TRACE_SYS(CVI_DBG_ERR, "unknown version(%#x)\n", tmp);
			return CVI_FAILURE;
		break;
		}
#endif
		close(fd);
	}

	*pu32ChipVersion = version;
	return CVI_SUCCESS;
}

#define CVI_EFUSE_CHIP_SN_SIZE 8
#define CVI_EFUSE_CHIP_SN_ADDR 0x0C

CVI_S32 CVI_SYS_GetChipSNSize(CVI_U32 *pu32SNSize)
{
	if (pu32SNSize)
		*pu32SNSize = CVI_EFUSE_CHIP_SN_SIZE;

	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_GetChipSN(CVI_U8 *pu8SN, CVI_U32 u32SNSize)
{
	FILE *fp;

	if (!pu8SN)
		return CVI_ERR_SYS_ILLEGAL_PARAM;

	fp = fopen("/sys/class/cvi-base/base_efuse_shadow", "r");
	if (!fp) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "Can't open efuse file.\n");
		return CVI_ERR_SYS_NOTREADY;
	}

	if (u32SNSize > CVI_EFUSE_CHIP_SN_SIZE)
		u32SNSize = CVI_EFUSE_CHIP_SN_SIZE;

	fseek(fp, CVI_EFUSE_CHIP_SN_ADDR, SEEK_SET);
	if (fread(pu8SN, 1, u32SNSize, fp) != u32SNSize) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "fread failed\n");
		return CVI_FAILURE;
	}

	fclose(fp);

	return CVI_SUCCESS;
}

void *CVI_SYS_Mmap(CVI_U64 u64PhyAddr, CVI_U32 u32Size)
{
	//poshiun
	//return devm_map(devm_fd, u64PhyAddr, u32Size);
	return 0;
}

/* CVI_SYS_MmapCache - mmap the physical address to cached virtual-address
 *
 * @param pu64PhyAddr: the phy-address of the buffer
 * @param u32Size: the length of the buffer
 * @return virtual-address if success; 0 if fail.
 */
void *CVI_SYS_MmapCache(CVI_U64 u64PhyAddr, CVI_U32 u32Size)
{
//poshiun
#if 0
#ifdef RPC_MULTI_PROCESS
	if (!isMaster()) {
		return rpc_client_sys_mmapcache(u64PhyAddr, u32Size);
	}
#endif
	void *addr = devm_map(devm_cached_fd, u64PhyAddr, u32Size);

	if (addr)
		CVI_SYS_IonInvalidateCache(u64PhyAddr, addr, u32Size);
	return addr;
#else
	return 0;
#endif
}

CVI_S32 CVI_SYS_Munmap(void *pVirAddr, CVI_U32 u32Size)
{
#ifdef RPC_MULTI_PROCESS
	if (!isMaster()) {
		return rpc_client_sys_munmap(pVirAddr, u32Size);
	}
#endif
	//poshiun
	//devm_unmap(pVirAddr, u32Size);
	return CVI_SUCCESS;
}

//poshiun
#if 0
int queryHeapID(int devFd, enum ion_heap_type type)
{
	int ret;
	struct ion_heap_query heap_query;
	struct ion_heap_data *heap_data;
	CVI_U32 heap_id = -1;

	memset(&heap_query, 0, sizeof(heap_query));
	ret = ioctl(devFd, ION_IOC_HEAP_QUERY, &heap_query);
	if (ret < 0 || heap_query.cnt == 0) {
		CVI_TRACE_VB(CVI_DBG_ERR, "ioctl ION_IOC_HEAP_QUERY failed\n");
		return -1;
	}

	heap_data = (struct ion_heap_data *)calloc(heap_query.cnt, sizeof(struct ion_heap_data));
	if (heap_data == NULL) {
		CVI_TRACE_VB(CVI_DBG_ERR, "calloc failed\n");
		return -1;
	}

	heap_query.heaps = (unsigned long)heap_data;
	ret = ioctl(devFd, ION_IOC_HEAP_QUERY, &heap_query);
	if (ret < 0) {
		CVI_TRACE_VB(CVI_DBG_ERR, "ioctl ION_IOC_HEAP_QUERY failed\n");
		return -1;
	}

	heap_id = heap_query.cnt;
	for (CVI_U32 i = 0; i < heap_query.cnt; i++) {
		if (heap_data[i].type == type) {
			heap_id = heap_data[i].heap_id;
			break;
		}
	}
	if (heap_id == heap_query.cnt)
		heap_id = -1;
	free(heap_data);

	return heap_id;
}
#endif

int ionMalloc(int devFd, struct ion_allocation_data *para, bool isCache)
{
	int ret;
	int heap_id;
//poshiun
#if 0
#if ION_FLAG_CACHED == 0
	if (isCache) {
		CVI_TRACE_VB(CVI_DBG_ERR, "ION cached not supported\n");
		return CVI_FAILURE;
	}
#endif

	para->flags = (isCache) ? ION_FLAG_CACHED : 0;

	heap_id = queryHeapID(devFd, ION_HEAP_TYPE_CARVEOUT);
	if (heap_id < 0) {
		CVI_TRACE_VB(CVI_DBG_ERR, "ioctl ION_IOC_HEAP_QUERY failed\n");
		return CVI_FAILURE;
	}
	para->heap_id_mask = (1 << heap_id);

	ret = ioctl(devFd, ION_IOC_ALLOC, para);
	if (ret < 0) {
		CVI_TRACE_VB(CVI_DBG_ERR, "ioctl ION_IOC_ALLOC failed\n");
		return CVI_FAILURE;
	}

//	CVI_TRACE_VB(CVI_DBG_DEBUG, "dev: %d, mem: %d, paddr: %"PRIx64", len: %"PRId64"\n"
//		, devFd, para->fd, para->paddr, para->len);
#endif
	return CVI_SUCCESS;
}

//poshiun
#if 0
void ionFree(struct ion_allocation_data *para)
{
	close(para->fd);
}
#endif
static CVI_S32 _SYS_IonAlloc(CVI_U64 *pu64PhyAddr, CVI_VOID **ppVirAddr, CVI_U32 u32Len, CVI_BOOL cached)
{
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pu64PhyAddr);
//poshiun
#if 0
	struct ion_allocation_data *ion_data;

	if (ionFd < 0) {
		ionFd = open("/dev/ion", O_RDWR | O_DSYNC);
		if (ionFd < 0) {
			CVI_TRACE_SYS(CVI_DBG_ERR, "ion fd open failed.\n");
			return CVI_ERR_SYS_NOTREADY;
		}
		ionHashmap = hashmapCreate(20, _ion_hash_key, _ion_hash_equals);
	}

	ion_data = malloc(sizeof(*ion_data));
	ion_data->len = u32Len;
	if (ionMalloc(ionFd, ion_data, cached) != CVI_SUCCESS) {
		free(ion_data);
		CVI_TRACE_SYS(CVI_DBG_ERR, "alloc failed.\n");
		return CVI_ERR_SYS_NOMEM;
	}
	*pu64PhyAddr = ion_data->paddr;
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
			return CVI_ERR_SYS_NOMEM;
		}
	}
#endif
	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_IonAlloc(CVI_U64 *pu64PhyAddr, CVI_VOID **ppVirAddr, CVI_U32 u32Len)
{
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pu64PhyAddr);
#ifdef RPC_MULTI_PROCESS
	if (!isMaster()) {
		return rpc_client_sys_ionalloc(pu64PhyAddr, ppVirAddr, u32Len);
	}
#endif
	return _SYS_IonAlloc(pu64PhyAddr, ppVirAddr, u32Len, CVI_FALSE);
}

/* CVI_SYS_IonAlloc_Cached - acquire buffer of u32Len from ion
 *
 * @param pu64PhyAddr: the phy-address of the buffer
 * @param ppVirAddr: the cached vir-address of the buffer
 * @param u32Len: the length of the buffer acquire
 * @return CVI_SUCCES if ok
 */
CVI_S32 CVI_SYS_IonAlloc_Cached(CVI_U64 *pu64PhyAddr, CVI_VOID **ppVirAddr, CVI_U32 u32Len)
{
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pu64PhyAddr);
	return _SYS_IonAlloc(pu64PhyAddr, ppVirAddr, u32Len, CVI_TRUE);
}

CVI_S32 CVI_SYS_IonFree(CVI_U64 u64PhyAddr, CVI_VOID *pVirAddr)
{
//poshiun
#if 0
#ifdef RPC_MULTI_PROCESS
	if (!isMaster()) {
		return rpc_client_sys_ionfree(u64PhyAddr, pVirAddr);
	}
#endif
	struct ion_allocation_data *ion_data;

	if (ionHashmap == NULL) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "ion not alloc before.\n");
		return CVI_ERR_SYS_NOTREADY;
	}

	ion_data = hashmapGet(ionHashmap, (void *)(uintptr_t)u64PhyAddr);
	if (ion_data == NULL) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "u64PhyAddr(0x%"PRIx64") not found in ion.\n", u64PhyAddr);
		return CVI_ERR_SYS_ILLEGAL_PARAM;
	}
	if (pVirAddr)
		devm_unmap(pVirAddr, ion_data->len);
	ionFree(ion_data);
	free(ion_data);
#endif
	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_IonFlushCache(CVI_U64 u64PhyAddr, CVI_VOID *pVirAddr, CVI_U32 u32Len)
{
	CVI_S32 ret = CVI_SUCCESS;

//poshiun
#if 0
	struct ion_custom_data custom_data;
	struct cvitek_cache_range cache_range;

	if (pVirAddr == NULL) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "pVirAddr Null.\n");
		return CVI_ERR_SYS_NULL_PTR;
	}

	custom_data.cmd = ION_IOC_CVITEK_FLUSH_PHY_RANGE;
	custom_data.arg = (unsigned long)&cache_range;
	cache_range.paddr = u64PhyAddr;
	cache_range.size = u32Len;
	ret = ioctl(ionFd, ION_IOC_CUSTOM, &custom_data);
	if (ret < 0) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "ion flush err.\n");
		ret = CVI_ERR_SYS_NOTREADY;
	}
#endif
	return ret;
}

CVI_S32 CVI_SYS_IonInvalidateCache(CVI_U64 u64PhyAddr, CVI_VOID *pVirAddr, CVI_U32 u32Len)
{
	CVI_S32 ret = CVI_SUCCESS;

//poshiun
#if 0
	struct ion_custom_data custom_data;
	struct cvitek_cache_range cache_range;

	if (pVirAddr == NULL) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "pVirAddr Null.\n");
		return CVI_ERR_SYS_NULL_PTR;
	}

	custom_data.cmd = ION_IOC_CVITEK_INVALIDATE_PHY_RANGE;
	custom_data.arg = (unsigned long)&cache_range;
	cache_range.paddr = u64PhyAddr;
	cache_range.size = u32Len;
	ret = ioctl(ionFd, ION_IOC_CUSTOM, &custom_data);
	if (ret < 0) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "ion invalidate err.\n");
		ret = CVI_ERR_SYS_NOTREADY;
	}
#endif
	return ret;
}

CVI_S32 CVI_SYS_SetVIVPSSMode(const VI_VPSS_MODE_S *pstVIVPSSMode)
{
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pstVIVPSSMode);

	memcpy(&stVIVPSSMode, pstVIVPSSMode, sizeof(stVIVPSSMode));

	CVI_BOOL vi_online = (stVIVPSSMode.aenMode[0] == VI_ONLINE_VPSS_ONLINE)
			  || (stVIVPSSMode.aenMode[0] == VI_ONLINE_VPSS_OFFLINE);
	CVI_BOOL vpss_online = (stVIVPSSMode.aenMode[0] == VI_ONLINE_VPSS_ONLINE)
			    || (stVIVPSSMode.aenMode[0] == VI_OFFLINE_VPSS_ONLINE);
	struct vdev *isp_d = get_dev_info(VDEV_TYPE_ISP, 0);

	isp_d->is_online = vpss_online;
	isp_set_online(isp_d->fd, vi_online);
	isp_set_online2sc(isp_d->fd, vpss_online);

	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_GetVIVPSSMode(VI_VPSS_MODE_S *pstVIVPSSMode)
{
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pstVIVPSSMode);
	memcpy(pstVIVPSSMode, &stVIVPSSMode, sizeof(stVIVPSSMode));
	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_SetVPSSMode(VPSS_MODE_E enVPSSMode)
{
	stVPSSMode.enMode = enVPSSMode;
	return CVI_SYS_SetVPSSModeEx(&stVPSSMode);
}

VPSS_MODE_E CVI_SYS_GetVPSSMode(void)
{
	return stVPSSMode.enMode;
}

CVI_S32 CVI_SYS_SetVPSSModeEx(const VPSS_MODE_S *pstVPSSMode)
{
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pstVPSSMode);

	struct vdev *d = get_dev_info(VDEV_TYPE_SC, 0);
	struct vdev *img_d;
	CVI_U8 dev_num = (pstVPSSMode->enMode == VPSS_MODE_SINGLE) ? 1 : VPSS_IP_NUM;
	CVI_S32 input;
	CVI_BOOL vi_online = CVI_FALSE;

	sc_set_src_to_imgv(d->fd, (pstVPSSMode->enMode == VPSS_MODE_SINGLE));
	for (int i = 0; i < dev_num; ++i) {
		switch (pstVPSSMode->aenInput[i]) {
		default:
		case VPSS_INPUT_MEM:
			input = CVI_VIP_INPUT_MEM;
			break;
		case VPSS_INPUT_ISP:
			if (pstVPSSMode->ViPipe[i] >= VI_MAX_PIPE_NUM) {
				CVI_TRACE_SYS(CVI_DBG_ERR, "ViPipe(%d) invalid.\n", pstVPSSMode->ViPipe[i]);
				return CVI_ERR_SYS_ILLEGAL_PARAM;
			}
			if (stVIVPSSMode.aenMode[pstVPSSMode->ViPipe[i]] != VI_ONLINE_VPSS_ONLINE &&
			    stVIVPSSMode.aenMode[pstVPSSMode->ViPipe[i]] != VI_OFFLINE_VPSS_ONLINE) {
				CVI_TRACE_SYS(CVI_DBG_ERR, "can't go online if ViPipe(%d)'s mode(%d) isn't online.\n",
					      pstVPSSMode->ViPipe[i], stVIVPSSMode.aenMode[pstVPSSMode->ViPipe[i]]);
				return CVI_ERR_SYS_ILLEGAL_PARAM;
			}
			vi_online = (stVIVPSSMode.aenMode[pstVPSSMode->ViPipe[i]] == VI_ONLINE_VPSS_ONLINE) ||
				    (stVIVPSSMode.aenMode[pstVPSSMode->ViPipe[i]] == VI_ONLINE_VPSS_OFFLINE);
			input = vi_online ? CVI_VIP_INPUT_ISP : CVI_VIP_INPUT_ISP_POST;
			break;
		}
		img_d = get_dev_info(VDEV_TYPE_IMG, (dev_num == 1) ? 1 : i);
		img_d->is_online = vi_online;
		v4l2_sel_input(img_d->fd, input);
	}
	stVPSSMode = *pstVPSSMode;
	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_GetVPSSModeEx(VPSS_MODE_S *pstVPSSMode)
{
	*pstVPSSMode = stVPSSMode;
	return CVI_SUCCESS;
}

#define GENERATE_STRING(STRING) (#STRING),
static const char *const MOD_STRING[] = FOREACH_MOD(GENERATE_STRING);

const CVI_CHAR *CVI_SYS_GetModName(MOD_ID_E id)
{
	return (id < CVI_ID_BUTT) ? MOD_STRING[id] : "UNDEF";
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
