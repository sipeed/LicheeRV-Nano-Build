/*
 * Copyright Cvitek Technologies Inc.
 *
 * Created Time: July, 2020
 */
#include "cvi_vcodec_lib.h"
#include "vdi_osal.h"

#ifdef CLI_DEBUG_SUPPORT
#include "tcli.h"

void vcodec_register_cmd(void);
#endif

static atomic_t vcodecInited = ATOMIC_INIT(false);

unsigned int vcodec_mask = CVI_MASK_CURR;
unsigned int vcodec_level;
module_param(vcodec_mask, uint, 0644);
module_param(vcodec_level, uint, 0644);

bool addrRemapEn;
module_param(addrRemapEn, bool, 0644);
bool ARMode = AR_MODE_OFFSET;
module_param(ARMode, bool, 0644);
uint ARExtraLine = AR_DEFAULT_EXTRA_LINE;
module_param(ARExtraLine, uint, 0644);

int cviVcodecInit(void)
{
	bool expect = false;

	if (atomic_cmpxchg(&vcodecInited, expect, true)) {
		CVI_VC_INFO("vcodecInited\n");
		return -1;
	}

#ifdef CLI_DEBUG_SUPPORT
	TcliInit();
	vcodec_register_cmd();
#endif

	cvi_vdi_init();

	cviVcodecMask();

	return 0;
}

void cviVcodecMask(void)
{
	SetMaxLogLevel(vcodec_level);

	CVI_VC_TRACE("max_log_level = %d\n", GetMaxLogLevel());
}

int cviVcodecGetEnv(char *envVar)
{
	if (strcmp(envVar, "addrRemapEn") == 0)
		return addrRemapEn;

	if (strcmp(envVar, "ARMode") == 0)
		return ARMode;

	if (strcmp(envVar, "ARExtraLine") == 0)
		return ARExtraLine;

	return -1;
}

int cviSetCoreIdx(int *pCoreIdx, CodStd stdMode)
{
	if (stdMode == STD_HEVC)
		*pCoreIdx = CORE_H265;
	else if (stdMode == STD_AVC)
		*pCoreIdx = CORE_H264;
	else {
		CVI_VC_ERR("stdMode = %d\n", stdMode);
		return -1;
	}

	CVI_VC_TRACE("stdMode = %d, coreIdx = %d\n", stdMode, *pCoreIdx);

	return 0;
}

Uint64 cviGetCurrentTime(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct timespec64 ts;
#else
	struct timespec ts;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	ktime_get_ts64(&ts);
#else
	ktime_get_ts(&ts);
#endif

	return ts.tv_sec * 1000000 + ts.tv_nsec / 1000; // in us
}
