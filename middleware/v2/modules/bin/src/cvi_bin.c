#include <sys/stat.h>
#include "stdlib.h"
#include "cvi_bin.h"
#include "cvi_vi.h"
#include <linux/cvi_comm_sys.h>
#include <linux/cvi_comm_vi.h>
#include "rw_json.h"
#include "isp_bin.h"
#include "vpss_bin.h"
#include "md5.h"
#include "vo_bin.h"

#define MAGIC_NUMBER 0x1835 /*the macro is used as magic number and can't be modified!*/

//following info is V1.0 and V1.1 bin header field. difference of V1.0 and V1.1 is end of  V1.1's file that
//contains the whole file PQbin's md5 value.

/***The following macro comes from module/bin/cv18xx/isp_bin/src/isp_bin.c***/
//CVI_BIN_EXTRA_S.Desc[624-1023] 400 bytes
//[pqbin md5 50] + [reserved 50]
//[tool version 40] + [isp commitId 10] + [sensorNum 5] + [sensorName1 55] + [sensorName2 55]
//[sensorName3 55]  + [sensorName4 55] + [isp branch 20] + [version 4] + [generate mode - A:auto M:Manual 1]
#define PQBIN_VERSION_V10 "V1.0"
#define PQBIN_VERSION_V11 "V1.1"

#define DESC_SIZE 624
#define BIN_MD5_SIZE 50
#define PQBIN_RESERVE_SIZE 50
#define TOOLVERSION_SIZE 40
#define BIN_COMMIT_SIZE 10
#define SENSORNUM_SIZE 5
#define SENSORNAME_SIZE 55
#define BIN_GERRIT_SIZE 20
#define PQBINVERSION_SIZE 4
#define PQBINCREATE_MODE_SIZE 1
/****************************************************************************/
struct BIN_BUF_INFO {
	CVI_U32 u32BlkSize[CVI_BIN_ID_MAX];
	CVI_U32 u32BinParaSize;
	CVI_U32 u32JsonParaSize;
	CVI_U32 u32ParaTotalSize;
	CVI_U32 u32SensorNumber;
	CVI_CHAR achBinVersion[PQBINVERSION_SIZE + 1];
	CVI_CHAR achBinMode[PQBINCREATE_MODE_SIZE + 1];
	CVI_CHAR achBinMD5Code[BIN_MD5_SIZE + 1];
	CVI_CHAR achTotalParaMD5Value[MD5_STRING_LEN];
	CVI_BOOL bNewVerisonMatch; /*it's not match new version*/
};

static struct BIN_BUF_INFO g_stBinBufInfo = {0};
static CVI_BOOL g_bUseOldLoadAPI = CVI_TRUE;

static char userBinName[WDR_MODE_MAX][BIN_FILE_LENGTH] = {
	"/mnt/cfg/param/cvi_sdr_bin", "/mnt/cfg/param/cvi_sdr_bin",
	"/mnt/cfg/param/cvi_sdr_bin", "/mnt/cfg/param/cvi_wdr_bin",
	"/mnt/cfg/param/cvi_wdr_bin", "/mnt/cfg/param/cvi_wdr_bin",
	"/mnt/cfg/param/cvi_wdr_bin", "/mnt/cfg/param/cvi_wdr_bin",
	"/mnt/cfg/param/cvi_wdr_bin", "/mnt/cfg/param/cvi_wdr_bin",
	"/mnt/cfg/param/cvi_wdr_bin", "/mnt/cfg/param/cvi_wdr_bin" };
static CVI_S32 _setBinNameImp(WDR_MODE_E wdrMode, const CVI_CHAR *binName);
static CVI_S32 _getBinNameImp(CVI_CHAR *binName);
static CVI_S32 check_is_register_id(enum CVI_BIN_SECTION_ID id);
static CVI_S32 get_file_size(FILE *fp, CVI_U32 *size);
static CVI_S32 id_is_valid_IspID(enum CVI_BIN_SECTION_ID id);
static CVI_BOOL check_bin_file_is_new_version(CVI_CHAR *pchVersion);
static CVI_S32 write_md5_value_to_buf(CVI_U8 *buf, CVI_U32 u32BufSize);
static CVI_S32 check_sensor_num_is_exceeded(enum CVI_BIN_SECTION_ID id);

