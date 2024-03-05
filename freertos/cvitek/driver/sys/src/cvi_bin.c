
#include "cvi_bin.h"
#include "cvi_vi.h"
#include "cvi_comm_sys.h"
#include "cvi_comm_vi.h"

#define CHIP_ID 0x1835

static char userBinName[WDR_MODE_MAX][BIN_FILE_LENGTH] = {
	"/mnt/data/bin/cvi_sdr_bin",
	"/mnt/data/bin/cvi_sdr_bin",
	"/mnt/data/bin/cvi_sdr_bin",
	"/mnt/data/bin/cvi_wdr_bin",
	"/mnt/data/bin/cvi_wdr_bin",
	"/mnt/data/bin/cvi_wdr_bin",
	"/mnt/data/bin/cvi_wdr_bin",
	"/mnt/data/bin/cvi_wdr_bin",
	"/mnt/data/bin/cvi_wdr_bin",
	"/mnt/data/bin/cvi_wdr_bin",
	"/mnt/data/bin/cvi_wdr_bin",
	"/mnt/data/bin/cvi_wdr_bin",
};

static char defaultBinName[WDR_MODE_MAX][BIN_FILE_LENGTH] = {
	"/mnt/cfg/param/cvi_sdr_bin",
	"/mnt/cfg/param/cvi_sdr_bin",
	"/mnt/cfg/param/cvi_sdr_bin",
	"/mnt/cfg/param/cvi_wdr_bin",
	"/mnt/cfg/param/cvi_wdr_bin",
	"/mnt/cfg/param/cvi_wdr_bin",
	"/mnt/cfg/param/cvi_wdr_bin",
	"/mnt/cfg/param/cvi_wdr_bin",
	"/mnt/cfg/param/cvi_wdr_bin",
	"/mnt/cfg/param/cvi_wdr_bin",
	"/mnt/cfg/param/cvi_wdr_bin",
	"/mnt/cfg/param/cvi_wdr_bin",
};

static CVI_BOOL binEncypt = CVI_FALSE;

struct CVI_BIN_HEADER {
	CVI_U32 chipId;
	CVI_BIN_EXTRA_S extraInfo;
	CVI_U32 size[CVI_BIN_ID_MAX];
};

static CVI_S32 _isSectionIdValid(enum CVI_BIN_SECTION_ID id);
static CVI_S32 _setBinNameImp(WDR_MODE_E wdrMode, const CVI_CHAR *binName);
static CVI_S32 _getBinNameImp(CVI_CHAR *binName);

CVI_BIN_getBinSize getBinSizeFunc[CVI_BIN_ID_MAX];
CVI_BIN_getParamFromBin getParamFromBinFunc[CVI_BIN_ID_MAX];
CVI_BIN_setParamToBin setParamToBinFunc[CVI_BIN_ID_MAX];

CVI_S32 CVI_BIN_Register(struct CVI_BIN_REGISTER_PARAM *param)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = _isSectionIdValid(param->id);
	if (ret == CVI_SUCCESS) {
		if (param->funcSize != NULL) {
			getBinSizeFunc[param->id] = param->funcSize;
		} else {
			CVI_TRACE_SYS(CVI_DBG_WARN, "funcSize NULL(%p)\n", param->funcSize);
		}
		if (param->funcGetParam != NULL) {
			getParamFromBinFunc[param->id] = param->funcGetParam;
		} else {
			CVI_TRACE_SYS(CVI_DBG_WARN, "funcGetParam NULL(%p)\n", param->funcSize);
		}
		if (param->funcSetParam != NULL) {
			setParamToBinFunc[param->id] = param->funcSetParam;
		} else {
			CVI_TRACE_SYS(CVI_DBG_WARN, "funcSetParam NULL(%p)\n", param->funcSize);
		}
	}

	return ret;
}

CVI_S32 CVI_BIN_Unregister(enum CVI_BIN_SECTION_ID id)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = _isSectionIdValid(id);

	if (ret == CVI_SUCCESS) {
		getBinSizeFunc[id] = NULL;
		getParamFromBinFunc[id] = NULL;
		setParamToBinFunc[id] = NULL;
	}

	return ret;
}

CVI_S32 CVI_BIN_SaveParamToBin(FILE *fp, CVI_BIN_EXTRA_S *extraInfo)
{
	CVI_S32 ret = CVI_SUCCESS;

	struct CVI_BIN_HEADER header = {0};

	header.chipId = CHIP_ID;
	header.extraInfo = *extraInfo;
	header.size[CVI_BIN_ID_HEADER] = sizeof(struct CVI_BIN_HEADER);

	for (CVI_U32 idx = CVI_BIN_ID_MIN ; idx < CVI_BIN_ID_MAX ; idx++) {
		if (getBinSizeFunc[idx] != NULL)
			getBinSizeFunc[idx]((CVI_U32 *)&(header.size[idx]));
	}

	fwrite(&header, sizeof(struct CVI_BIN_HEADER), 1, fp);

	for (CVI_U32 idx = CVI_BIN_ID_MIN ; idx < CVI_BIN_ID_MAX ; idx++) {

		CVI_U32 addr = ftell(fp);

		if (setParamToBinFunc[idx] != NULL) {
			ret = setParamToBinFunc[idx](fp);

			if (header.size[idx] != (ftell(fp) - addr)) {
				CVI_TRACE_SYS(CVI_DBG_WARN, "Write data length mismatch (%ld > %d)\n",
					(ftell(fp) - addr), header.size[idx]);
				ret = CVI_FAILURE;
			}
		}
	}

	return ret;
}

