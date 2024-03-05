#include <stdio.h>
#include <stdint.h>
#include <string.h>

#ifdef RUN_IN_SRAM
#include "config.h"
#include "marsrv_common.h"
#include "system_common.h"

#include "fw_config.h"
#include "bitwise_ops.h"
// #include "dw_uart.h"
#include "mmio.h"
#include "clock.h"
#include "delay.h"
#include "ffs.h"

#include "reg_vip_sys.h"
// #include "vip_sys.h"

#include "drv/cif_drv.h"

#elif (RUN_TYPE == CVIRTOS)
// #include "kernel.h"
#include "delay.h"
#include "sleep.h"
#include "mmio.h"
#include "cvi_type.h"
#include "cif_drv.h"
// #include "linux/irqreturn.h"
#include "irqreturn.h"
#endif

#include "pinctrl.h"
#include "cif.h"
#include "cif_uapi.h"
#include "sensor.h"
#include "gpio.h"
#include "vip_sys.h"

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

enum {
	LANE_SKEW_CROSS_CLK,
	LANE_SKEW_CROSS_DATA_NEAR,
	LANE_SKEW_CROSS_DATA_FAR,
	LANE_SKEW_CLK,
	LANE_SKEW_DATA,
	LANE_SKEW_NUM,
};

static struct cvi_cif_dev cif_dev[MAX_LINK_NUM] = {0};
static int mclk0 = CAMPLL_FREQ_NONE;
static int mclk1 = CAMPLL_FREQ_NONE;
static int lane_phase[CIF_LANE_NUM] = {-1, 0x10, 0x10, 0x10, 0x10};

static int bypass_mac_clk = 0;

const struct sync_code_s default_sync_code = {
	.norm_bk_sav = 0xAB0,
	.norm_bk_eav = 0xB60,
	.norm_sav = 0x800,
	.norm_eav = 0x9D0,
	.n0_bk_sav = 0x2B0,
	.n0_bk_eav = 0x360,
	.n1_bk_sav = 0x6B0,
	.n1_bk_eav = 0x760,
};

static struct cvi_link *ctx_to_link(const struct cif_ctx *ctx)
{
	return container_of(ctx, struct cvi_link, cif_ctx);
}

#ifndef RUN_IN_SRAM

#if 0
#define dev_dbg(...)		printf(__VA_ARGS__)
// #define dev_err(...)		printf(__VA_ARGS__)
#define dev_info(...)		printf(__VA_ARGS__)
#else
#define dev_dbg(...)		{}
// #define dev_err(...)		{}
#define dev_info(...)		{}
#endif
#define dev_err(...)		printf(__VA_ARGS__)


const char *_to_string_input_mode(enum input_mode_e input_mode)
{
	switch (input_mode) {
	case INPUT_MODE_MIPI:
		return "MIPI";
	case INPUT_MODE_SUBLVDS:
		return "SUBLVDS";
	case INPUT_MODE_HISPI:
		return "HISPI";
	case INPUT_MODE_CMOS:
		return "CMOS";
	case INPUT_MODE_BT1120:
		return "BT1120";
	case INPUT_MODE_BT601_19B_VHS:
		return "INPUT_MODE_BT601_19B_VHS";
	case INPUT_MODE_BT656_9B:
		return "INPUT_MODE_BT656_9B";
	case INPUT_MODE_CUSTOM_0:
		return "INPUT_MODE_CUSTOM_0";
	case INPUT_MODE_BT_DEMUX:
		return "INPUT_MODE_BT_DEMUX";
	default:
		return "unknown";
	}
}

const char *_to_string_mac_clk(enum rx_mac_clk_e mac_clk)
{
	switch (mac_clk) {
	case RX_MAC_CLK_200M:
		return "200MHZ";
	case RX_MAC_CLK_400M:
		return "400MHZ";
	case RX_MAC_CLK_500M:
		return "500MHZ";
	case RX_MAC_CLK_600M:
		return "600MHZ";
	default:
		return "unknown";
	}
}

const char *_to_string_cmd(unsigned int cmd)
{
	switch (cmd) {
	case CVI_MIPI_SET_DEV_ATTR:
		return "CVI_MIPI_SET_DEV_ATTR";
	case CVI_MIPI_SET_OUTPUT_CLK_EDGE:
		return "CVI_MIPI_SET_OUTPUT_CLK_EDGE";
	case CVI_MIPI_RESET_MIPI:
		return "CVI_MIPI_RESET_MIPI";
	case CVI_MIPI_SET_CROP_TOP:
		return "CVI_MIPI_SET_CROP_TOP";
	case CVI_MIPI_SET_HDR_MANUAL:
		return "CVI_MIPI_SET_HDR_MANUAL";
	case CVI_MIPI_SET_LVDS_FP_VS:
		return "CVI_MIPI_SET_LVDS_FP_VS";
	case CVI_MIPI_RESET_SENSOR:
		return "CVI_MIPI_RESET_SENSOR";
	case CVI_MIPI_UNRESET_SENSOR:
		return "CVI_MIPI_UNRESET_SENSOR";
	case CVI_MIPI_ENABLE_SENSOR_CLOCK:
		return "CVI_MIPI_ENABLE_SENSOR_CLOCK";
	case CVI_MIPI_DISABLE_SENSOR_CLOCK:
		return "CVI_MIPI_DISABLE_SENSOR_CLOCK";
	case CVI_MIPI_RESET_LVDS:
		return "CVI_MIPI_RESET_LVDS";
	case CVI_MIPI_GET_CIF_ATTR:
		return "CVI_MIPI_GET_CIF_ATTR";
	case CVI_MIPI_SET_MAX_MAC_CLOCK:
		return "CVI_MIPI_SET_MAX_MAC_CLOCK";
	default:
		return "unknown";
	}
	return "unknown";
}

const char *_to_string_raw_data_type(enum raw_data_type_e raw_data_type)
{
	switch (raw_data_type) {
	case RAW_DATA_8BIT:
		return "RAW8";
	case RAW_DATA_10BIT:
		return "RAW10";
	case RAW_DATA_12BIT:
		return "RAW12";
	case YUV422_8BIT:
		return "YUV422_8BIT";
	case YUV422_10BIT:
		return "YUV422_10BIT";
	default:
		return "unknown";
	}
}

const char *_to_string_mipi_hdr_mode(enum mipi_hdr_mode_e hdr)
{
	switch (hdr) {
	case CVI_MIPI_HDR_MODE_NONE:
		return "NONE";
	case CVI_MIPI_HDR_MODE_VC:
		return "VC";
	case CVI_MIPI_HDR_MODE_DT:
		return "DT";
	case CVI_MIPI_HDR_MODE_DOL:
		return "DOL";
	case CVI_MIPI_HDR_MODE_MANUAL:
		return "MANUAL";
	default:
		return "unknown";
	}
}

const char *_to_string_hdr_mode(enum hdr_mode_e hdr)
{
	switch (hdr) {
	case CVI_HDR_MODE_NONE:
		return "NONE";
	case CVI_HDR_MODE_2F:
		return "2To1";
	case CVI_HDR_MODE_3F:
		return "3To1";
	case CVI_HDR_MODE_DOL_2F:
		return "DOL2To1";
	case CVI_HDR_MODE_DOL_3F:
		return "DOL3To1";
	default:
		return "unknown";
	}
}

const char *_to_string_lvds_sync_mode(enum lvds_sync_mode_e mode)
{
	switch (mode) {
	case LVDS_SYNC_MODE_SOF:
		return "SOF";
	case LVDS_SYNC_MODE_SAV:
		return "SAV";
	default:
		return "unknown";
	}
}

const char *_to_string_bit_endian(enum lvds_bit_endian endian)
{
	switch (endian) {
	case LVDS_ENDIAN_LITTLE:
		return "LITTLE";
	case LVDS_ENDIAN_BIG:
		return "BIG";
	default:
		return "unknown";
	}
}

const char *_to_string_lvds_vsync_type(enum lvds_vsync_type_e type)
{
	switch (type) {
	case LVDS_VSYNC_NORMAL:
		return "NORMAL";
	case LVDS_VSYNC_SHARE:
		return "SHARE";
	case LVDS_VSYNC_HCONNECT:
		return "HCONNECT";
	default:
		return "unknown";
	}
}

const char *_to_string_lvds_fid_type(enum lvds_fid_type_e type)
{
	switch (type) {
	case LVDS_FID_NONE:
		return "FID_NONE";
	case LVDS_FID_IN_SAV:
		return "FID_IN_SAV";
	default:
		return "unknown";
	}
}

const char *_to_string_mclk(enum cam_pll_freq_e freq)
{
	switch (freq) {
	case CAMPLL_FREQ_NONE:
		return "CAMPLL_FREQ_NONE";
	case CAMPLL_FREQ_37P125M:
		return "CAMPLL_FREQ_37P125M";
	case CAMPLL_FREQ_25M:
		return "CAMPLL_FREQ_25M";
	case CAMPLL_FREQ_27M:
		return "CAMPLL_FREQ_27M";
	case CAMPLL_FREQ_24M:
		return "CAMPLL_FREQ_24M";
	case CAMPLL_FREQ_26M:
		return "CAMPLL_FREQ_26M";
	default:
		return "unknown";
	}
}

const char *_to_string_csi_decode(enum csi_decode_fmt_e fmt)
{
	switch (fmt) {
	case DEC_FMT_YUV422_8:
		return "yuv422-8";
	case DEC_FMT_YUV422_10:
		return "yuv422-10";
	case DEC_FMT_RAW8:
		return "raw8";
	case DEC_FMT_RAW10:
		return "raw10";
	case DEC_FMT_RAW12:
		return "raw12";
	default:
		return "unknown";
	}
}

const char *_to_string_dlane_state(enum mipi_dlane_state_e state)
{
	switch (state) {
	case HS_IDLE:
		return "hs_idle";
	case HS_SYNC:
		return "hs_sync";
	case HS_SKEW_CAL:
		return "skew_cal";
	case HS_ALT_CAL:
		return "alt_cal";
	case HS_PREAMPLE:
		return "preample";
	case HS_HST:
		return "hs_hst";
	case HS_ERR:
		return "hs_err";
	default:
		return "unknown";
	}
}

