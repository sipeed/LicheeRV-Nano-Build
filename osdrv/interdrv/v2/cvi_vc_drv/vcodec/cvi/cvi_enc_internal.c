/*
 * Copyright Cvitek Technologies Inc.
 *
 * Created Time: May, 2020
 */
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/of_device.h>
#include <linux/poll.h>
#include <linux/io.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/dma-buf.h>
#include <asm/cacheflush.h>
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
#include <linux/dma-map-ops.h>
#endif
#include "cvi_enc_internal.h"
#include "main_enc_cvitest.h"

#include "cvi_vc_drv.h"
#include "cvi_vcodec_lib.h"
#include "module_common.h"


#define ROUND_UP_N_BIT(val, bit)	((((val) + (1 << (bit)) - 1) >> (bit)) << (bit))

static int cviInitPageTable(stTestEncoder *pTestEnc, int comp, Uint32 startAddr);
static int cviConfigPageTable(CodecInst *pCodecInst);
static int cviFindWriteEntry(CodecInst *pCodecInst, int comp);
static int cviConfigPageTableEntry(CodecInst *pCodecInst, int comp);
static void cviPrintPageTable(CodecInst *pCodecInst);

static int cviEncode265HeaderByType(stTestEncoder *pTestEnc,
				    HevcHeaderType enType)
{
	int ret = TRUE;
	NAL_TYPE nal_type;
	int nalIdx = 0;
#if CACHE_ENCODE_HEADER
	stPack *pPack;
	stStreamPack *psp = &pTestEnc->streamPack;
	Uint8 *pSpsBuf = NULL;
#endif

	switch (enType) {
	case CODEOPT_ENC_VPS:
		nal_type = NAL_VPS;
		nalIdx = 0;
		CVI_VC_FLOW("VPS\n");
		break;
	case CODEOPT_ENC_SPS:
		nal_type = NAL_SPS;
		nalIdx = 1;
		CVI_VC_FLOW("SPS\n");
		break;
	case CODEOPT_ENC_PPS:
		nal_type = NAL_PPS;
		nalIdx = 2;
		CVI_VC_FLOW("PPS\n");
	break;
	default:
		CVI_VC_ERR("unknown H265 header type %d\n", enType);
		return FALSE;
	}

	do {
		if (pTestEnc->encOP.ringBufferEnable == TRUE) {
			VPU_EncSetWrPtr(pTestEnc->handle,
					pTestEnc->encOP.bitstreamBuffer, 1);
		} else {
			pTestEnc->encHeaderParam.buf =
				pTestEnc->encOP.bitstreamBuffer;
			pTestEnc->encHeaderParam.size =
				pTestEnc->encOP.bitstreamBufferSize;
		}

	#if CACHE_ENCODE_HEADER
		if (pTestEnc->bEncHeader == 1) {
			MUTEX_LOCK(&psp->packMutex);
			if (psp->totalPacks >= MAX_NUM_PACKS) {
				CVI_VC_ERR("hevc cache header droped, type:%d, totalpacks:%d\n",
					nal_type, psp->totalPacks);
				MUTEX_UNLOCK(&psp->packMutex);
				return TRUE;
			}

			pPack = &psp->pack[psp->totalPacks];
			pPack->len = pTestEnc->headerBackup[nalIdx].size;
			pPack->addr = pTestEnc->headerBackup[nalIdx].pBuf;
			pPack->u64PhyAddr = pTestEnc->headerBackup[nalIdx].buf;
			pPack->cviNalType = nal_type;
			pPack->need_free = 0;
			pPack->u64PTS = pTestEnc->u64Pts;

			// sps maybe realloc mem for vui info later
			if (enType == CODEOPT_ENC_SPS) {
				pSpsBuf = (Uint8 *)osal_kmalloc(pPack->len);
				memcpy(pSpsBuf, pPack->addr, pPack->len);
				pPack->addr = pSpsBuf;
				pPack->u64PhyAddr = virt_to_phys(pSpsBuf);
				pPack->need_free = 1;
			}

			psp->totalPacks++;
			MUTEX_UNLOCK(&psp->packMutex);
			return TRUE;
		}
	#endif

		pTestEnc->encHeaderParam.headerType = enType;
		ret = VPU_EncGiveCommand(pTestEnc->handle, ENC_PUT_VIDEO_HEADER,
					 &pTestEnc->encHeaderParam);
		if (ret != RETCODE_SUCCESS) {
			CVI_VC_ERR(
				"VPU_EncGiveCommand(ENC_PUT_VIDEO_HEADER) for H265 header type %d failed. Error 0x%x\n",
				enType, ret);
			ret = FALSE;
			break;
		}

		if (pTestEnc->encHeaderParam.size == 0) {
			CVI_VC_ERR("encHeaderParam.size=0\n");
			ret = FALSE;
			break;
		}

#ifdef SUPPORT_DONT_READ_STREAM
		ret = updateBitstream(&pTestEnc->vbStream[0],
				      pTestEnc->encHeaderParam.size);
#else
		if (pTestEnc->encConfig.cviApiMode == API_MODE_SDK) {
		ret = cviPutEsInPack(
			pTestEnc, pTestEnc->encHeaderParam.buf,
			pTestEnc->encHeaderParam.buf + pTestEnc->encHeaderParam.size,
			pTestEnc->encHeaderParam.size, nal_type);
		}
#ifdef VC_DRIVER_TEST
		else {
			ret = BitstreamReader_Act(
				pTestEnc->bsReader,
				pTestEnc->encHeaderParam.buf,
				pTestEnc->encOP.bitstreamBufferSize,
				pTestEnc->encHeaderParam.size,
				pTestEnc->comparatorBitStream);
		}
#endif

#if CACHE_ENCODE_HEADER
		if (pTestEnc->bEncHeader == 0) {
			pPack = &psp->pack[nalIdx];
			pTestEnc->headerBackup[nalIdx].size = pPack->len;
			pTestEnc->headerBackup[nalIdx].pBuf = (Uint8 *)osal_kmalloc(pPack->len);
			pTestEnc->headerBackup[nalIdx].buf = virt_to_phys(pTestEnc->headerBackup[nalIdx].pBuf);
			memcpy(pTestEnc->headerBackup[nalIdx].pBuf, pPack->addr, pPack->len);
		}
#endif

		CVI_VC_BS("encHeaderParam.buf = 0x%llX, size = 0x%zx\n",
			  pTestEnc->encHeaderParam.buf,
			  pTestEnc->encHeaderParam.size);
#endif
	} while (0);

#if CACHE_ENCODE_HEADER
	if (enType == CODEOPT_ENC_PPS)
		pTestEnc->bEncHeader = 1;
#endif

	return ret;
}

