#ifndef _CVI_SCL_H_
#define _CVI_SCL_H_

#include <base_ctx.h>
#include <base_cb.h>
#include <vpss_cb.h>

#define SCL_INTR_SCL_NUM 139
#ifdef  __SOC_MARS__
#define SCL_MAX_INST 4
#define SCL_V_MAX_INST 3	// sc_v1, sc_v2, sc_v3
#else
#define SCL_MAX_INST 3
#define SCL_V_MAX_INST 2	// sc_v1, sc_v2
#endif
#define SCL_D_MAX_INST 1
#define SCL_MIN_WIDTH  32
#define SCL_MAX_WIDTH  2880
#define SCL_MAX_HEIGHT 2880
#define SCL_MAX_DSI_LP 16
#define SCL_MAX_DSI_SP 2
#define SCL_MAX_GOP_OW_INST 8
#ifdef  __SOC_PHOBOS__
#define SCL_MAX_GOP_INST 1
#define SCL_MAX_GOP_FB_INST 1
#else
#define SCL_MAX_GOP_INST 2
#define SCL_MAX_GOP_FB_INST 2
#endif
#define SCL_MAX_COVER_INST 4
#define SCL_DEFAULT_BURST 7
#define SCL_DISP_GOP_BURST 15
#define SCL_DISP_GAMMA_NODE 65
#define SCL_MAX_RGNEX 32

#define IS_YUV_FMT(x) \
	((x == SCL_FMT_YUV420) || (x == SCL_FMT_YUV422) || \
	 (x == SCL_FMT_Y_ONLY) || (x >= SCL_FMT_NV12))
#define IS_PACKED_FMT(x) \
	((x == SCL_FMT_RGB_PACKED) || (x == SCL_FMT_BGR_PACKED) || \
	 (x == SCL_FMT_YVYU) || (x == SCL_FMT_YUYV) || \
	 (x == SCL_FMT_VYUY) || (x == SCL_FMT_UYVY))

#define TILE_GUARD_PIXEL 32

#define SCL_TILE_LEFT   (1 << 0)
#define SCL_TILE_RIGHT  (1 << 1)
#define SCL_TILE_BOTH   (SCL_TILE_LEFT | SCL_TILE_RIGHT)

#define TILE_ON_IMG

// img_in, odma
enum sclr_sb_mode {
	SCLR_SB_DISABLE = 0,
	SCLR_SB_FREE_RUN = 1,
	SCLR_SB_FRAME_BASE = 2,
	SCLR_SB_MODE_MAX
};

enum sclr_sb_size {
	SCLR_SB_64_LINE = 0, // img_in, odma
	SCLR_SB_128_LINE = 1, // img_in, odma
	SCLR_SB_192_LINE = 2, // img_in
	SCLR_SB_256_LINE = 3, // img_in
	SCLR_SB_SIZE_MAX
};

struct sclr_size {
	u16 w;
	u16 h;
};

struct sclr_point {
	u16 x;
	u16 y;
};

struct sclr_rect {
	u16 x;
	u16 y;
	u16 w;
	u16 h;
};

struct sclr_status {
	u8 crop_idle : 1;
	u8 hscale_idle : 1;
	u8 vscale_idle : 1;
	u8 gop_idle : 1;
	u8 wdma_idle : 1;
	u8 rsv : 2;
};

enum sclr_img_in {
	SCL_IMG_V,  // for video-encoder
	SCL_IMG_D,  // for display
	SCL_IMG_MAX,
};

enum sclr_input {
	SCL_INPUT_ISP,
	SCL_INPUT_DWA,
	SCL_INPUT_MEM,
	SCL_INPUT_MAX,
};

enum sclr_gop {
	SCL_GOP_SCD,
	SCL_GOP_SCV1,
	SCL_GOP_SCV2,
	SCL_GOP_SCV3,
	SCL_GOP_DISP,
	SCL_GOP_MAX,
};

enum sclr_scaling_coef {
	SCL_COEF_BICUBIC,
	SCL_COEF_BILINEAR,
	SCL_COEF_NEAREST,
	SCL_COEF_Z2,
	SCL_COEF_Z3,
	SCL_COEF_DOWNSCALE_SMOOTH,
	SCL_COEF_USER,
	SCL_COEF_MAX
};

enum sclr_cir_mode {
	SCL_CIR_EMPTY,
	SCL_CIR_SHAPE = 2,
	SCL_CIR_LINE,
	SCL_CIR_DISABLE,
	SCL_CIR_MAX,
};

struct sclr_csc_matrix {
	u16 coef[3][3];
	u8 sub[3];
	u8 add[3];
};

struct sclr_quant_formula {
	u16 sc_frac[3];
	u8 sub[3];
	u16 sub_frac[3];
};

union sclr_intr {
	struct {
		u32 disp_frame_start : 1; //0
		u32 disp_frame_end : 1;
		u32 img_in_d_frame_start : 1; //2
		u32 img_in_d_frame_end : 1;
		u32 img_in_v_frame_start : 1; //4
		u32 img_in_v_frame_end : 1;
		u32 scl0_frame_end : 1; //6
		u32 scl1_frame_end : 1;
		u32 scl2_frame_end : 1;
		u32 scl3_frame_end : 1;
		u32 prog_too_late : 1; //10
		u32 scl0_line_target_hit : 1; //11
		u32 scl1_line_target_hit : 1;
		u32 scl2_line_target_hit : 1;
		u32 scl3_line_target_hit : 1;
		u32 scl0_cycle_line_target_hit : 1; //15
		u32 scl1_cycle_line_target_hit : 1;
		u32 scl2_cycle_line_target_hit : 1;
		u32 scl3_cycle_line_target_hit : 1;
		u32 i80_frame_end : 1; //19
		u32 disp_tgen_lite : 1; //20
		u32 osd_cmp_frame_end : 1; //21
		u32 resv_5 : 5;
		u32 cmdq : 1; //27
		u32 cmdq_start : 1;
		u32 cmdq_end : 1;
		u32 cmdq_lint_hit : 1;
		u32 cmdq_cycle_line_hit : 1;
	} b;
	u32 raw;
};

