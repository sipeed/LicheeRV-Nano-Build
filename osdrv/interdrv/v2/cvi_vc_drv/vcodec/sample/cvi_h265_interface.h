/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_common.h
 * Description: Common video definitions.
 */

#ifndef __CVI_H265_INTERFACE_H__
#define __CVI_H265_INTERFACE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef PhysicalAddress
/**
* @brief    This is a type for representing physical addresses which are
recognizable by VPU. In general, VPU hardware does not know about virtual
address space which is set and handled by host processor. All these virtual
addresses are translated into physical addresses by Memory Management Unit. All
data buffer addresses such as stream buffer and frame buffer should be given to
VPU as an address on physical address space.
*/
#include "vputypes.h"
#if defined(__aarch64__) || defined(__arm__) || defined(__riscv)
#include <linux/types.h>
typedef __u64 PhysicalAddress;
#else
#if defined(__amd64__) || defined(__x86_64__)
typedef Uint64 PhysicalAddress;
#else
typedef Uint32 PhysicalAddress;
#endif
#endif
#endif

typedef enum _CVI_H264E_PROFILE {
	CVI_H264E_PROFILE_BASELINE = 0,
	CVI_H264E_PROFILE_MAIN,
	CVI_H264E_PROFILE_HIGH,
} CVI_H264E_PROFILE;

typedef enum _RC_MODE_ {
	RC_MODE_CBR = 0,
	RC_MODE_VBR,
	RC_MODE_AVBR,
	RC_MODE_QVBR,
	RC_MODE_FIXQP,
	RC_MODE_QPMAP,
	RC_MODE_UBR,
	RC_MODE_MAX,
} RC_MODE;

enum _CODEC_ {
	CODEC_NONE = 0,
	CODEC_H264,
	CODEC_H265,
	CODEC_JPEG,
	CODEC_MAX,
};

typedef enum _NAL_TYPE_ {
	NAL_NONE = 0,
	NAL_I,
	NAL_P,
	NAL_B,
	NAL_IDR,
	NAL_SPS,
	NAL_PPS,
	NAL_SEI,
	NAL_VPS,
	NAL_MAX,
} NAL_TYPE;

/*the super frame mode*/
typedef enum _CVI_SUPERFRM_MODE_E_ {
	CVI_SUPERFRM_NONE = 0, /* sdk don't care super frame */
	CVI_SUPERFRM_REENCODE_IDR, /* the super frame is re-encode to IDR */
	CVI_SUPERFRM_MAX
} CVI_SUPERFRM_MODE_E;

// H265 decoding refresh type
typedef enum _H265_REFRESH_TYPE_ {
	H265_RT_NON_IRAP = 0,
	H265_RT_CRA,
	H265_RT_IDR,
} H265_REFRESH_TYPE;

typedef enum _ApiMode_ {
	API_MODE_DRIVER = 0,
	API_MODE_SDK,
	API_MODE_NON_OS,
} ApiMode;

typedef enum _CVI_VDEC_DEC_RET_ {
	CVI_VDEC_RET_FRM_DONE = 0,
	CVI_VDEC_RET_DEC_ERR,
	CVI_VDEC_RET_NO_FB,
	CVI_VDEC_RET_LAST_FRM,
	CVI_VDEC_RET_CONTI,
	CVI_VDEC_RET_STOP,
	CVI_VDEC_RET_LOCK_TIMEOUT,
	CVI_VDEC_RET_NO_FB_WITH_DISP,
} CVI_VDEC_DEC_RET;

typedef struct _cviInitEncConfig_ {
	int codec;
	int width;
	int height;
	unsigned int u32Profile;
	int rcMode;
	int s32IPQpDelta;
	int s32BgQpDelta;
	int s32ViQpDelta;
	int iqp;
	int pqp;
	int gop;
	int virtualIPeriod;
	int bitrate;
	int framerate;
	int bVariFpsEn;
	int maxbitrate;
	int s32ChangePos;
	int s32MinStillPercent;
	int u32MaxStillQP;
	int u32MotionSensitivity;
	int s32AvbrFrmLostOpen;
	int s32AvbrFrmGap;
	int s32AvbrPureStillThr;
	int bBgEnhanceEn;
	int s32BgDeltaQp;
	int statTime;
	unsigned int bitstreamBufferSize;
	int singleLumaBuf;
	int bSingleEsBuf;
	int userDataMaxLength;
	int decodingRefreshType;
	int initialDelay;
	int s32ChnNum;
	int bEsBufQueueEn;
	int bIsoSendFrmEn;
	int svc_enable;
	int fg_protect_en;
	int fg_dealt_qp;
	int complex_scene_detect_en;
	int complex_scene_low_th;
	int complex_scene_hight_th;
	int middle_min_percent;
	int complex_min_percent;
	int smart_ai_en;
} cviInitEncConfig;