const char *_to_string_deskew_state(enum mipi_deskew_state_e state)
{
	switch (state) {
	case DESKEW_IDLE:
		return "idle";
	case DESKEW_START:
		return "start";
	case DESKEW_DONE:
		return "done";
	default:
		return "unknown";
	}
}

static void cif_dump_dev_attr(struct cvi_cif_dev *dev,
			      struct combo_dev_attr_s *attr)
{
	int i;

	dev_dbg("devno = %d, input_mode = %s, mac_clk = %s\n",
		attr->devno,
		_to_string_input_mode(attr->input_mode),
		_to_string_mac_clk(attr->mac_clk));
	dev_dbg("mclk%d = %s\n",
		attr->mclk.cam, _to_string_mclk(attr->mclk.freq));
	dev_dbg("width = %d, height = %d\n",
		attr->img_size.width, attr->img_size.height);
	switch (attr->input_mode) {
	case INPUT_MODE_MIPI: {
		struct mipi_dev_attr_s *mipi = &attr->mipi_attr;

		dev_dbg("raw_data_type = %s\n",
			_to_string_raw_data_type(mipi->raw_data_type));

		for (i = 0; i < MIPI_LANE_NUM + 1; i++) {
			dev_dbg("lane_id[%d] = %d, pn_swap = %s", i,
				mipi->lane_id[i],
				mipi->pn_swap[i] ? "True" : "False");
		}
		dev_dbg("hdr_mode = %s\n",
			_to_string_mipi_hdr_mode(mipi->hdr_mode));
		for (i = 0; i < HDR_VC_NUM; i++) {
			dev_dbg("data_type[%d] = 0x%x\n",
				i, mipi->data_type[i]);
		}
	}
	break;
	case INPUT_MODE_SUBLVDS:
	case INPUT_MODE_HISPI: {
		struct lvds_dev_attr_s *lvds = &attr->lvds_attr;
		int j;

		dev_dbg("hdr_mode = %s\n",
			_to_string_hdr_mode(lvds->hdr_mode));
		dev_dbg("sync_mode = %s\n",
			_to_string_lvds_sync_mode(lvds->sync_mode));
		dev_dbg("raw_data_type = %s\n",
			_to_string_raw_data_type(lvds->raw_data_type));
		dev_dbg("data_endian = %s\n",
			_to_string_bit_endian(lvds->data_endian));
		dev_dbg("sync_code_endian = %s\n",
			_to_string_bit_endian(lvds->sync_code_endian));
		for (i = 0; i < MIPI_LANE_NUM + 1; i++) {
			dev_dbg("lane_id[%d] = %d, pn_swap = %s ", i,
				lvds->lane_id[i],
				lvds->pn_swap[i] ? "True" : "False");
		}
		dev_dbg("sync code = {\n");
		for (i = 0; i < MIPI_LANE_NUM; i++) {
			dev_dbg("\t{\n");
			for (j = 0; j < HDR_VC_NUM+1; j++) {
				dev_dbg("\t\t{ %3x, %3x, %3x, %3x },\n",
					lvds->sync_code[i][j][0],
					lvds->sync_code[i][j][1],
					lvds->sync_code[i][j][2],
					lvds->sync_code[i][j][3]);
			}
			dev_dbg("\t},\n");
		}
		dev_dbg("}\n");
		dev_dbg("vsync_type = %s\n",
			_to_string_lvds_vsync_type(lvds->vsync_type.sync_type));
		dev_dbg("fid = %s\n",
			_to_string_lvds_fid_type(lvds->fid_type.fid));

	}
	break;
	case INPUT_MODE_CMOS:
		break;
	case INPUT_MODE_BT1120:
		break;
	default:
		break;
	}
}
#else
#define dev_dbg(...)		{}
#define dev_err(...)		{}
#define dev_info(...)		{}

const inline char *_to_string_cmd(unsigned int cmd) {return "";}
static inline void cif_dump_dev_attr(struct cvi_cif_dev *dev,
			      struct combo_dev_attr_s *attr) {}
#endif

#define LANE_IS_PORT1(x)	((x > 2) ? 1 : 0)
#define IS_SAME_PORT(x, y)	(!((LANE_IS_PORT1(x))^(LANE_IS_PORT1(y))))

#define PAD_CTRL_PU		BIT(2)
#define PAD_CTRL_PD		BIT(3)
#define PSD_CTRL_OFFSET(n)	((5 - n) * 8)

#ifdef MIPI_IF
static int _cif_set_attr_mipi(struct cvi_cif_dev *dev,
			      struct cif_ctx *ctx,
			      struct mipi_dev_attr_s *attr)
{
	struct combo_dev_attr_s *combo =
		container_of(attr, struct combo_dev_attr_s, mipi_attr);
	struct cif_param *param = ctx->cur_config;
	struct param_csi *csi = &param->cfg.csi;
	struct mipi_demux_info_s *info = &attr->demux;
	struct cvi_link *link = ctx_to_link(ctx);
	uint8_t tbl = 0x1F;
	int i, j = 0, clk_port = 0;
	uint32_t value;

	param->type = CIF_TYPE_CSI;
	/* config the bit mode. */
	switch (attr->raw_data_type) {
	case RAW_DATA_8BIT:
		csi->fmt = CSI_RAW_8;
		csi->decode_type = 0x2A;
		break;
	case RAW_DATA_10BIT:
		csi->fmt = CSI_RAW_10;
		csi->decode_type = 0x2B;
		break;
	case RAW_DATA_12BIT:
		csi->fmt = CSI_RAW_12;
		csi->decode_type = 0x2C;
		break;
	case YUV422_8BIT:
		csi->fmt = CSI_YUV422_8B;
		csi->decode_type = 0x1E;
		break;
	case YUV422_10BIT:
		csi->fmt = CSI_YUV422_10B;
		csi->decode_type = 0x1F;
		break;
	default:
		return -EINVAL;
	}
	/* config the lane id */
	for (i = 0; i < CIF_LANE_NUM; i++) {
		if (attr->lane_id[i] < 0)
			continue;
		if (attr->lane_id[i] >= CIF_PHY_LANE_NUM)
			return -EINVAL;
		if (!i)
			clk_port = LANE_IS_PORT1(attr->lane_id[i]);
		else {
			if (LANE_IS_PORT1(attr->lane_id[i]) != clk_port)
				clk_port = -1;
		}
		cif_set_lane_id(ctx, i, attr->lane_id[i], attr->pn_swap[i]);
		/* clear pad ctrl pu/pd */
		if (dev->pad_ctrl) {
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
		}
		tbl &= ~(1<<attr->lane_id[i]);
		j++;
	}
	csi->lane_num = j - 1;
	while (ffs(tbl)) {
		uint32_t idx = ffs(tbl) - 1;

		cif_set_lane_id(ctx, j++, idx, 0);
		tbl &= ~(1 << idx);
	}
	/* config  clock buffer direction.
	 * 1. When clock is between 0~2 and 1c4d, direction is P0->P1.
	 * 2. When clock is between 3~5 and 1c4d, direction is P1->P0.
	 * 3. When clock and data is between 0~2 and 1c2d, direction bit is freerun.
	 * 4. When clock is between 0~2 but data is not, direction is P0->P1.
	 * 5. When clock and data is between 3~5 and 1c2d and mac1 is used, direction bit is freerun.
	 * 6. When clock is between 3~5 but data is not, direction is P1->P0.
	 */
	if (csi->lane_num == 4) {
		if (LANE_IS_PORT1(attr->lane_id[0]))
			cif_set_clk_dir(ctx, CIF_CLK_P12P0);
		else
			cif_set_clk_dir(ctx, CIF_CLK_P02P1);

		for (i = 0; (i < csi->lane_num + 1); i++) {
			if (!i)
				cif_set_lane_deskew(ctx, attr->lane_id[i],
						lane_phase[LANE_SKEW_CROSS_CLK]);
			else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
				cif_set_lane_deskew(ctx, attr->lane_id[i],
						lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
			else
				cif_set_lane_deskew(ctx, attr->lane_id[i],
						lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
		}
	} else if (csi->lane_num > 0) {
		if (ctx->mac_num || !clk_port) {
			cif_set_clk_dir(ctx, CIF_CLK_FREERUN);
		} else {
			if (LANE_IS_PORT1(attr->lane_id[0]))
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			else
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
		}
		/* if clk and data are in the same port.*/
		if (clk_port > 0) {
			for (i = 0; i < (csi->lane_num + 1); i++) {
				if (!i)
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CLK]);
				else
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_DATA]);
			}
		} else {
			for (i = 0; i < (csi->lane_num + 1); i++) {
				if (!i)
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CROSS_CLK]);
				else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
				else
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
			}
		}
	} else
		return -EINVAL;

	/* config the dphy seting. */
	if (attr->dphy.enable)
		cif_set_hs_settle(ctx, attr->dphy.hs_settle);
	/* config the hdr mode. */
	switch (attr->hdr_mode) {
	case CVI_MIPI_HDR_MODE_NONE:
		break;
	case CVI_MIPI_HDR_MODE_DT:
		csi->hdr_mode = CSI_HDR_MODE_DT;
		for (i = 0; i < MAX_HDR_FRAME_NUM; i++)
			csi->data_type[i] = attr->data_type[i];
		break;
	case CVI_MIPI_HDR_MODE_MANUAL:
		param->hdr_manual = combo->hdr_manu.manual_en;
		param->hdr_shift = combo->hdr_manu.l2s_distance;
		param->hdr_vsize = combo->hdr_manu.lsef_length;
		param->hdr_rm_padding = combo->hdr_manu.discard_padding_lines;
		cif_hdr_manual_config(ctx, param, !!combo->hdr_manu.update);
		break;
	case CVI_MIPI_HDR_MODE_VC:
		csi->hdr_mode = CSI_HDR_MODE_VC;
		break;
	case CVI_MIPI_HDR_MODE_DOL:
		csi->hdr_mode = CSI_HDR_MODE_DOL;
		break;
	default:
		return -EINVAL;
	}
	/* config vc mapping */
	if (info->demux_en) {
		for (i = 0; i < MIPI_DEMUX_NUM; i++) {
			csi->vc_mapping[i] = info->vc_mapping[i];
		}
	} else {
		for (i = 0; i < MIPI_DEMUX_NUM; i++) {
			csi->vc_mapping[i] = i;
		}
	}

	param->hdr_en = (attr->hdr_mode != CVI_MIPI_HDR_MODE_NONE);
	cif_streaming(ctx, 1, param->hdr_en);
	link->is_on = 1;

	return 0;
}
#endif //MIPI_IF