int cviCheckIdrPeriod(stTestEncoder *pTestEnc)
{
	EncOpenParam *pEncOP = &pTestEnc->encOP;
	EncParam *pEncParam = &pTestEnc->encParam;
	int isIframe, nthIframe;

	CVI_VC_TRACE("gopSize = %d, frameIdx = %d\n", pEncOP->gopSize,
		     pTestEnc->frameIdx);

	if (pTestEnc->frameIdx == 0) {
		pEncParam->is_i_period = TRUE;
		return TRUE;
	} else if (pEncOP->gopSize == 0) {
		pEncParam->is_i_period = FALSE;
		return FALSE;
	}
	isIframe = ((pTestEnc->frameIdx % pEncOP->gopSize) == 0);
	nthIframe = (pTestEnc->frameIdx / pEncOP->gopSize);

	CVI_VC_RC(
		"gopSize = %d, idrInterval = %d, isIframe = %d, nthIframe = %d\n",
		pEncOP->gopSize, pEncOP->idrInterval, isIframe, nthIframe);

	pEncParam->is_i_period = isIframe;
	if (isIframe) {
		if (pEncOP->idrInterval) {
			if ((nthIframe % pEncOP->idrInterval) == 0)
				return TRUE;
		} else {
			return FALSE;
		}
	}
	return FALSE;
}

static void cviBackupEncodeHeader(stTestEncoder *pTestEnc, int cacheIdx)
{
#if CACHE_ENCODE_HEADER
	stStreamPack *psp = &pTestEnc->streamPack;
	int totalPaks = psp->totalPacks;
	stPack *pPack = NULL;

	if (totalPaks >= 1) {
		pPack = &psp->pack[totalPaks-1];
		pTestEnc->headerBackup[cacheIdx].size = pPack->len;
		pTestEnc->headerBackup[cacheIdx].pBuf = (Uint8 *)osal_kmalloc(pPack->len);
		pTestEnc->headerBackup[cacheIdx].buf = virt_to_phys(pTestEnc->headerBackup[cacheIdx].pBuf);
		memcpy(pTestEnc->headerBackup[cacheIdx].pBuf, pPack->addr, pPack->len);
	}
#endif
}