typedef struct _cviEncOnePicCfg_ {
	void *addrY;
	void *addrCb;
	void *addrCr;
	PhysicalAddress phyAddrY;
	PhysicalAddress phyAddrCb;
	PhysicalAddress phyAddrCr;
	int stride;
	int cbcrInterleave;
	int nv21;
	int picMotionLevel;
	int picDciLv;
	unsigned char *picMotionMap;
	int picMotionMapSize;
	uint64_t u64Pts;
} cviEncOnePicCfg;

typedef struct _cviDispFrameCfg_ {
	void *addrY;
	void *addrCb;
	void *addrCr;
	PhysicalAddress phyAddrY;
	PhysicalAddress phyAddrCb;
	PhysicalAddress phyAddrCr;
	int width;
	int height;
	int strideY;
	int strideC;
	int cbcrInterleave;
	int nv21;
	int indexFrameDisplay;
	uint64_t decHwTime;
} cviDispFrameCfg;

typedef struct _cviBufInfo {
	PhysicalAddress phyAddr;
	void *virtAddr;
	int size;
} cviBufInfo;

#define MAX_NUM_PACKS 8
#define H26X_BLOCK_MODE (-1)
#define RET_VDEC_LOCK_TIMEOUT (-2)

typedef struct _stPack_ {
	uint64_t u64PhyAddr;
	void *addr;
	int len;
	int cviNalType;
	int need_free;
	uint64_t u64PTS;
	int bUsed;
} stPack;

typedef struct _stStreamPack_ {
	struct mutex packMutex;
	int totalPacks;
	unsigned int dropCnt;
	stPack pack[MAX_NUM_PACKS];
	unsigned int seq;
} stStreamPack;

typedef struct _cviVEncStreamInfo_ {
	stStreamPack *psp;
	uint64_t encHwTime;
	uint32_t u32MeanQp;
} cviVEncStreamInfo;

typedef struct _cviRcParam_ {
	unsigned int u32ThrdLv;
	unsigned int
		bBgEnhanceEn; /* RW; Range:[0, 1];  Enable background enhancement */
	int s32BgDeltaQp; /* RW; Range:[-51, 51]; Backgournd Qp Delta */
	unsigned int u32RowQpDelta;
	// RW; Range:[0, 10];the start QP value of each macroblock row relative to the start QP value
	int firstFrmstartQp;
	// RW; Range:[50, 100]; Indicates the ratio of the current bit rate to the maximum
	// bit rate when the QP value starts to be adjusted
	int s32ChangePos;
	unsigned int
		u32MaxIprop; /* RW; Range:[MinIprop, 100];the max I prop value */
	unsigned int
		u32MinIprop; /* RW; Range:[1, 100];the min I prop prop value */
	unsigned int u32MaxQp; /* RW; Range:(MinQp, 51];the max QP value */
	unsigned int u32MinQp; /* RW; Range:[0, 51]; the min QP value */
	unsigned int u32MaxIQp; /* RW; Range:(MinQp, 51]; max qp for i frame */
	unsigned int u32MinIQp; /* RW; Range:[0, 51]; min qp for i frame */
	int s32MinStillPercent; /* RW; Range:[20, 100]; the min percent of target bitrate for still scene */
	unsigned int
		u32MaxStillQP; /* RW; Range:[MinIQp, MaxIQp]; the max QP value of I frame for still scene*/
	unsigned int
		u32MotionSensitivity; /* RW; Range:[0, 100]; Motion Sensitivity */
	unsigned int s32AvbrFrmLostOpen;
	unsigned int s32AvbrFrmGap;
	unsigned int s32AvbrPureStillThr;
	int s32InitialDelay;
	int s32MaxReEncodeTimes; /* RW; Range:[0, 3]; Range:max number of re-encode times.*/
} cviRcParam;

