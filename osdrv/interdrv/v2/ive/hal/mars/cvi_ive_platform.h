/*
 * Copyright (C) Cvitek Co., Ltd. 2021-2022. All rights reserved.
 *
 * File Name: cvi_ive_platform.h
 * Description: cvitek ive driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef _CVI_IVE_PLATFORM_H_
#define _CVI_IVE_PLATFORM_H_

#ifdef __cplusplus
extern "C" {
#endif /* End of #ifdef __cplusplus */
#include "linux/cvi_comm_ive.h"
#include "cvi_ive_interface.h"
#include "linux/cvi_ive_ioctl.h"
#include "cvi_ive_reg.h"
#include "cvi_reg.h"

enum filterop_mod {
	MOD_BYP = 0,
	MOD_FILTER3CH = 1,
	MOD_DILA = 2,
	MOD_ERO = 3,
	MOD_CANNY = 4,
	MOD_STBOX = 5,
	MOD_GRADFG = 6,
	MOD_MAG = 7,
	MOD_NORMG = 8,
	MOD_SOBEL = 9,
	MOD_STCANDI = 10,
	MOD_MAP = 11,
	MOD_GMM = 12, //9c2fe24
	MOD_BGM = 13, //9c2fe24
	MOD_BGU = 14, //9c2fe24
	MOD_HIST,
	MOD_NCC,
	MOD_INTEG,
	MOD_DMA,
	MOD_SAD,
	MOD_ADD, //20
	MOD_AND,
	MOD_OR,
	MOD_SUB,
	MOD_XOR,
	MOD_THRESH,
	MOD_THRS16,
	MOD_THRU16,
	MOD_16To8,
	MOD_CSC,
	MOD_FILTERCSC, //30
	MOD_ORDSTAFTR,
	MOD_RESIZE,
	MOD_LBP,
	MOD_GMM2,
	MOD_BERNSEN,
	MOD_MD,
	MOD_CMDQ,
	MOD_ED,
	MOD_TEST,
	MOD_RESET, //40
	MOD_DUMP,
	MOD_QUERY,
	MOD_ALL, //43
};

enum dma_name {
	RDMA_IMG_IN = 0,
	RDMA_IMG1,
	RDMA_EIGVAL,
	RDMA_RADFG,
	RDMA_MM_FACTOR,
	RDMA_MM_MOD = 5,
	RDMA_GMODEL_0,
	RDMA_GMODEL_1,
	RDMA_GFLAG,
	RDMA_DMA,
	WDMA_DMA,
	WDMA_ODMA,
	WDMA_Y,
	WDMA_C,
	WDMA_HIST,
	WDMA_INTEG,
	WDMA_SAD,
	WDMA_SAD_THR,
	WDMA_GMM_MATCH,
	WDMA_GMM_MOD,
	WDMA_CHG,
	WDMA_BGMODEL_0,
	WDMA_BGMODEL_1,
	WDMA_FG,
	DMA_ALL,
};