int cviEncode264Header(stTestEncoder *pTestEnc)
{
	EncOpenParam *pEncOP = &pTestEnc->encOP;
	int ret = TRUE;
	int i;
#if CACHE_ENCODE_HEADER
	int cacheIdx = 0;	// 0: sei; 1:sps; 2:sps mvc; 3:pps; 4:pps mvc
	int nalType;
	stStreamPack *psp = &pTestEnc->streamPack;
	stPack *pPack = NULL;
	Uint8 *pSpsBuf = NULL;

	if (pTestEnc->bEncHeader == 1) {
		for (cacheIdx = 0; cacheIdx < 8; ++cacheIdx) {
			if (pTestEnc->headerBackup[cacheIdx].size) {
				switch (cacheIdx) {
				case 0:
					nalType = NAL_SEI;
					break;
				case 1:
				case 2:
					nalType = NAL_SPS;
					break;
				case 3:
				case 4:
					nalType = NAL_PPS;
					break;
				default:
					CVI_VC_ERR("unknown H264 header type %d\n", cacheIdx);
					continue;
				}

				MUTEX_LOCK(&psp->packMutex);
				if (psp->totalPacks >= MAX_NUM_PACKS) {
					CVI_VC_ERR("avc cache header droped, type:%d, totalpacks:%d\n",
						nalType, psp->totalPacks);
					MUTEX_UNLOCK(&psp->packMutex);
					return TRUE;
				}

				pPack = &psp->pack[psp->totalPacks];
				pPack->len = pTestEnc->headerBackup[cacheIdx].size;
				pPack->addr = pTestEnc->headerBackup[cacheIdx].pBuf;
				pPack->u64PhyAddr = pTestEnc->headerBackup[cacheIdx].buf;
				pPack->cviNalType = nalType;
				pPack->need_free = 0;
				pPack->u64PTS = pTestEnc->u64Pts;

				// sps maybe realloc mem for vui info later
				if (nalType == NAL_SPS) {
					pSpsBuf = (Uint8 *)osal_kmalloc(pPack->len);
					memcpy(pSpsBuf, pPack->addr, pPack->len);
					pPack->addr = pSpsBuf;
					pPack->u64PhyAddr = virt_to_phys(pSpsBuf);
					pPack->need_free = 1;
				}

				psp->totalPacks++;
				MUTEX_UNLOCK(&psp->packMutex);
			}
		}
		return TRUE;
	}
#endif

	do {
		pTestEnc->encHeaderParam.zeroPaddingEnable = 0;

		if (pEncOP->EncStdParam.avcParam.mvcExtension &&
		    pEncOP->EncStdParam.avcParam.parasetRefreshEn)
			break;

		if (pTestEnc->handle->CodecInfo->encInfo.max_temporal_id) {
			pTestEnc->encHeaderParam.headerType = SVC_RBSP_SEI;
			if (pEncOP->ringBufferEnable == FALSE) {
				pTestEnc->encHeaderParam.buf =
					pTestEnc->vbStream[0].phys_addr;
			}
			pTestEnc->encHeaderParam.size =
				pTestEnc->vbStream[0].size;

			ret = VPU_EncGiveCommand(
				     pTestEnc->handle, ENC_PUT_VIDEO_HEADER,
				     &pTestEnc->encHeaderParam);
			if (ret == RETCODE_SUCCESS) {
				ret = TRUE;
			} else {
				CVI_VC_ERR(
					"VPU_EncGiveCommand ( ENC_PUT_VIDEO_HEADER ) for SPS_RBSP failed Error code is 0x%x\n",
					ret);
				ret = FALSE;
				break;
			}

			if (pEncOP->ringBufferEnable == FALSE) {
				if (pTestEnc->encConfig.cviApiMode == API_MODE_SDK) {
					ret = cviPutEsInPack(
						pTestEnc,
						pTestEnc->encHeaderParam.buf,
						pTestEnc->encHeaderParam.buf,
						pTestEnc->encHeaderParam.size,
						NAL_SEI);
					cviBackupEncodeHeader(pTestEnc, 0);
				}
			#ifdef VC_DRIVER_TEST
				else {
					ret = BitstreamReader_Act(
						pTestEnc->bsReader,
						pTestEnc->encHeaderParam.buf,
						pTestEnc->encHeaderParam.size,
						0,
						pTestEnc->comparatorBitStream);
				}
			#endif
				if (ret == FALSE) {
					CVI_VC_ERR("\n");
					break;
				}
			}
		}

		CVI_VC_FLOW("SPS\n");
		pTestEnc->encHeaderParam.headerType = SPS_RBSP;
		if (pEncOP->ringBufferEnable == FALSE) {
			pTestEnc->encHeaderParam.buf =
				pTestEnc->vbStream[0].phys_addr;
		}
		pTestEnc->encHeaderParam.size = pTestEnc->vbStream[0].size;

		ret = VPU_EncGiveCommand(
			     pTestEnc->handle, ENC_PUT_VIDEO_HEADER,
			     &pTestEnc->encHeaderParam);
		if (ret == RETCODE_SUCCESS) {
			ret = TRUE;
		} else {
			CVI_VC_ERR(
				"VPU_EncGiveCommand ( ENC_PUT_VIDEO_HEADER ) for SPS_RBSP failed Error code is 0x%x\n",
				ret);
			ret = FALSE;
			break;
		}

		if (pEncOP->ringBufferEnable == FALSE) {
			if (pTestEnc->encConfig.cviApiMode == API_MODE_SDK) {
				ret = cviPutEsInPack(
					pTestEnc, pTestEnc->encHeaderParam.buf,
					pTestEnc->encHeaderParam.buf,
					pTestEnc->encHeaderParam.size, NAL_SPS);
				cviBackupEncodeHeader(pTestEnc, 1);
			}
		#ifdef VC_DRIVER_TEST
			else {
				ret = BitstreamReader_Act(
					pTestEnc->bsReader,
					pTestEnc->encHeaderParam.buf,
					pTestEnc->encHeaderParam.size, 0,
					pTestEnc->comparatorBitStream);
			}
		#endif
			if (ret == FALSE) {
				CVI_VC_ERR("\n");
				break;
			}
		}

		if (pEncOP->EncStdParam.avcParam.mvcExtension == TRUE) {
			if (pEncOP->ringBufferEnable == 0)
				pTestEnc->encHeaderParam.buf =
					pTestEnc->vbStream[0].phys_addr;

			pTestEnc->encHeaderParam.headerType = SPS_RBSP_MVC;
			pTestEnc->encHeaderParam.size =
				pTestEnc->vbStream[0].size;
			ret = VPU_EncGiveCommand(pTestEnc->handle,
						 ENC_PUT_VIDEO_HEADER,
						 &pTestEnc->encHeaderParam);
			if (ret == RETCODE_SUCCESS) {
				ret = TRUE;
			} else {
				CVI_VC_ERR(
					"VPU_EncGiveCommand ( ENC_PUT_VIDEO_HEADER ) for SPS_RBSP_MVC failed Error code is 0x%x\n",
					ret);
				ret = FALSE;
				break;
			}
			if (pEncOP->ringBufferEnable == FALSE) {
				if (pTestEnc->encConfig.cviApiMode == API_MODE_SDK) {
					ret = cviPutEsInPack(
						pTestEnc,
						pTestEnc->encHeaderParam.buf,
						pTestEnc->encHeaderParam.buf,
						pTestEnc->encHeaderParam.size,
						NAL_SPS);
					cviBackupEncodeHeader(pTestEnc, 2);
				}
			#ifdef VC_DRIVER_TEST
				else {
					ret = BitstreamReader_Act(
						pTestEnc->bsReader,
						pTestEnc->encHeaderParam.buf,
						pTestEnc->encHeaderParam.size,
						0,
						pTestEnc->comparatorBitStream);
				}
			#endif
				if (ret == FALSE) {
					CVI_VC_ERR("\n");
					break;
				}
			}
		}

		CVI_VC_FLOW("PPS, ppsNum = %d\n",
			    pEncOP->EncStdParam.avcParam.ppsNum);
		pTestEnc->encHeaderParam.headerType = PPS_RBSP;
		for (i = 0; i < pEncOP->EncStdParam.avcParam.ppsNum; i++) {
			if (pEncOP->ringBufferEnable == FALSE) {
				pTestEnc->encHeaderParam.buf =
					pTestEnc->vbStream[0].phys_addr;
				pTestEnc->encHeaderParam.pBuf =
					(BYTE *)pTestEnc->vbStream[0].virt_addr;
			}
			pTestEnc->encHeaderParam.size =
				pTestEnc->vbStream[0].size;
			ret = VPU_EncGiveCommand(pTestEnc->handle,
						 ENC_PUT_VIDEO_HEADER,
						 &pTestEnc->encHeaderParam);
			if (ret == RETCODE_SUCCESS) {
				ret = TRUE;
			} else {
				CVI_VC_ERR(
					"VPU_EncGiveCommand ( ENC_PUT_VIDEO_HEADER ) for PPS_RBSP failed Error code is 0x%x\n",
					ret);
				ret = FALSE;
				break;
			}
			if (pEncOP->ringBufferEnable == FALSE) {
				if (pTestEnc->encConfig.cviApiMode ==
				    API_MODE_SDK) {
					ret = cviPutEsInPack(
						pTestEnc,
						pTestEnc->encHeaderParam.buf,
						pTestEnc->encHeaderParam.buf,
						pTestEnc->encHeaderParam.size,
						NAL_PPS);
					cviBackupEncodeHeader(pTestEnc, 3);
				}
			#ifdef VC_DRIVER_TEST
				else {
					ret = BitstreamReader_Act(
						pTestEnc->bsReader,
						pTestEnc->encHeaderParam.buf,
						pTestEnc->encHeaderParam.size,
						0,
						pTestEnc->comparatorBitStream);
				}
			#endif
				if (ret == FALSE) {
					CVI_VC_ERR("\n");
					break;
				}
			}
		}

		CVI_VC_TRACE("\n");

		if (pEncOP->EncStdParam.avcParam.mvcExtension) {
			pTestEnc->encHeaderParam.headerType = PPS_RBSP_MVC;
			pTestEnc->encHeaderParam.size =
				pTestEnc->vbStream[0].size;
			ret = VPU_EncGiveCommand(
				     pTestEnc->handle, ENC_PUT_VIDEO_HEADER,
				     &pTestEnc->encHeaderParam);
			if (ret == RETCODE_SUCCESS) {
				ret = TRUE;
			} else {
				CVI_VC_ERR(
					"VPU_EncGiveCommand ( ENC_PUT_VIDEO_HEADER ) for PPS_RBSP_MVC failed Error code is 0x%x\n",
					ret);
				ret = FALSE;
				break;
			}
			if (pEncOP->ringBufferEnable == FALSE) {
				if (pTestEnc->encConfig.cviApiMode ==
				    API_MODE_SDK) {
					ret = cviPutEsInPack(
						pTestEnc,
						pTestEnc->encHeaderParam.buf,
						pTestEnc->encHeaderParam.buf,
						pTestEnc->encHeaderParam.size,
						NAL_PPS);
					cviBackupEncodeHeader(pTestEnc, 4);
				}
			#ifdef VC_DRIVER_TEST
				else {
					ret = BitstreamReader_Act(
						pTestEnc->bsReader,
						pTestEnc->encHeaderParam.buf,
						pTestEnc->encHeaderParam.size,
						0,
						pTestEnc->comparatorBitStream);
				}
			#endif
				if (ret == FALSE) {
					CVI_VC_ERR("\n");
					break;
				}
			}
		}

		CVI_VC_TRACE("\n");

	} while (0);

#if CACHE_ENCODE_HEADER
	if (pTestEnc->bEncHeader == 0) {
		pTestEnc->bEncHeader = 1;
	}
#endif
	CVI_VC_TRACE("\n");

	return ret;
}

