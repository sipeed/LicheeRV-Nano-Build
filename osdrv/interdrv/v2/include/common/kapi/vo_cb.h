#ifndef __VO_CB_H__
#define __VO_CB_H__

#ifdef __cplusplus
	extern "C" {
#endif
#include <dwa_cb.h>

enum VO_CB_CMD {
	VO_CB_IRQ_HANDLER,
	VO_CB_GET_RGN_HDLS,
	VO_CB_SET_RGN_HDLS,
	VO_CB_SET_RGN_CFG,
	VO_CB_SET_RGN_COVEREX_CFG,
	VO_CB_GET_CHN_SIZE,
	VO_CB_QBUF_TRIGGER,
	VO_CB_QBUF_VO_GET_CHN_ROTATION,
	VO_CB_SET_FB_ON_VPSS,
	VO_CB_GDC_OP_DONE = DWA_CB_GDC_OP_DONE,
	VO_CB_MAX
};

struct vo_get_chnrotation_cfg {
	u8 VoLayer;
	u8 VoChn;
	u8 enRotation;
};

#ifdef __cplusplus
}
#endif

#endif /* __VO_CB_H__ */
