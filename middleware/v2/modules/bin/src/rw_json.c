#ifndef DISABLE_PQBIN_JSON
#include <stdlib.h>
#include "rw_json.h"
#include "cvi_miniz.h"
#include <linux/cvi_comm_sys.h>
#include "cvi_bin.h"
#include "cvi_debug.h"

static CVI_S32 _isSectionIdValid(enum CVI_BIN_SECTION_ID id)
{
	CVI_S32 ret = CVI_SUCCESS;

	if ((id < CVI_BIN_ID_MIN) || (id >= CVI_BIN_ID_MAX)) {
		ret = CVI_FAILURE;
	}

	return ret;
}

CVI_S32 CVI_JSON_LoadParamFromBuffer(enum CVI_BIN_SECTION_ID id, CVI_U8 *buf,
pfn_cvi_json_getparamfrombuf pFuncgetParam)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_U8 *addr = (CVI_U8 *)buf;
	CVI_U8 *pu8Uncomprebuf = NULL;
	uLongf UncompreBufLen = 0;
	uLongf CompreBufLen = 0;
	int iResult = 0;

	ret = _isSectionIdValid(id);
	if (ret != CVI_SUCCESS) {
		ret = CVI_BIN_ID_ERROR;
		CVI_TRACE_SYS(LOG_ERR, "Invalid id(%d)\n", id);
		goto ERROR_HANDLER;
	}

	if (buf == NULL) {
		ret = CVI_BIN_NULL_POINT;
		CVI_TRACE_SYS(LOG_ERR, "input buffer pointer is null!");
		goto ERROR_HANDLER;
	}
	if (pFuncgetParam == NULL) {
		ret = CVI_BIN_NULL_POINT;
		CVI_TRACE_SYS(LOG_ERR, "get json callback isn't registered!");
		goto ERROR_HANDLER;
	}

	/*get addr of bin header*/
	CVI_BIN_HEADER *header = (CVI_BIN_HEADER *)buf;

	for (CVI_U32 idx = CVI_BIN_ID_MIN; idx < CVI_BIN_ID_MAX; idx++) {
		addr += header->size[idx];
	}
	/*get addr of json header*/
	CVI_JSON_HEADER *pJsonHeader = (CVI_JSON_HEADER *)addr;

	for (CVI_U32 idx = CVI_BIN_ID_MIN; idx < id; idx++) {
		addr += pJsonHeader->size[idx].u32CompreSize;
	}
	UncompreBufLen = pJsonHeader->size[id].u32InitSize;
	pu8Uncomprebuf = (CVI_U8 *)malloc(UncompreBufLen);
	if (pu8Uncomprebuf == NULL) {
		ret = CVI_BIN_MALLOC_ERR;
		goto ERROR_HANDLER;
	}

	/*start to uncompress*/
	CompreBufLen = pJsonHeader->size[id].u32CompreSize;
	iResult = cvi_uncompress2(pu8Uncomprebuf, &UncompreBufLen, addr, &CompreBufLen);
	if (iResult != Z_OK) {
		ret = CVI_BIN_UNCOMPRESS_ERROR;
		CVI_TRACE_SYS(LOG_ERR, "block(%d) Uncompress abnormally! abnormal value is:%d\n", id, iResult);
		goto ERROR_HANDLER;
	}
	/*load json para to struct*/
	ret = pFuncgetParam((char *)pu8Uncomprebuf, id);

ERROR_HANDLER:
	if (pu8Uncomprebuf != NULL) {
		free(pu8Uncomprebuf);
	}

	return ret;
}