#ifdef SUBLVDS_IF
static int _cif_set_lvds_vsync_type(struct cif_ctx *ctx,
				    struct lvds_dev_attr_s *attr,
				    struct param_sublvds *sublvds)
{
	struct combo_dev_attr_s *combo =
		container_of(attr, struct combo_dev_attr_s, lvds_attr);
	struct lvds_vsync_type_s *type = &attr->vsync_type;
	struct cvi_link *link = ctx_to_link(ctx);

	switch (type->sync_type) {
	case LVDS_VSYNC_NORMAL:
		sublvds->hdr_mode = CIF_SLVDS_HDR_PAT1;
		/* [TODO] use other api to set the fp */
		link->distance_fp = 15;
		cif_set_lvds_vsync_gen(ctx, 15);
		break;
	case LVDS_VSYNC_HCONNECT:
		sublvds->hdr_mode = CIF_SLVDS_HDR_PAT2;
		/* [TODO] use other api to set the fp */
		link->distance_fp = 1;
		cif_set_lvds_vsync_gen(ctx, 1);
		sublvds->hdr_hblank[0] = type->hblank1;
		sublvds->hdr_hblank[1] = type->hblank2;
		sublvds->h_size = combo->img_size.width;
		/* [TODO] use other api to strip the info line*/
		/* strip the top info line. */
		cif_crop_info_line(ctx, 1, 1);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int _cif_set_attr_sublvds(struct cvi_cif_dev *dev,
				 struct cif_ctx *ctx,
				 struct lvds_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct cvi_link *link = ctx_to_link(ctx);
	struct param_sublvds *sublvds = &param->cfg.sublvds;
	uint8_t tbl = 0x1F;
	struct sublvds_sync_code *sc;
	int i, j = 0, clk_port = 0;
	int rc = 0;
	uint32_t value;

	param->type = CIF_TYPE_SUBLVDS;
	/* config the bit mode. */
	switch (attr->raw_data_type) {
	case RAW_DATA_8BIT:
		sublvds->fmt = CIF_SLVDS_8_BIT;
		break;
	case RAW_DATA_10BIT:
		sublvds->fmt = CIF_SLVDS_10_BIT;
		break;
	case RAW_DATA_12BIT:
		sublvds->fmt = CIF_SLVDS_12_BIT;
		break;
	default:
		return -EINVAL;
	}
	/* config the endian. */
	if (attr->data_endian == LVDS_ENDIAN_BIG &&
	    attr->sync_code_endian == LVDS_ENDIAN_BIG) {
		sublvds->endian = CIF_SLVDS_ENDIAN_MSB;
		sublvds->wrap_endian = CIF_SLVDS_ENDIAN_MSB;

	} else if (attr->data_endian == LVDS_ENDIAN_LITTLE &&
		   attr->sync_code_endian == LVDS_ENDIAN_BIG) {
		sublvds->endian = CIF_SLVDS_ENDIAN_LSB;
		sublvds->wrap_endian = CIF_SLVDS_ENDIAN_MSB;

	} else if (attr->data_endian == LVDS_ENDIAN_BIG &&
		   attr->sync_code_endian == LVDS_ENDIAN_LITTLE) {
		sublvds->endian = CIF_SLVDS_ENDIAN_LSB;
		sublvds->wrap_endian = CIF_SLVDS_ENDIAN_LSB;
	} else {
		sublvds->endian = CIF_SLVDS_ENDIAN_MSB;
		sublvds->wrap_endian = CIF_SLVDS_ENDIAN_LSB;
	}
	/* check the sync mode. */
	if (attr->sync_mode != LVDS_SYNC_MODE_SAV)
		return -EINVAL;
	/* config the lane id*/
	for (i = 0; i < CIF_LANE_NUM; i++) {
		if (attr->lane_id[i] < 0)
			continue;
		if (attr->lane_id[i] >= CIF_PHY_LANE_NUM)
			return -EINVAL;
		if (!i)
			clk_port = LANE_IS_PORT1(attr->lane_id[i]);
		else {
			if (LANE_IS_PORT1(attr->lane_id[i]) != clk_port)
				clk_port = -1;
		}
		cif_set_lane_id(ctx, i, attr->lane_id[i], attr->pn_swap[i]);
		/* clear pad ctrl pu/pd */
		if (dev->pad_ctrl) {
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
		}
		tbl &= ~(1<<attr->lane_id[i]);
		j++;
	}
	sublvds->lane_num = j - 1;
	while (ffs(tbl)) {
		uint32_t idx = ffs(tbl) - 1;

		cif_set_lane_id(ctx, j++, idx, 0);
		tbl &= ~(1 << idx);
	}
	/* config  clock buffer direction.
	 * 1. When clock is between 0~2 and 1c4d, direction is P0->P1.
	 * 2. When clock is between 3~5 and 1c4d, direction is P1->P0.
	 * 3. When clock and data is between 0~2 and 1c2d, direction bit is freerun.
	 * 4. When clock is between 0~2 but data is not, direction is P0->P1.
	 * 5. When clock and data is between 3~5 and 1c2d and mac1 is used, direction bit is freerun.
	 * 6. When clock is between 3~5 but data is not, direction is P1->P0.
	 */
	if (sublvds->lane_num == 4) {
		if (LANE_IS_PORT1(attr->lane_id[0]))
			cif_set_clk_dir(ctx, CIF_CLK_P12P0);
		else
			cif_set_clk_dir(ctx, CIF_CLK_P02P1);
		for (i = 0; (i < sublvds->lane_num + 1); i++) {
			if (!i)
				cif_set_lane_deskew(ctx, attr->lane_id[i],
						lane_phase[LANE_SKEW_CROSS_CLK]);
			else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
				cif_set_lane_deskew(ctx, attr->lane_id[i],
						lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
			else
				cif_set_lane_deskew(ctx, attr->lane_id[i],
						lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
		}
	} else if (sublvds->lane_num > 0) {
		if (ctx->mac_num || !clk_port) {
			cif_set_clk_dir(ctx, CIF_CLK_FREERUN);
		} else {
			if (LANE_IS_PORT1(attr->lane_id[0]))
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			else
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
		}
		/* if clk and data are in the same port.*/
		if (clk_port > 0) {
			for (i = 0; i < (sublvds->lane_num + 1); i++) {
				if (!i)
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CLK]);
				else
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_DATA]);
			}
		} else {
			for (i = 0; i < (sublvds->lane_num + 1); i++) {
				if (!i)
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CROSS_CLK]);
				else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
				else
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
			}
		}
	} else
		return -EINVAL;

	/* config the sync code */
	memcpy(&sublvds->sync_code, &default_sync_code,
	       sizeof(default_sync_code));
	sc = &sublvds->sync_code.slvds;
	sc->n0_lef_sav = attr->sync_code[0][0][0];
	sc->n0_lef_eav = attr->sync_code[0][0][1];
	sc->n1_lef_sav = attr->sync_code[0][0][2];
	sc->n1_lef_eav = attr->sync_code[0][0][3];
	sc->n0_sef_sav = attr->sync_code[0][1][0];
	sc->n0_sef_eav = attr->sync_code[0][1][1];
	sc->n1_sef_sav = attr->sync_code[0][1][2];
	sc->n1_sef_eav = attr->sync_code[0][1][3];
	sc->n0_lsef_sav = attr->sync_code[0][2][0];
	sc->n0_lsef_eav = attr->sync_code[0][2][1];
	sc->n1_lsef_sav = attr->sync_code[0][2][2];
	sc->n1_lsef_eav = attr->sync_code[0][2][3];

	/* config the hdr */
	switch (attr->hdr_mode) {
	case CVI_HDR_MODE_NONE:
		/* [TODO] use other api to set the fp */
		link->distance_fp = 6;
		cif_set_lvds_vsync_gen(ctx, 6);
		break;
	case CVI_HDR_MODE_DOL_2F:
	case CVI_HDR_MODE_DOL_3F:
		/* [TODO] 3 exposure hdr hw is not ready. */
		/* config th Vsync type */
		rc = _cif_set_lvds_vsync_type(ctx, attr, sublvds);
		if (rc < 0)
			return rc;
		break;
	default:
		return -EINVAL;
	}
	param->hdr_en = (attr->hdr_mode != CVI_HDR_MODE_NONE);
	/* [TODO] config the fid type. */
	cif_streaming(ctx, 1, attr->hdr_mode != CVI_HDR_MODE_NONE);

	return 0;
}
#endif // SUBLVDS_IF

#ifdef HISPI_IF
static int _cif_set_hispi_vsync_type(struct cif_ctx *ctx,
				     struct lvds_dev_attr_s *attr,
				     struct cif_param *param)
{
	struct combo_dev_attr_s *combo =
		container_of(attr, struct combo_dev_attr_s, lvds_attr);
	struct lvds_vsync_type_s *type = &attr->vsync_type;

