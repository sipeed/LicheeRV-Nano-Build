//------------------------------------------------------------------------------
// File: main.c
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------
#ifdef ENABLE_DEC
#ifdef VC_DRIVER_TEST
#include <linux/cvi_base_ctx.h>
#include <base_cb.h>

#include "cvi_vc_drv.h"
#include "cvi_vc_drv_proc.h"
#include "vcodec_cb.h"
#include "cvi_vc_getopt.h"

#include "vdi_osal.h"
#include "config.h"
#include "main_helper.h"
#include "vpuapifunc.h"
#include "cvi_vcodec_lib.h"
#include "cvi_dec_internal.h"
#include "cvi_h265_interface.h"

#define STREAM_BUF_SIZE_HEVC                                                   \
	0xA00000 // max bitstream size(HEVC:10MB,VP9:not specified)
#define STREAM_BUF_SIZE_VP9                                                    \
	0x1400000 // max bitstream size(HEVC:10MB,VP9:not specified)
#define USERDATA_BUFFER_SIZE (512 * 1024)
#define EXTRA_FRAME_BUFFER_NUM 1
#define MAX_SEQUENCE_MEM_COUNT 16
#define MAX_NOT_DEC_COUNT 200
#define NUM_VCORES 1
#define FEEDING_SIZE 0x20000

BOOL h265_TestDecoder(cviVideoDecoder *pvdec)
{
	int decStatus;

	/********************************************************************************
	 * PIC_RUN *
	 ********************************************************************************/
	while (TRUE) {
		decStatus = (CVI_DEC_STATUS)cviVDecDecOnePic(pvdec, NULL,
							     H26X_BLOCK_MODE);
		if (decStatus == CVI_VDEC_RET_CONTI) {
			CVI_VC_TRACE("CVI_VDEC_RET_CONTI\n");
			continue;
		} else if (decStatus == CVI_VDEC_RET_STOP) {
			CVI_VC_TRACE("CVI_VDEC_RET_STOP\n");
			break;
		} else if (decStatus == CVI_VDEC_RET_LAST_FRM) {
			CVI_VC_TRACE("CVI_VDEC_RET_LAST_FRM\n");
			break;
		} else if (decStatus == CVI_VDEC_RET_DEC_ERR) {
			CVI_VC_ERR("decStatus = %d\n", decStatus);
			return CVI_VDEC_RET_DEC_ERR;
		}
	}

	pvdec->success = TRUE;

	return (pvdec->success == TRUE);
}

int h265_dec_main(int argc, char **argv)
{
	cviInitDecConfig initDecCfg, *pInitDecCfg;
	cviVideoDecoder *pvdec;
	Int32 ret = FALSE;
	CVI_DEC_STATUS decStatus;

	pInitDecCfg = &initDecCfg;
	memset(pInitDecCfg, 0, sizeof(cviInitDecConfig));
	pInitDecCfg->cviApiMode = API_MODE_DRIVER;
	pInitDecCfg->codec = CODEC_H265;
	pInitDecCfg->argc = argc;
	pInitDecCfg->argv = argv;

	decStatus = cviVDecOpen(pInitDecCfg, (void *)&pvdec);
	if (decStatus < 0) {
		CVI_VC_ERR("cviVDecOpen, %d\n", decStatus);
		return 1;
	}

	ret = h265_TestDecoder(pvdec);

	cviVDecClose((void *)pvdec);

	return ret == TRUE ? 0 : 1;
}

int h265_dec_test(u_long arg)
{
#define MAX_ARG_CNT 30
	char buf[512];
	char *pArgv[MAX_ARG_CNT] = {0};
	char *save_ptr;
	unsigned int u32Argc = 0;
	char *pBuf;
	unsigned int __user *argp = (unsigned int __user *)arg;

	memset(buf, 0, 512);
	if (argp != NULL) {
		if (copy_from_user(buf, (char *)argp, 512))
			return -1;
	}
	pBuf = buf;

	while (NULL != (pArgv[u32Argc] = cvi_strtok_r(pBuf, " ", &save_ptr))) {
		u32Argc++;
		if (u32Argc >= MAX_ARG_CNT) {
			break;
		}
		pBuf = NULL;
	}

	return h265_dec_main(u32Argc, pArgv);
}
#endif
#endif