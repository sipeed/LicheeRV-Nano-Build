#ifndef _FRM_RC_H_
#define _FRM_RC_H_

#include "hevc_enc_rc.h"

typedef struct {
	rc_pic_t rc_pic;
} frm_rc_t;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef AUTO_FRM_SKIP_DROP
void frm_rc_set_auto_skip_param(frm_rc_t *rc, int en_auto_frm_skip,
				int en_auto_frm_drop, int vbv_threshold,
				int qp_threshold,
				int max_continuos_frame_skip_num,
				int max_continuos_frame_drop_num);
#endif
void frm_rc_seq_init(
	// IN
	frm_rc_t *rc,
	int bps, // unit is bits per second
	int buf_size_ms, // HRD buffer size in unit of millisecond
	int frame_rate, int pic_width, int pic_height,
	int intra_period, // 0(only first), 1(all intra),..
	int rc_mode, int gop_size, gop_entry_t gop[MAX_GOP_SIZE],
	int longterm_delta_qp, int rc_initial_qp, int is_first_pic, int gamma,
	int rc_weight_factor
#ifdef CLIP_PIC_DELTA_QP
	,
	int max_delta_qp_minus,
	int max_delta_qp_plus
#endif
); //-1(internally calculate initial QP), 0~51(use this for initial QP)

void frm_rc_pic_init(
	// IN
	frm_rc_t *rc, int is_intra, int min_qp, int max_qp,
#ifdef AUTO_FRM_SKIP_DROP
	int target_rate,
#endif
// OUT
#ifdef CLIP_PIC_DELTA_QP
	int *max_delta_qp_minus, int *max_delta_qp_plus,
#endif

	int curr_long_term, int *out_qp, int *pic_target_bit,
	int *hrd_buf_level,
	int *hrd_buf_size); // when en_roi=1, it's non-ROI QP. otherwise it's
	// frame QP.

void frm_rc_pic_end(
	// IN
	frm_rc_t *rc, int real_pic_bit, int avg_qp, int frame_skip_flag);
int get_temporal_dqp(frm_rc_t *rc, int enc_idx_modulo, int slice_type,
		     int is_LongTerm, int PicQpY);

#ifdef __cplusplus
}
#endif

#endif
