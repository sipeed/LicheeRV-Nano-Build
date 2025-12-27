/*
 * Copyright Cvitek Technologies Inc.
 *
 * Created Time: May, 2020
 */

#ifndef __CVI_VCODEC_LIB_H__
#define __CVI_VCODEC_LIB_H__

#include "vpuapi.h"
#include "product.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define AR_DEFAULT_EXTRA_LINE	512

int cviVcodecInit(void);
void cviVcodecMask(void);
int cviVcodecGetEnv(char *envVar);
int cviSetCoreIdx(int *pCoreIdx, CodStd stdMode);
Uint64 cviGetCurrentTime(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