enum sclr_format {
	SCL_FMT_YUV420,
	SCL_FMT_YUV422,
	SCL_FMT_RGB_PLANAR,
	SCL_FMT_BGR_PACKED, // B lsb
	SCL_FMT_RGB_PACKED, // R lsb
	SCL_FMT_Y_ONLY,
	SCL_FMT_BF16, // odma only
	SCL_FMT_NV12 = 8,
	SCL_FMT_NV21,
	SCL_FMT_YUV422SP1,
	SCL_FMT_YUV422SP2,
	SCL_FMT_YVYU,
	SCL_FMT_YUYV,
	SCL_FMT_VYUY,
	SCL_FMT_UYVY,
	SCL_FMT_MAX
};

enum sclr_out_mode {
	SCL_OUT_CSC,
	SCL_OUT_QUANT,
	SCL_OUT_HSV,
	SCL_OUT_QUANT_BF16,
	SCL_OUT_DISABLE
};

enum sclr_csc {
	SCL_CSC_NONE,
	SCL_CSC_601_LIMIT_YUV2RGB,
	SCL_CSC_601_FULL_YUV2RGB,
	SCL_CSC_709_LIMIT_YUV2RGB,
	SCL_CSC_709_FULL_YUV2RGB,
	SCL_CSC_601_LIMIT_RGB2YUV,
	SCL_CSC_601_FULL_RGB2YUV,
	SCL_CSC_709_LIMIT_RGB2YUV,
	SCL_CSC_709_FULL_RGB2YUV,
	SCL_CSC_MAX,
};


enum sclr_hsv_rounding {
	SCL_HSV_ROUNDING_AWAY_FROM_ZERO = 0,
	SCL_HSV_ROUNDING_TOWARD_ZERO,
	SCL_HSV_ROUNDING_MAX,
};

enum sclr_quant_rounding {
	SCL_QUANT_ROUNDING_TO_EVEN = 0,
	SCL_QUANT_ROUNDING_AWAY_FROM_ZERO = 1,
	SCL_QUANT_ROUNDING_TRUNCATE = 2,
	SCL_QUANT_ROUNDING_MAX,
};

struct sclr_csc_cfg {
	enum sclr_out_mode mode;
	bool work_on_border;
	union {
		enum sclr_csc csc_type;
		struct sclr_quant_formula quant_form;
	};
	union {
		enum sclr_hsv_rounding hsv_round;
		enum sclr_quant_rounding quant_round;
	};

	enum sclr_quant_border_type {
		SCL_QUANT_BORDER_TYPE_255 = 0, // 0 ~ 255
		SCL_QUANT_BORDER_TYPE_127, // -128 ~ 127
		SCL_QUANT_BORDER_TYPE_MAX,
	} quant_border_type;
	enum sclr_quant_gain_mode {
		SCL_QUANT_GAIN_MODE_1 = 0, // max 8191/8192
		SCL_QUANT_GAIN_MODE_2, // max 1 + 4095/4096
		SCL_QUANT_GAIN_MAX,
	} quant_gain_mode;
};

enum sclr_flip_mode {
	SCL_FLIP_NO,
	SCL_FLIP_HFLIP,
	SCL_FLIP_VFLIP,
	SCL_FLIP_HVFLIP,
	SCL_FLIP_MAX
};

enum sclr_gop_format {
	SCL_GOP_FMT_ARGB8888,
	SCL_GOP_FMT_ARGB4444,
	SCL_GOP_FMT_ARGB1555,
	SCL_GOP_FMT_256LUT,
	SCL_GOP_FMT_16LUT,
	SCL_GOP_FMT_FONT,
	SCL_GOP_FMT_MAX
};

enum sclr_disp_drop_mode {
	SCL_DISP_DROP_MODE_DITHER = 1,
	SCL_DISP_DROP_MODE_ROUNDING = 2,
	SCL_DISP_DROP_MODE_DROP = 3,
	SCL_DISP_DROP_MODE_MAX,
};

enum sclr_img_trig_src {
	SCL_IMG_TRIG_SRC_SW = 0,
	SCL_IMG_TRIG_SRC_DISP,
	SCL_IMG_TRIG_SRC_ISP,
	SCL_IMG_TRIG_SRC_MAX,
};

union sclr_top_cfg_01 {
	struct {
		u32 sc_d_enable   : 1;    // 0
		u32 sc_v1_enable  : 1;    // 1
		u32 sc_v2_enable  : 1;    // 2
		u32 sc_v3_enable  : 1;    // 3
		u32 disp_enable   : 1;    // 4: no use anymore
		u32 resv_5	  : 3;    // 5-7
		u32 disp_from_sc  : 1;    // 8: 0(DRAM), 1(SCL_D)
		u32 sc_d_src	  : 1;    // 9: 0(IMG_D), 1(IMG_V)
		u32 sc_rot_sw_rt  : 1;    // 10
		u32 resv_11	  : 1;    // 11
		u32 sc_debug_en   : 1;    // 12
		u32 resv_13	  : 3;    // 13-15
		u32 qos_en        : 8;    // 16
	} b;
	u32 raw;
};

/*
 * reg_sb_wr_ctrl0:
 *   slice buffer push signal (sb_push_128t[0]) to VENC config
 *   0 : sc_d
 *   1 : sc_v1
 *   2 : sc_v2
 *   3 : sc_v3
 *
 * reg_sb_wr_ctrl1:
 *   slice buffer push signal (sb_push_128t[1]) to VENC config
 *   0 : sc_d
 *   1 : sc_v1
 *   2 : sc_v2
 *   3 : sc_v3
 *
 * reg_sb_rd_ctrl:
 *   slice buffer pop signal (sb_pop_128t) to LDC config
 *   0 : img_in_d
 *   1 : img_in_v
 */
struct sclr_top_sb_cfg {
	u8 sb_wr_ctrl0;
	u8 sb_wr_ctrl1;
	u8 sb_rd_ctrl;
};