int cviEncode265Header(stTestEncoder *pTestEnc)
{
	int ret = TRUE;

	do {
		ret = cviEncode265HeaderByType(pTestEnc, CODEOPT_ENC_VPS);
		if (ret == FALSE)
			break;

		ret = cviEncode265HeaderByType(pTestEnc, CODEOPT_ENC_SPS);
		if (ret == FALSE)
			break;

		ret = cviEncode265HeaderByType(pTestEnc, CODEOPT_ENC_PPS);
		if (ret == FALSE)
			break;
	} while (0);

	CVI_VC_TRACE("\n");

	return ret;
}

static int cviInsertOneUserDataSegment(stStreamPack *psp, Uint8 *pUserData,
				       Uint32 userDataLen)
{
	stPack *pPack;
	Uint8 *pBuffer;

	MUTEX_LOCK(&psp->packMutex);
	if (psp->totalPacks >= MAX_NUM_PACKS) {
		CVI_VC_ERR("totalPacks (%d) >= MAX_NUM_PACKS\n",
			   psp->totalPacks);
		MUTEX_UNLOCK(&psp->packMutex);
		return FALSE;
	}
	MUTEX_UNLOCK(&psp->packMutex);

	pBuffer = (Uint8 *)osal_kmalloc(userDataLen);
	if (pBuffer == NULL) {
		CVI_VC_ERR("out of memory\n");
		return FALSE;
	}

	memcpy(pBuffer, pUserData, userDataLen);

	MUTEX_LOCK(&psp->packMutex);
	pPack = &psp->pack[psp->totalPacks++];
	pPack->addr = pBuffer;
	pPack->len = userDataLen;
	pPack->cviNalType = NAL_SEI;
	pPack->need_free = TRUE;
	pPack->u64PhyAddr = virt_to_phys(pBuffer);
	vdi_flush_ion_cache(pPack->u64PhyAddr, pBuffer, userDataLen);
	MUTEX_UNLOCK(&psp->packMutex);

	return TRUE;
}

int cviInsertUserData(stTestEncoder *pTestEnc)
{
	int ret = TRUE;
	TestEncConfig *pEncCfg = &pTestEnc->encConfig;
	UserDataList *userdataNode = NULL;
	UserDataList *n;

	list_for_each_entry_safe(userdataNode, n, &pEncCfg->userdataList, list) {
		if (userdataNode->userDataBuf != NULL && userdataNode->userDataLen != 0) {
			if (!cviInsertOneUserDataSegment(&pTestEnc->streamPack,
							 userdataNode->userDataBuf,
							 userdataNode->userDataLen)) {
				CVI_VC_ERR("failed to insert user data\n");
				ret = FALSE;
			}
			osal_free(userdataNode->userDataBuf);
			list_del(&userdataNode->list);
			osal_free(userdataNode);
			return ret;
		}
	}

	return ret;
}