static pfn_cvi_bin_getbinsize getBinSizeFunc[CVI_BIN_ID_MAX] = {
	header_bin_getBinSize, /*CVI_BIN_ID_HEADER*/
	isp_bin_getBinSize_p0, /*CVI_BIN_ID_ISP0*/
	isp_bin_getBinSize_p1, /*CVI_BIN_ID_ISP1*/
	isp_bin_getBinSize_p2, /*CVI_BIN_ID_ISP2*/
	isp_bin_getBinSize_p3, /*CVI_BIN_ID_ISP3*/
	vpss_bin_getbinsize,	/*CVI_BIN_ID_VPSS*/
	NULL, /*CVI_BIN_ID_VDEC*/
	NULL, /*CVI_BIN_ID_VENC*/
	vo_bin_getbinsize, /*CVI_BIN_ID_VO*/
};
static pfn_cvi_bin_getparamfrombin getParamFromBinFunc[CVI_BIN_ID_MAX] = {
	header_bin_getBinParam, /*CVI_BIN_ID_HEADER*/
	isp_bin_getBinParam_p0, /*CVI_BIN_ID_ISP0*/
	isp_bin_getBinParam_p1, /*CVI_BIN_ID_ISP1*/
	isp_bin_getBinParam_p2, /*CVI_BIN_ID_ISP2*/
	isp_bin_getBinParam_p3, /*CVI_BIN_ID_ISP3*/
	vpss_bin_getparamfrombin, /*CVI_BIN_ID_VPSS*/
	NULL, /*CVI_BIN_ID_VDEC*/
	NULL, /*CVI_BIN_ID_VENC*/
	vo_bin_getparamfrombin, /*CVI_BIN_ID_VO*/
};
static pfn_cvi_bin_setparamtobin setParamToBinFunc[CVI_BIN_ID_MAX] = {
	header_bin_setBinParam, /*CVI_BIN_ID_HEADER*/
	isp_bin_setBinParam_p0, /*CVI_BIN_ID_ISP0*/
	isp_bin_setBinParam_p1, /*CVI_BIN_ID_ISP1*/
	isp_bin_setBinParam_p2, /*CVI_BIN_ID_ISP2*/
	isp_bin_setBinParam_p3, /*CVI_BIN_ID_ISP3*/
	vpss_bin_setparamtobin,	/*CVI_BIN_ID_VPSS*/
	NULL, /*CVI_BIN_ID_VDEC*/
	NULL, /*CVI_BIN_ID_VENC*/
	vo_bin_setparamtobin, /*CVI_BIN_ID_VO*/
};
static pfn_cvi_bin_setparamtobuf setParamToBufFunc[CVI_BIN_ID_MAX] = {
	header_bin_setBinParambuf, /*CVI_BIN_ID_HEADER*/
	isp_bin_setBinParambuf_p0, /*CVI_BIN_ID_ISP0*/
	isp_bin_setBinParambuf_p1, /*CVI_BIN_ID_ISP1*/
	isp_bin_setBinParambuf_p2, /*CVI_BIN_ID_ISP2*/
	isp_bin_setBinParambuf_p3, /*CVI_BIN_ID_ISP3*/
	vpss_bin_setparamtobuf,	/*CVI_BIN_ID_VPSS*/
	NULL, /*CVI_BIN_ID_VDEC*/
	NULL, /*CVI_BIN_ID_VENC*/
	vo_bin_setparamtobuf, /*CVI_BIN_ID_VO*/
};
#ifndef DISABLE_PQBIN_JSON
static pfn_cvi_json_getparamfrombuf getParamFromJsonBuf[CVI_BIN_ID_MAX] = {
	NULL, /*CVI_BIN_ID_HEADER*/
	isp_json_getParamFromJsonbuffer, /*CVI_BIN_ID_ISP0*/
	isp_json_getParamFromJsonbuffer, /*CVI_BIN_ID_ISP1*/
	isp_json_getParamFromJsonbuffer, /*CVI_BIN_ID_ISP2*/
	isp_json_getParamFromJsonbuffer, /*CVI_BIN_ID_ISP3*/
	vpss_json_getParamFromJsonbuffer, /*CVI_BIN_ID_VPSS*/
	NULL, /*CVI_BIN_ID_VDEC*/
	NULL, /*CVI_BIN_ID_VENC*/
	vo_json_getParamFromJsonbuffer, /*CVI_BIN_ID_VO*/
};
static pfn_cvi_json_setparamtobuf setParamToJsonBuf[CVI_BIN_ID_MAX] = {
	NULL, /*CVI_BIN_ID_HEADER*/
	isp_json_setParamToJsonbuffer, /*CVI_BIN_ID_ISP0*/
	isp_json_setParamToJsonbuffer, /*CVI_BIN_ID_ISP1*/
	isp_json_setParamToJsonbuffer, /*CVI_BIN_ID_ISP2*/
	isp_json_setParamToJsonbuffer, /*CVI_BIN_ID_ISP3*/
	vpss_json_setParamToJsonbuffer, /*CVI_BIN_ID_VPSS*/
	NULL, /*CVI_BIN_ID_VDEC*/
	NULL, /*CVI_BIN_ID_VENC*/
	vo_json_setParamToJsonbuffer, /*CVI_BIN_ID_VO*/
};
#endif
static CVI_S32 _isSectionIdValid(enum CVI_BIN_SECTION_ID id)
{
	CVI_S32 ret = CVI_SUCCESS;

	if ((id < CVI_BIN_ID_MIN) || (id >= CVI_BIN_ID_MAX)) {
		ret = CVI_FAILURE;
	}

	return ret;
}

static CVI_S32 get_file_size(FILE *fp, CVI_U32 *size)
{
	CVI_S32 ret = CVI_SUCCESS;

	fseek(fp, 0L, SEEK_END);
	*size = ftell(fp);
	rewind(fp);

	return ret;
}