struct sclr_top_cfg {
	bool ip_trig_src;    // 0(IMG_V), 1(IMG_D)
	bool force_clk_enable;
	bool sclr_enable[SCL_MAX_INST];	// 0(scl_d), 1(sc_v1), 2(sc_v2), 3(sc_v3)
	bool disp_enable;
	bool disp_from_sc;   // 0(DRAM), 1(SCL_D)
	bool sclr_d_src;     // 0(IMG_D), 1(IMG_V)
	enum sclr_img_trig_src img_in_d_trig_src;
	enum sclr_img_trig_src img_in_v_trig_src;
};

struct sclr_bld_cfg {
	union {
		struct {
			u32 enable	 : 1;
			u32 fix_alpha	 : 1; // 0 : gradient alpha with x position (stiching)
					      // 1 : fix alpha value , blending setting in alpha_factor (blending)
			u32 blend_y	 : 1; // blending on Y/R only
			u32 y2r_enable	 : 1; // apply y2r csc on output
			u32 resv_b4	 : 4;
			u32 alpha_factor : 9; // blending factor
		} b;
		u32 raw;
	} ctrl;
	u16 width; // blending image width
};

union sclr_sc_top_vo_mux_sel {
	struct {
		u32 vo_sel_type	: 4;
	} b;
	u32 raw;
};


union sclr_sc_top_vo_mux {
	struct {
		u32 vod_sel0	: 8;
		u32 vod_sel1	: 8;
		u32 vod_sel2	: 8;
		u32 vod_sel3	: 8;
	} b;
	u32 raw;
};

union sclr_rt_cfg {
	struct {
		u32 sc_d_rt	: 1;
		u32 sc_v_rt	: 1;
		u32 sc_rot_rt	: 1;
		u32 img_d_rt	: 1;
		u32 img_v_rt	: 1;
		u32 img_d_sel	: 1;
	} b;
	u32 raw;
};

struct sclr_img_in_sb_cfg {
	u8 sb_mode;
	u8 sb_size;
	u8 sb_nb;
	u8 sb_sw_rptr;
	u8 sb_set_str;
	u8 sb_sw_clr;
};

struct sclr_mem {
	u64 addr0;
	u64 addr1;
	u64 addr2;
	u16 pitch_y;
	u16 pitch_c;
	u16 start_x;
	u16 start_y;
	u16 width;
	u16 height;
};

struct sclr_img_cfg {
	enum sclr_input src;      // 0(ISP), 1(map_cnv if img_d/dwa if img_v), ow(DRAM)
	enum sclr_img_trig_src trig_src;
	bool csc_en;
	enum sclr_csc csc;
	u8 burst;       // 0~15
	enum sclr_format fmt;
	struct sclr_mem mem;
	bool force_clk;
};

union sclr_img_dbg_status {
	struct {
		u32 err_fwr_y   : 1;
		u32 err_fwr_u   : 1;
		u32 err_fwr_v   : 1;
		u32 err_fwr_clr : 1;
		u32 err_erd_y   : 1;
		u32 err_erd_u   : 1;
		u32 err_erd_v   : 1;
		u32 err_erd_clr : 1;
		u32 lb_full_y   : 1;
		u32 lb_full_u   : 1;
		u32 lb_full_v   : 1;
		u32 resv1       : 1;
		u32 lb_empty_y  : 1;
		u32 lb_empty_u  : 1;
		u32 lb_empty_v  : 1;
		u32 resv2       : 1;
		u32 ip_idle     : 1;
		u32 ip_int      : 1;
		u32 ip_clr      : 1;
		u32 ip_int_clr  : 1;
		u32 resv        : 13;
	} b;
	u32 raw;
};

struct sclr_img_checksum_status {
	union{
		struct {
			u32 output_data : 8;
			u32 reserv      : 23;
			u32 enable      : 1;
		} b;
		u32 raw;
	} checksum_base;
	u32 axi_read_data;
};

struct sclr_cir_cfg {
	enum sclr_cir_mode mode;
	u8 line_width;
	struct sclr_point center;
	u16 radius;
	struct sclr_rect rect;
	u8 color_r;
	u8 color_g;
	u8 color_b;
};

struct sclr_border_cfg {
	union {
		struct {
			u32 bd_color_r	: 8;
			u32 bd_color_g	: 8;
			u32 bd_color_b	: 8;
			u32 resv	: 7;
			u32 enable	: 1;
		} b;
		u32 raw;
	} cfg;
	struct sclr_point start;
};

struct sclr_oenc_int {
	union {
		struct {
			u32 go      : 1;
			u32 resv7   : 7;
			u32 done    : 1;
			u32 resv6   : 6;
			u32 intr_clr: 1;
			u32 intr_vec: 16;
		} b;
		u32 raw;
	}go_intr;
};

struct sclr_oenc_cfg {
	enum sclr_gop_format fmt: 4;
	union {
		struct {
			u32 fmt         : 4;
			u32 resv4       : 4;
			u32 alpha_zero  : 1;
			u32 resv3       : 3;
			u32 rgb_trunc   : 2;
			u32 alpha_trunc : 2;
			u32 limit_bsz_en: 1;
			u32 limit_bsz_bypass: 1;
			u32 wprot_en    : 1;
			u32 resv11      : 11;
			u32 wdog_en     : 1;
			u32 intr_en     : 1;
		} b;
		u32 raw;
	} cfg;
	struct sclr_size src_picture_size;
	struct sclr_size src_mem_size;
	u16 src_pitch;
	u64 src_adr;
	u32 wprot_laddr;
	u32 wprot_uaddr;
	u64 bso_adr;
	u32 limit_bsz;
	u32 bso_sz; //OSD encoder original data of bso_sz
	struct sclr_size bso_mem_size; //for setting VGOP bitstream size
};

struct sclr_gop_ow_cfg {
	enum sclr_gop_format fmt;
	struct sclr_point start;
	struct sclr_point end;
	u64 addr;
	u16 crop_pixels;
	u16 pitch;
	struct sclr_size mem_size;
	struct sclr_size img_size;
};

