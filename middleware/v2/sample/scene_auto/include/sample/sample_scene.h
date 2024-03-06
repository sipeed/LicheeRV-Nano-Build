#ifndef __SAMPLE_SCENE_H__
#define __SAMPLE_SCENE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <linux/cvi_common.h>

#include "sample_comm.h"

typedef struct {
	SAMPLE_VI_CONFIG_S  stViConfig;
	SAMPLE_VO_CONFIG_S  stVoConfig;
	VI_DEV              ViDev;
	VI_PIPE             ViPipe;
	VI_CHN              ViChn;
	VPSS_GRP            VpssGrp;
	VPSS_CHN            VpssChn;
	VO_DEV              VoDev;
	VO_CHN              VoChn;
	CVI_BOOL            abChnEnable[VPSS_MAX_PHY_CHN_NUM];
} TStreamInfo;

CVI_S32 SAMPLE_SCENE_VoRotation_Start(TStreamInfo *ptStreamInfo);
CVI_S32 SAMPLE_SCENE_VoRotation_Stop(TStreamInfo *ptStreamInfo);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // __SAMPLE_SCENE_H__