CVI_S32 CVI_BIN_LoadParamFromBin(enum CVI_BIN_SECTION_ID id, CVI_U8 *buf)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = _isSectionIdValid(id);
	if (ret == CVI_FAILURE) {
		CVI_TRACE_SYS(CVI_DBG_WARN, "Invalid id(%d)\n", id);
		goto CVI_BIN_loadParamFromBin_EXIT;
	}

	struct CVI_BIN_HEADER *header = (struct CVI_BIN_HEADER *) buf;

	CVI_U32 secSize = header->size[CVI_BIN_ID_HEADER] - sizeof(CVI_U32) - sizeof(CVI_BIN_EXTRA_S);
	CVI_U32 secCnt = secSize / sizeof(CVI_U32);
	CVI_U8 *addr = (CVI_U8 *) buf;

	if (secCnt < id) {
		CVI_TRACE_SYS(CVI_DBG_WARN, "Module(%d) not contained in this BIN\n", id);
	} else {
		for (CVI_U32 idx = CVI_BIN_ID_MIN ; idx < id; idx++) {
			addr += header->size[idx];
		}
		if (getParamFromBinFunc[id] != NULL) {
			ret = getParamFromBinFunc[id](addr, header->size[id]);
		}
	}

CVI_BIN_loadParamFromBin_EXIT:
	return ret;
}

CVI_S32 CVI_BIN_GetBinExtraAttr(FILE *fp, CVI_BIN_EXTRA_S *extraInfo)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct CVI_BIN_HEADER header;

	fread(&header, sizeof(struct CVI_BIN_HEADER), 1, fp);
	memcpy(extraInfo, &(header.extraInfo), sizeof(CVI_BIN_EXTRA_S));

	return ret;
}

CVI_S32 CVI_BIN_SetEncrypt(void)
{
	binEncypt = CVI_TRUE;
	return CVI_SUCCESS;
}

CVI_S32 CVI_BIN_SetBinName(WDR_MODE_E wdrMode, const CVI_CHAR *binName)
{
	if (binEncypt == CVI_TRUE) {
		CVI_TRACE_SYS(CVI_DBG_WARN, "Call fail, due to encrypt protection\n");
		return CVI_FAILURE;
	}

	return _setBinNameImp(wdrMode, binName);
}

CVI_S32 CVI_BIN_GetBinName(CVI_CHAR *binName)
{
	if (binEncypt == CVI_TRUE) {
		CVI_TRACE_SYS(CVI_DBG_WARN, "Call fail, due to encrypt protection\n");
		return CVI_FAILURE;
	}

	return _getBinNameImp(binName);
}


/* Static function */

static CVI_S32 _setBinNameImp(WDR_MODE_E wdrMode, const CVI_CHAR *binName)
{
	CVI_U32 len = (CVI_U32)strlen(binName);

	if (binName == NULL) {
		CVI_TRACE_SYS(CVI_DBG_WARN, "binName is NULL\n");
		return CVI_FAILURE;
	}

	if (len >= BIN_FILE_LENGTH) {
		CVI_TRACE_SYS(CVI_DBG_WARN, "Set bin name failed, strlen(%u) >= %d!\n",
			len, BIN_FILE_LENGTH);
		return CVI_FAILURE;
	}

	strncpy(userBinName[wdrMode], binName, BIN_FILE_LENGTH);

	return CVI_SUCCESS;
}

static CVI_S32 _getBinNameImp(CVI_CHAR *binName)
{
	VI_DEV_ATTR_S pstDevAttr;
	WDR_MODE_E wdrMode;

	CVI_VI_GetDevAttr(0, &pstDevAttr);
	wdrMode = pstDevAttr.stWDRAttr.enWDRMode;

	if (binName == NULL) {
		CVI_TRACE_SYS(CVI_DBG_WARN, "binName is NULL\n");
		return CVI_FAILURE;
	}

	if (binEncypt == CVI_TRUE) {
		sprintf(binName, "%s", defaultBinName[wdrMode]);
	} else {
		sprintf(binName, "%s", userBinName[wdrMode]);
	}

	return CVI_SUCCESS;
}

static CVI_S32 _isSectionIdValid(enum CVI_BIN_SECTION_ID id)
{
	CVI_S32 ret = CVI_SUCCESS;

	if (((id < CVI_BIN_ID_MIN) || (id >= CVI_BIN_ID_MAX)))
		ret = CVI_FAILURE;

	return ret;
}