static struct BIN_BUF_INFO *get_current_buf_info(void)
{
	return &g_stBinBufInfo;
}

static CVI_S32 get_bin_Info_from_buf(CVI_U8 *buf, struct BIN_BUF_INFO *pstBufInfo)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_BIN_HEADER *pstHeader = (CVI_BIN_HEADER *)buf;
	CVI_JSON_HEADER *pstJsonHeader = NULL;
	CVI_CHAR *pchDesc = (CVI_CHAR *)pstHeader->extraInfo.Desc;
	CVI_U32 u32BinSize = 0, u32JsonSize = 0;
	CVI_U32 u32SensorNum = 0;
	CVI_BOOL bBlkSizeInvalid = CVI_FALSE;
	CVI_CHAR achSensorNum[SENSORNUM_SIZE];

	/*get sensor number,version and mode from buffer.*/
	memset(achSensorNum, 0, SENSORNUM_SIZE);
	memset(pstBufInfo->achBinVersion, 0, sizeof(pstBufInfo->achBinVersion));
	memset(pstBufInfo->achBinMode, 0, sizeof(pstBufInfo->achBinMode));
	memset(pstBufInfo->achBinMD5Code, 0, sizeof(pstBufInfo->achBinMD5Code));
	pchDesc += DESC_SIZE;
	strncpy(pstBufInfo->achBinMD5Code, pchDesc, BIN_MD5_SIZE);
	pchDesc += BIN_MD5_SIZE;
	pchDesc += PQBIN_RESERVE_SIZE;
	pchDesc += TOOLVERSION_SIZE;
	pchDesc += BIN_COMMIT_SIZE;
	strncpy(achSensorNum, pchDesc, SENSORNUM_SIZE);
	pstBufInfo->u32SensorNumber = (CVI_U32)atoi(achSensorNum);
	pchDesc += SENSORNUM_SIZE;
	pchDesc += SENSORNAME_SIZE * VI_MAX_PIPE_NUM;
	pchDesc += BIN_GERRIT_SIZE;
	strncpy(pstBufInfo->achBinVersion, pchDesc, PQBINVERSION_SIZE);
	pchDesc += PQBINVERSION_SIZE;
	strncpy(pstBufInfo->achBinMode, pchDesc, PQBINCREATE_MODE_SIZE);
	if (check_bin_file_is_new_version(pstBufInfo->achBinVersion)) {
		pstBufInfo->bNewVerisonMatch = CVI_TRUE;
	} else {
		pstBufInfo->bNewVerisonMatch = CVI_FALSE;
		for (CVI_U32 idx = CVI_BIN_ID_MIN; idx < CVI_BIN_ID_MAX; idx++) {
			if (id_is_valid_IspID(idx) == CVI_SUCCESS) {
				bBlkSizeInvalid = pstHeader->size[idx];
				if (bBlkSizeInvalid) {
					u32SensorNum++;
				}
			}
		}
		pstBufInfo->u32SensorNumber = u32SensorNum;
	}

	/*get bin size total size.*/
	for (CVI_U32 idx = CVI_BIN_ID_MIN; idx < CVI_BIN_ID_MAX; idx++) {
		if ((getBinSizeFunc[idx] != NULL) && (check_sensor_num_is_exceeded(idx) == CVI_SUCCESS)) {
			pstBufInfo->u32BlkSize[idx] = pstHeader->size[idx];
			u32BinSize += pstHeader->size[idx];
		}
	}

	/*get json size total size.*/
	if (check_bin_file_is_new_version(pstBufInfo->achBinVersion)) {
		pstJsonHeader = (CVI_JSON_HEADER *)(buf + u32BinSize);
		for (CVI_U32 idx = CVI_BIN_ID_MIN; idx < CVI_BIN_ID_MAX; idx++) {
			if ((getBinSizeFunc[idx] != NULL) && (check_sensor_num_is_exceeded(idx)) == CVI_SUCCESS) {
				u32JsonSize += pstJsonHeader->size[idx].u32CompreSize;
			}
		}
	}

	/*set bufinfo struct.*/
	pstBufInfo->u32BinParaSize = u32BinSize;
	pstBufInfo->u32JsonParaSize = u32JsonSize;
	pstBufInfo->u32ParaTotalSize = u32BinSize + u32JsonSize;
	if (strncmp(pstBufInfo->achBinVersion, PQBIN_VERSION_V11, PQBINVERSION_SIZE) >= 0) {
		strncpy(pstBufInfo->achTotalParaMD5Value,
					(CVI_CHAR *)buf + pstBufInfo->u32ParaTotalSize, MD5_STRING_LEN);
		pstBufInfo->u32ParaTotalSize += MD5_STRING_LEN;
	}

	return ret;
}

static CVI_S32 id_is_valid_IspID(enum CVI_BIN_SECTION_ID id)
{
	if ((id <= CVI_BIN_ID_ISP3) && (id >= CVI_BIN_ID_ISP0)) {
		return CVI_SUCCESS;
	}

	return CVI_FAILURE;
}