#define MAX_REG_FRAME_H26X 62
typedef struct _cviVbVencBufConfig {
	int VbType;
	//vb type private mode, get calculate buf size
	int VbGetFrameBufSize;
	int VbGetFrameBufCnt;
	//vb type private mode & user mode
	//set in frm cnt & size by user
	int VbSetFrameBufSize;
	int VbSetFrameBufCnt;
	uint64_t vbModeAddr[10];
} cviVbVencBufConfig;

typedef struct _cviPred_ {
	unsigned int u32IntraCost;
} cviPred;

typedef struct _cviInPixelFormat_ {
	int bCbCrInterleave;
	int bNV21;
} cviInPixelFormat;

typedef struct _cviRoiParam_ {
	int roi_index;
	int roi_enable;
	int roi_qp_mode;
	int roi_qp;
	int roi_rect_x; // pixel position
	int roi_rect_y; // pixel position
	unsigned int roi_rect_width; // pixel width
	unsigned int roi_rect_height; // pixel width
} cviRoiParam;

typedef struct _cviVidChnAttr_ {
	unsigned int
		u32SrcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc channel */
	unsigned int
		fr32DstFrameRate; /* RW; Range:[0.015625, u32SrcFrmRate]; the target frame rate of the venc channel */
	unsigned int u32BitRate; /* RW; Range:[2, 409600]; average bitrate */
} cviVidChnAttr;

typedef struct _cviFrameLost_ {
	int frameLostMode;
	unsigned int u32EncFrmGaps;
	int bFrameLostOpen;
	unsigned int u32FrmLostBpsThr;
} cviFrameLost;

typedef struct _cviUserData_ {
	unsigned char *userData;
	unsigned int len;
} cviUserData;

typedef struct _cviH264Entropy_ {
	unsigned int
		entropyEncModeI; /* RW; Range:[0,1]; Entropy encoding mode for the I frame, 0:cavlc, 1:cabac */
	unsigned int
		entropyEncModeP; /* RW; Range:[0,1]; Entropy encoding mode for the P frame, 0:cavlc, 1:cabac */
	unsigned int
		entropyEncModeB; /* RW; Range:[0,1]; Entropy encoding mode for the B frame, 0:cavlc, 1:cabac */
	unsigned int
		cabac_init_idc; /* RW; Range:[0,2]; see the H.264 protocol for the meaning */
} cviH264Entropy;

typedef struct _cviH264Trans_ {
	unsigned int u32IntraTransMode; // RW; Range:[0,2]; Conversion mode for
		// intra-prediction,0: trans4x4, trans8x8; 1: trans4x4, 2: trans8x8
	unsigned int u32InterTransMode; // RW; Range:[0,2]; Conversion mode for
		// inter-prediction,0: trans4x4, trans8x8; 1: trans4x4, 2: trans8x8

	int bScalingListValid; /* RW; Range:[0,1]; enable Scaling,default: CVI_FALSE  */
	unsigned char InterScalingList8X8
		[64]; /* RW; Range:[1,255]; A quantization table for 8x8 inter-prediction*/
	unsigned char IntraScalingList8X8
		[64]; /* RW; Range:[1,255]; A quantization table for 8x8 intra-prediction*/

	int chroma_qp_index_offset; /* RW; Range:[-12,12];default value: 0, see the H.264 protocol for the meaning*/
} cviH264Trans;

typedef struct _cviH265Trans_ {
	int cb_qp_offset; /* RW; Range:[-12,12]; see the H.265 protocol for the meaning. */
	int cr_qp_offset; /* RW; Range:[-12,12]; see the H.265 protocol for the meaning. */

	int bScalingListEnabled; /* RW; Range:[0,1]; If 1, specifies that a scaling list is used.*/

	int bScalingListTu4Valid; /* RW; Range:[0,1]; If 1, ScalingList4X4 belows will be encoded.*/
	unsigned char InterScalingList4X4
		[2][16]; /* RW; Range:[1,255]; Scaling List for inter 4X4 block.*/
	unsigned char IntraScalingList4X4
		[2][16]; /* RW; Range:[1,255]; Scaling List for intra 4X4 block.*/

	int bScalingListTu8Valid; /* RW; Range:[0,1]; If 1, ScalingList8X8 belows will be encoded.*/
	unsigned char InterScalingList8X8
		[2][64]; /* RW; Range:[1,255]; Scaling List for inter 8X8 block.*/
	unsigned char IntraScalingList8X8
		[2][64]; /* RW; Range:[1,255]; Scaling List for intra 8X8 block.*/

	int bScalingListTu16Valid; /* RW; Range:[0,1]; If 1, ScalingList16X16 belows will be encoded.*/
	unsigned char InterScalingList16X16
		[2]
		[64]; /* RW; Range:[1,255]; Scaling List for inter 16X16 block..*/
	unsigned char IntraScalingList16X16
		[2]
		[64]; /* RW; Range:[1,255]; Scaling List for inter 16X16 block.*/

	int bScalingListTU32Valid; /* RW; Range:[0,1]; If 1, ScalingList32X32 belows will be encoded.*/
	unsigned char InterScalingList32X32
		[64]; /* RW; Range:[1,255]; Scaling List for inter 32X32 block..*/
	unsigned char IntraScalingList32X32
		[64]; /* RW; Range:[1,255]; Scaling List for inter 32X32 block.*/
} cviH265Trans;

