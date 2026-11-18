//--=========================================================================--
//  This file is a part of VPU Reference API project
//-----------------------------------------------------------------------------
//       This confidential and proprietary software may be used only
//     as authorized by a licensing agreement from Chips&Media Inc.
//     In the event of publication, the following notice is applicable:
//
//            (C) COPYRIGHT 2006 - 2013  CHIPS&MEDIA INC.
//                      ALL RIGHTS RESERVED
//
//       The entire notice above must be reproduced on all authorized
//       copies.
//
//--=========================================================================--

#include "main_helper.h"
#include "vdi.h"
typedef struct {
	Uint32 type;
	EndianMode endian;
	BitstreamReaderImpl *impl;
	osal_file_t *fp;
	EncHandle *handle;
	vpu_buffer_t *streamVb;
} AbstractBitstreamReader;

BitstreamReader BitstreamReader_Create(Uint32 type, char *path,
				       EndianMode endian, EncHandle *handle)
{
	AbstractBitstreamReader *reader;
	osal_file_t *fp;

	CVI_VC_TRACE("\n");

	if (path == NULL) {
		CVI_VC_ERR("path is NULL\n");
		return NULL;
	}

	if (strcmp(path, "CVISDK")) {
		fp = osal_fopen(path, "wb");
		if (fp == NULL) {
			VLOG(ERR, "%s:%d failed to open bin file: %s\n",
			     __func__, __LINE__, path);
			return FALSE;
		}
	} else {
		fp = NULL;
	}

	CVI_VC_TRACE("output bin file: %s\n", path);

	reader = (AbstractBitstreamReader *)osal_malloc(
		sizeof(AbstractBitstreamReader));

	reader->fp = fp;
	reader->handle = handle;
	reader->type = type;
	reader->endian = endian;

	return reader;
}

BOOL BitstreamReader_SetVbStream(BitstreamReader reader, vpu_buffer_t *vb)
{
	AbstractBitstreamReader *absReader = (AbstractBitstreamReader *)reader;
	BOOL success = TRUE;

	absReader->streamVb = vb;
	CVI_VC_TRACE("phys_addr = 0x%llX, virt_addr = 0x%p\n",
		     absReader->streamVb->phys_addr,
		     (void *)absReader->streamVb->virt_addr);

	return success;
}

BOOL BitstreamReader_Act(BitstreamReader reader,
			 PhysicalAddress bitstreamBuffer,
			 Uint32 bitstreamBufferSize, Uint32 streamReadSize,
			 Comparator comparator)
{
	AbstractBitstreamReader *absReader = (AbstractBitstreamReader *)reader;
	osal_file_t *fp;
	EncHandle *handle;
	RetCode ret = RETCODE_SUCCESS;
	int size = 0;
	Int32 loadSize = 0;
	PhysicalAddress paBsBufEnd = bitstreamBuffer + bitstreamBufferSize;
	PhysicalAddress paBsBufStart = bitstreamBuffer;
	Uint8 *buf = NULL;
	BOOL success = TRUE;
	BOOL bSkipCopy = FALSE;

	CVI_VC_TRACE("\n");

	if (reader == NULL) {
#ifdef SUPPORT_DONT_READ_STREAM
		return TRUE;
#else
		CVI_VC_ERR("Invalid handle\n");
		return FALSE;
#endif
	}
	fp = absReader->fp;
	handle = absReader->handle;

	CVI_VC_TRACE("\n");

	ret = cviCopyStreamToBuf(absReader, &loadSize, &buf, streamReadSize,
				 &size, paBsBufStart, paBsBufEnd, &bSkipCopy, NAL_NONE);
	if (ret < 0) {
		CVI_VC_ERR("cviCopyStreamToBuf, ret = %d\n", ret);
		return FALSE;
	}

	if (size > 0) {
		CVI_VC_TRACE("loadSize = 0x%X\n", loadSize);

		if (fp != NULL) {
			osal_fwrite((void *)buf, sizeof(Uint8), loadSize, fp);
		}

		CVI_VC_TRACE("comparator = 0x%p\n", comparator);
#ifdef REDUNDENT_CODE
		if (comparator != NULL) {
			if (Comparator_Act(comparator, buf, loadSize) ==
			    FALSE) {
				success = FALSE;
			}
		}
#endif
		osal_free(buf);
	}

	if (size > 0) {
		CVI_VC_TRACE("\n");

		ret = VPU_EncUpdateBitstreamBuffer(*handle, loadSize);
		if (ret != RETCODE_SUCCESS) {
			VLOG(ERR,
			     "VPU_EncUpdateBitstreamBuffer failed Error code is 0x%x\n",
			     ret);
			success = FALSE;
		}
	}
	CVI_VC_TRACE("\n");

	return success;
}