struct sclr_gop_fb_cfg {
	union {
		struct {
			u32 width	: 7;
			u32 resv_b7	: 1;
			u32 pix_thr	: 5;
			u32 sample_rate	: 2;
			u32 resv_b15	: 1;
			u32 fb_num	: 5;
			u32 resv_b21	: 3;
			u32 attach_ow	: 3;
			u32 resv_b27	: 1;
			u32 enable	: 1;
		} b;
		u32 raw;
	} fb_ctrl;
	u32 init_st;
};

struct sclr_gop_odec_cfg {
	union {
		struct {
			u32 odec_en		: 1;
			u32 odec_int_en		: 1;
			u32 odec_int_clr	: 1;
			u32 odec_wdt_en		: 1;
			u32 odec_dbg_ridx	: 4;
			u32 odec_done		: 1;
			u32 resv_b9		: 3;
			u32 odec_attached_idx	: 3;
			u32 resv_b15		: 1;
			u32 odec_wdt_fdiv_bit	: 3;
			u32 resv_b19		: 5;
			u32 odec_int_vec	: 8;
		} b;
		u32 raw;
	} odec_ctrl;
	u32 odec_debug;
	u64 bso_addr;
	u32 bso_sz; //OSD encoder original data of bso_sz
};

struct sclr_cover_cfg {
    union {
        struct {
            u32 x       : 16;
            u32 y       : 15;
            u32 enable  : 1;
        }b;
        u32 raw;
    } start;
    struct sclr_size img_size;
    union {
        struct {
            u32 cover_color_r   : 8;
            u32 cover_color_g   : 8;
            u32 cover_color_b   : 8;
            u32 resv            : 8;
        } b;
        u32 raw;
    } color;
};

struct sclr_gop_cfg {
	union {
		struct {
			u32 ow0_en : 1;
			u32 ow1_en : 1;
			u32 ow2_en : 1;
			u32 ow3_en : 1;
			u32 ow4_en : 1;
			u32 ow5_en : 1;
			u32 ow6_en : 1;
			u32 ow7_en : 1;
			u32 hscl_en: 1;
			u32 vscl_en: 1;
			u32 colorkey_en : 1;
			u32 resv   : 1;
			u32 burst  : 4;
			u32 resv_b16 : 15;
			u32 sw_rst : 1;
		} b;
		u32 raw;
	} gop_ctrl;
	u32 colorkey;       // RGB888
	u16 font_fg_color;  // ARGB4444
	u16 font_bg_color;  // ARGB4444
	struct sclr_gop_ow_cfg ow_cfg[SCL_MAX_GOP_OW_INST];
	union {
		struct {
			u32 hi_thr	: 6;
			u32 resv_b6	: 2;
			u32 lo_thr	: 6;
			u32 resv_b14	: 2;
			u32 fb_init	: 1;
			u32 lo_thr_inv	: 1;
			u32 resv_b18	: 2;
			u32 detect_fnum	: 6;
		} b;
		u32 raw;
	} fb_ctrl;
	struct sclr_gop_fb_cfg fb_cfg[SCL_MAX_GOP_FB_INST];
	struct sclr_gop_odec_cfg odec_cfg;
};

struct sclr_odma_sb_cfg {
	u8 sb_mode;
	u8 sb_size;
	u8 sb_nb;
	u8 sb_full_nb;
	u8 sb_sw_wptr;
	u8 sb_set_str;
	u8 sb_sw_clr;
};

struct sclr_odma_cfg {
	enum sclr_flip_mode flip;
	bool burst;     // burst(0: 8, 1:16)
	enum sclr_format fmt;
	struct sclr_mem mem;
	struct sclr_csc_cfg csc_cfg;
	struct sclr_size frame_size;
};

union sclr_odma_dbg_status {
	struct {
		u32 axi_status  : 4;
		u32 v_buf_empty : 1;
		u32 v_buf_full  : 1;
		u32 u_buf_empty : 1;
		u32 u_buf_full  : 1;
		u32 y_buf_empty : 1;
		u32 y_buf_full  : 1;
		u32 v_axi_active: 1;
		u32 u_axi_active: 1;
		u32 y_axi_active: 1;
		u32 axi_active  : 1;
		u32 resv        : 18;
	} b;
	u32 raw;
};

struct sclr_tile_cfg {
	struct sclr_size src;
	struct sclr_size out;
	//u16 src_l_offset;
	u16 src_l_width;
	u16 src_r_offset;
	u16 src_r_width;
	u16 r_ini_phase;
	u16 out_l_width;
	u16 dma_l_x;
	u16 dma_l_y;
	u16 dma_r_x;
	u16 dma_r_y;
	u16 dma_l_width;
	bool border_enable;
};

struct sclr_scale_cfg {
	struct sclr_size src;
	struct sclr_rect crop;
	struct sclr_size dst;
	struct sclr_tile_cfg tile;
	u32 mir_enable  : 1;
	u32 cb_enable   : 1;
	u32 tile_enable : 1;
};

struct sclr_scale_2tap_cfg {
	u32 src_wd;
	u32 src_ht;
	u32 dst_wd;
	u32 dst_ht;

	u32 area_fast;
	u32 scale_x;
	u32 scale_y;
	u32 h_nor;
	u32 v_nor;
	u32 h_ph;
	u32 v_ph;
};

struct sclr_core_cfg {
	union {
		struct {
			u32 resv_b0	: 1;
			u32 sc_bypass	: 1;
			u32 gop_bypass	: 1;
			u32 resv_b3	: 1;
			u32 dbg_en	: 1;
			u32 cir_bypass	: 1;
			u32 odma_bypass	: 1;
			u32 resv_b7	: 23;
			u32 pwr_test	: 1;
			u32 force_clk	: 1;
		};
		u32 raw;
	};
	struct sclr_scale_cfg sc;
	struct sclr_tile_cfg tile;
	struct sclr_cover_cfg cover_cfg[SCL_MAX_COVER_INST];
	enum sclr_scaling_coef coef;
};