#define EXTENDED_SAR 255

typedef struct _cviVuiAspectRatio_ {
	unsigned char
		aspect_ratio_info_present_flag;
	// RW; Range: [0, 1]; If 1, aspect ratio info below will be encoded into vui.
	unsigned char
		aspect_ratio_idc; // RW; Range: [0, 255]; 17~254 is reserved. See the protocol for the meaning.
	unsigned char
		overscan_info_present_flag; // RW; Range: [0, 1]; If 1, overscan info below will be encoded into vui.
	unsigned char
		overscan_appropriate_flag; // RW; Range: [0, 1]; See the protocol for the meaning.
	unsigned short
		sar_width; // RW; Range: [1, 65535]; See the protocol for the meaning.
	unsigned short
		sar_height; // RW; Range: [1, 65535]; See the protocol for the meaning.
		//      note: sar_width and sar_height shall be relatively prime.
} cviVuiAspectRatio;

typedef struct _cviVuiH264TimingInfo_ {
	unsigned char
		timing_info_present_flag; // RW; Range: [0, 1]; If 1, timing info below will be encoded into vui.
	unsigned char
		fixed_frame_rate_flag; // RW; Range: [0, 1]; See the H.264 protocol for the meaning.
	unsigned int
		num_units_in_tick; // RW; Range: [1, 4294967295]; See the H.264 protocol for the meaning.
	unsigned int
		time_scale; // RW; Range: [1, 4294967295]; See the H.264 protocol for the meaning.
} cviVuiH264TimingInfo;

typedef struct _cviVuiH265TimingInfo_ {
	unsigned int
		timing_info_present_flag; // RW; Range: [0, 1]; If 1, timing info below will be encoded into vui.
	unsigned int
		num_units_in_tick; // RW; Range: [1, 4294967295]; See the H.265 protocol for the meaning.
	unsigned int
		time_scale; // RW; Range: [1, 4294967295]; See the H.265 protocol for the meaning.
	unsigned int
		num_ticks_poc_diff_one_minus1; // RW; Range: [0, 4294967294]; See the H.265 protocol for the meaning.
} cviVuiH265TimingInfo;

typedef struct _cviVuiVideoSignalType_ {
	unsigned char
		video_signal_type_present_flag; // RW; Range: [0, 1]; If 1, video singnal info will be encoded into vui.
	unsigned char
		video_format; // RW; H.264e Range: [0, 7], H.265e Range: [0, 5]; See the protocol for the meaning.
	unsigned char
		video_full_range_flag; // RW; Range: [0, 1]; See the protocol for the meaning.
	unsigned char
		colour_description_present_flag; // RO; Range: [0, 1]; See the protocol for the meaning.
	unsigned char
		colour_primaries; // RO; Range: [0, 255]; See the protocol for the meaning.
	unsigned char
		transfer_characteristics; // RO; Range: [0, 255]; See the protocol for the meaning.
	unsigned char
		matrix_coefficients; // RO; Range: [0, 255]; See the protocol for the meaning.
} cviVuiVideoSignalType;

typedef struct _cviVuiBitstreamRestriction_ {
	unsigned char
		bitstream_restriction_flag; // RW; Range: {0, 1}; See the protocol for the meaning.
} cviVuiBitstreamRestriction;

typedef struct _cviH264Vui_ {
	cviVuiAspectRatio aspect_ratio_info;
	cviVuiH264TimingInfo timing_info;
	cviVuiVideoSignalType video_signal_type;
	cviVuiBitstreamRestriction bitstream_restriction;
} cviH264Vui;