static CVI_S32 check_sensor_num_is_exceeded(enum CVI_BIN_SECTION_ID id)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_U32 u32CurSensorNum = id - CVI_BIN_ID_ISP0 + 1;
	struct BIN_BUF_INFO *pstBufInfo = get_current_buf_info();

	if (id_is_valid_IspID(id) == CVI_SUCCESS) {
		if (u32CurSensorNum > pstBufInfo->u32SensorNumber) {
			ret = CVI_BIN_SENSORNUM_ERROR;
		}
	}

	return ret;
}

static CVI_S32 check_is_register_id(enum CVI_BIN_SECTION_ID id)
{
	CVI_U32 u32DevNum = 0;

	if (id_is_valid_IspID(id) == CVI_SUCCESS) {
		CVI_VI_GetDevNum(&u32DevNum);
		if (id - CVI_BIN_ID_ISP0 >= u32DevNum) {
			return CVI_FAILURE;
		}
	}
	return CVI_SUCCESS;
}
/* Static function */
static CVI_S32 _setBinNameImp(WDR_MODE_E wdrMode, const CVI_CHAR *binName)
{
	if (binName == NULL) {
		CVI_TRACE_SYS(CVI_DBG_WARN, "binName is NULL\n");
		return CVI_FAILURE;
	}

	CVI_U32 len = (CVI_U32)strlen(binName);

	if (len >= BIN_FILE_LENGTH) {
		CVI_TRACE_SYS(CVI_DBG_WARN, "Set bin name failed, strlen(%u) >= %d!\n", len, BIN_FILE_LENGTH);
		return CVI_FAILURE;
	}

	strncpy(userBinName[wdrMode], binName, BIN_FILE_LENGTH);

	return CVI_SUCCESS;
}

static CVI_S32 _getBinNameImp(CVI_CHAR *binName)
{
	if (binName == NULL) {
		CVI_TRACE_SYS(CVI_DBG_WARN, "binName is NULL\n");
		return CVI_FAILURE;
	}

	VI_DEV_ATTR_S pstDevAttr;
	WDR_MODE_E wdrMode;

	CVI_VI_GetDevAttr(0, &pstDevAttr);
	wdrMode = pstDevAttr.stWDRAttr.enWDRMode;

	sprintf(binName, "%s", userBinName[wdrMode]);

	return CVI_SUCCESS;
}

static CVI_BOOL check_bin_file_is_new_version(CVI_CHAR *pchVersion)
{
	if (strncmp(pchVersion, PQBIN_VERSION_V10, strlen(PQBIN_VERSION_V10)) >= 0) {
		return CVI_TRUE;
	} else {
		return CVI_FALSE;
	}
}

static CVI_S32 check_bin_file_validity(CVI_U8 *buf, CVI_U32 u32DataLen)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_U8 au8Md5Value[MD5_STRING_LEN] = { 0 };
	CVI_BIN_HEADER *pstHeader = (CVI_BIN_HEADER *)buf;
	struct BIN_BUF_INFO *pstBufInfo = get_current_buf_info();

	if (pstHeader->chipId != MAGIC_NUMBER) {
		ret = CVI_BIN_FILE_ERROR;
		CVI_TRACE_SYS(CVI_DBG_ERR, "PQbin file isn't valid!\n");
		goto FINISH_HANDLER;
	}
	/*Get PQbin information from buf.*/
	get_bin_Info_from_buf(buf, pstBufInfo);

	/*calcute md5 of buf. if u32DataLen is 0, it's old loading param API. */
	if (u32DataLen != 0) {
		if (strncmp(pstBufInfo->achBinVersion, PQBIN_VERSION_V11, PQBINVERSION_SIZE) >= 0) {
			calcute_md5_value(buf, u32DataLen - MD5_STRING_LEN, au8Md5Value);
			if (strncmp((char *)au8Md5Value, (char *)pstBufInfo->achTotalParaMD5Value,
				MD5_STRING_LEN) != 0) {
				ret = CVI_BIN_DATA_ERR;
				CVI_TRACE_SYS(CVI_DBG_ERR, "Data of PQbin file isn't normal!\n");
			}
		}
	}

FINISH_HANDLER:
	return ret;
}
#ifndef DISABLE_PQBIN_JSON
static CVI_S32 get_json_para_from_buffer(enum CVI_BIN_SECTION_ID id, CVI_U8 *buf)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct BIN_BUF_INFO *pstBufInfo = get_current_buf_info();

	if (buf == NULL) {
		ret = CVI_BIN_NULL_POINT;
		return ret;
	}

	/*check that json is valid or not in cur file.*/
	if (pstBufInfo->bNewVerisonMatch != CVI_TRUE) {
		ret = CVI_BIN_NOJSON_ERROR;
		CVI_TRACE_SYS(CVI_DBG_WARN, "Json is invalid in current PQBin file!\n");
		goto FINISH_HANDLER;
	}

	/*load para from json.*/
	ret = CVI_JSON_LoadParamFromBuffer(id, buf, getParamFromJsonBuf[id]);
	if (ret != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "Load Json failed!\n");
		goto FINISH_HANDLER;
	}