struct sclr_core_checksum_status {
	union{
		struct {
			u32 data_in_from_img_in : 8;
			u32 data_out            : 8;
			u32 reserv              : 15;
			u32 enable              : 1;
		} b;
		u32 raw;
	} checksum_base;
	u32 axi_read_gop0_data;
	u32 axi_read_gop1_data;
	u32 axi_write_data;
};

enum sclr_disp_pat_color {
	SCL_PAT_COLOR_WHITE,
	SCL_PAT_COLOR_RED,
	SCL_PAT_COLOR_GREEN,
	SCL_PAT_COLOR_BLUE,
	SCL_PAT_COLOR_CYAN,
	SCL_PAT_COLOR_MAGENTA,
	SCL_PAT_COLOR_YELLOW,
	SCL_PAT_COLOR_BAR,
	SCL_PAT_COLOR_USR,
	SCL_PAT_COLOR_MAX
};

enum sclr_disp_pat_type {
	SCL_PAT_TYPE_FULL,
	SCL_PAT_TYPE_H_GRAD,
	SCL_PAT_TYPE_V_GRAD,
	SCL_PAT_TYPE_AUTO,
	SCL_PAT_TYPE_SNOW,
	SCL_PAT_TYPE_OFF,
	SCL_PAT_TYPE_MAX
};

struct sclr_disp_cfg {
	bool disp_from_sc;  // 0(DRAM), 1(scaler_d)
	bool cache_mode;
	bool sync_ext;
	bool tgen_en;
	enum sclr_format fmt;
	enum sclr_csc in_csc;
	enum sclr_csc out_csc;
	u8 burst;       // 0~15
	u8 out_bit;     // 6/8/10-bit
	enum sclr_disp_drop_mode drop_mode;
	struct sclr_mem mem;
	struct sclr_gop_cfg gop_cfg;
};

/**
 * @ vsync_pol: vsync polarity
 * @ hsync_pol: hsync polarity
 * @ vtotal: total line of each frame, should sub 1,
 *           start line is included, end line isn't included
 * @ htotal: total pixel of each line, should sub 1,
 *           start pixel is included, end pixel isn't included
 * @ vsync_start: start line of vsync
 * @ vsync_end: end line of vsync, should sub 1
 * @ vfde_start: start line of actually video data
 * @ vfde_end: end line of actually video data, should sub 1
 * @ vmde_start: equal to vfde_start
 * @ vmde_end: equal to vfde_end
 * @ hsync_start: start pixel of hsync
 * @ hsync_end: end pixel of hsync, should sub 1
 * @ hfde_start: start pixel of actually video data in each line
 * @ hfde_end: end pixel of actually video data in each line, should sub 1
 * @ hmde_start: equal to hfde_start
 * @ hmde_end: equal to hfde_end
 */
struct sclr_disp_timing {
	bool vsync_pol;
	bool hsync_pol;
	u16 vtotal;
	u16 htotal;
	u16 vsync_start;
	u16 vsync_end;
	u16 vfde_start;
	u16 vfde_end;
	u16 vmde_start;
	u16 vmde_end;
	u16 hsync_start;
	u16 hsync_end;
	u16 hfde_start;
	u16 hfde_end;
	u16 hmde_start;
	u16 hmde_end;
};

union sclr_disp_dbg_status {
	struct {
		u32 bw_fail     : 1;
		u32 bw_fail_clr : 1;
		u32 osd_bw_fail : 1;
		u32 osd_bw_fail_clr : 1;
		u32 err_fwr_y   : 1;
		u32 err_fwr_u   : 1;
		u32 err_fwr_v   : 1;
		u32 err_fwr_clr : 1;
		u32 err_erd_y   : 1;
		u32 err_erd_u   : 1;
		u32 err_erd_v   : 1;
		u32 err_erd_clr : 1;
		u32 lb_full_y   : 1;
		u32 lb_full_u   : 1;
		u32 lb_full_v   : 1;
		u32 resv1       : 1;
		u32 lb_empty_y  : 1;
		u32 lb_empty_u  : 1;
		u32 lb_empty_v  : 1;
		u32 resv2       : 13;
	} b;
	u32 raw;
};

struct sclr_disp_checksum_status {
	union{
		struct {
			u32 data_in_from_sc_d   : 8;
			u32 data_out            : 8;
			u32 reserv              : 15;
			u32 enable              : 1;
		} b;
		u32 raw;
	} checksum_base;
	u32 axi_read_from_dram;
	u32 axi_read_from_gop;
};

struct sclr_ctrl_cfg {
	enum sclr_img_in img_inst;
	enum sclr_input input;
	enum sclr_format src_fmt;
	enum sclr_csc src_csc;
	struct sclr_size src;
	struct sclr_rect src_crop;
	u64 src_addr0;
	u64 src_addr1;
	u64 src_addr2;

	struct {
		u8 inst;
		enum sclr_format fmt;
		enum sclr_csc csc;
		struct sclr_size frame;
		struct sclr_size window;
		struct sclr_point offset;
		u64 addr0;
		u64 addr1;
		u64 addr2;
	} dst[4];
};

struct sclr_rgnex_cfg {
	enum sclr_format fmt;
	enum sclr_csc src_csc;
	enum sclr_csc dst_csc;
	struct sclr_rect rgnex_rect;
	u32 bytesperline[2];
	u64 addr0;
	u64 addr1;
	u64 addr2;
	struct sclr_gop_cfg gop_cfg;
};

struct sclr_rgnex_list {
	struct sclr_rgnex_cfg cfg[SCL_MAX_RGNEX];
	u8 num_of_cfg;
};

/**
 * @ out_bit: 0(6-bit), 1(8-bit), others(10-bit)
 * @ vesa_mode: 0(JEIDA), 1(VESA)
 * @ dual_ch: dual link
 * @ vs_out_en: vs output enable
 * @ hs_out_en: hs output enable
 * @ hs_blk_en: vertical blanking hs output enable
 * @ ml_swap: lvdstx hs data msb/lsb swap
 * @ ctrl_rev: serializer 0(msb first), 1(lsb first)
 * @ oe_swap: lvdstx even/odd link swap
 * @ en: lvdstx enable
 */