#ifdef DEBUG
static char IveRegister[53][100] = {
	"IVE_TOP                 ", /* NULL if not initialized. */
	"IMG_IN                  ", /* NULL if not initialized. */
	"RDMA_IMG1               ", /* NULL if not initialized. */
	"MAP                     ", /* NULL if not initialized. */
	"HIST                    ", /* NULL if not initialized. */
	"HIST_WDMA               ", /* NULL if not initialized. */
	"INTG                    ", /* NULL if not initialized. */
	"INTG_WDMA               ", /* NULL if not initialized. */
	"SAD                     ", /* NULL if not initialized. */
	"SAD_WDMA                ", /* NULL if not initialized. */
	"SAD_WDMA_THR            ", /* NULL if not initialized. */
	"NCC                     ", /* NULL if not initialized. */
	"GMM_MODEL_RDMA_0        ", /* NULL if not initialized. */
	"GMM_MODEL_RDMA_1        ", /* NULL if not initialized. */
	"GMM_MODEL_RDMA_2        ", /* NULL if not initialized. */
	"GMM_MODEL_RDMA_3        ", /* NULL if not initialized. */
	"GMM_MODEL_RDMA_4        ", /* NULL if not initialized. */
	"GMM_MODEL_WDMA_0        ", /* NULL if not initialized. */
	"GMM_MODEL_WDMA_1        ", /* NULL if not initialized. */
	"GMM_MODEL_WDMA_2        ", /* NULL if not initialized. */
	"GMM_MODEL_WDMA_3        ", /* NULL if not initialized. */
	"GMM_MODEL_WDMA_4        ", /* NULL if not initialized. */
	"GMM_MATCH_WDMA          ", /* NULL if not initialized. */
	"GMM                     ", /* NULL if not initialized. */
	"GMM_FACTOR_RDMA         ", /* NULL if not initialized. */
	"BG_MATCH_FGFLAG_RDMA    ", /* NULL if not initialized. */
	"BG_MATCH_BGMODEL_0_RDMA ", /* NULL if not initialized. */
	"BG_MATCH_BGMODEL_1_RDMA ", /* NULL if not initialized. */
	"BG_MATCH_DIFFFG_WDMA    ", /* NULL if not initialized. */
	"BG_MATCH_IVE_MATCH_BG   ", /* NULL if not initialized. */
	"BG_UPDATE_FG_WDMA       ", /* NULL if not initialized. */
	"BG_UPDATE_BGMODEL_0_WDMA", /* NULL if not initialized. */
	"BG_UPDATE_BGMODEL_1_WDMA", /* NULL if not initialized. */
	"BG_UPDATE_CHG_WDMA      ", /* NULL if not initialized. */
	"BG_UPDATE_UPDATE_BG     ", /* NULL if not initialized. */
	"FILTEROP_RDMA           ", /* NULL if not initialized. */
	"FILTEROP_WDMA_Y         ", /* NULL if not initialized. */
	"FILTEROP_WDMA_C         ", /* NULL if not initialized. */
	"FILTEROP                ", /* NULL if not initialized. */
	"CCL                     ", /* NULL if not initialized. */
	"CCL_SRC_RDMA            ", /* NULL if not initialized. */
	"CCL_DST_WDMA            ", /* NULL if not initialized. */
	"CCL_REGION_WDMA         ", /* NULL if not initialized. */
	"CCL_SRC_RDMA_RELABEL    ", /* NULL if not initialized. */
	"CCL_DST_WDMA_RELABEL    ", /* NULL if not initialized. */
	"DMAF                    ", /* NULL if not initialized. */
	"DMAF_WDMA               ", /* NULL if not initialized. */
	"DMAF_RDMA               ", /* NULL if not initialized. */
	"LK                      ", /* NULL if not initialized. */
	"RDMA_EIGVAL             ", /* NULL if not initialized. */
	"WDMA                    ", /* NULL if not initialized. */
	"RDMA                    ", /* NULL if not initialized. */
	"CMDQ                    " /* NULL if not initialized. */
};
#endif

struct _IVE_ADDR_S {
	char addr_name[16];
	bool addr_en;
	int addr_l;
	int addr_h;
};

struct _IVE_MODE_S {
	bool op_en;
	int op_sel;
};

struct _IVE_DEBUG_INFO_S {
	char op_name[16];
	int src_w;
	int src_h;
	int src_fmt;
	int dst_fmt;
	struct _IVE_MODE_S op[2];
	struct _IVE_ADDR_S addr[24];
};

struct _IVE_IP_BLOCK_S {
	void __iomem *IVE_TOP; /* NULL if not initialized. */
	void __iomem *IMG_IN; /* NULL if not initialized. */
	void __iomem *RDMA_IMG1; /* NULL if not initialized. */
	void __iomem *MAP; /* NULL if not initialized. */
	void __iomem *HIST; /* NULL if not initialized. */
	void __iomem *HIST_WDMA; /* NULL if not initialized. */
	void __iomem *INTG; /* NULL if not initialized. */
	void __iomem *INTG_WDMA; /* NULL if not initialized. */
	void __iomem *SAD; /* NULL if not initialized. */
	void __iomem *SAD_WDMA; /* NULL if not initialized. */
	void __iomem *SAD_WDMA_THR; /* NULL if not initialized. */
	void __iomem *NCC; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_RDMA_0; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_RDMA_1; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_RDMA_2; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_RDMA_3; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_RDMA_4; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_WDMA_0; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_WDMA_1; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_WDMA_2; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_WDMA_3; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_WDMA_4; /* NULL if not initialized. */
	void __iomem *GMM_MATCH_WDMA; /* NULL if not initialized. */
	void __iomem *GMM; /* NULL if not initialized. */
	void __iomem *GMM_FACTOR_RDMA; /* NULL if not initialized. */
	void __iomem *BG_MATCH_FGFLAG_RDMA; /* NULL if not initialized. */
	void __iomem *BG_MATCH_BGMODEL_0_RDMA; /* NULL if not initialized. */
	void __iomem *BG_MATCH_BGMODEL_1_RDMA; /* NULL if not initialized. */
	void __iomem *BG_MATCH_DIFFFG_WDMA; /* NULL if not initialized. */
	void __iomem *BG_MATCH_IVE_MATCH_BG; /* NULL if not initialized. */
	void __iomem *BG_UPDATE_FG_WDMA; /* NULL if not initialized. */
	void __iomem *BG_UPDATE_BGMODEL_0_WDMA; /* NULL if not initialized. */
	void __iomem *BG_UPDATE_BGMODEL_1_WDMA; /* NULL if not initialized. */
	void __iomem *BG_UPDATE_CHG_WDMA; /* NULL if not initialized. */
	void __iomem *BG_UPDATE_UPDATE_BG; /* NULL if not initialized. */
	void __iomem *FILTEROP_RDMA; /* NULL if not initialized. */
	void __iomem *FILTEROP_WDMA_Y; /* NULL if not initialized. */
	void __iomem *FILTEROP_WDMA_C; /* NULL if not initialized. */
	void __iomem *FILTEROP; /* NULL if not initialized. */

