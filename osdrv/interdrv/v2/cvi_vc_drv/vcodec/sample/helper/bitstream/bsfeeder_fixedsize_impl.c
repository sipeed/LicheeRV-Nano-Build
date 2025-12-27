//--=========================================================================--
//  This file is a part of VPU Reference API project
//-----------------------------------------------------------------------------
//
//       This confidential and proprietary software may be used only
//     as authorized by a licensing agreement from Chips&Media Inc.
//     In the event of publication, the following notice is applicable:
//
//            (C) COPYRIGHT CHIPS&MEDIA INC.
//                      ALL RIGHTS RESERVED
//
//       The entire notice above must be reproduced on all authorized
//       copies.
//
//--=========================================================================--
#ifdef VC_DRIVER_TEST

#include "vpuapifunc.h"
#include "main_helper.h"

#define MAX_FEEDING_SIZE 0x400000 /* 4MBytes */

typedef struct struct_ReaderConext {
	osal_file_t *fp;
	Uint32 feedingSize;
	BOOL eos;
} ReaderContext;

void *BSFeederFixedSize_Create(const char *path, Uint32 feedingSize)
{
	osal_file_t *fp = NULL;
	ReaderContext *context = NULL;

	fp = osal_fopen(path, "rb");
	if (fp == NULL) {
		CVI_VC_ERR("failed to open %s\n", path);
		return NULL;
	}

	context = (ReaderContext *)osal_malloc(sizeof(ReaderContext));
	if (context == NULL) {
		CVI_VC_ERR("failed to allocate memory\n");
		osal_fclose(fp);
		return NULL;
	}

	context->fp = fp;
	context->feedingSize = feedingSize;
	context->eos = FALSE;

	return (void *)context;
}

BOOL BSFeederFixedSize_Destroy(void *feeder)
{
	ReaderContext *context = (ReaderContext *)feeder;

	if (context == NULL) {
		CVI_VC_ERR("Null handle\n");
		return FALSE;
	}

	if (context->fp)
		osal_fclose(context->fp);

	osal_free(context);

	return TRUE;
}

Int32 BSFeederFixedSize_Act(void *feeder, BSChunk *chunk)
{
	ReaderContext *context = (ReaderContext *)feeder;
	size_t nRead;
	Uint32 size;
	Uint32 feedingSize;

	if (context == NULL) {
		CVI_VC_ERR("Null handle\n");
		return 0;
	}

	if (context->eos == TRUE) {
		chunk->eos = TRUE;
		return 0;
	}

	feedingSize = context->feedingSize;
	if (feedingSize == 0) {
		Uint32 KB = 1024;
		BOOL probability10;

		feedingSize = osal_rand() % MAX_FEEDING_SIZE;
		probability10 = (BOOL)((feedingSize % 100) < 10);
		if (feedingSize < KB) {
			if (probability10 == FALSE)
				feedingSize *= 100;
		}
	}

	size = (chunk->size < feedingSize) ? chunk->size : feedingSize;

	do {
		nRead = osal_fread(chunk->data, 1, size, context->fp);
		if (nRead > size) {

			return 0;
		} else if (nRead < size) {
			context->eos = TRUE;
			chunk->eos = TRUE;
		}
	} while (FALSE);

	return nRead;
}

BOOL BSFeederFixedSize_Rewind(void *feeder)
{
	ReaderContext *context = (ReaderContext *)feeder;
	Int32 ret;

	ret = osal_fseek(context->fp, 0, SEEK_SET);
	if (ret != 0) {
		CVI_VC_ERR("Failed to fseek(ret: %d)\n", ret);
		return FALSE;
	}
	context->eos = FALSE;

	return TRUE;
}

Int32 BSFeederFixedSize_GetFeedingSize(void *feeder)
{
	ReaderContext *context = (ReaderContext *)feeder;

	return context->feedingSize;
}
#endif