	switch (type->sync_type) {
	case LVDS_VSYNC_NORMAL:
		break;
	case LVDS_VSYNC_SHARE:
		param->hdr_manual = combo->hdr_manu.manual_en;
		param->hdr_shift = combo->hdr_manu.l2s_distance;
		param->hdr_vsize = combo->hdr_manu.lsef_length;
		param->hdr_rm_padding = combo->hdr_manu.discard_padding_lines;
		cif_hdr_manual_config(ctx, param, !!combo->hdr_manu.update);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int _cif_set_attr_hispi(struct cvi_cif_dev *dev,
			       struct cif_ctx *ctx,
			       struct lvds_dev_attr_s *attr)
{
	struct combo_dev_attr_s *combo =
		container_of(attr, struct combo_dev_attr_s, lvds_attr);
	struct cif_param *param = ctx->cur_config;
	struct param_hispi *hispi = &param->cfg.hispi;
	uint8_t tbl = 0x1F;
	struct hispi_sync_code *sc;
	int i, j = 0, clk_port = 0;
	int rc = 0;
	uint32_t value;

	param->type = CIF_TYPE_HISPI;
	/* config the bit mode. */
	switch (attr->raw_data_type) {
	case RAW_DATA_8BIT:
		hispi->fmt = CIF_SLVDS_8_BIT;
		break;
	case RAW_DATA_10BIT:
		hispi->fmt = CIF_SLVDS_10_BIT;
		break;
	case RAW_DATA_12BIT:
		hispi->fmt = CIF_SLVDS_12_BIT;
		break;
	default:
		return -EINVAL;
	}
	/* config the endian. */
	if (attr->data_endian == LVDS_ENDIAN_BIG &&
	    attr->sync_code_endian == LVDS_ENDIAN_BIG) {
		hispi->endian = CIF_SLVDS_ENDIAN_MSB;
		hispi->wrap_endian = CIF_SLVDS_ENDIAN_MSB;
	} else if (attr->data_endian == LVDS_ENDIAN_LITTLE &&
		   attr->sync_code_endian == LVDS_ENDIAN_BIG) {
		hispi->endian = CIF_SLVDS_ENDIAN_LSB;
		hispi->wrap_endian = CIF_SLVDS_ENDIAN_MSB;
	} else if (attr->data_endian == LVDS_ENDIAN_BIG &&
		   attr->sync_code_endian == LVDS_ENDIAN_LITTLE) {
		hispi->endian = CIF_SLVDS_ENDIAN_LSB;
		hispi->wrap_endian = CIF_SLVDS_ENDIAN_LSB;
	} else {
		hispi->endian = CIF_SLVDS_ENDIAN_MSB;
		hispi->wrap_endian = CIF_SLVDS_ENDIAN_LSB;
	}
	/* check the sync mode. */
	if (attr->sync_mode == LVDS_SYNC_MODE_SOF) {
		hispi->mode = CIF_HISPI_MODE_PKT_SP;
	} else if (attr->sync_mode == LVDS_SYNC_MODE_SAV) {
		hispi->mode = CIF_HISPI_MODE_STREAM_SP;
		hispi->h_size = combo->img_size.width;
	} else {
		return -EINVAL;
	}
	/* config the lane id*/
	for (i = 0; i < CIF_LANE_NUM; i++) {
		if (attr->lane_id[i] < 0)
			continue;
		if (attr->lane_id[i] >= CIF_PHY_LANE_NUM)
			return -EINVAL;
		if (!i)
			clk_port = LANE_IS_PORT1(attr->lane_id[i]);
		else {
			if (LANE_IS_PORT1(attr->lane_id[i]) != clk_port)
				clk_port = -1;
		}
		cif_set_lane_id(ctx, i, attr->lane_id[i], attr->pn_swap[i]);
		/* clear pad ctrl pu/pd */
		if (dev->pad_ctrl) {
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
		}
		tbl &= ~(1<<attr->lane_id[i]);
		j++;
	}
	hispi->lane_num = j - 1;
	while (ffs(tbl)) {
		uint32_t idx = ffs(tbl) - 1;

		cif_set_lane_id(ctx, j++, idx, 0);
		tbl &= ~(1 << idx);
	}
	/* config  clock buffer direction.
	 * 1. When clock is between 0~2 and 1c4d, direction is P0->P1.
	 * 2. When clock is between 3~5 and 1c4d, direction is P1->P0.
	 * 3. When clock and data is between 0~2 and 1c2d, direction bit is freerun.
	 * 4. When clock is between 0~2 but data is not, direction is P0->P1.
	 * 5. When clock and data is between 3~5 and 1c2d and mac1 is used, direction bit is freerun.
	 * 6. When clock is between 3~5 but data is not, direction is P1->P0.
	 */
	if (hispi->lane_num == 4) {
		if (LANE_IS_PORT1(attr->lane_id[0]))
			cif_set_clk_dir(ctx, CIF_CLK_P12P0);
		else
			cif_set_clk_dir(ctx, CIF_CLK_P02P1);
		for (i = 0; (i < hispi->lane_num + 1); i++) {
			if (!i)
				cif_set_lane_deskew(ctx, attr->lane_id[i],
						lane_phase[LANE_SKEW_CROSS_CLK]);
			else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
				cif_set_lane_deskew(ctx, attr->lane_id[i],
						lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
			else
				cif_set_lane_deskew(ctx, attr->lane_id[i],
						lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
		}
	} else if (hispi->lane_num > 0) {
		if (ctx->mac_num || !clk_port) {
			cif_set_clk_dir(ctx, CIF_CLK_FREERUN);
		} else {
			if (LANE_IS_PORT1(attr->lane_id[0]))
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			else
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
		}
		/* if clk and data are in the same port.*/
		if (clk_port > 0) {
			for (i = 0; i < (hispi->lane_num + 1); i++) {
				if (!i)
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CLK]);
				else
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_DATA]);
			}
		} else {
			for (i = 0; i < (hispi->lane_num + 1); i++) {
				if (!i)
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CROSS_CLK]);
				else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
				else
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
			}
		}
	} else
		return -EINVAL;

	/* config the sync code */
	memcpy(&hispi->sync_code, &default_sync_code,
	       sizeof(default_sync_code));
	sc = &hispi->sync_code.hispi;
	sc->t1_sol = attr->sync_code[0][0][0];
	sc->t1_eol = attr->sync_code[0][0][1];
	sc->t1_sof = attr->sync_code[0][0][2];
	sc->t1_eof = attr->sync_code[0][0][3];
	sc->t2_sol = attr->sync_code[0][1][0];
	sc->t2_eol = attr->sync_code[0][1][1];
	sc->t2_sof = attr->sync_code[0][1][2];
	sc->t2_eof = attr->sync_code[0][1][3];
	sc->vsync_gen = sc->t1_sof;

	/* config the hdr */
	switch (attr->hdr_mode) {
	case CVI_HDR_MODE_NONE:
		break;
	case CVI_HDR_MODE_2F:
	case CVI_HDR_MODE_3F:
		/* [TODO] 3 exposure hdr hw is not ready. */
		/* config th Vsync type */
		rc = _cif_set_hispi_vsync_type(ctx, attr, param);
		if (rc < 0)
			return rc;
		break;
	default:
		return -EINVAL;
	}
	param->hdr_en = (attr->hdr_mode != CVI_HDR_MODE_NONE);
	/* [TODO] config the fid type. */
	cif_streaming(ctx, 1, attr->hdr_mode != CVI_HDR_MODE_NONE);

	return 0;
}
#endif // HISPI_IF

#ifdef DVP_IF
struct vi_pin_info {
	uint32_t	addr;
	uint32_t	offset;
	uint32_t	mask;
	uint32_t	func;
};

const struct vi_pin_info vi_pin[TTL_VI_SRC_NUM][MAX_PAD_NUM] = {
	[TTL_VI_SRC_VI0] = {
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX4P,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX4P_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX4P_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3N,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3N_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3N_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3P,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3P_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3P_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2N,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2N_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2N_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2P,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2P_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2P_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX1N,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX1N_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX1N_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX1P,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX1P_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX1P_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX0N,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX0N_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX0N_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX0P,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX0P_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX0P_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXM2,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXM2_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXM2_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXP2,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXP2_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXP2_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXM1,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXM1_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXM1_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXP1,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXP1_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXP1_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXM0,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXM0_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXM0_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXP0,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXP0_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXP0_MASK,
			1,
		},
	},
#ifdef __CV181X__
	[TTL_VI_SRC_VI1] = {
		{
			FMUX_GPIO_FUNCSEL_VIVO_D0,
			FMUX_GPIO_FUNCSEL_VIVO_D0_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D0_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D1,
			FMUX_GPIO_FUNCSEL_VIVO_D1_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D1_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D2,
			FMUX_GPIO_FUNCSEL_VIVO_D2_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D2_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D3,
			FMUX_GPIO_FUNCSEL_VIVO_D3_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D3_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D4,
			FMUX_GPIO_FUNCSEL_VIVO_D4_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D4_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D5,
			FMUX_GPIO_FUNCSEL_VIVO_D5_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D5_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D6,
			FMUX_GPIO_FUNCSEL_VIVO_D6_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D6_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D7,
			FMUX_GPIO_FUNCSEL_VIVO_D7_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D7_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D8,
			FMUX_GPIO_FUNCSEL_VIVO_D8_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D8_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D9,
			FMUX_GPIO_FUNCSEL_VIVO_D9_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D9_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D10,
			FMUX_GPIO_FUNCSEL_VIVO_D10_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D10_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX5N,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX5N_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX5N_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX5P,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX5P_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX5P_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX4N,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX4N_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX4N_MASK,
			2,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX4P,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX4P_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX4P_MASK,
			2,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3N,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3N_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3N_MASK,
			2,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3P,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3P_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3P_MASK,
			2,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2N,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2N_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2N_MASK,
			4,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2P,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2P_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2P_MASK,
			4
		},
	},
	[TTL_VI_SRC_VI2] = {
		{
			FMUX_GPIO_FUNCSEL_VIVO_D0,
			FMUX_GPIO_FUNCSEL_VIVO_D0_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D0_MASK,
			0,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D1,
			FMUX_GPIO_FUNCSEL_VIVO_D1_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D1_MASK,
			0,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D2,
			FMUX_GPIO_FUNCSEL_VIVO_D2_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D2_MASK,
			0,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D3,
			FMUX_GPIO_FUNCSEL_VIVO_D3_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D3_MASK,
			0,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D4,
			FMUX_GPIO_FUNCSEL_VIVO_D4_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D4_MASK,
			0,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D5,
			FMUX_GPIO_FUNCSEL_VIVO_D5_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D5_MASK,
			0,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D6,
			FMUX_GPIO_FUNCSEL_VIVO_D6_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D6_MASK,
			0,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D7,
			FMUX_GPIO_FUNCSEL_VIVO_D7_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D7_MASK,
			0,
		},
	},