	void __iomem *CCL; /* NULL if not initialized. */
	void __iomem *CCL_SRC_RDMA; /* NULL if not initialized. */
	void __iomem *CCL_DST_WDMA; /* NULL if not initialized. */
	void __iomem *CCL_REGION_WDMA; /* NULL if not initialized. */
	void __iomem *CCL_SRC_RDMA_RELABEL; /* NULL if not initialized. */
	void __iomem *CCL_DST_WDMA_RELABEL; /* NULL if not initialized. */

	void __iomem *DMAF; /* NULL if not initialized. */
	void __iomem *DMAF_WDMA; /* NULL if not initialized. */
	void __iomem *DMAF_RDMA; /* NULL if not initialized. */
	void __iomem *LK; /* NULL if not initialized. */
	void __iomem *RDMA_EIGVAL; /* NULL if not initialized. */
	void __iomem *WDMA; /* NULL if not initialized. */
	void __iomem *RDMA; /* NULL if not initialized. */
	void __iomem *CMDQ; /* NULL if not initialized. */
};

struct cmdq_adma {
	uint64_t addr;
	uint32_t size;
	uint32_t flags_end : 1;
	uint32_t rsv : 2;
	uint32_t flags_link : 1;
	uint32_t rsv2 : 28;
};

enum {
	CMDQ_SET_REG,
	CMDQ_SET_WAIT_TIMER,
	CMDQ_SET_WAIT_FLAG,
};

struct cmdq_set_reg {
	uint32_t data;
	uint32_t addr : 20;
	uint32_t byte_mask : 4;
	uint32_t intr_end : 1;
	uint32_t intr_int : 1;
	uint32_t intr_last : 1;
	uint32_t intr_rsv : 1;
	uint32_t action : 4;  // 0 for this case
};

struct cmdq_set_wait_timer {
	uint32_t counter;
	uint32_t rsv : 24;
	uint32_t intr_end : 1;
	uint32_t intr_int : 1;
	uint32_t intr_last : 1;
	uint32_t intr_rsv : 1;
	uint32_t action : 4;  // 1 for this case
};

struct cmdq_set_wait_flags {
	uint32_t flag_num;   // 0 ~ 15, depending on each module
	uint32_t rsv : 24;
	uint32_t intr_end : 1;
	uint32_t intr_int : 1;
	uint32_t intr_last : 1;
	uint32_t intr_rsv : 1;
	uint32_t action : 4;  // 2 for this case
};

union cmdq_set {
	struct cmdq_set_reg reg;
	struct cmdq_set_wait_timer wait_timer;
	struct cmdq_set_wait_flags wait_flags;
};


CVI_S32 cvi_ive_go(struct cvi_ive_device *ndev, IVE_TOP_C *ive_top_c,
		   CVI_BOOL bInstant, int done_mask, int optype);

CVI_S32 assign_ive_block_addr(void __iomem *ive_phy_base);

irqreturn_t platform_ive_irq(struct cvi_ive_device *ndev);