union sclr_lvdstx {
	struct {
		u32 out_bit	: 2;
		u32 vesa_mode	: 1;
		u32 dual_ch	: 1;
		u32 vs_out_en	: 1;
		u32 hs_out_en	: 1;
		u32 hs_blk_en	: 1;
		u32 resv_1	: 1;
		u32 ml_swap	: 1;
		u32 ctrl_rev	: 1;
		u32 oe_swap	: 1;
		u32 en		: 1;
		u32 resv	: 20;
	} b;
	u32 raw;
};

/**
 * @fmt_sel: [0] clk select
 *		0: bt clock 2x of disp clock
 *		1: bt clock 2x of disp clock
 *	     [1] sync signal index
 *		0: with sync pattern
 *		1: without sync pattern
 * @hde_gate: gate output hde with vde
 * @data_seq: fmt_sel[0] = 0
 *		00: Cb0Y0Cr0Y1
 *		01: Cr0Y0Cb0Y1
 *		10: Y0Cb0Y1Cr0
 *		11: Y0Cr0Y1Cb0
 *	      fmt_sel[0] = 1
 *		0: Cb0Cr0
 *		1: Cr0Cb0
 * @clk_inv: clock rising edge at middle of data
 * @vs_inv: vs low active
 * @hs_inv: hs low active
 */
union sclr_bt_enc {
	struct {
		u32 fmt_sel	: 2;
		u32 resv_1	: 1;
		u32 hde_gate	: 1;
		u32 data_seq	: 2;
		u32 resv_2	: 2;
		u32 clk_inv	: 1;
		u32 hs_inv	: 1;
		u32 vs_inv	: 1;
	} b;
	u32 raw;
};

/**
 * @ sav_vld: sync pattern for start of valid data
 * @ sav_blk: sync pattern for start of blanking data
 * @ eav_vld: sync pattern for end of valid data
 * @ eav_blk: sync pattern for end of blanking data
 */
union sclr_bt_sync_code {
	struct {
		u8 sav_vld;
		u8 sav_blk;
		u8 eav_vld;
		u8 eav_blk;
	} b;
	u32 raw;
};

enum sclr_vo_sel {
	SCLR_VO_SEL_DISABLE,
	SCLR_VO_SEL_RGB,
	SCLR_VO_SEL_SW,
	SCLR_VO_SEL_I80,
	SCLR_VO_SEL_BT601,
	SCLR_VO_SEL_BT656,
	SCLR_VO_SEL_BT1120,
	SCLR_VO_SEL_BT1120R,
	SCLR_VO_SEL_SERIAL_RGB,
	SCLR_VO_SEL_HW_MCU,
	SCLR_VO_SEL_MAX,
};

enum sclr_vo_intf {
	SCLR_VO_INTF_DISABLE,
	SCLR_VO_INTF_SW,
	SCLR_VO_INTF_I80,
	SCLR_VO_INTF_HW_MCU,
	SCLR_VO_INTF_BT601,
	SCLR_VO_INTF_BT656,
	SCLR_VO_INTF_BT1120,
	SCLR_VO_INTF_MIPI,
	SCLR_VO_INTF_LVDS,
	SCLR_VO_INTF_MAX,
};

enum sclr_dsi_mode {
	SCLR_DSI_MODE_IDLE = 0,
	SCLR_DSI_MODE_SPKT = 1,
	SCLR_DSI_MODE_ESC = 2,
	SCLR_DSI_MODE_HS = 4,
	SCLR_DSI_MODE_UNKNOWN,
	SCLR_DSI_MODE_MAX = SCLR_DSI_MODE_UNKNOWN,
};

enum sclr_dsi_fmt {
	SCLR_DSI_FMT_RGB888 = 0,
	SCLR_DSI_FMT_RGB666,
	SCLR_DSI_FMT_RGB565,
	SCLR_DSI_FMT_RGB101010,
	SCLR_DSI_FMT_MAX,
};

struct sclr_privacy_map_cfg {
	u32 base;
	u16 axi_burst;
	u8 no_mask_idx;
	u16 alpha_factor;
};

struct sclr_privacy_cfg {
	union {
		struct {
			u32 enable	: 1;
			u32 mode	: 1; // 0 : grid mode, 1 : pixel mode
			u32 force_alpha	: 1; // 0 : depend on no_mask_idx, 1: force alpha
			u32 mask_rgb332	: 1; // 0 : y, 1: rgb332
			u32 blend_y	: 1; // blending on Y/R only
			u32 y2r_enable	: 1; // apply y2r csc on output
			u32 grid_size	: 1; // 0 : 8x8, 1: 16x16
			u32 fit_picture	: 1; // 0: customized size, 1: same size as sc_core
		} b;
		u32 raw;
	} cfg;
	struct sclr_point start;
	struct sclr_point end;
	struct sclr_privacy_map_cfg map_cfg;
};

/**
 * @ enable: gamma enbale
 * @ pre_osd: 0:osd-->gamma 1:gamma-->osd
 * @ r: LUT gamma red
 * @ g: LUT gamma green
 * @ b: LUT gamma blue
 */
struct sclr_disp_gamma_attr {
	bool enable;
	bool pre_osd;
	__u8  table[SCL_DISP_GAMMA_NODE];
};

void sclr_set_base_addr(void *base);
void sclr_top_set_cfg(struct sclr_top_cfg *cfg);
struct sclr_top_cfg *sclr_top_get_cfg(void);
void sclr_rt_set_cfg(union sclr_rt_cfg cfg);
union sclr_rt_cfg sclr_rt_get_cfg(void);
void sclr_top_reg_done(void);
void sclr_top_reg_force_up(void);
u8 sclr_top_pg_late_get_bus(void);
void sclr_top_pg_late_clr(void);
void sclr_top_set_shd_reg(u32 val);
u32 sclr_top_get_shd_reg(void);
void sclr_top_bld_set_cfg(struct sclr_bld_cfg *cfg);
void sclr_top_get_sb_default(struct sclr_top_sb_cfg *cfg);
void sclr_top_set_sb(struct sclr_top_sb_cfg *cfg);
void sclr_top_vo_mux_sel(int vo_sel, int vo_mux);
void sclr_disp_mux_sel(enum sclr_vo_sel sel);
void sclr_disp_bt_en(u8 vo_intf);
int hw_mcu_cmd_send(void *cmds, int size);
void sclr_disp_set_mcu_en(u8 mode);