#endif
};

static void cif_config_pinmux(enum ttl_src_e vi, uint32_t pad)
{
	mmio_clrsetbits_32(PINMUX_BASE + vi_pin[vi][pad].addr,
			vi_pin[vi][pad].mask << vi_pin[vi][pad].offset,
			vi_pin[vi][pad].func);
}

static int _cif_set_attr_cmos(struct cvi_cif_dev *dev,
			      struct cif_ctx *ctx,
			      struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_ttl *ttl = &param->cfg.ttl;
	enum ttl_src_e vi = attr->ttl_attr.vi;
	int i;

	if (vi == TTL_VI_SRC_VI2)
		return -EINVAL;

	/* config the pinmux */
	for (i = 0; i < TTL_PIN_FUNC_NUM; i++) {
		if (attr->ttl_attr.func[i] < 0)
			continue;
		if (attr->ttl_attr.func[i] >= TTL_PIN_FUNC_NUM)
			return -EINVAL;
		cif_set_ttl_pinmux(ctx, vi, i, attr->ttl_attr.func[i]);
		cif_config_pinmux(vi, attr->ttl_attr.func[i]);
	}
	if (vi == TTL_VI_SRC_VI0)
		PINMUX_CONFIG(PAD_MIPIRX4N, VI0_CLK);
#ifdef __CV181X__
	else if (vi == TTL_VI_SRC_VI1)
		PINMUX_CONFIG(VIVO_CLK, VI1_CLK);
	else
		PINMUX_CONFIG(VIVO_CLK, VI2_CLK);
#endif

	param->type = CIF_TYPE_TTL;
	ttl->vi_from = vi;
	ttl->fmt = TTL_VDE_SENSOR;
	ttl->sensor_fmt = TTL_SENSOR_12_BIT;

	cif_streaming(ctx, 1, 0);

	return 0;
}
#endif // DVP_IF

#ifdef BT1120_IF
static int _cif_set_attr_bt1120(struct cvi_cif_dev *dev,
				struct cif_ctx *ctx,
				struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_ttl *ttl = &param->cfg.ttl;
	struct cvi_link *link = ctx_to_link(ctx);
	enum ttl_src_e vi = attr->ttl_attr.vi;
	int i;

	if (vi == TTL_VI_SRC_VI2)
		return -EINVAL;

	/* config the pinmux */
	for (i = 0; i < TTL_PIN_FUNC_NUM; i++) {
		if (attr->ttl_attr.func[i] < 0)
			continue;
		if (attr->ttl_attr.func[i] >= TTL_PIN_FUNC_NUM)
			return -EINVAL;
		cif_set_ttl_pinmux(ctx, vi, i, attr->ttl_attr.func[i]);
		cif_config_pinmux(vi, attr->ttl_attr.func[i]);
	}
	if (vi == TTL_VI_SRC_VI0)
		PINMUX_CONFIG(PAD_MIPIRX4N, VI0_CLK);
#ifdef __CV181X__
	else if (vi == TTL_VI_SRC_VI1)
		PINMUX_CONFIG(VIVO_CLK, VI1_CLK);
	else
		PINMUX_CONFIG(VIVO_CLK, VI2_CLK);
#endif

	param->type = CIF_TYPE_TTL;
	ttl->fmt = TTL_SYNC_PAT_17B_BT1120;
	ttl->width = attr->img_size.width - 1;
	ttl->height = attr->img_size.height - 1;
	ttl->sensor_fmt = TTL_SENSOR_12_BIT;
	ttl->clk_inv = link->clk_edge;
	ttl->vi_sel = VI_BT1120;
	ttl->v_bp = (!attr->ttl_attr.v_bp) ? 9 : attr->ttl_attr.v_bp;
	ttl->h_bp = (!attr->ttl_attr.h_bp) ? 8 : attr->ttl_attr.h_bp;

	cif_streaming(ctx, 1, 0);

	return 0;
}
#endif // BT1120_IF

#ifdef BT601_IF
static int _cif_set_attr_bt601_19b_vhs(struct cvi_cif_dev *dev,
				       struct cif_ctx *ctx,
				       struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_ttl *ttl = &param->cfg.ttl;
	struct cvi_link *link = ctx_to_link(ctx);
	enum ttl_src_e vi = attr->ttl_attr.vi;
	int i;

	if (vi == TTL_VI_SRC_VI2)
		return -EINVAL;

	/* config the pinmux */
	for (i = 0; i < TTL_PIN_FUNC_NUM; i++) {
		if (attr->ttl_attr.func[i] < 0)
			continue;
		if (attr->ttl_attr.func[i] >= TTL_PIN_FUNC_NUM)
			return -EINVAL;
		cif_set_ttl_pinmux(ctx, vi, i, attr->ttl_attr.func[i]);
		cif_config_pinmux(vi, attr->ttl_attr.func[i]);
	}
	if (vi == TTL_VI_SRC_VI0)
		PINMUX_CONFIG(PAD_MIPIRX4N, VI0_CLK);
#ifdef __CV181X__
	else if (vi == TTL_VI_SRC_VI1)
		PINMUX_CONFIG(VIVO_CLK, VI1_CLK);
	else
		PINMUX_CONFIG(VIVO_CLK, VI2_CLK);
#endif

	param->type = CIF_TYPE_TTL;
	ttl->vi_from = vi;
	ttl->fmt = TTL_VHS_19B_BT601;
	ttl->width = attr->img_size.width - 1;
	ttl->height = attr->img_size.height - 1;
	ttl->sensor_fmt = TTL_SENSOR_12_BIT;
	ttl->clk_inv = link->clk_edge;
	ttl->vi_sel = VI_BT601;
	ttl->v_bp = (!attr->ttl_attr.v_bp) ? 0x23 : attr->ttl_attr.v_bp;
	ttl->h_bp = (!attr->ttl_attr.h_bp) ? 0xbf : attr->ttl_attr.h_bp;

	cif_streaming(ctx, 1, 0);

	return 0;
}
#endif // BT601_IF

#ifdef BT_DEMUX_IF
static int _cif_set_attr_bt_demux(struct cvi_cif_dev *dev,
					struct cif_ctx *ctx,
					struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_btdemux *btdemux = &param->cfg.btdemux;
	struct cvi_link *link = ctx_to_link(ctx);
	struct bt_demux_attr_s *info = &attr->bt_demux_attr;
	int i;

	/* config the pinmux */
	for (i = TTL_PIN_FUNC_D0; i < TTL_PIN_FUNC_D8; i++) {
		if (info->func[i] < 0)
			continue;
		if (info->func[i] >= TTL_PIN_FUNC_D8)
			return -EINVAL;
		cif_set_ttl_pinmux(ctx, FROM_VI2, i, info->func[i]);
		cif_config_pinmux(FROM_VI2, info->func[i]);
	}
#ifdef __CV181X__
	PINMUX_CONFIG(VIVO_CLK, VI2_CLK);
#endif
	param->type = CIF_TYPE_BT_DMUX;
	btdemux->fmt = TTL_SYNC_PAT_9B_BT656;
	btdemux->width = attr->img_size.width - 1;
	btdemux->height = attr->img_size.height - 1;
	btdemux->clk_inv = link->clk_edge;
	btdemux->v_fp = (!info->v_fp) ? 0x0f : info->v_fp;
	btdemux->h_fp = (!info->h_fp) ? 0x0f : info->h_fp;
	btdemux->sync_code_part_A[0] = info->sync_code_part_A[0];
	btdemux->sync_code_part_A[1] = info->sync_code_part_A[1];
	btdemux->sync_code_part_A[2] = info->sync_code_part_A[2];
	for (i = 0; i < BT_DEMUX_NUM; i++) {
		btdemux->sync_code_part_B[i].sav_vld = info->sync_code_part_B[i].sav_vld;
		btdemux->sync_code_part_B[i].sav_blk = info->sync_code_part_B[i].sav_blk;
		btdemux->sync_code_part_B[i].eav_vld = info->sync_code_part_B[i].eav_vld;
		btdemux->sync_code_part_B[i].eav_blk = info->sync_code_part_B[i].eav_blk;
	}
	btdemux->demux = info->mode;
	btdemux->yc_exchg = info->yc_exchg;

	cif_streaming(ctx, 1, 0);

	return 0;
}
#endif // BT_DEMUX_IF

#ifdef BT656_IF
static int _cif_set_attr_bt656_9b(struct cvi_cif_dev *dev,
				  struct cif_ctx *ctx,
				  struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_ttl *ttl = &param->cfg.ttl;
	struct cvi_link *link = ctx_to_link(ctx);
	enum ttl_src_e vi = attr->ttl_attr.vi;
	int i;

	/* config the pinmux */
	for (i = 0; i < TTL_PIN_FUNC_NUM; i++) {
		if (attr->ttl_attr.func[i] < 0)
			continue;
		if (attr->ttl_attr.func[i] >= TTL_PIN_FUNC_NUM)
			return -EINVAL;
		cif_set_ttl_pinmux(ctx, vi, i, attr->ttl_attr.func[i]);
		cif_config_pinmux(vi, attr->ttl_attr.func[i]);
	}
	if (vi == TTL_VI_SRC_VI0)
		PINMUX_CONFIG(PAD_MIPIRX4N, VI0_CLK);
#ifdef __CV181X__
	else if (vi == TTL_VI_SRC_VI1)
		PINMUX_CONFIG(VIVO_CLK, VI1_CLK);
	else
		PINMUX_CONFIG(VIVO_CLK, VI2_CLK);
#endif

	param->type = CIF_TYPE_TTL;
	ttl->vi_from = vi;
	ttl->fmt_out = TTL_BT_FMT_OUT_CBYCRY;
	ttl->fmt = TTL_SYNC_PAT_9B_BT656;
	ttl->width = attr->img_size.width - 1;
	ttl->height = attr->img_size.height - 1;
	ttl->sensor_fmt = TTL_SENSOR_12_BIT;
	ttl->clk_inv = link->clk_edge;
	ttl->vi_sel = VI_BT656;
	ttl->v_bp = (!attr->ttl_attr.v_bp) ? 0xf : attr->ttl_attr.v_bp;
	ttl->h_bp = (!attr->ttl_attr.h_bp) ? 0xf : attr->ttl_attr.h_bp;

	cif_streaming(ctx, 1, 0);

	return 0;
}
#endif // BT656_IF