CVI_S32 cvi_ive_dump_hw_flow(void);
CVI_S32 cvi_ive_dump_op1_op2_info(void);
CVI_S32 cvi_ive_dump_reg_state(CVI_BOOL enable);
CVI_S32 cvi_ive_set_dma_dump(CVI_BOOL enable);
CVI_S32 cvi_ive_set_reg_dump(CVI_BOOL enable);
CVI_S32 cvi_ive_set_img_dump(CVI_BOOL enable);
void stcandicorner_workaround(struct cvi_ive_device *ndev);

CVI_S32 cvi_ive_reset(struct cvi_ive_device *ndev, int select);

CVI_S32 cvi_ive_test(struct cvi_ive_device *ndev, char *addr, CVI_U16 *w,
		     CVI_U16 *h);

CVI_S32 cvi_ive_CmdQ(struct cvi_ive_device *ndev);

CVI_S32 cvi_ive_Query(struct cvi_ive_device *ndev, CVI_BOOL *pbFinish,
		      CVI_BOOL bBlock);

CVI_S32 cvi_ive_DMA(struct cvi_ive_device *ndev, IVE_DATA_S *pstSrc,
		    IVE_DATA_S *pstDst, IVE_DMA_CTRL_S *pstDmaCtrl,
		    CVI_BOOL bInstant);

CVI_S32 cvi_ive_And(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
		    IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
		    CVI_BOOL bInstant);

CVI_S32 cvi_ive_Or(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
		   IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
		   CVI_BOOL bInstant);

CVI_S32 cvi_ive_Xor(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
		    IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
		    CVI_BOOL bInstant);

CVI_S32 cvi_ive_Add(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
		    IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
		    IVE_ADD_CTRL_S *pstAddCtrl, CVI_BOOL bInstant);

CVI_S32 cvi_ive_Sub(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
		    IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
		    IVE_SUB_CTRL_S *pstSubCtrl, CVI_BOOL bInstant);

CVI_S32 cvi_ive_Thresh(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		       IVE_DST_IMAGE_S *pstDst, IVE_THRESH_CTRL_S *pstThrCtrl,
		       CVI_BOOL bInstant);

CVI_S32 cvi_ive_Dilate(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		       IVE_DST_IMAGE_S *pstDst,
		       IVE_DILATE_CTRL_S *pstDilateCtrl, CVI_BOOL bInstant);

CVI_S32 cvi_ive_Erode(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		      IVE_DST_IMAGE_S *pstDst, IVE_ERODE_CTRL_S *pstErodeCtrl,
		      CVI_BOOL bInstant);

CVI_S32 cvi_ive_MatchBgModel(struct cvi_ive_device *ndev,
			     IVE_SRC_IMAGE_S *pstCurImg, IVE_DATA_S *pstBgModel,
			     IVE_IMAGE_S *pstFgFlag, IVE_DST_IMAGE_S *pstDiffFg,
			     IVE_DST_MEM_INFO_S *pstStatData,
			     IVE_MATCH_BG_MODEL_CTRL_S *pstMatchBgModelCtrl,
			     CVI_BOOL bInstant);

CVI_S32 cvi_ive_UpdateBgModel(struct cvi_ive_device *ndev,
			      IVE_DATA_S *pstBgModel, IVE_IMAGE_S *pstFgFlag,
			      IVE_DST_IMAGE_S *pstBgImg,
			      IVE_DST_IMAGE_S *pstChgSta,
			      IVE_DST_MEM_INFO_S *pstStatData,
			      IVE_UPDATE_BG_MODEL_CTRL_S *pstUpdateBgModelCtrl,
			      CVI_BOOL bInstant);

CVI_S32 cvi_ive_GMM(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		    IVE_DST_IMAGE_S *pstFg, IVE_DST_IMAGE_S *pstBg,
		    IVE_MEM_INFO_S *pstModel, IVE_GMM_CTRL_S *pstGmmCtrl,
		    CVI_BOOL bInstant);

CVI_S32 cvi_ive_GMM2(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		     IVE_SRC_IMAGE_S *pstFactor, IVE_DST_IMAGE_S *pstFg,
		     IVE_DST_IMAGE_S *pstBg, IVE_DST_IMAGE_S *pstMatchModelInfo,
		     IVE_MEM_INFO_S *pstModel, IVE_GMM2_CTRL_S *pstGmm2Ctrl,
		     CVI_BOOL bInstant);