static int cviConcatePartialEsBuffer(stTestEncoder *pTestEnc, stPack *pPack,
				     Uint8 *esBuf, Int32 esCopiedSize)
{
	Uint8 *pu8ResultBitStream =
		osal_ion_alloc(esCopiedSize + pTestEnc->u32PreStreamLen);

	if (!pu8ResultBitStream) {
		CVI_VC_ERR("can not allocate memory for result bitStream\n");
		osal_free(pTestEnc->pu8PreStream);
		pTestEnc->pu8PreStream = NULL;
		pTestEnc->u32PreStreamLen = 0;
		return FALSE;
	}

	CVI_VC_INFO("u32PreStreamLen 0x%x\n", pTestEnc->u32PreStreamLen);

	memcpy(pu8ResultBitStream, pTestEnc->pu8PreStream,
	       pTestEnc->u32PreStreamLen);
	memcpy(pu8ResultBitStream + pTestEnc->u32PreStreamLen, esBuf,
	       esCopiedSize);

	if (pPack->need_free) {
		if (pPack->cviNalType >= NAL_I && pPack->cviNalType <= NAL_IDR) {
			osal_ion_free(esBuf);
		} else {
			osal_kfree(esBuf);
		}
		esBuf = NULL;
	}

	pPack->addr = pu8ResultBitStream;
	pPack->len = esCopiedSize + pTestEnc->u32PreStreamLen;
	pPack->u64PhyAddr = virt_to_phys(pu8ResultBitStream);
	vdi_flush_ion_cache(pPack->u64PhyAddr, pPack->addr, pPack->len);
	pPack->need_free = TRUE;

	osal_free(pTestEnc->pu8PreStream);
	pTestEnc->pu8PreStream = NULL;
	pTestEnc->u32PreStreamLen = 0;

	return TRUE;
}

int cviPutEsInPack(stTestEncoder *pTestEnc, PhysicalAddress paBsBufStart,
		   PhysicalAddress paBsBufEnd, Uint32 esSize, Int32 cviNalType)
{
	stStreamPack *psp = &pTestEnc->streamPack;
	stPack *pPack;
	Int32 esCopiedSize = 0;
	Uint8 *esBuf = NULL;
	int esBufValidSize, ret;
	BOOL bSkipCopy = FALSE;

#if defined(CVI_H26X_USE_ION_MEM)
	if (!vdi_get_is_single_es_buf(pTestEnc->encConfig.coreIdx)) {
		if (cviNalType >= NAL_I && cviNalType <= NAL_IDR)
			bSkipCopy = TRUE;
	}
#endif

	ret = cviCopyStreamToBuf(pTestEnc->bsReader, &esCopiedSize, &esBuf,
				 esSize, &esBufValidSize, paBsBufStart,
				 paBsBufEnd, &bSkipCopy, cviNalType);

	if (ret < 0) {
		CVI_VC_ERR("cviCopyStreamToBuf, ret = %d\n", ret);
		return FALSE;
	}

	MUTEX_LOCK(&psp->packMutex);
	if (psp->totalPacks >= MAX_NUM_PACKS) {
		CVI_VENC_DEBUG("[WARN]totalPacks (%d) >= MAX_NUM_PACKS,drop this packet\n",
			   psp->totalPacks);
		if (!bSkipCopy) {
			if (esBuf && !bSkipCopy) {
				if (cviNalType >= NAL_I && cviNalType <= NAL_IDR) {
					osal_ion_free(esBuf);
				} else {
					osal_kfree(esBuf);
				}
			}
		}

		if (esBufValidSize > 0) {
			ret = VPU_EncUpdateBitstreamBuffer(pTestEnc->handle,
							   esCopiedSize);
			if (ret != RETCODE_SUCCESS) {
				CVI_VC_ERR("VPU_EncUpdateBitstreamBuffer, 0x%x\n", ret);
				MUTEX_UNLOCK(&psp->packMutex);
				return FALSE;
			}
		}
		psp->dropCnt++;
		psp->seq++;
		MUTEX_UNLOCK(&psp->packMutex);
		return TRUE;
	}

	pPack = &psp->pack[psp->totalPacks];
	pPack->addr = esBuf;
	pPack->len = esCopiedSize;
	pPack->cviNalType = cviNalType;
	pPack->need_free = !bSkipCopy;
	pPack->u64PTS = pTestEnc->u64Pts;
	if (pPack->need_free) {
		pPack->u64PhyAddr = virt_to_phys(esBuf);
		vdi_flush_ion_cache(pPack->u64PhyAddr, pPack->addr, pPack->len);
	} else {
		CodecInst *pCodecInst = (CodecInst *)pTestEnc->handle;
		EncInfo *pEncInfo = &pCodecInst->CodecInfo->encInfo;

		pPack->u64PhyAddr = vdi_remap_memory_address(
			pTestEnc->coreIdx, pEncInfo->streamRdPtr);
	}
	if (pPack->cviNalType <= NAL_NONE || pPack->cviNalType >= NAL_MAX) {
		CVI_VC_ERR("cviNalType = %d\n", cviNalType);
		MUTEX_UNLOCK(&psp->packMutex);
		return FALSE;
	}

	psp->totalPacks++;
	if (pTestEnc->pu8PreStream) {
		if (cviConcatePartialEsBuffer(pTestEnc, pPack, esBuf,
					      esCopiedSize) != TRUE) {
			CVI_VC_ERR("cviConcatePartialEsBuffer failed %d,drop this packet\n", ret);
		}
	}
	MUTEX_UNLOCK(&psp->packMutex);

	CVI_VC_BS("pack[%d] addr = %p, len = 0x%x, cviNalType = %d\n",
		  psp->totalPacks, pPack->addr, pPack->len, pPack->cviNalType);



	if (esBufValidSize > 0) {
		ret = VPU_EncUpdateBitstreamBuffer(pTestEnc->handle,
						   esCopiedSize);
		if (ret != RETCODE_SUCCESS) {
			CVI_VC_ERR("VPU_EncUpdateBitstreamBuffer, 0x%x\n", ret);
			return FALSE;
		}
	}

	return TRUE;
}

void cviPrintRc(stTestEncoder *pTestEnc)
{
	EncOpenParam *pEncOP = &pTestEnc->encOP;

	UNUSED(pEncOP);
	CVI_VC_RC("-------------------\n");
	CVI_VC_RC("gopPreset = %d, LongTermPeriod = %d, LongTermDeltaQp = %d\n",
		  pEncOP->gopPreset, pEncOP->LongTermPeriod,
		  pEncOP->LongTermDeltaQp);
	CVI_VC_RC("HvsQpScaleDiv2 = %d, EnHvsQp = %d, EnRowLevelRc = %d\n",
		  pEncOP->HvsQpScaleDiv2, pEncOP->EnHvsQp,
		  pEncOP->EnRowLevelRc);
	CVI_VC_RC("RcInitialQp = %d, enAutoFrmSkip = %d, vbvThreshold = %d\n",
		  pEncOP->RcInitialQp, pEncOP->enAutoFrmSkip,
		  pEncOP->vbvThreshold);
	CVI_VC_RC(
		"rcWeightFactor = %d, coda9RoiEnable = %d, RoiPicAvgQp = %d\n",
		pEncOP->rcWeightFactor, pEncOP->coda9RoiEnable,
		pEncOP->RoiPicAvgQp);
}