#ifdef	CUSTOM0_IF
static int _cif_set_attr_custom0(struct cvi_cif_dev *dev,
				struct cif_ctx *ctx,
				struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_ttl *ttl = &param->cfg.ttl;
	enum ttl_src_e vi = attr->ttl_attr.vi;
	int i;

	if (vi == TTL_VI_SRC_VI2)
		return -EINVAL;

	/* config the pinmux */
	for (i = 0; i < TTL_PIN_FUNC_NUM; i++) {
		if (attr->ttl_attr.func[i] < 0)
			continue;
		if (attr->ttl_attr.func[i] >= TTL_PIN_FUNC_NUM)
			return -EINVAL;
		cif_set_ttl_pinmux(ctx, vi, i, attr->ttl_attr.func[i]);
		cif_config_pinmux(vi, attr->ttl_attr.func[i]);
	}
	if (vi == TTL_VI_SRC_VI0)
		PINMUX_CONFIG(PAD_MIPIRX4N, VI0_CLK);
#ifdef __CV181X__
	else if (vi == TTL_VI_SRC_VI1)
		PINMUX_CONFIG(VIVO_CLK, VI1_CLK);
	else
		PINMUX_CONFIG(VIVO_CLK, VI2_CLK);
#endif

	param->type = CIF_TYPE_TTL;
	ttl->vi_from = vi;
	ttl->fmt_out = TTL_BT_FMT_OUT_CBYCRY;
	ttl->fmt = TTL_CUSTOM_0;
	ttl->width = attr->img_size.width - 1;
	ttl->height = attr->img_size.height - 1;
	ttl->sensor_fmt = TTL_SENSOR_12_BIT;
	ttl->vi_sel = VI_BT601;
	ttl->v_bp = (!attr->ttl_attr.v_bp) ? 4095 : attr->ttl_attr.v_bp;
	ttl->h_bp = (!attr->ttl_attr.h_bp) ? 4 : attr->ttl_attr.h_bp;

	cif_streaming(ctx, 1, 0);

	return 0;
}
#endif // CUSTOM0_IF

/*
 * 1. Precondition: the mac_max = 400M by default, can be adjust by ioctl (app).
 * 2. If mac_max is below 400M, the vip_sys_2 is 400M (parent: mipimpll).
 * 3. If mac_max = 500M, the vip_sys_2 is 500M. (parent: fpll, 1835 OD case)
 * 4. If mac_max = 600M, the vip_sys_2 is 600M. (parent: mipimpll 1838 case)
 * 5. Return error in the others.
 */

#define MAC0_CLK_CTRL1_OFFSET		4U
#define MAC1_CLK_CTRL1_OFFSET		8U
#define MAC2_CLK_CTRL1_OFFSET		30U
#define MAC_CLK_CTRL1_MASK		0x3U

static int cif_set_mac_clk(struct cvi_cif_dev *cdev, uint32_t devno,
		enum rx_mac_clk_e mac_clk)
{
	struct cvi_link *link = &cdev->link[devno];
	uint32_t data, mask, tmp;
	uint32_t clk_val = (mac_clk + 99) / 100;

	link->mac_clk = mac_clk;

	/* select the source to vip_sys2 */
	switch (devno) {
	case 1:
		mask = (MAC_CLK_CTRL1_MASK << MAC1_CLK_CTRL1_OFFSET);
		data = 0x2 << MAC1_CLK_CTRL1_OFFSET;
		break;
	case 2:
		mask = (MAC_CLK_CTRL1_MASK << MAC2_CLK_CTRL1_OFFSET);
		data = 0x2 << MAC2_CLK_CTRL1_OFFSET;
		break;
	case 0:
	default:
		mask = (MAC_CLK_CTRL1_MASK << MAC0_CLK_CTRL1_OFFSET);
		data = 0x2 << MAC0_CLK_CTRL1_OFFSET;
		break;
	}
	vip_sys_reg_write_mask(VIP_SYS_VIP_CLK_CTRL1, mask, data);

	if (bypass_mac_clk)
		return 0;

#ifdef FPGA_PORTING
	// FPGA don't set mac spped.
	return 0;
#endif

	if (clk_val > cdev->max_mac_clk) {
		dev_err("cannot reach %dM since the max clk is %dM\n", clk_val, cdev->max_mac_clk);
		return -EINVAL;
	}

	/* target = source * (1 + ratio) / 32, ratio <= 0x1F */
	tmp = clk_val * 32 / cdev->max_mac_clk;
	/* roundup */
	if ((clk_val * 32) % cdev->max_mac_clk)
		tmp++;
	tmp--;

	switch (devno) {
	case 1:
		VIP_NORM_CLK_RATIO_CONFIG(VAL_CSI_MAC1, tmp);
		VIP_NORM_CLK_RATIO_CONFIG(EN_CSI_MAC1, 1);
		VIP_UPDATE_CLK_RATIO(SEL_CSI_MAC1);
		break;
	case 2:
		VIP_NORM_CLK_RATIO_CONFIG(VAL_CSI_MAC2, tmp);
		VIP_NORM_CLK_RATIO_CONFIG(EN_CSI_MAC2, 1);
		VIP_UPDATE_CLK_RATIO(SEL_CSI_MAC2);
		break;
	case 0:
	default:
		VIP_NORM_CLK_RATIO_CONFIG(VAL_CSI_MAC0, tmp);
		VIP_NORM_CLK_RATIO_CONFIG(EN_CSI_MAC0, 1);
		VIP_UPDATE_CLK_RATIO(SEL_CSI_MAC0);
		break;
	}

	switch (mac_clk) {
	case RX_MAC_CLK_200M:
		/* vipsys2 dividor factor */
		// iowrite32((6<<16)|0x209, ioremap(0x03002110, 4));
		SET_VIP_SYS_2_CLK_DIV(VIP_SYS_2_SRC_DISPPLL, 6);
		break;
	case RX_MAC_CLK_300M:
		/* vipsys2 dividor factor */
		// iowrite32((4<<16)|0x209, ioremap(0x03002110, 4));
		SET_VIP_SYS_2_CLK_DIV(VIP_SYS_2_SRC_DISPPLL, 4);
		break;
	case RX_MAC_CLK_500M:
		/* vipsys2 dividor factor */
		// iowrite32((3<<16)|0x309, ioremap(0x03002110, 4));
		SET_VIP_SYS_2_CLK_DIV(VIP_SYS_2_SRC_FPLL, 3);
		break;
	case RX_MAC_CLK_600M:
		/* vipsys2 dividor factor */
		// iowrite32((2<<16)|0x209, ioremap(0x03002110, 4));
		SET_VIP_SYS_2_CLK_DIV(VIP_SYS_2_SRC_DISPPLL, 2);
		break;
	case RX_MAC_CLK_400M:
	default:
		/* vipsys2 dividor factor */
		// iowrite32((3<<16)|0x209, ioremap(0x03002110, 4));
		SET_VIP_SYS_2_CLK_DIV(VIP_SYS_2_SRC_DISPPLL, 3);
		break;
	}
	udelay(5);

	return 0;
}

static int cif_set_output_clk_edge(struct cvi_cif_dev *dev,
				   struct clk_edge_s *clk_edge)
{
	struct cif_ctx *ctx = &dev->link[clk_edge->devno].cif_ctx;

	dev->link[clk_edge->devno].clk_edge = clk_edge->edge;

	cif_set_clk_edge(ctx, CIF_PHY_LANE_0,
			 (clk_edge->edge == CLK_UP_EDGE)
			 ? CIF_CLK_RISING : CIF_CLK_FALLING);
	cif_set_clk_edge(ctx, CIF_PHY_LANE_1,
			 (clk_edge->edge == CLK_UP_EDGE)
			 ? CIF_CLK_RISING : CIF_CLK_FALLING);
	cif_set_clk_edge(ctx, CIF_PHY_LANE_2,
			 (clk_edge->edge == CLK_UP_EDGE)
			 ? CIF_CLK_RISING : CIF_CLK_FALLING);
	cif_set_clk_edge(ctx, CIF_PHY_LANE_3,
			 (clk_edge->edge == CLK_UP_EDGE)
			 ? CIF_CLK_RISING : CIF_CLK_FALLING);
	cif_set_clk_edge(ctx, CIF_PHY_LANE_4,
			 (clk_edge->edge == CLK_UP_EDGE)
			 ? CIF_CLK_RISING : CIF_CLK_FALLING);
	cif_set_clk_edge(ctx, CIF_PHY_LANE_5,
			 (clk_edge->edge == CLK_UP_EDGE)
			 ? CIF_CLK_RISING : CIF_CLK_FALLING);

	return 0;
}

static int cif_set_dev_attr(struct cvi_cif_dev *dev,
			    struct combo_dev_attr_s *attr)
{
	enum input_mode_e input_mode = attr->input_mode;
	struct cif_ctx *ctx;
	struct cif_param *param;
	struct combo_dev_attr_s *rx_attr;
	int rc = -1;

	/* force the btdmux to devno 0*/
	if (input_mode == INPUT_MODE_BT_DEMUX) {
		attr->devno = 0;
	}

	ctx = &dev->link[attr->devno].cif_ctx;
	param = &dev->link[attr->devno].param;
	rx_attr = &dev->link[attr->devno].attr;

	cif_dump_dev_attr(dev, attr);

	memset(param, 0, sizeof(*param));
	ctx->cur_config = param;
	memcpy(rx_attr, attr, sizeof(*attr));