CVI_S32 cvi_ive_Bernsen(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			IVE_DST_IMAGE_S *pstDst, IVE_BERNSEN_CTRL_S *pstLbpCtrl,
			CVI_BOOL bInstant);

CVI_S32 cvi_ive_Filter(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		       IVE_DST_IMAGE_S *pstDst, IVE_FILTER_CTRL_S *pstFltCtrl,
		       CVI_BOOL bInstant);

CVI_S32 cvi_ive_Sobel(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		      IVE_DST_IMAGE_S *pstDstH, IVE_DST_IMAGE_S *pstDstV,
		      IVE_SOBEL_CTRL_S *pstSobelCtrl, CVI_BOOL bInstant);

CVI_S32 cvi_ive_MagAndAng(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			  IVE_DST_IMAGE_S *pstDstMag,
			  IVE_DST_IMAGE_S *pstDstAng,
			  IVE_MAG_AND_ANG_CTRL_S *pstMagAndAngCtrl,
			  CVI_BOOL bInstant);

CVI_S32 cvi_ive_CSC(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		    IVE_DST_IMAGE_S *pstDst, IVE_CSC_CTRL_S *pstCscCtrl,
		    CVI_BOOL bInstant);

CVI_S32 cvi_ive_FilterAndCSC(struct cvi_ive_device *ndev,
			     IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
			     IVE_FILTER_AND_CSC_CTRL_S *pstFltCscCtrl,
			     CVI_BOOL bInstant);

CVI_S32 cvi_ive_Hist(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		     IVE_DST_MEM_INFO_S *pstDst, CVI_BOOL bInstant);

CVI_S32 cvi_ive_Map(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		    IVE_SRC_MEM_INFO_S *pstMap, IVE_DST_IMAGE_S *pstDst,
		    IVE_MAP_CTRL_S *pstMapCtrl, CVI_BOOL bInstant);

CVI_S32 cvi_ive_NCC(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
		    IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_MEM_INFO_S *pstDst,
		    CVI_BOOL bInstant);

CVI_S32 cvi_ive_Integ(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		      IVE_DST_MEM_INFO_S *pstDst,
		      IVE_INTEG_CTRL_S *pstIntegCtrl, CVI_BOOL bInstant);

CVI_S32 cvi_ive_LBP(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		    IVE_DST_IMAGE_S *pstDst, IVE_LBP_CTRL_S *pstLbpCtrl,
		    CVI_BOOL bInstant);

CVI_S32 cvi_ive_Thresh_S16(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			   IVE_DST_IMAGE_S *pstDst,
			   IVE_THRESH_S16_CTRL_S *pstThrS16Ctrl,
			   CVI_BOOL bInstant);

CVI_S32 cvi_ive_Thresh_U16(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			   IVE_DST_IMAGE_S *pstDst,
			   IVE_THRESH_U16_CTRL_S *pstThrU16Ctrl,
			   CVI_BOOL bInstant);

CVI_S32 cvi_ive_16BitTo8Bit(struct cvi_ive_device *ndev,
			    IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
			    IVE_16BIT_TO_8BIT_CTRL_S *pst16BitTo8BitCtrl,
			    CVI_BOOL bInstant);

CVI_S32 cvi_ive_OrdStatFilter(struct cvi_ive_device *ndev,
			      IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
			      IVE_ORD_STAT_FILTER_CTRL_S *pstOrdStatFltCtrl,
			      CVI_BOOL bInstant);

CVI_S32 cvi_ive_CannyHysEdge(struct cvi_ive_device *ndev,
			     IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstEdge,
			     IVE_DST_MEM_INFO_S *pstStack,
			     IVE_CANNY_HYS_EDGE_CTRL_S *pstCannyHysEdgeCtrl,
			     CVI_BOOL bInstant);

CVI_S32 cvi_ive_NormGrad(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			 IVE_DST_IMAGE_S *pstDstH, IVE_DST_IMAGE_S *pstDstV,
			 IVE_DST_IMAGE_S *pstDstHV,
			 IVE_NORM_GRAD_CTRL_S *pstNormGradCtrl,
			 CVI_BOOL bInstant);

CVI_S32 cvi_ive_GradFg(struct cvi_ive_device *ndev,
		       IVE_SRC_IMAGE_S *pstBgDiffFg,
		       IVE_SRC_IMAGE_S *pstCurGrad, IVE_SRC_IMAGE_S *pstBgGrad,
		       IVE_DST_IMAGE_S *pstGradFg,
		       IVE_GRAD_FG_CTRL_S *pstGradFgCtrl, CVI_BOOL bInstant);