FINISH_HANDLER:
	return ret;
}
#endif
static CVI_S32 get_file_md5_value(FILE *fp, CVI_U8 *md5_value)
{
	CVI_U32 u32FileSize = 0;
	CVI_U8 *buf = NULL;
	CVI_U32 u32TempLen = 0;
	CVI_S32 ret = CVI_SUCCESS;

	get_file_size(fp, &u32FileSize);
	buf = (CVI_U8 *)malloc(u32FileSize);
	if (buf == NULL) {
		ret = CVI_BIN_MALLOC_ERR;
		CVI_TRACE_SYS(CVI_DBG_WARN, "calcute md5: Allocate memory fail\n");
		goto ERROR_HANDLER;
	}
	u32TempLen = fread(buf, u32FileSize, 1, fp);
	if (u32TempLen <= 0) {
		CVI_TRACE_SYS(CVI_DBG_WARN, "calcute md5: read data to buff fail!\n");
		ret = CVI_BIN_READ_ERROR;
		goto ERROR_HANDLER;
	}
	calcute_md5_value(buf, u32FileSize, md5_value);

ERROR_HANDLER:
	if (buf != NULL) {
		free(buf);
	}
	return ret;
}

static CVI_S32 write_md5_value_to_file(FILE *fp)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_U32 u32TempLen = 0;
	CVI_U8 au8Md5Value[MD5_STRING_LEN] = { 0 };

	ret = get_file_md5_value(fp, au8Md5Value);
	fseek(fp, 0L, SEEK_END);
	u32TempLen = fwrite(au8Md5Value, MD5_STRING_LEN, 1, fp);
	if (u32TempLen != 1) {
		ret = CVI_BIN_UPDATE_ERROR;
		CVI_TRACE_SYS(CVI_DBG_WARN, "Write md5 to end of file fail!\n");
	}

	return ret;
}

static CVI_S32 write_md5_value_to_buf(CVI_U8 *buf, CVI_U32 u32BufSize)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_U8 au8Md5Value[MD5_STRING_LEN] = { 0 };
	CVI_U32 u32ValidDataLen = u32BufSize - MD5_STRING_LEN;

	calcute_md5_value(buf, u32ValidDataLen, au8Md5Value);
	memcpy(buf + u32ValidDataLen, au8Md5Value, MD5_STRING_LEN);

	return ret;
}

CVI_U32 CVI_BIN_GetBinTotalLen(void)
{
	CVI_BIN_HEADER stHeader = { 0 };
	CVI_U32 u32ParaTotalSize = 0;
	CVI_JSON_HEADER stJsonHeader = { 0 };

	/*get size of json and bin content.*/
	for (CVI_U32 idx = CVI_BIN_ID_MIN; idx < CVI_BIN_ID_MAX; idx++) {
		if (idx == CVI_BIN_ID_HEADER) {
			stJsonHeader.size[CVI_BIN_ID_HEADER].u32InitSize = sizeof(CVI_JSON_HEADER);
			stJsonHeader.size[CVI_BIN_ID_HEADER].u32CompreSize = sizeof(CVI_JSON_HEADER);
		} else {
			#ifndef DISABLE_PQBIN_JSON
			if ((getBinSizeFunc[idx] != NULL) && (check_is_register_id(idx) == CVI_SUCCESS)) {
				CVI_JSON_SaveParamToBuffer(NULL, idx, &stJsonHeader.size[idx], setParamToJsonBuf[idx],
				0);
			}
			#endif
		}
	}

	for (CVI_U32 idx = CVI_BIN_ID_MIN; idx < CVI_BIN_ID_MAX; idx++) {
		if ((getBinSizeFunc[idx] != NULL) && (check_is_register_id(idx) == CVI_SUCCESS)) {
			getBinSizeFunc[idx]((CVI_U32 *)&(stHeader.size[idx]));
		}
		u32ParaTotalSize += stJsonHeader.size[idx].u32CompreSize + stHeader.size[idx];
	}

	/*plus length of Md5.*/
	u32ParaTotalSize += MD5_STRING_LEN;

	return u32ParaTotalSize;
}