	/* Setting for serial interface. */
	if (input_mode == INPUT_MODE_MIPI ||
	    input_mode == INPUT_MODE_SUBLVDS ||
	    input_mode == INPUT_MODE_HISPI) {
		struct clk_edge_s clk_edge;

		/* set the default clock edge. */
		clk_edge.devno = attr->devno;
		clk_edge.edge = CLK_DOWN_EDGE;
		cif_set_output_clk_edge(dev, &clk_edge);
	}

	/* set mac clk */
	rc = cif_set_mac_clk(dev, attr->devno, attr->mac_clk);
	if (rc < 0)
		return rc;

	/* decide the mclk */
	if (!attr->mclk.cam)
		mclk0 = attr->mclk.freq;
	else if (attr->mclk.cam == 1)
		mclk1 = attr->mclk.freq;

	switch (input_mode) {
#ifdef	MIPI_IF
	case INPUT_MODE_MIPI:
		rc = _cif_set_attr_mipi(dev, ctx, &rx_attr->mipi_attr);
		break;
#endif
#ifdef	SUBLVDS_IF
	case INPUT_MODE_SUBLVDS:
		rc = _cif_set_attr_sublvds(dev, ctx,
					   &rx_attr->lvds_attr);
		break;
#endif
#ifdef	HISPI_IF
	case INPUT_MODE_HISPI:
		rc = _cif_set_attr_hispi(dev, ctx, &rx_attr->lvds_attr);
		break;
#endif
#ifdef	DVP_IF
	case INPUT_MODE_CMOS:
		rc = _cif_set_attr_cmos(dev, ctx, rx_attr);
		break;
#endif
#ifdef	BT1120_IF
	case INPUT_MODE_BT1120:
		rc = _cif_set_attr_bt1120(dev, ctx, rx_attr);
		break;
#endif
#ifdef	BT601_IF
	case INPUT_MODE_BT601_19B_VHS:
		rc = _cif_set_attr_bt601_19b_vhs(dev, ctx, rx_attr);
		break;
#endif
#ifdef	BT656_IF
	case INPUT_MODE_BT656_9B:
		rc = _cif_set_attr_bt656_9b(dev, ctx, rx_attr);
		break;
#endif
#ifdef	CUSTOM0_IF
	case INPUT_MODE_CUSTOM_0:
		rc = _cif_set_attr_custom0(dev, ctx, rx_attr);
		break;
#endif
#ifdef	BTDEMUX_IF
	case INPUT_MODE_BT_DEMUX:
		rc = _cif_set_attr_bt_demux(dev, ctx, rx_attr);
		break;
#endif
	default:
		return -EINVAL;
	}
	if (rc < 0) {
		dev_err("set attr fail %d\n", rc);
		return rc;
	}
	dev->link[attr->devno].is_on = 1;
	/* unmask the interrupts */
	cif_unmask_csi_int_sts(ctx, 0x1F);

	return 0;
}

static void cif_reset_param(struct cvi_link *link)
{
	link->is_on = 0;
	link->clk_edge = CLK_UP_EDGE;
	link->msb = OUTPUT_NORM_MSB;
	link->crop_top = 0;
	link->distance_fp = 0;
	memset(&link->param, 0, sizeof(struct cif_param));
	memset(&link->attr, 0, sizeof(struct combo_dev_attr_s));
	memset(&link->sts_csi, 0, sizeof(struct cvi_csi_status));
	memset(&link->sts_lvds, 0, sizeof(struct cvi_lvds_status));
}

static int cif_reset_mipi(struct cvi_cif_dev *dev, uint32_t devno)
{
	struct cvi_link *link = &dev->link[devno];

	/* mask the interrupts */
	if (link->is_on)
		cif_mask_csi_int_sts(&link->cif_ctx, 0x1F);

	switch (devno) {
	case 1:
		/* reset phy/mac */
		mmio_clrbits_32(0x03003008, (1 << 8) | (1 << 9));
		usleep(5);
		mmio_setbits_32(0x03003008, (1 << 8) | (1 << 9));

		/* sw reset mac by vip register */
		vip_toggle_reset((1 << VIP_SYS_REG_RST_CSI_MAC1_OFFSET));
		break;
	case 2:
		/* reset phy/mac */
		mmio_clrbits_32(0x03003008, (1 << 8) | (1 << 9));
		usleep(5);
		mmio_setbits_32(0x03003008, (1 << 8) | (1 << 9));

		/* sw reset mac by vip register */
		vip_toggle_reset((1 << VIP_SYS_REG_RST_CSI_MAC2_OFFSET));
		break;
	case 0:
		/* reset phy/mac */
		mmio_clrbits_32(0x03003008, (1 << 6) | (1 << 7));
		usleep(5);
		mmio_setbits_32(0x03003008, (1 << 6) | (1 << 7));

		/* sw reset mac by vip register */
		vip_toggle_reset((1 << VIP_SYS_REG_RST_CSI_MAC0_OFFSET));
		break;
	}

	/* reset parameters. */
	cif_reset_param(link);

	return 0;
}

static int cif_set_crop_top(struct cvi_cif_dev *dev,
			    struct crop_top_s *crop)
{
	struct cif_ctx *ctx = &dev->link[crop->devno].cif_ctx;

	dev->link[crop->devno].crop_top = crop->crop_top;
	/* strip the top info line. */
	cif_crop_info_line(ctx, crop->crop_top, crop->update);

	return 0;
}

static int cif_set_hdr_manual(struct cvi_cif_dev *dev,
			      struct manual_hdr_s *manual)
{
	struct cif_ctx *ctx = &dev->link[manual->devno].cif_ctx;
	struct cif_param *param = &dev->link[manual->devno].param;

	param->hdr_manual = manual->attr.manual_en;
	param->hdr_shift = manual->attr.l2s_distance;
	param->hdr_vsize = manual->attr.lsef_length;
	param->hdr_rm_padding = manual->attr.discard_padding_lines;

	cif_hdr_manual_config(ctx, param, !!manual->attr.update);

	return 0;
}

static int cif_get_cif_attr(struct cvi_cif_dev *dev,
			    struct cif_attr_s *cif_attr)
{
	struct cif_param *param = &dev->link[cif_attr->devno].param;

	if (param->type == CIF_TYPE_CSI && param->hdr_en) {
		struct param_csi *csi = &param->cfg.csi;

		cif_attr->stagger_vsync = (csi->hdr_mode == CSI_HDR_MODE_VC);
	} else
		cif_attr->stagger_vsync = 0;

	return 0;
}

static int cif_set_lvds_fp_vs(struct cvi_cif_dev *dev,
			      struct vsync_gen_s *vs)
{
	struct cif_ctx *ctx = &dev->link[vs->devno].cif_ctx;

	dev->link[vs->devno].distance_fp = vs->distance_fp;
	cif_set_lvds_vsync_gen(ctx, vs->distance_fp);

	return 0;
}

struct cam_pll_s {
//	uint32_t	parent_clk;
	uint8_t		cam_src;
	uint8_t		clk_div;
};

const struct cam_pll_s cam_pll_setting[CAMPLL_FREQ_NUM] = {
	[CAMPLL_FREQ_37P125M] = {
		// .parent_clk = 1188000000UL
		.cam_src = 0,
		.clk_div = 32,
	},
	[CAMPLL_FREQ_27M] = {
		// .parent_clk = 1188000000UL
		.cam_src = 0,
		.clk_div = 44,
	},
	[CAMPLL_FREQ_26M] = { // = 25.8Mhz
		// .parent_clk = 1188000000UL,
		.cam_src = 0,
		.clk_div = 46,
	},
	[CAMPLL_FREQ_25M] = { // set mipimpll_D3
		// .parent_clk = 900000000UL,
		.cam_src = 3,
		.clk_div = 12,
	},
	[CAMPLL_FREQ_24M] = {
		// .parent_clk = 792000000UL
		.cam_src = 2,
		.clk_div = 33,
	},
};

static int _cif_enable_snsr_clk(struct cvi_cif_dev *cdev,
				uint32_t devno, uint8_t on)
{
#ifndef FPGA_PORTING
	uint32_t value;

	struct cvi_link *link;
	const struct cam_pll_s *clk;

	if (devno >= MAX_LINK_NUM)
		return -EINVAL;

	link = &cdev->link[devno];
	if (link->attr.mclk.freq == CAMPLL_FREQ_NONE)
		return 0;

	switch (link->attr.mclk.freq) {
		case CAMPLL_FREQ_37P125M:
		case CAMPLL_FREQ_24M:
		case CAMPLL_FREQ_26M:
		case CAMPLL_FREQ_25M:
			clk = &cam_pll_setting[link->attr.mclk.freq];
			break;
		case CAMPLL_FREQ_27M:
		default:
			clk = &cam_pll_setting[CAMPLL_FREQ_27M];
			break;
	}

	if (mclk0 > CAMPLL_FREQ_NONE && mclk0 < CAMPLL_FREQ_NUM) {
		const struct cam_pll_s *clk = &cam_pll_setting[mclk0];

		mmio_clrsetbits_32(CLK_CAM0_SRC_DIV, REG_CAM_SRC_MASK,
			clk->cam_src << REG_CAM_SRC);
		mmio_clrsetbits_32(CLK_CAM0_SRC_DIV, REG_CAM_DIV_MASK,
			clk->clk_div << REG_CAM_DIV);

	}

	if (mclk1 > CAMPLL_FREQ_NONE && mclk1 < CAMPLL_FREQ_NUM) {
		const struct cam_pll_s *clk = &cam_pll_setting[mclk1];

		mmio_clrsetbits_32(CLK_CAM1_SRC_DIV, REG_CAM_SRC_MASK,
			clk->cam_src << REG_CAM_SRC);
		mmio_clrsetbits_32(CLK_CAM1_SRC_DIV, REG_CAM_DIV_MASK,
			clk->clk_div << REG_CAM_DIV);
	}
	udelay(100);

#else
	if (on) {
		iowrite32((ioread32(ioremap(0x0a0c8018, 0x4)) | 0x02),
				    ioremap(0x0a0c8018, 0x4));
		// #ifdef FPGA_PORTING
		iowrite32(0x32100000, ioremap(0x0A0880F8, 0x4));
		// mdelay(10);
		udelay(100);
		iowrite32(0x32100000, ioremap(0x0A0880F8, 0x4));
		// #else
		// iowrite32((ioread32(ioremap(0x0A0880F8, 0x4)) | 0x03100039),
		// 		    ioremap(0x0A0880F8, 0x4));
		// #endif
	} else
		iowrite32((ioread32(ioremap(0x0a0c8018, 0x4)) & ~0x02),
				    ioremap(0x0a0c8018, 0x4));
#endif
	return 0;
}