int cviInitAddrRemap(stTestEncoder *pTestEnc)
{
	TestEncConfig *pEncCfg = &pTestEnc->encConfig;
	EncOpenParam *pEncOP = &pTestEnc->encOP;
	CodecInst *pCodecInst = (CodecInst *) pTestEnc->handle;
	EncInfo *pEncInfo = &pCodecInst->CodecInfo->encInfo;
	AddrRemap *par = &pEncInfo->addrRemap;
	int coreIdx = pEncCfg->coreIdx;
	char ionName[MAX_VPU_ION_BUFFER_NAME];
	int extraSizeInPage[2], compSize[2], compSizeInPage[2];
	int framebufStride = 0, ret = 0;
	int comp, productId;

	par->mode = cviVcodecGetEnv("ARMode");

	par->numExtraLine = cviVcodecGetEnv("ARExtraLine");

	par->pageSizeSel = AR_PAGE_256KB;
	par->mmuEnable = AR_MMU_BIT30;
	par->pageSizeInBit = par->pageSizeSel + AR_PAGE_SIZE_OFFSET;
	par->pageSize = 1 << par->pageSizeInBit;
	par->lastWriteFb = 1;
	memset(par->u32ReadPhyAddr, 0, sizeof(par->u32ReadPhyAddr));

	CVI_VC_CFG("mode = %d, pageSize = 0x%X, numExtraLine = %d\n",
			par->mode, par->pageSize, par->numExtraLine);

	productId = ProductVpuGetId(coreIdx);

	framebufStride =
		CalcStride(pTestEnc->framebufWidth,
				pTestEnc->framebufHeight,
				pEncOP->srcFormat,
				pEncOP->cbcrInterleave,
				(TiledMapType)(pEncCfg->mapType & 0x0f),
				FALSE, FALSE);

	compSize[AR_COMP_LUMA] =
		CalcLumaSize(productId,
				framebufStride, pTestEnc->framebufHeight,
				pEncOP->cbcrInterleave, (TiledMapType)(pEncCfg->mapType & 0x0f), NULL);

	compSize[AR_COMP_CHROMA] =
		CalcChromaSize(productId,
				framebufStride, pTestEnc->framebufHeight, pEncOP->srcFormat,
				pEncOP->cbcrInterleave, (TiledMapType)(pEncCfg->mapType & 0x0f), NULL);
	compSize[AR_COMP_CHROMA] = compSize[AR_COMP_CHROMA] << 1;

	extraSizeInPage[AR_COMP_LUMA] =
		ROUND_UP_N_BIT(framebufStride * par->numExtraLine, par->pageSizeInBit);
	extraSizeInPage[AR_COMP_CHROMA] =
		ROUND_UP_N_BIT(extraSizeInPage[AR_COMP_LUMA] >> 1, par->pageSizeInBit);

	/* STD_AVC Luma use one frame size*/
	if (pEncOP->bitstreamFormat == STD_AVC)
		extraSizeInPage[AR_COMP_LUMA] = 0;

	for (comp = AR_COMP_LUMA; comp < AR_COMP_MAX; comp++) {
		vpu_buffer_t *pvbRecFb = &pTestEnc->vbReconFrameBuf[comp];

		CVI_VC_AR("comp = %d\n", comp);

		compSizeInPage[comp] = ROUND_UP_N_BIT(compSize[comp], par->pageSizeInBit);
		par->numFramePage[comp] = compSizeInPage[comp] >> par->pageSizeInBit;

		compSizeInPage[comp] += extraSizeInPage[comp];

		par->numVirPageInFbs[comp] = compSizeInPage[comp] >> par->pageSizeInBit;

		if (par->mode == AR_MODE_ORIG)
			compSizeInPage[comp] += par->pageSize;

		pvbRecFb->size = compSizeInPage[comp];

		par->numPhyPageInFbs[comp] = compSizeInPage[comp] >> par->pageSizeInBit;

		CVI_VC_AR("compSize = 0x%X, compSizeInPage = 0x%X, extraSizeInPage = 0x%X\n",
				compSize[comp], compSizeInPage[comp], extraSizeInPage[comp]);
		CVI_VC_AR("numPhyPageInFbs = 0x%X, numVirPageInFbs = 0x%X\n",
				par->numPhyPageInFbs[comp], par->numVirPageInFbs[comp]);

		memset(ionName, 0, MAX_VPU_ION_BUFFER_NAME);
		sprintf(ionName, "VENC_%d_ReconFb_%d", pCodecInst->s32ChnNum, comp);

		if (VDI_ALLOCATE_MEMORY(coreIdx, pvbRecFb, 0, ionName) < 0) {
			CVI_VC_ERR("fail to allocate recon buffer, %d\n", comp);
			return TE_ERR_ENC_OPEN;
		}
	}


	for (comp = AR_COMP_LUMA; comp < AR_COMP_MAX; comp++) {
		vpu_buffer_t *pvbRecFb = &pTestEnc->vbReconFrameBuf[comp];
		FrameBuffer *pfb = &pTestEnc->fbRecon[comp];

		pfb->size = pvbRecFb->size;
		pfb->bufCb = (PhysicalAddress) - 1;
		pfb->bufCr = (PhysicalAddress) - 1;
		pfb->updateFbInfo = TRUE;

		if (par->mode == AR_MODE_ORIG) {
			pfb->bufY = ROUND_UP_N_BIT(pvbRecFb->phys_addr, par->pageSizeInBit);
		} else {
			pfb->bufY = pvbRecFb->phys_addr;
		}

		CVI_VC_AR("fbRecon[%d], bufY = 0x%llX, bufCb = 0x%llX, bufCr = 0x%llX\n",
				comp, pfb->bufY, pfb->bufCb, pfb->bufCr);
		CVI_VC_AR("vbReconFrameBuf[%d], phys_addr = 0x%llX\n",
				comp, pvbRecFb->phys_addr);

		ret = cviInitPageTable(pTestEnc, comp, pfb->bufY);
		if (ret != 0) {
			CVI_VC_ERR("cviInitPageTable\n");
			return ret;
		}
	}

	return ret;
}