CVI_S32 CVI_BIN_ExportBinData(CVI_U8 *pu8Buffer, CVI_U32 u32DataLength)
{
	if (pu8Buffer == NULL) {
		return CVI_BIN_NULL_POINT;
	}

	CVI_S32 ret = CVI_SUCCESS;
	CVI_BIN_HEADER *pstHeader = (CVI_BIN_HEADER *)pu8Buffer;
	CVI_U8 *pu8JsonAddr = NULL;
	CVI_U8 *pu8BufStartAddr = pu8Buffer;
	CVI_U32 u32BufSize = u32DataLength;
	CVI_JSON_HEADER stJsonHeader = { 0 };

	/*remove md5 length.*/
	u32DataLength -= MD5_STRING_LEN;
	pstHeader->chipId = MAGIC_NUMBER;

	for (CVI_U32 idx = CVI_BIN_ID_MIN; idx < CVI_BIN_ID_MAX; idx++) {
		if ((getBinSizeFunc[idx] != NULL) && (check_is_register_id(idx) == CVI_SUCCESS)) {
			getBinSizeFunc[idx]((CVI_U32 *)&(pstHeader->size[idx]));
		}
		u32DataLength -= pstHeader->size[idx];
	}

	/*get bin param to buf*/
	for (CVI_U32 idx = CVI_BIN_ID_MIN; idx < CVI_BIN_ID_MAX; idx++) {
		if ((setParamToBufFunc[idx] != NULL) && (check_is_register_id(idx) == CVI_SUCCESS)) {
			ret = setParamToBufFunc[idx](pu8Buffer);
		}
		pu8Buffer += pstHeader->size[idx];
	}

	/*set json param to buf*/
	pu8JsonAddr = pu8Buffer;

	/*save compress json para to cvi_sdr_bin*/
	for (CVI_U32 idx = CVI_BIN_ID_MIN; idx < CVI_BIN_ID_MAX; idx++) {
		if (idx == CVI_BIN_ID_HEADER) {
			stJsonHeader.size[CVI_BIN_ID_HEADER].u32InitSize = sizeof(CVI_JSON_HEADER);
			stJsonHeader.size[CVI_BIN_ID_HEADER].u32CompreSize = sizeof(CVI_JSON_HEADER);
		} else {
			#ifndef DISABLE_PQBIN_JSON
			if ((setParamToBufFunc[idx] != NULL) && (check_is_register_id(idx) == CVI_SUCCESS)) {
				ret = CVI_JSON_SaveParamToBuffer(pu8Buffer, idx, &stJsonHeader.size[idx],
				setParamToJsonBuf[idx], u32DataLength);
				if (ret != CVI_SUCCESS) {
					break;
				}
			}
			#endif
		}
		u32DataLength -= stJsonHeader.size[idx].u32CompreSize;
		pu8Buffer += stJsonHeader.size[idx].u32CompreSize;
	}
	memcpy(pu8JsonAddr, &stJsonHeader, sizeof(CVI_JSON_HEADER));
	write_md5_value_to_buf(pu8BufStartAddr, u32BufSize);

	return ret;
}

CVI_S32 CVI_BIN_ImportBinData(CVI_U8 *pu8Buffer, CVI_U32 u32DataLength)
{
	CVI_S32 ret = CVI_SUCCESS, tmpRet = CVI_SUCCESS;

	if (pu8Buffer == NULL) {
		return CVI_BIN_NULL_POINT;
	}
	if (u32DataLength == 0) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "input length of buffer can't be 0!\n");
		return CVI_BIN_SIZE_ERR;
	}

	#ifdef RPC_MULTI_PROCESS
	if (!isMaster()) {
		for (CVI_U32 idx = CVI_BIN_ID_MIN; idx < CVI_BIN_ID_MAX; idx++) {
			ret = rpc_client_bin_loadparamfrombin(idx, pu8Buffer);
		}
		return ret;
	}
	#endif

	CVI_BIN_HEADER *pstHeader = (CVI_BIN_HEADER *)pu8Buffer;
	CVI_U8 *pu8BlockAddr = (CVI_U8 *)pu8Buffer;
	CVI_BOOL bBlkSizeInvalid = CVI_FALSE;

	ret = check_bin_file_validity(pu8Buffer, u32DataLength);
	if (ret == (CVI_S32)CVI_BIN_FILE_ERROR) {
		goto ERROR_HANDLER;
	}

	for (CVI_U32 idx = CVI_BIN_ID_MIN; idx < CVI_BIN_ID_MAX; idx++) {
		if ((getParamFromBinFunc[idx] != NULL) && (check_is_register_id(idx) == CVI_SUCCESS)) {
			ret = check_sensor_num_is_exceeded(idx);
			if (ret != CVI_SUCCESS) {
				CVI_TRACE_SYS(CVI_DBG_WARN, "Sensor number exceeds specified number in PQbin file\n");
				goto ERROR_HANDLER;
			}
			bBlkSizeInvalid = !(pstHeader->size[idx]);
			if (bBlkSizeInvalid) {
				#ifndef DISABLE_PQBIN_JSON
				CVI_TRACE_SYS(CVI_DBG_WARN, "Size of block(%d) is 0, get para from json!\n", idx);
				ret = get_json_para_from_buffer(idx, pu8Buffer);
				if (ret != CVI_SUCCESS) {
					tmpRet = ret;
				}
				#else
				CVI_TRACE_SYS(CVI_DBG_WARN, "Size of block(%d) is 0 & json inexistence!\n", idx);
				tmpRet = CVI_BIN_JSON_ERR;
				#endif
			} else {
				ret = getParamFromBinFunc[idx](pu8BlockAddr, pstHeader->size[idx]);
				if (ret != CVI_SUCCESS) {
					#ifndef DISABLE_PQBIN_JSON
					CVI_TRACE_SYS(CVI_DBG_WARN, "Get cur block(%d) para from json!\n", idx);
					ret = get_json_para_from_buffer(idx, pu8Buffer);
					if (ret != CVI_SUCCESS) {
						tmpRet = ret;
					}
					#else
					CVI_TRACE_SYS(CVI_DBG_WARN,
						"Get cur block(%d) err=0x%08x & json inexistence!\n", idx, ret);
					tmpRet = CVI_BIN_JSON_ERR;
					#endif
				}
			}
		}
		pu8BlockAddr += pstHeader->size[idx];
	}
	ret = tmpRet;