int cviCopyStreamToBuf(BitstreamReader reader, Int32 *pEsCopiedSize,
		       Uint8 **ppOutputEsBuf, Uint32 userEsReadSize,
		       int *pEsBufValidSize, PhysicalAddress paBsBufStart,
		       PhysicalAddress paBsBufEnd, BOOL *bSkipCopy, Int32 cviNalType)
{
	int ret = 0;
	AbstractBitstreamReader *absReader = (AbstractBitstreamReader *)reader;
	EncHandle *handle = absReader->handle;
	Int32 esCopiedSize = 0;
	Uint8 *pOutputEsBuf = 0;
	Int32 esBufValidSize = 0;
	Uint32 coreIdx = VPU_HANDLE_CORE_INDEX(*handle);
	PhysicalAddress paRdPtr;
	PhysicalAddress paWrPtr;

	ret = VPU_EncGetBitstreamBuffer(*handle, &paRdPtr, &paWrPtr,
					&esBufValidSize);

	CVI_VC_BS(
		"esBufValidSize = 0x%X, userEsReadSize = 0x%X, bSkipCopy %d\n",
		esBufValidSize, userEsReadSize, *bSkipCopy);
	if (esBufValidSize > 0) {
		if (userEsReadSize > 0) {
			if ((Uint32)esBufValidSize < userEsReadSize) {
				CVI_VC_WARN(
					"esBufValidSize (0x%X) < esSize(0x%X)\n",
					esBufValidSize, userEsReadSize);
				esCopiedSize = esBufValidSize;
			} else {
				esCopiedSize = userEsReadSize;
			}
		} else {
			esCopiedSize = esBufValidSize;
		}

#if defined(CVI_H26X_USE_ION_MEM)
		if (*bSkipCopy &&
		    absReader->type != BUFFER_MODE_TYPE_RINGBUFFER) {
			*bSkipCopy = TRUE;
			pOutputEsBuf = vdi_get_vir_addr(
				coreIdx,
				vdi_remap_memory_address(coreIdx, paRdPtr));
#if defined(BITSTREAM_ION_CACHED_MEM)
			vdi_invalidate_ion_cache(
				vdi_remap_memory_address(coreIdx, paRdPtr),
				(void *)pOutputEsBuf, esCopiedSize);
#endif
		} else
#endif
		{
			*bSkipCopy = FALSE;
			if (cviNalType >= NAL_I && cviNalType <= NAL_IDR) {
				pOutputEsBuf = (Uint8 *)osal_ion_alloc(esCopiedSize);
			} else if (cviNalType == NAL_NONE) {
				pOutputEsBuf = (Uint8 *)osal_malloc(esCopiedSize);
			}  else {
				pOutputEsBuf = (Uint8 *)osal_kmalloc(esCopiedSize);
			}
			if (pOutputEsBuf == NULL) {
				CVI_VC_ERR("malloc size %d, cviNalType:%d fail\n", esCopiedSize, cviNalType);
				return -1;
			}

			CVI_VC_TRACE("pOutputEsBuf = %p, type = %d, paRdPtr = 0x%llX\n",
				pOutputEsBuf, absReader->type, paRdPtr);
			CVI_VC_TRACE("streamVb phy addr = 0x%llX, virt addr = 0x%p, esCopiedSize = 0x%X\n",
				absReader->streamVb->phys_addr,
				(void *)absReader->streamVb->virt_addr,
				esCopiedSize);

			if (absReader->type == BUFFER_MODE_TYPE_RINGBUFFER) {
				if ((paRdPtr + esCopiedSize) > paBsBufEnd) {
					Uint32 room;
					room = paBsBufEnd - paRdPtr;
#if (defined CVI_H26X_USE_ION_MEM && defined BITSTREAM_ION_CACHED_MEM)
					vdi_invalidate_ion_cache(
						paRdPtr, (void *)pOutputEsBuf,
						room);
#endif
					vdi_read_memory(coreIdx, paRdPtr,
							pOutputEsBuf, room,
							absReader->endian);
#if (defined CVI_H26X_USE_ION_MEM && defined BITSTREAM_ION_CACHED_MEM)
					vdi_invalidate_ion_cache(
						paBsBufStart,
						(void *)(pOutputEsBuf + room),
						(esCopiedSize - room));
#endif
					vdi_read_memory(coreIdx, paBsBufStart,
							pOutputEsBuf + room,
							(esCopiedSize - room),
							absReader->endian);
				} else {
#if (defined CVI_H26X_USE_ION_MEM && defined BITSTREAM_ION_CACHED_MEM)
					vdi_invalidate_ion_cache(
						paRdPtr, (void *)pOutputEsBuf,
						esCopiedSize);
#endif
					vdi_read_memory(coreIdx, paRdPtr,
							pOutputEsBuf,
							esCopiedSize,
							absReader->endian);
				}
			} else {
/* Linebuffer */
#if (defined CVI_H26X_USE_ION_MEM && defined BITSTREAM_ION_CACHED_MEM)
				vdi_invalidate_ion_cache(
					vdi_remap_memory_address(coreIdx,
								 paRdPtr),
					(void *)pOutputEsBuf, esCopiedSize);
#endif
				vdi_read_memory(coreIdx,
						vdi_remap_memory_address(
							coreIdx, paRdPtr),
						pOutputEsBuf, esCopiedSize,
						absReader->endian);
			}
		}
	}

	*pEsCopiedSize = esCopiedSize;
	*ppOutputEsBuf = pOutputEsBuf;
	*pEsBufValidSize = esBufValidSize;

	return ret;
}

BOOL BitstreamReader_Destroy(BitstreamReader reader)
{
	AbstractBitstreamReader *absReader = (AbstractBitstreamReader *)reader;

	if (reader == NULL) {
		CVI_VC_ERR("Invalid handle\n");
		return FALSE;
	}

	if (absReader->fp)
		osal_fclose(absReader->fp);
	osal_free(absReader);

	return TRUE;
}