CVI_S32 cvi_ive_SAD(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
		    IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstSad,
		    IVE_DST_IMAGE_S *pstThr, IVE_SAD_CTRL_S *pstSadCtrl,
		    CVI_BOOL bInstant);

CVI_S32 cvi_ive_Resize(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S astSrc[],
		       IVE_DST_IMAGE_S astDst[],
		       IVE_RESIZE_CTRL_S *pstResizeCtrl, CVI_BOOL bInstant);

CVI_S32 cvi_ive_imgInToOdma(struct cvi_ive_device *ndev,
			    IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
			    IVE_FILTER_CTRL_S *pstFltCtrl, CVI_BOOL bInstant);

CVI_S32 cvi_ive_rgbPToYuvToErodeToDilate(struct cvi_ive_device *ndev,
					 IVE_SRC_IMAGE_S *pstSrc,
					 IVE_DST_IMAGE_S *pstDst,
					 IVE_DST_IMAGE_S *pstDst2,
					 IVE_FILTER_CTRL_S *pstFltCtrl,
					 CVI_BOOL bInstant);

CVI_S32 cvi_ive_STCandiCorner(struct cvi_ive_device *ndev,
			      IVE_SRC_IMAGE_S *pstSrc,
			      IVE_DST_IMAGE_S *pstCandiCorner,
			      IVE_ST_CANDI_CORNER_CTRL_S *pstStCandiCornerCtrl,
			      CVI_BOOL bInstant);

CVI_S32 cvi_ive_EqualizeHist(struct cvi_ive_device *ndev,
			     IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
			     IVE_EQUALIZE_HIST_CTRL_S *pstEqualizeHistCtrl,
			     CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_CCL(IVE_HANDLE *pIveHandle, IVE_IMAGE_S *pstSrcDst,
			IVE_DST_MEM_INFO_S *pstBlob, IVE_CCL_CTRL_S *pstCclCtrl,
			CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_CannyEdge(IVE_IMAGE_S *pstEdge, IVE_MEM_INFO_S *pstStack);

CVI_S32 CVI_MPI_IVE_LKOpticalFlowPyr(
	IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S astSrcPrevPyr[],
	IVE_SRC_IMAGE_S astSrcNextPyr[], IVE_SRC_MEM_INFO_S *pstPrevPts,
	IVE_MEM_INFO_S *pstNextPts, IVE_DST_MEM_INFO_S *pstStatus,
	IVE_DST_MEM_INFO_S *pstErr,
	IVE_LK_OPTICAL_FLOW_PYR_CTRL_S *pstLkOptiFlowPyrCtrl,
	CVI_BOOL bInstant);

CVI_S32
cvi_ive_FrameDiffMotion(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
			IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
			IVE_FRAME_DIFF_MOTION_CTRL_S *ctrl, CVI_BOOL bInstant);

CVI_S32
CVI_MPI_IVE_STCandiCorner(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			  IVE_DST_IMAGE_S *pstCandiCorner,
			  IVE_ST_CANDI_CORNER_CTRL_S *pstStCandiCornerCtrl,
			  CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_STCorner(IVE_SRC_IMAGE_S *pstCandiCorner,
			     IVE_DST_MEM_INFO_S *pstCorner,
			     IVE_ST_CORNER_CTRL_S *pstStCornerCtrl);

CVI_S32 CVI_MPI_IVE_ANN_MLP_LoadModel(const CVI_CHAR *pchFileName,
				      IVE_ANN_MLP_MODEL_S *pstAnnMlpModel);

CVI_VOID CVI_MPI_IVE_ANN_MLP_UnloadModel(IVE_ANN_MLP_MODEL_S *pstAnnMlpModel);

CVI_S32 CVI_MPI_IVE_ANN_MLP_Predict(IVE_HANDLE *pIveHandle,
				    IVE_SRC_DATA_S *pstSrc,
				    IVE_LOOK_UP_TABLE_S *pstActivFuncTab,
				    IVE_ANN_MLP_MODEL_S *pstAnnMlpModel,
				    IVE_DST_DATA_S *pstDst, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_Query(IVE_HANDLE IveHandle, CVI_BOOL *pbFinish,
			  CVI_BOOL bBlock);

#ifdef __cplusplus
}
#endif
#endif /*_CVI_IVE_PLATFORM_H_*/
