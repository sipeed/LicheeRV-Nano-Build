#include <linux/cvi_type.h>
#include "cvi_bin.h"
#include "cvi_base.h"
#include "vpss_bin.h"
#include "cvi_vpss.h"
#include "rw_json.h"
#include <linux/cvi_defines.h>
#include "vpss_json_struct.h"
#include "cvi_json_struct_comm.h"
#include "vpss_ioctl.h"


static VPSS_BIN_DATA vpss_bin_data[VPSS_MAX_GRP_NUM];
static CVI_BOOL g_bLoadBinDone = CVI_FALSE;

VPSS_BIN_DATA *get_vpssbindata_addr(void)
{
	return vpss_bin_data;
}

CVI_BOOL get_loadbin_state(void)
{
	return g_bLoadBinDone;
}

static CVI_S32 get_vpss_ctx_proc_amp(VPSS_BIN_DATA *pBinData)
{
	CVI_S32 fd = get_vpss_fd();
	CVI_S32 s32Ret;
	struct vpss_all_proc_amp_cfg cfg;

	s32Ret = vpss_get_all_proc_amp(fd, &cfg);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "vpss_get_all_proc_amp fail\n");
		return s32Ret;
	}
	for (int i = 0; i < VPSS_MAX_GRP_NUM; ++i)
		memcpy(pBinData[i].proc_amp, cfg.proc_amp[i], sizeof(pBinData[i].proc_amp));

	return CVI_SUCCESS;
}

/**************************************************************************
 *   Bin related APIs.
 **************************************************************************/
CVI_S32 vpss_bin_getbinsize(CVI_U32 *size)
{
	*size = sizeof(VPSS_BIN_DATA) * VPSS_MAX_GRP_NUM;

	return CVI_SUCCESS;
}

CVI_S32 vpss_bin_getparamfrombin(CVI_U8 *addr, CVI_U32 size)
{
	CVI_U32 u32DataSize = 0;
	VPSS_BIN_DATA *pstVpssBinData = vpss_bin_data;

	vpss_bin_getbinsize(&u32DataSize);
	memset(pstVpssBinData, 0, u32DataSize);
	if (size > u32DataSize) {
		CVI_TRACE_VPSS(CVI_DBG_WARN, "Bin size(%d) > max size(%d).\n", size, u32DataSize);
		return CVI_FAILURE;
	}
	memcpy(pstVpssBinData, addr, size);
	g_bLoadBinDone = CVI_TRUE;

	return CVI_SUCCESS;
}

CVI_S32 vpss_bin_setparamtobuf(CVI_U8 *buffer)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_U32 u32DataSize = 0;
	VPSS_BIN_DATA *pstVpssBinData = vpss_bin_data;
	VPSS_BIN_DATA stVpssCtxProcAmp[VPSS_MAX_GRP_NUM];

	get_vpss_ctx_proc_amp(stVpssCtxProcAmp);
	vpss_bin_getbinsize(&u32DataSize);
	for (int i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		memcpy(pstVpssBinData[i].proc_amp, stVpssCtxProcAmp[i].proc_amp, sizeof(pstVpssBinData[i].proc_amp));
	}
	memcpy(buffer, pstVpssBinData, u32DataSize);
	return ret;
}

CVI_S32 vpss_bin_setparamtobin(FILE *fp)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_U32 u32DataSize = 0;
	VPSS_BIN_DATA *pstVpssBinData = vpss_bin_data;
	VPSS_BIN_DATA stVpssCtxProcAmp[VPSS_MAX_GRP_NUM];

	get_vpss_ctx_proc_amp(stVpssCtxProcAmp);
	vpss_bin_getbinsize(&u32DataSize);
	for (int i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		memcpy(pstVpssBinData[i].proc_amp, stVpssCtxProcAmp[i].proc_amp, sizeof(pstVpssBinData[i].proc_amp));
	}
	fwrite(pstVpssBinData, u32DataSize, 1, fp);
	return ret;
}