CVI_S32 CVI_JSON_SaveParamToBuffer(unsigned char *buffer, enum CVI_BIN_SECTION_ID id, CVI_JSON_INFO *pJsonInfo,
pfn_cvi_json_setparamtobuf pFuncsetParam, CVI_U32 u32FreeSpaceSize)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_S8 *buf = NULL;
	CVI_U8 *pu8CompressDestBuf = NULL;
	uLongf CompressBufLen = 0;
	CVI_U32 u32BufferLen = 0;
	int iResult = 0;

	ret = _isSectionIdValid(id);
	if (ret != CVI_SUCCESS) {
		ret = CVI_BIN_ID_ERROR;
		CVI_TRACE_SYS(LOG_ERR, "Invalid id(%d)\n", id);
		goto ERROR_HANDLER;
	}

	if (pJsonInfo == NULL) {
		ret = CVI_BIN_NULL_POINT;
		CVI_TRACE_SYS(LOG_ERR, "input pJsonInfo pointer is null!");
		goto ERROR_HANDLER;
	}
	if (pFuncsetParam == NULL) {
		ret = CVI_BIN_NULL_POINT;
		CVI_TRACE_SYS(LOG_ERR, "set json callback isn't registered!");
		goto ERROR_HANDLER;
	}

	pFuncsetParam(&buf, id, (CVI_S32 *)&u32BufferLen);
	CompressBufLen = cvi_compressBound(u32BufferLen);

	/*compress json buf*/
	pu8CompressDestBuf = (CVI_U8 *)malloc(CompressBufLen);
	if (pu8CompressDestBuf == NULL) {
		ret = CVI_BIN_MALLOC_ERR;
		CVI_TRACE_SYS(LOG_WARNING, "%s\n", "Allocate memory fail");
		goto ERROR_HANDLER;
	}

	iResult = cvi_compress2((Bytef *)pu8CompressDestBuf, &CompressBufLen, (const Bytef *)buf,
			    (uLong)u32BufferLen, Z_DEFAULT_COMPRESSION);
	if (iResult != Z_OK) {
		ret = CVI_BIN_COMPRESS_ERROR;
		CVI_TRACE_SYS(LOG_ERR, "block(%d) Compress abnormally! abnormal value is:%d\n", id, iResult);
		goto ERROR_HANDLER;
	}
	pJsonInfo->u32InitSize = u32BufferLen;
	pJsonInfo->u32CompreSize = (CVI_U32)CompressBufLen;
	if ((CompressBufLen > u32FreeSpaceSize) && (buffer != NULL)) {
		ret = CVI_BIN_SAPCE_ERR;
		CVI_TRACE_SYS(LOG_ERR, "%s\n", "Remaining space of input buffer isn't enough!");
		goto ERROR_HANDLER;
	}

	if (buffer != NULL) {
		memcpy(buffer, pu8CompressDestBuf, CompressBufLen);
	}

ERROR_HANDLER:
	if (pu8CompressDestBuf) {
		free(pu8CompressDestBuf);
	}
	if (buf) {
		free(buf);
	}
	return ret;
}

CVI_S32 CVI_JSON_SaveParamToFile(FILE *fp, enum CVI_BIN_SECTION_ID id, CVI_JSON_INFO *pJsonInfo,
pfn_cvi_json_setparamtobuf pFuncsetParam)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_S8 *buf = NULL;
	CVI_U8 *pu8CompressDestBuf = NULL;
	uLongf CompressBufLen = 0;
	CVI_U32 u32BufferLen = 0;
	int iResult = 0;
	size_t size = 0;

	ret = _isSectionIdValid(id);
	if (ret != CVI_SUCCESS) {
		ret = CVI_BIN_ID_ERROR;
		CVI_TRACE_SYS(LOG_ERR, "Invalid id(%d)\n", id);
		goto ERROR_HANDLER;
	}

	if (pJsonInfo == NULL) {
		ret = CVI_BIN_NULL_POINT;
		CVI_TRACE_SYS(LOG_ERR, "input pJsonInfo pointer is null!");
		goto ERROR_HANDLER;
	}
	if (pFuncsetParam == NULL) {
		ret = CVI_BIN_NULL_POINT;
		CVI_TRACE_SYS(LOG_ERR, "set json callback isn't registered!");
		goto ERROR_HANDLER;
	}

	pFuncsetParam(&buf, id, (CVI_S32 *)&u32BufferLen);
	CompressBufLen = cvi_compressBound(u32BufferLen);

	/*compress json buf*/
	pu8CompressDestBuf = (CVI_U8 *)malloc(CompressBufLen);
	if (pu8CompressDestBuf == NULL) {
		ret = CVI_BIN_MALLOC_ERR;
		CVI_TRACE_SYS(LOG_WARNING, "%s\n", "Allocate memory fail");
		goto ERROR_HANDLER;
	}

	iResult = cvi_compress2((Bytef *)pu8CompressDestBuf, &CompressBufLen, (const Bytef *)buf,
			    (uLong)u32BufferLen, Z_DEFAULT_COMPRESSION);
	if (iResult != Z_OK) {
		ret = CVI_BIN_COMPRESS_ERROR;
		CVI_TRACE_SYS(LOG_ERR, "block(%d) Compress abnormally! abnormal value is:%d\n", id, iResult);
		goto ERROR_HANDLER;
	}
	pJsonInfo->u32InitSize = u32BufferLen;
	pJsonInfo->u32CompreSize = (CVI_U32)CompressBufLen;
	size = fwrite(pu8CompressDestBuf, CompressBufLen, 1, fp);
	if (size != 1) {
		ret = CVI_BIN_UPDATE_ERROR;
		CVI_TRACE_SYS(LOG_WARNING, "Update block(%d) para fail!\n", id);
	}

ERROR_HANDLER:
	if (pu8CompressDestBuf) {
		free(pu8CompressDestBuf);
	}
	if (buf) {
		free(buf);
	}
	return ret;
}
#endif