void sclr_init(u8 inst);
void sclr_reg_shadow_sel(u8 inst, bool read_shadow);
void sclr_sc_reset(u8 inst);
void sclr_reg_force_up(u8 inst);
void sclr_set_cfg(u8 inst, bool sc_bypass, bool gop_bypass,
		  bool cir_bypass, bool odma_bypass);
struct sclr_core_cfg *sclr_get_cfg(u8 inst);
void sclr_set_input_size(u8 inst, struct sclr_size src_rect, bool update);
void sclr_set_crop(u8 inst, struct sclr_rect crop_rect, bool update);
void sclr_set_output_size(u8 inst, struct sclr_size rect);
void sclr_set_scale_mode(u8 inst, bool mir_enable, bool cb_enable, bool update);
void sclr_set_scale_mir(u8 inst, bool enable);
void sclr_set_scale_phase(u8 inst, u16 h_ph, u16 v_ph);
bool sclr_check_factor_over4(u8 inst);
void sclr_set_scale(u8 inst);
struct sclr_status sclr_get_status(u8 inst);
void sclr_update_coef(u8 inst, enum sclr_scaling_coef coef, void *param);
void sclr_set_opencv_scale(u8 inst);
void sclr_read_2tap_nor(u8 inst, u16 *resize_hnor, u16 *resize_vnor);

void sclr_img_reg_shadow_sel(u8 inst, bool read_shadow);
void sclr_img_set_cfg(u8 inst, struct sclr_img_cfg *cfg);
struct sclr_img_cfg *sclr_img_get_cfg(u8 inst);
void sclr_img_reg_force_up(u8 inst);
void sclr_img_reset(u8 inst);
void sclr_img_start(u8 inst);
void sclr_img_set_fmt(u8 inst, enum sclr_format fmt);
void sclr_img_set_mem(u8 inst, struct sclr_mem *mem, bool update);
void sclr_img_set_addr(u8 inst, u64 addr0, u64 addr1, u64 addr2);
void sclr_img_csc_en(u8 inst, bool enable);
void sclr_img_set_csc(u8 inst, struct sclr_csc_matrix *cfg);
union sclr_img_dbg_status sclr_img_get_dbg_status(u8 inst, bool clr);
void sclr_img_checksum_en(u8 inst, bool enable);
void sclr_img_get_checksum_status(u8 inst, struct sclr_img_checksum_status *status);
void sclr_oenc_set_cfg(struct sclr_oenc_cfg *oenc_cfg);
struct sclr_oenc_cfg *sclr_oenc_get_cfg(void);
void sclr_cover_set_cfg(u8 inst, u8 cover_w_inst, struct sclr_cover_cfg *cover_cfg);
void sclr_img_set_trig(u8 inst, enum sclr_img_trig_src trig_src);
void sclr_img_get_sb_default(struct sclr_img_in_sb_cfg *cfg);
void sclr_img_set_sb(u8 inst, struct sclr_img_in_sb_cfg *cfg);
void sclr_img_set_dwa_to_sclr_sb(u8 inst, u8 sb_mode, u8 sb_size, u8 sb_nb);
void sclr_cir_set_cfg(u8 inst, struct sclr_cir_cfg *cfg);
void sclr_odma_set_cfg(u8 inst, struct sclr_odma_cfg *cfg);
struct sclr_odma_cfg *sclr_odma_get_cfg(u8 inst);
void sclr_odma_set_fmt(u8 inst, enum sclr_format fmt);
void sclr_odma_set_mem(u8 inst, struct sclr_mem *mem);
void sclr_odma_set_addr(u8 inst, u64 addr0, u64 addr1, u64 addr2);
union sclr_odma_dbg_status sclr_odma_get_dbg_status(u8 inst);
void sclr_odma_get_sb_default(struct sclr_odma_sb_cfg *cfg);
void sclr_odma_set_sb(u8 inst, struct sclr_odma_sb_cfg *cfg);
void sclr_odma_clear_sb(u8 inst);
void sclr_set_out_mode(u8 inst, enum sclr_out_mode mode);
void sclr_set_quant(u8 inst, struct sclr_quant_formula *cfg);
void sclr_set_quant_drop_mode(u8 inst, enum sclr_quant_rounding mode);
void sclr_border_set_cfg(u8 inst, struct sclr_border_cfg *cfg);
struct sclr_border_cfg *sclr_border_get_cfg(u8 inst);
void sclr_set_csc(u8 inst, struct sclr_csc_matrix *cfg);
void sclr_get_csc(u8 inst, struct sclr_csc_matrix *cfg);
void sclr_core_checksum_en(u8 inst, bool enable);
void sclr_core_get_checksum_status(u8 inst, struct sclr_core_checksum_status *status);
void sclr_intr_ctrl(union sclr_intr intr_mask);
union sclr_intr sclr_get_intr_mask(void);
void sclr_set_intr_mask(union sclr_intr intr_mask);
void sclr_intr_clr(union sclr_intr intr_mask);
union sclr_intr sclr_intr_status(void);
void sclr_cmdq_intr_clr(u8 intr_mask);
u8 sclr_cmdq_intr_status(void);

void sclr_disp_reg_shadow_sel(bool read_shadow);
void sclr_disp_set_cfg(struct sclr_disp_cfg *cfg);
struct sclr_disp_cfg *sclr_disp_get_cfg(void);
void sclr_disp_set_timing(struct sclr_disp_timing *timing);
struct sclr_disp_timing *sclr_disp_get_timing(void);
void sclr_disp_get_hw_timing(struct sclr_disp_timing *timing);
int sclr_disp_set_rect(struct sclr_rect rect);
void sclr_disp_set_mem(struct sclr_mem *mem);
void sclr_disp_set_addr(u64 addr0, u64 addr1, u64 addr2);
void sclr_disp_set_csc(struct sclr_csc_matrix *cfg);
void sclr_disp_set_in_csc(enum sclr_csc csc);
void sclr_disp_set_out_csc(enum sclr_csc csc);
void sclr_disp_set_pattern(enum sclr_disp_pat_type type,
			   enum sclr_disp_pat_color color, const u16 *rgb);