#ifndef DISABLE_PQBIN_JSON
/**************************************************************************
 *   Json related APIs.
 **************************************************************************/

static CVI_S32 vpss_json_getparam(CVI_U8 *addr)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_U32 u32DataSize = 0;
	VPSS_BIN_DATA *pstPtr = (VPSS_BIN_DATA *)addr;
	VPSS_BIN_DATA *pstVpssBinData = vpss_bin_data;
	VPSS_BIN_DATA stVpssCtxProcAmp[VPSS_MAX_GRP_NUM];

	get_vpss_ctx_proc_amp(stVpssCtxProcAmp);
	vpss_bin_getbinsize(&u32DataSize);
	for (int i = 0; i < VPSS_MAX_GRP_NUM; ++i)
		memcpy(pstVpssBinData[i].proc_amp, stVpssCtxProcAmp[i].proc_amp, sizeof(pstVpssBinData[i].proc_amp));

	memcpy(pstPtr, pstVpssBinData, u32DataSize);

	return ret;
}

static CVI_S32 vpss_json_setparam(CVI_U8 *addr)
{
	CVI_U32 u32DataSize = 0;
	VPSS_BIN_DATA *pstVpssBinData = vpss_bin_data;

	vpss_bin_getbinsize(&u32DataSize);
	memcpy(pstVpssBinData, addr, u32DataSize);
	g_bLoadBinDone = CVI_TRUE;

	return CVI_SUCCESS;
}

CVI_S32 vpss_json_getParamFromJsonbuffer(const char *buffer, enum CVI_BIN_SECTION_ID id)
{
	JSON *json_object;
	CVI_S32 ret = CVI_SUCCESS;
	VPSS_PARAMETER_BUFFER vpss_parameter = { 0 };

	json_object = JSON_TokenerParse(buffer);
	if (json_object) {
		vpss_json_getparam((CVI_U8 *)vpss_parameter.vpss_bin_data);
		JSON_(R_FLAG, json_object, VPSS_PARAMETER_BUFFER, "vpss_parameter", &vpss_parameter);
		vpss_json_setparam((CVI_U8 *)vpss_parameter.vpss_bin_data);

		JSON_ObjectPut(json_object);
	} else {
		CVI_TRACE_VPSS(LOG_WARNING, "(id:%d)Creat json tokener fail.\n", id);
		ret = CVI_BIN_JSONHANLE_ERROR;
	}

	UNUSED(id);
	return ret;
}

CVI_S32 vpss_json_setParamToJsonbuffer(CVI_S8 **buffer, enum CVI_BIN_SECTION_ID id, CVI_S32 *len)
{
	VPSS_PARAMETER_BUFFER vpss_parameter = { 0 };
	JSON *json_object;
	CVI_S32 ret = CVI_SUCCESS;

	json_object = JSON_GetNewObject();
	if (json_object) {
		vpss_json_getparam((CVI_U8 *)vpss_parameter.vpss_bin_data);
		JSON_(W_FLAG, json_object, VPSS_PARAMETER_BUFFER, "vpss_parameter", &vpss_parameter);

		*len = JSON_GetJsonStrLen(json_object);
		*buffer = (CVI_S8 *)malloc(*len);
		if (*buffer == NULL) {
			ret = CVI_BIN_MALLOC_ERR;
			CVI_TRACE_VPSS(LOG_WARNING, "%s\n", "Allocate memory fail");
			goto ERROR_HANDLER;
		}
		memcpy(*buffer, JSON_GetJsonStrContent(json_object), *len);
	} else {
		CVI_TRACE_VPSS(LOG_WARNING, "(id:%d)Get New Object fail.\n", id);
		ret = CVI_BIN_JSONHANLE_ERROR;
	}

ERROR_HANDLER:
	if (json_object) {
		JSON_ObjectPut(json_object);
	}

	UNUSED(id);

	return ret;
}
#endif