ERROR_HANDLER:
	return ret;
}

CVI_S32 CVI_BIN_SaveParamToBin(FILE *fp, CVI_BIN_EXTRA_S *extraInfo)
{
#ifdef RPC_MULTI_PROCESS
	if (!isMaster()) {
		return rpc_client_bin_saveparamtobin(fp, extraInfo);
	}
#endif

	CVI_S32 ret = CVI_SUCCESS;
	CVI_BIN_HEADER header = { 0 };
	CVI_U32 u32BinParaSize = 0, u32TempLen = 0;
	CVI_JSON_HEADER stJsonHeader = { 0 };

	header.chipId = MAGIC_NUMBER;
	header.extraInfo = *extraInfo;
	header.size[CVI_BIN_ID_HEADER] = sizeof(CVI_BIN_HEADER);

	for (CVI_U32 idx = CVI_BIN_ID_MIN; idx < CVI_BIN_ID_MAX; idx++) {
		if ((getBinSizeFunc[idx] != NULL) && (check_is_register_id(idx) == CVI_SUCCESS)) {
			getBinSizeFunc[idx]((CVI_U32 *)&(header.size[idx]));
		}
	}

	u32TempLen = fwrite(&header, sizeof(CVI_BIN_HEADER), 1, fp);
	if (u32TempLen != 1) {
		ret = CVI_BIN_UPDATE_ERROR;
		CVI_TRACE_SYS(CVI_DBG_WARN, "Write bin header infor to file fail!\n");
	}

	for (CVI_U32 idx = CVI_BIN_ID_MIN; idx < CVI_BIN_ID_MAX; idx++) {
		CVI_U32 addr = ftell(fp);

		u32BinParaSize += header.size[idx];
		if ((setParamToBinFunc[idx] != NULL) && (check_is_register_id(idx) == CVI_SUCCESS)) {
			ret = setParamToBinFunc[idx](fp);
			if (header.size[idx] != (ftell(fp) - addr) && idx != CVI_BIN_ID_MIN) {
				CVI_TRACE_SYS(CVI_DBG_WARN, "Write data length mismatch (%ld > %d)\n",
					      (ftell(fp) - addr), header.size[idx]);
				ret = CVI_FAILURE;
			}
		}
	}

	/*save json para to bin file.*/
	fseek(fp, u32BinParaSize + sizeof(CVI_JSON_HEADER), SEEK_SET);
	/*save compress jsonfile para to cvi_sdr_bin*/
	for (CVI_U32 idx = CVI_BIN_ID_MIN; idx < CVI_BIN_ID_MAX; idx++) {
		if (idx == CVI_BIN_ID_HEADER) {
			stJsonHeader.size[idx].u32InitSize = sizeof(CVI_JSON_HEADER);
			stJsonHeader.size[idx].u32CompreSize = sizeof(CVI_JSON_HEADER);
		} else {
			#ifndef DISABLE_PQBIN_JSON
			if ((setParamToBinFunc[idx] != NULL) && (check_is_register_id(idx) == CVI_SUCCESS)) {
				CVI_JSON_SaveParamToFile(fp, idx, &stJsonHeader.size[idx], setParamToJsonBuf[idx]);
			}
			#endif
		}
	}
	fseek(fp, u32BinParaSize, SEEK_SET);
	u32TempLen = fwrite(&stJsonHeader, sizeof(CVI_JSON_HEADER), 1, fp);
	if (u32TempLen != 1) {
		ret = CVI_BIN_UPDATE_ERROR;
		CVI_TRACE_SYS(CVI_DBG_WARN, "Write json header infor to file fail!\n");
	}

	/*calcute md5 value and write md5 to end of file.*/
	ret = write_md5_value_to_file(fp);

	return ret;
}

