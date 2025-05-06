#ifndef __VENC_DEBUG_H__
#define __VENC_DEBUG_H__

#include "cvi_venc.h"

#ifdef CLI_DEBUG_SUPPORT
#include "tcli.h"
extern void venc_register_cmd(void);
extern void cviCliDumpSrcYuv(int chn, const VIDEO_FRAME_INFO_S *pstFrame);
extern CVI_S32 cviDumpVencBitstream(VENC_CHN VeChn, VENC_STREAM_S *pstStream,
				    PAYLOAD_TYPE_E enType);
extern void venc_set_handler_state(VENC_CHN VeChn, int state);
extern void venc_channel_run_status_addself(VENC_CHN VeChn, int index);
extern void venc_handler_exit_retcode(VENC_CHN VeChn, int retcode);
extern void cviGetMaxBitstreamSize(VENC_CHN VeChn, VENC_STREAM_S *pstStream);

#define VENC_SET_HANDLER_STATE(state)	venc_set_handler_state(VencChn, state)

#define VENC_SET_HANDLER_EXIT_RETCODE(retcode)	venc_handler_exit_retcode(VencChn, retcode)

#define VENC_STATUS_RUN_ADDSELF(index)	venc_channel_run_status_addself(VencChn, index)

#else

#define VENC_SET_HANDLER_STATE(state)
#define VENC_SET_HANDLER_EXIT_RETCODE(retcode)
#define VENC_STATUS_RUN_ADDSELF(index)

#endif

#endif //__VENC_DEBUG_H__
