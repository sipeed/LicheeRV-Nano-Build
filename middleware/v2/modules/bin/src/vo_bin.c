#include <linux/cvi_comm_vo.h>
#include "vo_bin.h"
#include "cvi_base.h"
#include "cvi_bin.h"
#include "cvi_vo.h"
#include "vo_json_struct.h"
#include "cvi_json_struct_comm.h"

/**************************************************************************
 *   Bin related APIs.
 **************************************************************************/
CVI_S32 vo_bin_getbinsize(CVI_U32 *size)
{
	MOD_CHECK_NULL_PTR(CVI_ID_VO, size);
	*size = sizeof(VO_BIN_INFO_S);

	return CVI_SUCCESS;
}

CVI_S32 vo_bin_getparamfrombin(CVI_U8 *addr, CVI_U32 size)
{
	CVI_U32 data_size;
	VO_BIN_INFO_S info_from_bin;

	MOD_CHECK_NULL_PTR(CVI_ID_VO, addr);

	vo_bin_getbinsize(&data_size);
	if (size > data_size) {
		CVI_TRACE_VO(CVI_DBG_ERR, "Bin size(%d) > max size(%d).\n", size, data_size);
		return CVI_FAILURE;
	}
	memcpy(&info_from_bin, addr, size);

	//check guard pattern
	if (info_from_bin.guard_magic != get_vo_bin_guardmagic_code()) {
		CVI_TRACE_VO(CVI_DBG_ERR, "readback guardpattern incorrect guard_magic(0x%x)\n",
			     info_from_bin.guard_magic);
	} else {
		CVI_TRACE_VO(CVI_DBG_DEBUG, "get param from bin success\n");
		CVI_VO_SetGammaInfo(&(info_from_bin.gamma_info));
	}

	return CVI_SUCCESS;
}

CVI_S32 vo_bin_setparamtobuf(CVI_U8 *buffer)
{
	CVI_U32 data_size = 0;
	VO_BIN_INFO_S *pstVoBinInfo = get_vo_bin_info_addr();

	MOD_CHECK_NULL_PTR(CVI_ID_VO, buffer);

	vo_bin_getbinsize(&data_size);
	memcpy(buffer, pstVoBinInfo, data_size);

	return CVI_SUCCESS;
}

CVI_S32 vo_bin_setparamtobin(FILE *fp)
{
	CVI_U32 data_size = 0;
	VO_BIN_INFO_S *pstVoBinInfo = get_vo_bin_info_addr();

	MOD_CHECK_NULL_PTR(CVI_ID_VO, fp);

	vo_bin_getbinsize(&data_size);
	fwrite(pstVoBinInfo, data_size, 1, fp);

	return CVI_SUCCESS;
}

#ifndef DISABLE_PQBIN_JSON
CVI_S32 vo_json_getParamFromJsonbuffer(const char *buffer, enum CVI_BIN_SECTION_ID id)
{
	JSON *json_object;
	CVI_S32 ret = CVI_SUCCESS;
	VO_GAMMA_INFO_S vo_parameter = { 0 };

	json_object = JSON_TokenerParse(buffer);
	if (json_object) {
		CVI_VO_GetGammaInfo(&vo_parameter);
		JSON_(R_FLAG, json_object, VO_GAMMA_INFO_S, "vo_parameter", &vo_parameter);
		CVI_VO_SetGammaInfo(&vo_parameter);

		JSON_ObjectPut(json_object);
	} else {
		CVI_TRACE_VO(LOG_WARNING, "(id:%d)Creat json tokener fail.\n", id);
		ret = CVI_BIN_JSONHANLE_ERROR;
	}

	UNUSED(id);

	return ret;
}

CVI_S32 vo_json_setParamToJsonbuffer(CVI_S8 **buffer, enum CVI_BIN_SECTION_ID id, CVI_S32 *len)
{
	VO_GAMMA_INFO_S vo_parameter = { 0 };
	JSON *json_object;
	CVI_S32 ret = CVI_SUCCESS;

	json_object = JSON_GetNewObject();
	if (json_object) {
		CVI_VO_GetGammaInfo(&vo_parameter);
		JSON_(W_FLAG, json_object, VO_GAMMA_INFO_S, "vo_parameter", &vo_parameter);

		*len = JSON_GetJsonStrLen(json_object);
		*buffer = (CVI_S8 *)malloc(*len);
		if (*buffer == NULL) {
			ret = CVI_BIN_MALLOC_ERR;
			CVI_TRACE_VO(LOG_WARNING, "%s\n", "Allocate memory fail");
			goto ERROR_HANDLER;
		}
		memcpy(*buffer, JSON_GetJsonStrContent(json_object), *len);
	} else {
		CVI_TRACE_VO(LOG_WARNING, "(id:%d)Get New Object fail.\n", id);
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