typedef struct _cvi_h265Vui_ {
	cviVuiAspectRatio aspect_ratio_info;
	cviVuiH265TimingInfo timing_info;
	cviVuiVideoSignalType video_signal_type;
	cviVuiBitstreamRestriction bitstream_restriction;
} cviH265Vui;

typedef struct _cvi_h264Dblk_ {
	CVI_U32 disable_deblocking_filter_idc;
	CVI_S32 slice_alpha_c0_offset_div2;
	CVI_S32 slice_beta_offset_div2;
} cviH264Dblk;

typedef struct _cvi_h265Dblk_ {
	CVI_U32 slice_deblocking_filter_disabled_flag;
	CVI_S32 slice_beta_offset_div2;
	CVI_S32 slice_tc_offset_div2;
} cviH265Dblk;

typedef struct _cviUserRcInfo_ {
	unsigned int bQpMapValid;
	unsigned int bRoiBinValid;
	int roideltaqp;
	PhysicalAddress u64QpMapPhyAddr;
} cviUserRcInfo;

typedef struct _cviSuperFrame {
	CVI_SUPERFRM_MODE_E enSuperFrmMode;
	unsigned int
		u32SuperIFrmBitsThr; // RW; Range:[0, 33554432];Indicate the threshold
	// of the super I frame for enabling the super frame processing mode
	unsigned int
		u32SuperPFrmBitsThr; // RW; Range:[0, 33554432];Indicate the threshold
	// of the super P frame for enabling the super frame processing mode
} cviSuperFrame;

typedef struct _cviFrameParam_ {
	unsigned int u32FrameQp;
	unsigned int u32FrameBits;
} cviFrameParam;

typedef struct _cvi_h264Split_ {
	int bSplitEnable;
	// RW; Range:[0,1]; slice split enable, CVI_TRUE:enable, CVI_FALSE:disable, default value:CVI_FALSE
	unsigned int u32MbLineNum;
	// RW; Range:[1,(Picture height + 15)/16]; this value presents the mb line number of one slice
} cviH264Split;

typedef struct _cvi_h265Split_ {
	int bSplitEnable; // RW; Range:[0,1]; slice split enable, CVI_TRUE:enable, default value:CVI_FALSE
	unsigned int u32LcuLineNum; // RW; Range:(Picture height + lcu size minus one)/lcu, lcu=64
} cviH265Split;

typedef struct _cvi_h264IntraPred_ {
	CVI_U32 constrained_intra_pred_flag;
	// RW; Range:[0,1];default: CVI_FALSE, see the H.264 protocol for the meaning
} cviH264IntraPred;

typedef struct _cvi_svcParam_ {
	int fg_protect_en;
	int fg_dealt_qp;

	/*static scene auto change bitrate according dci_lv threshold*/
	int complex_scene_detect_en;
	/*dci_lv avg < threshold as simple scene Range：[0, 512]*/
	unsigned int complex_scene_low_th;
	/*dci_lv avg > threshold as complex scene Range：[0, 512]*/
	unsigned int complex_scene_hight_th;
	/*middle scene still case min bitrate*/
	unsigned int middle_min_percent;
	/*complex scene still case min bitrate*/
	unsigned int complex_min_percent;

	/*when smart_ai_en is 1, user show pass level roi used USER_FRAME_INFO_S
	 * and bRoiBinValid set valid
	 */
	int smart_ai_en;
} cviSvcParam;

#define CVI_H26X_OP_BASE 0x100
#define CVI_H26X_OP_MASK 0xFF00
#define CVI_H26X_OP_SHIFT 8