static int cviInitPageTable(stTestEncoder *pTestEnc, int comp, Uint32 startAddr)
{
	CodecInst *pCodecInst = (CodecInst *) pTestEnc->handle;
	EncInfo *pEncInfo = &pCodecInst->CodecInfo->encInfo;
	AddrRemap *par = &pEncInfo->addrRemap;
	Uint32 currAddr = startAddr;
	int ret = 0;
	Int32 i, j;

	CVI_VC_AR("PageTable[%d]\n", comp);
	CVI_VC_AR("numPhyPageInFbs = %d\n", par->numPhyPageInFbs[comp]);

	for (i = 0; i < par->numVirPageInFbs[comp]; i++) {
		PageEntry *pEntry = &par->pageTable[comp][i];

		pEntry->phyAddr = currAddr;
		pEntry->flag = pTestEnc->regFrameBufCount;

		CVI_VC_AR("[%d] phyAddr = 0x%X, flag = 0x%X\n", i, pEntry->phyAddr, pEntry->flag);

		currAddr += par->pageSize;
	}

	if (pTestEnc->regFrameBufCount > AR_MAX_FB_NUM) {
		CVI_VC_ERR("PageTable[%d]\n", comp);
		return -1;
	}

	for (i = 0; i < pTestEnc->regFrameBufCount; i++) {
		for (comp = AR_COMP_LUMA; comp < AR_COMP_MAX; comp++) {
			for (j = 0; j < par->numFramePage[comp]; j++) {
				par->fbPageIndex[i][comp][j] = j;
			}
		}
	}

	return ret;
}

int cviInitAddrRemapFb(CodecInst *pCodecInst)
{
	EncInfo *pEncInfo = &pCodecInst->CodecInfo->encInfo;
	AddrRemap *par = &pEncInfo->addrRemap;
	Uint32 val, reg;
	int i;

	// Enable vc addr remap, reg VC_FAB_MMU_CTRL
	val = 0;
	val |= (par->mmuEnable & 0x3);
	val |= ((par->mode & 0x1) << 3);
	val |= ((par->pageSizeSel & 0x3) << 4);

	reg = CtrlReadReg(coreIdx, VC_FAB_MMU_CTRL);

	if (pCodecInst->productId == PRODUCT_ID_420L) {
		reg &= 0x0000FFFF;
		reg |= (val << 16);
	} else {
		reg &= 0xFFFF0000;
		reg |= val;
	}
	CtrlWriteReg(coreIdx, VC_FAB_MMU_CTRL, reg);

	val = CtrlReadReg(coreIdx, VC_FAB_MMU_CTRL);
	CVI_VC_AR("VC CTRL Reg 0x%x = 0x%x\n", VC_FAB_MMU_CTRL, val);

	// Setup recon frame YUV virtual address
	val = 1 << (AR_MMU_ENABLE_OFFSET - par->mmuEnable);

	for (i = 0; i < pEncInfo->numFrameBuffers; i++) {
		FrameBuffer *pFb = &pEncInfo->frameBufPool[i];

		pFb->bufY = val;
		val += (par->numFramePage[AR_COMP_LUMA] << par->pageSizeInBit);

		pFb->bufCb = val;
		val += (par->numFramePage[AR_COMP_CHROMA] << par->pageSizeInBit);

		CVI_VC_AR("frameBufPool[%d]  bufY = 0x%llX, bufCb = 0x%llX\n",
			i, pFb->bufY, pFb->bufCb);
	}

	return RETCODE_SUCCESS;
}

int cviSetupPicConfig(stTestEncoder *pTestEnc)
{
	EncOpenParam *pEncOP = &pTestEnc->encOP;
	EncParam *pEncParam = &pTestEnc->encParam;
	stRcInfo *pRcInfo;
	CodecInst *pCodecInst;
	EncInfo *pEncInfo;
	AddrRemap *par;
	int ret = 0;

	pCodecInst = pTestEnc->handle;
	pEncInfo = &pCodecInst->CodecInfo->encInfo;
	par = &pEncInfo->addrRemap;

	pRcInfo = &pCodecInst->rcInfo;

	pRcInfo->frameIdx = pTestEnc->frameIdx;

	if (pEncInfo->cviRcEn)
		cviEncRc_RcKernelEstimatePic(pRcInfo, pEncParam, pEncInfo->frameIdx);

	if (pEncInfo->addrRemapEn) {
		if (pEncOP->bitstreamFormat == STD_HEVC) {
			if (pEncParam->is_idr_frame)
				par->lastWriteFb = 1;
		}

		ret = cviConfigPageTable(pCodecInst);
		if (ret < 0) {
			CVI_VC_ERR("cviConfigPageTable, %d\n", ret);
			return ret;
		}

		cviPrintPageTable(pCodecInst);
	}

	return ret;
}

static int cviConfigPageTable(CodecInst *pCodecInst)
{
	EncInfo *pEncInfo = &pCodecInst->CodecInfo->encInfo;
	AddrRemap *par = &pEncInfo->addrRemap;
	int comp;
	int ret = 0;

	CVI_VC_AR("frameIdx = %d, lastWriteFb = %d\n", pEncInfo->frameIdx, par->lastWriteFb);

	for (comp = AR_COMP_LUMA; comp < AR_COMP_MAX; comp++) {
		ret = cviFindWriteEntry(pCodecInst, comp);
		if (ret < 0) {
			CVI_VC_ERR("cviFindWriteEntry, comp = %d\n", comp);
			return -1;
		}

		ret = cviConfigPageTableEntry(pCodecInst, comp);
	}

	par->lastWriteFb = (par->lastWriteFb + 1) & 0x1;

	return ret;
}