CVI_S32 CVI_BIN_LoadParamFromBin(enum CVI_BIN_SECTION_ID id, CVI_U8 *buf)
{
#ifdef RPC_MULTI_PROCESS
	if (!isMaster()) {
		return rpc_client_bin_loadparamfrombin(id, buf);
	}
#endif
	if (buf == NULL) {
		return CVI_BIN_NULL_POINT;
	}

	CVI_S32 ret = CVI_SUCCESS;
	CVI_BOOL bBlkSizeInvalid = CVI_FALSE;

	/*reserve the API. if customer use directly, it may report following warning log.*/
	if (g_bUseOldLoadAPI) {
		CVI_TRACE_SYS(CVI_DBG_WARN,
				"Note: this API interface is deprecated and may be removed in the future.\n");
		ret = check_bin_file_validity(buf, 0);
		if (ret == (CVI_S32)CVI_BIN_FILE_ERROR) {
			goto ERROR_HANDLER;
		}
	}
	ret = _isSectionIdValid(id);
	if (ret != CVI_SUCCESS) {
		ret = CVI_BIN_ID_ERROR;
		CVI_TRACE_SYS(CVI_DBG_WARN, "Invalid id(%d)\n", id);
		goto ERROR_HANDLER;
	}

	CVI_BIN_HEADER *pstHeader = (CVI_BIN_HEADER *)buf;
	CVI_U32 secSize = pstHeader->size[CVI_BIN_ID_HEADER] - sizeof(CVI_U32) - sizeof(CVI_BIN_EXTRA_S);
	CVI_U32 secCnt = secSize / sizeof(CVI_U32);
	CVI_U8 *addr = (CVI_U8 *)buf;

	if (secCnt < id) {
		ret = CVI_BIN_ID_ERROR;
		CVI_TRACE_SYS(CVI_DBG_WARN, "Module(%d) not contained in this BIN\n", id);
	} else {
		for (CVI_U32 idx = CVI_BIN_ID_MIN; idx < id; idx++) {
			addr += pstHeader->size[idx];
		}
		if ((getParamFromBinFunc[id] != NULL) && (check_is_register_id(id) == CVI_SUCCESS)) {
			ret = check_sensor_num_is_exceeded(id);
			if (ret != CVI_SUCCESS) {
				CVI_TRACE_SYS(CVI_DBG_WARN, "Sensor number exceeds specified number in PQbin file\n");
				goto ERROR_HANDLER;
			}
			bBlkSizeInvalid = !(pstHeader->size[id]);
			if (bBlkSizeInvalid) {
				#ifndef DISABLE_PQBIN_JSON
				CVI_TRACE_SYS(CVI_DBG_WARN, "Size of block(%d) is 0, get para from json!\n", id);
				ret = get_json_para_from_buffer(id, buf);
				#else
				CVI_TRACE_SYS(CVI_DBG_WARN, "Size of block(%d) is 0 & json inexistence!\n", id);
				ret = CVI_BIN_JSON_ERR;
				#endif
			} else {
				ret = getParamFromBinFunc[id](addr, pstHeader->size[id]);
				if (ret != CVI_SUCCESS) {
					#ifndef DISABLE_PQBIN_JSON
					CVI_TRACE_SYS(CVI_DBG_WARN, "Get cur block(%d) para from json!\n", id);
					ret = get_json_para_from_buffer(id, buf);
					#else
					CVI_TRACE_SYS(CVI_DBG_WARN,
						"Get cur block(%d) err=%08x & json inexistence!\n", id, ret);
					ret = CVI_BIN_JSON_ERR;
					#endif
				}
			}
		}
	}

ERROR_HANDLER:
	return ret;
}

CVI_S32 CVI_BIN_LoadParamFromBinEx(enum CVI_BIN_SECTION_ID id, CVI_U8 *buf, CVI_U32 u32DataLength)
{
	CVI_S32 ret = CVI_SUCCESS;

	if (u32DataLength == 0) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "input length of buffer can't be 0!\n");
		return CVI_BIN_SIZE_ERR;
	}

	g_bUseOldLoadAPI = CVI_FALSE;
	ret = check_bin_file_validity(buf, u32DataLength);
	if (ret == (CVI_S32)CVI_BIN_FILE_ERROR) {
		goto ERROR_HANDLER;
	}
	ret = CVI_BIN_LoadParamFromBin(id, buf);
	g_bUseOldLoadAPI = CVI_TRUE;

ERROR_HANDLER:
	return ret;
}

CVI_S32 CVI_BIN_GetBinExtraAttr(FILE *fp, CVI_BIN_EXTRA_S *extraInfo)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_BIN_HEADER header;

	fread(&header, sizeof(CVI_BIN_HEADER), 1, fp);
	memcpy(extraInfo, &(header.extraInfo), sizeof(CVI_BIN_EXTRA_S));

	return ret;
}

CVI_S32 CVI_BIN_SetBinName(WDR_MODE_E wdrMode, const CVI_CHAR *binName)
{
#ifdef RPC_MULTI_PROCESS
	if (!isMaster()) {
		return rpc_client_bin_setbinname(wdrMode, binName);
	}
#endif
	if (binName == NULL) {
		CVI_TRACE_SYS(CVI_DBG_WARN, "binName is NULL\n");
		return CVI_FAILURE;
	}

	return _setBinNameImp(wdrMode, binName);
}

CVI_S32 CVI_BIN_GetBinName(CVI_CHAR *binName)
{
#ifdef RPC_MULTI_PROCESS
	if (!isMaster()) {
		return rpc_client_bin_getbinname(binName);
	}
#endif

	if (binName == NULL) {
		CVI_TRACE_SYS(CVI_DBG_WARN, "binName is NULL\n");
		return CVI_FAILURE;
	}

	return _getBinNameImp(binName);
}