enum EncIoctlOp {
	OP_NONE = 0,
	OP_SET_RC_PARAM,
	OP_START,
#if 0
	OP_GET_FD,
#endif
	OP_SET_REQUEST_IDR,
	OP_SET_CHN_ATTR,
	OP_SET_REF_PARAM,
	OP_SET_ROI_PARAM,
	OP_GET_ROI_PARAM,
	OP_SET_FRAME_LOST_STRATEGY,
	OP_GET_VB_INFO,
	OP_SET_VB_BUFFER,
	OP_SET_USER_DATA,
	OP_SET_PREDICT,
	OP_SET_H264_ENTROPY,
	OP_SET_H264_TRANS,
	OP_SET_H265_TRANS,
	OP_REG_VB_BUFFER,
	OP_SET_IN_PIXEL_FORMAT,
	OP_GET_CHN_INFO,
	OP_SET_USER_RC_INFO,
	OP_SET_SUPER_FRAME,
	OP_SET_H264_VUI,
	OP_SET_H265_VUI,
	OP_SET_FRAME_PARAM,
	OP_CALC_FRAME_PARAM,
#if 0
	OP_SET_SB_MODE,
	OP_START_SB_MODE,
	OP_UPDATE_SB_WPTR,
	OP_RESET_SB,
	OP_SB_EN_DUMMY_PUSH,
	OP_SB_TRIG_DUMMY_PUSH,
	OP_SB_DIS_DUMMY_PUSH,
	OP_SB_GET_SKIP_FRM_STATUS,
#endif
	OP_SET_SBM_ENABLE,
	OP_WAIT_FRAME_DONE,
	OP_SET_H264_SPLIT,
	OP_SET_H265_SPLIT,
	OP_SET_H264_DBLK,
	OP_SET_H265_DBLK,
	OP_SET_H264_INTRA_PRED,
	OP_DROP_FRAME,
	SET_SBM_SETTING,
	OP_SET_ENABLE_SVC,
	OP_SET_SVC_PARAM,
	OP_GET_RC_REAL_INFO,
	OP_MAX,
};