static int cviFindWriteEntry(CodecInst *pCodecInst, int comp)
{
	EncInfo *pEncInfo = &pCodecInst->CodecInfo->encInfo;
	AddrRemap *par = &pEncInfo->addrRemap;
	int currWriteFb = (par->lastWriteFb + 1) & 0x1;
	int foundPage, currPage;
	int j;

	// find from unused pages
	currPage = 0;
	for (j = 0; j < par->numVirPageInFbs[comp]; j++) {
		if (par->pageTable[comp][j].flag != par->lastWriteFb) {
			foundPage = j;

			par->fbPageIndex[currWriteFb][comp][currPage] = foundPage;
			par->pageTable[comp][foundPage].flag = currWriteFb;

			currPage++;

			if (currPage >= par->numFramePage[comp]) {
				CVI_VC_AR("find finished, currPage = %d\n", currPage);
				return 0;
			}
		}
	}

	// find from the last written Frame Buffer
	for (j = 0; j < par->numVirPageInFbs[comp]; j++) {
		foundPage = par->fbPageIndex[par->lastWriteFb][comp][j];

		par->fbPageIndex[currWriteFb][comp][currPage] = foundPage;
		par->pageTable[comp][foundPage].flag = currWriteFb;

		currPage++;

		if (currPage >= par->numFramePage[comp]) {
			CVI_VC_AR("top of lastWriteFb, currPage = %d\n", currPage);
			return 0;
		}
	}

	CVI_VC_ERR("currPage = %d, not enough free page\n", currPage);

	return -1;
}

static int cviConfigPageTableEntry(CodecInst *pCodecInst, int comp)
{
	EncInfo *pEncInfo = &pCodecInst->CodecInfo->encInfo;
	EncOpenParam *pOpenParam = &pEncInfo->openParam;
	AddrRemap *par = &pEncInfo->addrRemap;
	int regStartAddr;
	int currWriteFb = (par->lastWriteFb + 1) & 0x1;
	int i;

	CVI_VC_AR("chn:%d PageTable[%d]\n", pCodecInst->s32ChnNum, comp);

	if (par->u32ReadPhyAddr[comp][0]) {
		regStartAddr = (par->lastWriteFb & 0x1) *
			(par->numFramePage[AR_COMP_LUMA] + par->numFramePage[AR_COMP_CHROMA]);
		regStartAddr += comp * par->numFramePage[AR_COMP_LUMA];
		regStartAddr <<= 2;
		if (pOpenParam->bitstreamFormat == STD_HEVC)
			regStartAddr += H265_MAP_REG_OFFSET;
		for (i = 0; i < par->numFramePage[comp]; i++) {
			RemapWriteReg(pCodecInst->coreIdx, regStartAddr, par->u32ReadPhyAddr[comp][i]);
			CVI_VC_AR("[%d] last write, regStartAddr = 0x%X, phyAddr = 0x%X\n",
				i, regStartAddr, par->u32ReadPhyAddr[comp][i]);
			regStartAddr += 4;
		}
	}

	regStartAddr = currWriteFb *
		(par->numFramePage[AR_COMP_LUMA] + par->numFramePage[AR_COMP_CHROMA]);
	regStartAddr += comp * par->numFramePage[AR_COMP_LUMA];
	regStartAddr <<= 2;

	if (pOpenParam->bitstreamFormat == STD_HEVC)
		regStartAddr += H265_MAP_REG_OFFSET;

	for (i = 0; i < par->numFramePage[comp]; i++) {
		int newEntry = par->fbPageIndex[currWriteFb][comp][i];
		int phyAddr = par->pageTable[comp][newEntry].phyAddr;

		if (newEntry >= AR_MAX_NUM_PAGE_TABLE_ENTRY) {
			CVI_VC_ERR("comp = %d, i = %d\n", comp, i);
			return -1;
		}

		CVI_VC_AR("[%d] page idx = %d, regStartAddr = 0x%X, phyAddr = 0x%X\n",
				i, newEntry, regStartAddr, phyAddr);

		RemapWriteReg(pCodecInst->coreIdx, regStartAddr, phyAddr);
		par->u32ReadPhyAddr[comp][i] = phyAddr;

		regStartAddr += 4;
	}

	return 0;
}

static void cviPrintPageTable(CodecInst *pCodecInst)
{
	EncInfo *pEncInfo = &pCodecInst->CodecInfo->encInfo;
	EncOpenParam *pOpenParam = &pEncInfo->openParam;
	AddrRemap *par = &pEncInfo->addrRemap;
	PageEntry *pEntry;

	int comp, i, val, addr;
	int fb;

	if ((vcodec_mask & CVI_MASK_AR) == 0)
		return;

	UNUSED(pEntry);
	CVI_VC_AR("PageTable -------------------------------------\n");

	for (comp = AR_COMP_LUMA; comp < AR_COMP_MAX; comp++) {
		CVI_VC_AR("PageTable[%d]\n", comp);
		CVI_VC_AR("numPhyPageInFbs = %d\n", par->numPhyPageInFbs[comp]);

		for (i = 0; i < par->numVirPageInFbs[comp]; i++) {
			pEntry = &par->pageTable[comp][i];
			CVI_VC_AR("[%d] phyAddr = 0x%X, flag = 0x%X\n", i, pEntry->phyAddr, pEntry->flag);
		}
	}

	CVI_VC_AR("Addr Remap registers --------------------------\n");

	addr = (pOpenParam->bitstreamFormat == STD_HEVC) ? H265_MAP_REG_OFFSET : 0;

	for (fb = 0; fb < 2; fb++) {
		CVI_VC_AR("fb = %d\n", fb);

		for (comp = AR_COMP_LUMA; comp < AR_COMP_MAX; comp++) {
			CVI_VC_AR("comp = %d\n", comp);

			for (i = 0; i < par->numFramePage[comp]; i++) {

				val = RemapReadReg(pCodecInst->coreIdx, addr);
				CVI_VC_AR("addr = 0x%X, val = 0x%X\n", addr, val);
				addr += 4;
			}
		}
	}
}

void cviSetApiMode(EncHandle pHandle, int cviApiMode)
{
	((CodecInst *)pHandle)->cviApiMode = cviApiMode;
}