static int cif_enable_snsr_clk(struct cvi_cif_dev *dev,
				uint32_t devno, uint8_t on)
{
	return _cif_enable_snsr_clk(dev, devno, on);
}

static int cif_reset_snsr_gpio(struct cvi_cif_dev *dev,
			       unsigned int devno, uint8_t on)
{
	struct cvi_link *link;
	int reset;

	if (devno >= MAX_LINK_NUM)
		return -EINVAL;

	link = &dev->link[devno];
	reset = (link->snsr_rst_pol == OF_GPIO_ACTIVE_LOW) ? 0 : 1;

	if (!gpio_is_valid(link->snsr_rst_pin))
		return 0;
	if (on)
		gpio_direction_output(link->snsr_rst_pin, reset);
	else
		gpio_direction_output(link->snsr_rst_pin, !reset);

	return 0;
}

static int cif_reset_lvds(struct cvi_cif_dev *dev,
				unsigned int devno)
{
	return 0;
}

static int cif_bt_fmt_out(struct cvi_cif_dev *dev,
			  struct bt_fmt_out_s *fmt_out)
{
	struct cif_ctx *ctx = &dev->link[fmt_out->devno].cif_ctx;

	dev->link[fmt_out->devno].bt_fmt_out = fmt_out->fmt_out;
	cif_set_bt_fmt_out(ctx, fmt_out->fmt_out);

	return 0;
}

long cif_ioctl(uint32_t devno, unsigned int cmd, unsigned long arg)
{
	struct cvi_cif_dev *dev = &cif_dev[devno];

	dev_info("%s\n", _to_string_cmd(cmd));

	switch (cmd) {
	case CVI_MIPI_SET_DEV_ATTR:
	{
		struct combo_dev_attr_s *attr = (struct combo_dev_attr_s *) arg;

		return cif_set_dev_attr(dev, attr);
	}
	case CVI_MIPI_SET_OUTPUT_CLK_EDGE:
	{
		struct clk_edge_s *clk = (struct clk_edge_s *) arg;

		return cif_set_output_clk_edge(dev, clk);
	}
	case CVI_MIPI_RESET_MIPI:
		return cif_reset_mipi(dev, devno);
	case CVI_MIPI_SET_CROP_TOP:
	{
		struct crop_top_s *crop = (struct crop_top_s *) arg;

		return cif_set_crop_top(dev, crop);
	}
	case CVI_MIPI_SET_HDR_MANUAL:
	{
		struct manual_hdr_s *hdr_manu = (struct manual_hdr_s *) arg;

		return cif_set_hdr_manual(dev, hdr_manu);
	}
	case CVI_MIPI_SET_LVDS_FP_VS:
	{
		struct vsync_gen_s *vsync = (struct vsync_gen_s *) arg;

		return cif_set_lvds_fp_vs(dev, vsync);
	}
	case CVI_MIPI_RESET_SENSOR:
		return cif_reset_snsr_gpio(dev, devno, 1);
	case CVI_MIPI_UNRESET_SENSOR:
		return cif_reset_snsr_gpio(dev, devno, 0);
	case CVI_MIPI_ENABLE_SENSOR_CLOCK:
		return cif_enable_snsr_clk(dev, devno, 1);
	case CVI_MIPI_DISABLE_SENSOR_CLOCK:
		return cif_enable_snsr_clk(dev, devno, 0);
	case CVI_MIPI_RESET_LVDS:
		return cif_reset_lvds(dev, devno);
	case CVI_MIPI_GET_CIF_ATTR:
	{
		struct cif_attr_s *cif_attr = (struct cif_attr_s *) arg;

		cif_get_cif_attr(dev, cif_attr);

		return 0;
	}
	case CVI_MIPI_SET_BT_FMT_OUT:
	{
		struct bt_fmt_out_s *bt_fmt = (struct bt_fmt_out_s *) arg;

		return cif_bt_fmt_out(dev, bt_fmt);
	}
	case CVI_MIPI_SET_SENSOR_CLOCK:
	{
		struct mclk_pll_s *mclk = (struct mclk_pll_s *) arg;

		/* sensor enable */
		if (mclk->cam >= MAX_LINK_NUM)
			return -EINVAL;
		if (!mclk->cam)
			mclk0 = mclk->freq;
		else if (mclk->cam == 1)
			mclk1 = mclk->freq;

		return _cif_enable_snsr_clk(dev, mclk->cam, mclk->freq != CAMPLL_FREQ_NONE);
	}
	case CVI_MIPI_SET_MAX_MAC_CLOCK:
		dev->max_mac_clk = *(uint32_t *)arg;

		if (dev->max_mac_clk <= 400)
			dev->max_mac_clk = 400;
		else if (dev->max_mac_clk <= 500)
			dev->max_mac_clk = 500;
		else
			dev->max_mac_clk = 600;
		break;
	default:
		return -ENOIOCTLCMD;
	}
	return 0;
}

static irqreturn_t cif_isr(int irq, void *_link)
{
	struct cvi_link *link = (struct cvi_link *)_link;
	struct cif_ctx *ctx = &link->cif_ctx;

	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_ECC_ERR_MASK))
		link->sts_csi.errcnt_ecc++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_CRC_ERR_MASK))
		link->sts_csi.errcnt_crc++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_WC_ERR_MASK))
		link->sts_csi.errcnt_wc++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_HDR_ERR_MASK))
		link->sts_csi.errcnt_hdr++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_FIFO_FULL_MASK))
		link->sts_csi.fifo_full++;

	if (link->sts_csi.errcnt_ecc > 0xFFFF ||
		link->sts_csi.errcnt_crc > 0xFFFF ||
		link->sts_csi.errcnt_hdr > 0xFFFF ||
		link->sts_csi.fifo_full > 0xFFFF ||
		link->sts_csi.errcnt_wc > 0xFFFF) {

		cif_mask_csi_int_sts(ctx, 0x1F);
		dev_err("mask the interrupt since err cnt is full\n");
		dev_err("ecc = %u, crc = %u, wc = %u, hdr = %u, fifo_full = %u\n",
				link->sts_csi.errcnt_ecc,
				link->sts_csi.errcnt_crc,
				link->sts_csi.errcnt_wc,
				link->sts_csi.errcnt_hdr,
				link->sts_csi.fifo_full);
	}

	cif_clear_csi_int_sts(ctx);

	return IRQ_HANDLED;
}

int cif_open(void *param)
{
	SENSOR_INFO *pSenInfo = (SENSOR_INFO *) param;
	SENSOR_USR_CFG *pSenCfg = pSenInfo->cfg;

	uint32_t i, devno = pSenCfg->devno;
	struct cvi_cif_dev *dev = &cif_dev[devno];
	struct cvi_link *link;

	/* Init link */
	link = &dev->link[devno];
	link->snsr_rst_pin = pSenCfg->snsr_reset;
	link->snsr_rst_pol = (pSenCfg->reset_act == 0) ? OF_GPIO_ACTIVE_LOW : 0;

	/* init cif */
	for (i = 0; i < MAX_LINK_NUM; i++) {
		struct cif_ctx *ctx = &dev->link[i].cif_ctx;

		ctx->mac_phys_regs = cif_get_mac_phys_reg_bases(i);
		ctx->wrap_phys_regs = cif_get_wrap_phys_reg_bases(i);
	}

	/* Init cif base address */
	cif_set_base_addr(0, (uint32_t *)SENSOR_MAC0_BASE, (uint32_t *)DPHY_TOP_BASE);
	cif_set_base_addr(1, (uint32_t *)SENSOR_MAC1_BASE, (uint32_t *)DPHY_TOP_BASE);
	cif_set_base_addr(2, (uint32_t *)SENSOR_MAC_VI_BASE, (uint32_t *)DPHY_TOP_BASE);

	/* Init max mac clock. */
	dev->max_mac_clk = 600;

	/* Interrupt */
	if ((pSenInfo->vi_mode == INPUT_MODE_MIPI ||
		pSenInfo->vi_mode == INPUT_MODE_SUBLVDS ||
		pSenInfo->vi_mode == INPUT_MODE_HISPI) &&
		devno < CIF_MAX_CSI_NUM)
	{
		if (!devno) {
			/* Interrupt */
			link->irq_num = CSIMAC0_INTR_NUM;
			request_irq(link->irq_num, cif_isr, 0, "csi0 intr", link);

			/* sw reset */
			// link->phy_reset = devm_reset_control_get(&pdev->dev, "phy0");
			// if (link->phy_reset)
			// 	dev_info(&pdev->dev, "phy0 reset installed\n");
			// link->phy_apb_reset = devm_reset_control_get(&pdev->dev, "phy-apb0");
			// if (link->phy_apb_reset)
			// 	dev_info(&pdev->dev, "phy0 apb reset installed\n");
		} else {
			/* Interrupt */
			link->irq_num = CSIMAC1_INTR_NUM;
			request_irq(link->irq_num, cif_isr, 0, "csi1 intr", link);

			/* sw reset */
			// link->phy_reset = devm_reset_control_get(&pdev->dev, "phy0");
			// if (link->phy_reset)
			// 	dev_info(&pdev->dev, "phy0 reset installed\n");
			// link->phy_apb_reset = devm_reset_control_get(&pdev->dev, "phy-apb0");
			// if (link->phy_apb_reset)
			// 	dev_info(&pdev->dev, "phy0 apb reset installed\n");
		}
		/* set the port id */
		link->cif_ctx.mac_num = devno;
	}

	return 0;
}

int cif_release()
{
	// un-request irq0 here
	disable_irq(CSIMAC0_INTR_NUM);
	disable_irq(CSIMAC1_INTR_NUM);

	return 0;
}