void sclr_disp_set_frame_bgcolor(u16 r, u16 g, u16 b);
void sclr_disp_set_window_bgcolor(u16 r, u16 g, u16 b);
void sclr_disp_enable_window_bgcolor(bool enable);
bool sclr_disp_tgen_enable(bool enable);
bool sclr_disp_check_i80_enable(void);
bool sclr_disp_check_tgen_enable(void);
union sclr_disp_dbg_status sclr_disp_get_dbg_status(bool clr);
void sclr_disp_gamma_ctrl(bool enable, bool pre_osd);
void sclr_disp_gamma_lut_update(const u8 *b, const u8 *g, const u8 *r);
void sclr_disp_gamma_lut_read(struct sclr_disp_gamma_attr *gamma_attr);
void sclr_disp_reg_force_up(void);
void sclr_lvdstx_set(union sclr_lvdstx cfg);
void sclr_lvdstx_get(union sclr_lvdstx *cfg);
void sclr_bt_set(union sclr_bt_enc enc, union sclr_bt_sync_code sync);
void sclr_bt_get(union sclr_bt_enc *enc, union sclr_bt_sync_code *sync);
enum sclr_vo_sel sclr_disp_mux_get(void);
void sclr_disp_set_intf(enum sclr_vo_intf intf);
void sclr_disp_timing_setup_from_reg(void);
void sclr_disp_cfg_setup_from_reg(void);

void sclr_disp_checksum_en(bool enable);
void sclr_disp_get_checksum_status(struct sclr_disp_checksum_status *status);
enum sclr_dsi_mode sclr_dsi_get_mode(void);
int sclr_dsi_set_mode(enum sclr_dsi_mode mode);
void sclr_dsi_clr_mode(void);
int sclr_dsi_chk_mode_done(enum sclr_dsi_mode mode);
int sclr_dsi_long_packet(u8 di, const u8 *data, u8 count, bool sw_mode);
int sclr_dsi_long_packet_raw(const u8 *data, u8 count);
int sclr_dsi_short_packet(u8 di, const u8 *data, u8 count, bool sw_mode);
int sclr_dsi_dcs_write_buffer(u8 di, const void *data, size_t len, bool sw_mode);
int sclr_dsi_dcs_read_buffer(u8 di, const u16 data_param, u8 *data, size_t len);
int sclr_dsi_config(u8 lane_num, enum sclr_dsi_fmt fmt, u16 width);
void sclr_i80_sw_mode(bool enable);
void sclr_i80_packet(u32 cmd);
void sclr_i80_run(void);

void sclr_gop_set_cfg(u8 inst, u8 layer, struct sclr_gop_cfg *cfg, bool update);
struct sclr_gop_cfg *sclr_gop_get_cfg(u8 inst, u8 layer);
void sclr_gop_ow_set_cfg(u8 inst, u8 layer, u8 ow_inst, struct sclr_gop_ow_cfg *cfg, bool update);
int sclr_gop_setup_256LUT(u8 inst, u8 layer, u16 length, u16 *data);
int sclr_gop_update_256LUT(u8 inst, u8 layer, u16 index, u16 data);
int sclr_gop_setup_16LUT(u8 inst, u8 layer, u8 length, u16 *data);
int sclr_gop_update_16LUT(u8 inst, u8 layer, u8 index, u16 data);
void sclr_gop_fb_set_cfg(u8 inst, u8 layer, u8 fb_inst, struct sclr_gop_fb_cfg *cfg);
u32 sclr_gop_fb_get_record(u8 inst, u8 layer, u8 fb_inst);

void sclr_pri_set_cfg(u8 inst, struct sclr_privacy_cfg *cfg);

void sclr_ctrl_init(bool is_resume);
void sclr_get_2tap_scale(struct sclr_scale_2tap_cfg *cfg);
void sclr_ctrl_set_scale(u8 inst, struct sclr_scale_cfg *cfg);
int sclr_ctrl_set_input(enum sclr_img_in inst, enum sclr_input input,
			enum sclr_format fmt, enum sclr_csc csc, bool isp_online);
int sclr_ctrl_set_output(u8 inst, struct sclr_csc_cfg *cfg,
			 enum sclr_format fmt);
int sclr_ctrl_set_disp_src(bool disp_from_sc);
void sclr_engine_cmdq(struct sclr_ctrl_cfg *cfg, u8 cnt, void *cmdq_addr, uintptr_t phyaddr);
void sclr_engine_cmdq_rgnex(struct sclr_rgnex_cfg *cfgs, u8 cnt, uintptr_t cmdq_addr);

bool sclr_left_tile(u8 inst, u16 src_l_w);
bool sclr_right_tile(u8 inst, u16 src_offset);
u8 sclr_tile_cal_size(u8 inst, bool is_online_from_isp, struct sc_cfg_cb *post_para);
void vip_axi_realtime_fab_priority(void);
void _ddr_ctrl_patch(bool enable);

void sclr_set_sclr_to_vc_sb(u8 inst, u8 sb_mode, u8 sb_size, u8 sb_nb, u8 sb_wr_ctrl_idx);

struct sclr_csc_matrix *sclr_get_csc_mtrx(enum sclr_csc csc);

void sclr_dump_top_register(void);
void sclr_dump_disp_register(void);
void sclr_dump_img_in_register(int img_inst);
void sclr_dump_core_register(int inst);
void sclr_dump_odma_register(int inst);
void sclr_dump_gop_register(int inst);
void sclr_check_register(void);
void sclr_check_overflow_reg(struct cvi_vpss_info *vpss_info);

void sclr_gop_ow_get_addr(u8 inst, u8 layer, u8 ow_inst, u64 *addr);

#endif  //_CVI_SCL_H_
