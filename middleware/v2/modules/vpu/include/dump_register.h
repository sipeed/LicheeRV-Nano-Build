/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: module/vpu/include/dump_register.h
 * Description:
 *   dump hw register and lut for ISP module.
 */

#ifndef __DUMP_REGISTER_H__
#define __DUMP_REGISTER_H__

#include <linux/cvi_comm_vi.h>

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

CVI_S32 dump_register_182x(VI_PIPE ViPipe, FILE *fp, VI_DUMP_REGISTER_TABLE_S *pstRegTbl);
CVI_S32 dump_register_183x(VI_PIPE ViPipe, FILE *fp, VI_DUMP_REGISTER_TABLE_S *pstRegTbl);
CVI_S32 dump_hw_register(VI_PIPE ViPipe, FILE *fp, VI_DUMP_REGISTER_TABLE_S *pstRegTbl);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /*__DUMP_REGISTER_H__ */