typedef enum _cviVEncIoctlOp_ {
	CVI_H26X_OP_NONE = 0,
	CVI_H26X_OP_SET_RC_PARAM =            (OP_SET_RC_PARAM << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_START =                   (OP_START << CVI_H26X_OP_SHIFT),
#if 0
	CVI_H26X_OP_GET_FD =                  (OP_GET_FD << CVI_H26X_OP_SHIFT),
#endif
	CVI_H26X_OP_SET_REQUEST_IDR =         (OP_SET_REQUEST_IDR << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_CHN_ATTR  =           (OP_SET_CHN_ATTR << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_REF_PARAM =           (OP_SET_REF_PARAM << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_ROI_PARAM =           (OP_SET_ROI_PARAM << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_GET_ROI_PARAM =           (OP_GET_ROI_PARAM << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_FRAME_LOST_STRATEGY = (OP_SET_FRAME_LOST_STRATEGY << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_GET_VB_INFO =             (OP_GET_VB_INFO << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_VB_BUFFER =           (OP_SET_VB_BUFFER << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_USER_DATA =           (OP_SET_USER_DATA << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_PREDICT =             (OP_SET_PREDICT << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_H264_ENTROPY =        (OP_SET_H264_ENTROPY << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_H264_TRANS =          (OP_SET_H264_TRANS << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_H265_TRANS =          (OP_SET_H265_TRANS << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_REG_VB_BUFFER =           (OP_REG_VB_BUFFER << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_IN_PIXEL_FORMAT =     (OP_SET_IN_PIXEL_FORMAT << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_GET_CHN_INFO =            (OP_GET_CHN_INFO << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_USER_RC_INFO =        (OP_SET_USER_RC_INFO << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_SUPER_FRAME =         (OP_SET_SUPER_FRAME << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_H264_VUI =            (OP_SET_H264_VUI << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_H265_VUI =            (OP_SET_H265_VUI << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_FRAME_PARAM =         (OP_SET_FRAME_PARAM << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_CALC_FRAME_PARAM =        (OP_CALC_FRAME_PARAM << CVI_H26X_OP_SHIFT),
#if 0
	CVI_H26X_OP_SET_SB_MODE =             (OP_SET_SB_MODE << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_START_SB_MODE =           (OP_START_SB_MODE << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_UPDATE_SB_WPTR =          (OP_UPDATE_SB_WPTR << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_RESET_SB =                (OP_RESET_SB << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SB_EN_DUMMY_PUSH =        (OP_SB_EN_DUMMY_PUSH << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SB_TRIG_DUMMY_PUSH =      (OP_SB_TRIG_DUMMY_PUSH << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SB_DIS_DUMMY_PUSH =       (OP_SB_DIS_DUMMY_PUSH << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SB_GET_SKIP_FRM_STATUS =  (OP_SB_GET_SKIP_FRM_STATUS << CVI_H26X_OP_SHIFT),
#endif
	CVI_H26X_OP_SET_SBM_ENABLE =          (OP_SET_SBM_ENABLE << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_WAIT_FRAME_DONE =         (OP_WAIT_FRAME_DONE << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_H264_SPLIT =          (OP_SET_H264_SPLIT << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_H265_SPLIT =          (OP_SET_H265_SPLIT << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_H264_DBLK =           (OP_SET_H264_DBLK << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_H265_DBLK  =          (OP_SET_H265_DBLK << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_H264_INTRA_PRED =     (OP_SET_H264_INTRA_PRED << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_DROP_FRAME =              (OP_DROP_FRAME << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_SBM_SETTING =         (SET_SBM_SETTING << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_ENABLE_SVC =          (OP_SET_ENABLE_SVC << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_SET_SVC_PARAM  =          (OP_SET_SVC_PARAM << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_GET_RC_REAL_INFO =        (OP_GET_RC_REAL_INFO << CVI_H26X_OP_SHIFT),
	CVI_H26X_OP_MAX =                     (OP_MAX << CVI_H26X_OP_SHIFT),
} cviVEncIoctlOp;

void cviVcodecGetVersion(void);
void *cviVEncOpen(cviInitEncConfig *pInitEncCfg);
int cviVEncClose(void *handle);
int cviVEncEncOnePic(void *handle, cviEncOnePicCfg *pPicCfg, int s32MilliSec);
int cviVEncGetStream(void *handle, cviVEncStreamInfo *pStreamInfo,
		     int s32MilliSec);
int cviVEncReleaseStream(void *handle, cviVEncStreamInfo *pStreamInfo);
int cviVEncIoctl(void *handle, int op, void *arg);
void cviCopyMotionMap(void *handle, cviEncOnePicCfg *pPicCfg, void *phandle);
typedef struct _cviInitDecConfig_ {
	int codec;
	int cviApiMode;
	int vbSrcMode;
	int chnNum;
	int argc;
	char **argv;
	int bsBufferSize;
	int reorderEnable;
} cviInitDecConfig;

typedef struct _cviDecOnePicCfg_ {
	void *bsAddr;
	int bsLen;
	int bEndOfStream;

	// output buffer format
	int cbcrInterleave;
	int nv21;
} cviDecOnePicCfg;

typedef struct _stCviCb_HostAllocFB_ {
	int iFrmBufSize;
	int iFrmBufNum;
	int iTmpBufSize;
	int iTmpBufNum;
} stCviCb_HostAllocFB;

typedef struct _cviVencSbSetting_ {
	unsigned int codec;  // 0x1:h265, 0x2:h264, 0x4:jpeg
	CVI_U32 sb_mode;
	CVI_U32 sb_size;
	CVI_U32 sb_nb;
	CVI_U32 y_stride;
	CVI_U32 uv_stride;
	CVI_U32 src_height;
	//pri sb address
	CVI_U32 sb_ybase;
	CVI_U32 sb_uvbase;
	CVI_U32 src_ybase;  //[out]
	CVI_U32 src_uvbase; //[out]
	// sec sb address
	CVI_U32 sb_ybase1;
	CVI_U32 sb_uvbase1;
	CVI_U32 src_ybase1;  //[out]
	CVI_U32 src_uvbase1; //[out]
	// status
	unsigned int status;
	//bind Vpss Chn
	CVI_S32 VpssGrp;
	CVI_S32 VpssChn;
} cviVencSbSetting;

typedef enum _eCviVDecCbOp_ {
	CVI_H26X_DEC_CB_NONE = 0,
	CVI_H26X_DEC_CB_AllocFB,
	CVI_H26X_DEC_CB_FreeFB,
	CVI_H26X_DEC_CB_GET_DISPQ_COUNT,
	CVI_H26X_DEC_CB_MAX,
} eCviVDecCbOp;

typedef int (*CVI_VDEC_DRV_CALLBACK)(unsigned int nChn, unsigned int nCbType,
				     void *pArg);

int cviVDecOpen(cviInitDecConfig *pInitDecCfg, void *pHandle);
int cviVDecClose(void *pHandle);
int cviVDecReset(void *pHandle);
int cviVDecDecOnePic(void *pHandle, cviDecOnePicCfg *pdopc, int timeout_ms);
int cviVDecGetFrame(void *pHandle, cviDispFrameCfg *pdfc);
void cviVDecReleaseFrame(void *pHandle, void *arg);
void cviVDecAttachFrmBuf(void *pHandle, void *pFrmBufArray, int nFrmNum);
void cviVDecAttachCallBack(CVI_VDEC_DRV_CALLBACK pCbFunc);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
