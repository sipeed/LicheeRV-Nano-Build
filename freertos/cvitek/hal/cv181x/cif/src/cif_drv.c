// #include <linux/types.h>
// #include <linux/delay.h>
// #include "reg.h"
#include <stdint.h>
#include <string.h>

#include "mmio.h"
#include "delay.h"
#include "cif_reg.h"
#include "cif_drv.h"

/* SubLVDS Normal Sync code */
#define SLVDS_SYNC_CODE_1ST		0xFFFu
#define SLVDS_SYNC_CODE_2ND		0u
#define SLVDS_SYNC_CODE_3RD		0u
#define SLVDS_SYNC_CODE_NORM_BK_SAV	0xAB0u
#define SLVDS_SYNC_CODE_NORM_BK_EAV	0xB60u
#define SLVDS_SYNC_CODE_NORM_SAV	0x800u
#define SLVDS_SYNC_CODE_NORM_EAV	0x9D0u
#define SLVDS_SYNC_CODE_N0_BK_SAV	0x2B0u
#define SLVDS_SYNC_CODE_N0_BK_EAV	0x360u
#define SLVDS_SYNC_CODE_N1_BK_SAV	0x6B0u
#define SLVDS_SYNC_CODE_N1_BK_EAV	0x760u

/* 10 bit SubLVDS HDR-realted Sync code */
#define SLVDS_SYNC_CODE_N0_LEF_SAV	0x004u
#define SLVDS_SYNC_CODE_N0_LEF_EAV	0x1D4u
#define SLVDS_SYNC_CODE_N0_SEF_SAV	0x008u
#define SLVDS_SYNC_CODE_N0_SEF_EAV	0x1D8u
#define SLVDS_SYNC_CODE_N1_LEF_SAV	0x404u
#define SLVDS_SYNC_CODE_N1_LEF_EAV	0x5D4u
#define SLVDS_SYNC_CODE_N1_SEF_SAV	0x408u
#define SLVDS_SYNC_CODE_N1_SEF_EAV	0x5D8u
#define SLVDS_SYNC_CODE_N0_LSEF_SAV	0x00Cu
#define SLVDS_SYNC_CODE_N0_LSEF_EAV	0x1DCu
#define SLVDS_SYNC_CODE_N1_LSEF_SAV	0x40Cu
#define SLVDS_SYNC_CODE_N1_LSEF_EAV	0x5DCu

/* HiSPi PKT-SP HDR-realted Sync code */
#define HISPI_SYNC_CODE_T1_SOL		0x800u
#define HISPI_SYNC_CODE_T1_EOL		0xA00u
#define HISPI_SYNC_CODE_T2_SOL		0x820u
#define HISPI_SYNC_CODE_T2_EOL		0xA20u
#define HISPI_SYNC_CODE_T1_SOF		0xC00u
#define HISPI_SYNC_CODE_T1_EOF		0xE00u
#define HISPI_SYNC_CODE_T2_SOF		0xC20u
#define HISPI_SYNC_CODE_T2_EOF		0xE20u
#define HISPI_SYNC_CODE_VSYNC_GEN	0xC00u

/****************************************************************************
 * Global parameters
 ****************************************************************************/
static uintptr_t mac_reg_base[MAX_LINK_NUM];
static uintptr_t wrap_reg_base[MAX_LINK_NUM];

/****************************************************************************
 * Interfaces
 ****************************************************************************/
void cif_set_base_addr(uint32_t link, void *mac_base, void *wrap_base)
{
	uintptr_t *addr = cif_get_mac_phys_reg_bases(link);
	int i = 0;

	for (i = 0; i < CIF_MAC_BLK_ID_MAX; ++i) {
		addr[i] -= mac_reg_base[link];
		addr[i] += (uintptr_t)mac_base;
	}
	mac_reg_base[link] = (uintptr_t)mac_base;

	addr = cif_get_wrap_phys_reg_bases(link);

	for (i = 0; i < CIF_WRAP_BLK_ID_MAX; ++i) {
		addr[i] -= wrap_reg_base[link];
		addr[i] += (uintptr_t)wrap_base;
	}
	wrap_reg_base[link] = (uintptr_t)wrap_base;
}

void cif_init(struct cif_ctx *ctx)
{
}

void cif_uninit(struct cif_ctx *ctx)
{
}

void cif_reset(struct cif_ctx *ctx)
{
}

static void _cif_sublvds_config(struct cif_ctx *ctx,
			       struct param_sublvds *param)
{
	uintptr_t wrap_4l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L];
	uintptr_t wrap_2l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L];
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];
	uintptr_t mac_slvds = ctx->mac_phys_regs[CIF_MAC_BLK_ID_SLVDS];
	struct sync_code_s *sc = &param->sync_code;

	/* Config the sync code */
	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_18, SLVDS_N0_LEF_SAV,
			       sc->slvds.n0_lef_sav);
	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_1C, SLVDS_N0_LEF_EAV,
			       sc->slvds.n0_lef_eav);
	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_1C, SLVDS_N0_SEF_SAV,
			       sc->slvds.n0_sef_sav);
	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_20, SLVDS_N0_SEF_EAV,
			       sc->slvds.n0_sef_eav);
	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_20, SLVDS_N1_LEF_SAV,
			       sc->slvds.n1_lef_sav);
	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_24, SLVDS_N1_LEF_EAV,
			       sc->slvds.n1_lef_eav);
	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_24, SLVDS_N1_SEF_SAV,
			       sc->slvds.n1_sef_sav);
	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_28, SLVDS_N1_SEF_EAV,
			       sc->slvds.n1_sef_eav);
	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_50, SLVDS_N0_LSEF_SAV,
			       sc->slvds.n0_lsef_sav);
	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_50, SLVDS_N0_LSEF_EAV,
			       sc->slvds.n0_lsef_eav);
	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_54, SLVDS_N1_LSEF_SAV,
			       sc->slvds.n1_lsef_sav);
	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_54, SLVDS_N1_LSEF_EAV,
			       sc->slvds.n1_lsef_eav);

	/* Config the sensor mode */
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_00, SENSOR_MAC_MODE,
			     2);
	/* invert the HS/VS/HDR */
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_00, SUBLVDS_VS_INV,
			     1);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_00, SUBLVDS_HS_INV,
			     1);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_00, SUBLVDS_HDR_INV,
			     1);
	/* subLVDS controller enable */
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_00, SUBLVDS_CTRL_ENABLE,
			     1);
	/* disable HiSPi mode */
	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_60, HISPI_MODE,
			       0);
	/* Config the lane enable */
	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_00, SLVDS_ENABLE,
			       (1 << param->lane_num) - 1);
	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_30, SLVDS_LANE_MODE,
			       param->lane_num - 1);
	/* Config the raw format. */
	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_00, SLVDS_BIT_MODE,
			       param->fmt);
	/* Config the endian. */
	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_00, SLVDS_DATA_REVERSE,
			       param->endian == CIF_SLVDS_ENDIAN_LSB);
	if (!ctx->mac_num) {
		/* DPHY sensor mode select */
		CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				     REG_00, SENSOR_MODE,
				     1);
		/* DPHY bit mode select */
		CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				     REG_20, SLVDS_BIT_MODE,
				     param->fmt);
		/* DPHY endian mode select */
		CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				     REG_20, SLVDS_INV_EN,
				     param->wrap_endian == CIF_SLVDS_ENDIAN_MSB);
	} else {
		/* DPHY sensor mode select */
		CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				     REG_00, SENSOR_MODE,
				     1);
		/* DPHY bit mode select */
		CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				     REG_20, SLVDS_BIT_MODE,
				     param->fmt);
		/* DPHY endian mode select */
		CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				     REG_20, SLVDS_INV_EN,
				     param->wrap_endian == CIF_SLVDS_ENDIAN_MSB);
	}
}

static void _cif_hispi_config(struct cif_ctx *ctx,
			     struct param_hispi *param)
{
	uintptr_t wrap_4l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L];
	uintptr_t wrap_2l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L];
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];
	uintptr_t mac_slvds = ctx->mac_phys_regs[CIF_MAC_BLK_ID_SLVDS];

	/* Config the sensor mode */
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_00, SENSOR_MAC_MODE,
			     2);
	/* invert the HS/VS */
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_00, SUBLVDS_VS_INV,
			     1);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_00, SUBLVDS_HS_INV,
			     1);
	/* subLVDS controller enable */
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_00, SUBLVDS_CTRL_ENABLE,
			     1);
	/* Config the raw format. */
	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_00, SLVDS_BIT_MODE,
			       param->fmt);
	/* Enable HiSPi mode */
	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_60, HISPI_MODE,
			       1);
	/* Config the lane enable */
	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_00, SLVDS_ENABLE,
			       (1 << param->lane_num) - 1);
	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_30, SLVDS_LANE_MODE,
			       param->lane_num - 1);
	/* Config the endian. */
	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_00, SLVDS_DATA_REVERSE,
			       param->endian);
	/* Config the HiSPi mode*/
	if (param->mode == CIF_HISPI_MODE_PKT_SP) {
		CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
				       REG_60, HISPI_USE_HSIZE,
				       0);
	} else {
		CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
				       REG_58, SLVDS_HDR_P2_HSIZE,
				       param->h_size/param->lane_num);
		CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
				       REG_60, HISPI_USE_HSIZE,
				       1);
	}
	if (!ctx->mac_num) {
		/* DPHY sensor mode select */
		CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				     REG_00, SENSOR_MODE,
				     1);
		/* DPHY bit mode select */
		CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				     REG_20, SLVDS_BIT_MODE,
				     param->fmt);
		/* DPHY endian mode select */
		CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				     REG_20, SLVDS_INV_EN,
				     param->wrap_endian == CIF_SLVDS_ENDIAN_MSB);
	} else {
		/* DPHY sensor mode select */
		CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				     REG_00, SENSOR_MODE,
				     1);
		/* DPHY bit mode select */
		CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				     REG_20, SLVDS_BIT_MODE,
				     param->fmt);
		/* DPHY endian mode select */
		CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				     REG_20, SLVDS_INV_EN,
				     param->wrap_endian == CIF_SLVDS_ENDIAN_MSB);
	}
}

static void _cif_ttl_config(struct cif_ctx *ctx,
			   struct param_ttl *param)
{
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];

	/* Config the sensor mode */
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_00, SENSOR_MAC_MODE,
			     3);
	/* Config TTL sensor format */
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_10, TTL_SENSOR_BIT,
			     param->sensor_fmt);
	/* Config TTL clock invert */
	if (param->vi_from == FROM_VI0) {
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_30, PAD_VI0_CLK_INV,
				     param->clk_inv);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_30, VI_FROM,
				     0);
	} else if (param->vi_from == FROM_VI1) {
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_30, PAD_VI1_CLK_INV,
				     param->clk_inv);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_30, VI_FROM,
				     1);
	} else {
		return;
	}

	switch (param->fmt) {
	case TTL_SYNC_PAT_17B_BT1120:
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_10, TTL_FMT_IN,
				     TTL_SYNC_PAT_17B_BT1120);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_10, TTL_BT_FMT_OUT,
				     param->fmt_out);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_14, TTL_VS_BP,
				     param->v_bp);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_14, TTL_HS_BP,
				     param->h_bp);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_18, TTL_IMG_WD,
				     param->width);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_18, TTL_IMG_HT,
				     param->height);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_1C, TTL_SYNC_0,
				     0xFFFF);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_1C, TTL_SYNC_1,
				     0);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_20, TTL_SYNC_2,
				     0);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_24, TTL_SAV_VLD,
				     0x8000);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_24, TTL_SAV_BLK,
				     0xab00);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_28, TTL_EAV_VLD,
				     0x9d00);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_28, TTL_EAV_BLK,
				     0xb600);

		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_30, VI_SEL,
				     param->vi_sel);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_30, VI_V_SEL_VS,
				     1);
		break;
	case TTL_VHS_19B_BT601:
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_10, TTL_FMT_IN,
				     TTL_VHS_19B_BT601);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_10, TTL_BT_FMT_OUT,
				     param->fmt_out);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_10, TTL_VS_INV,
				     1);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_10, TTL_HS_INV,
				     1);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_14, TTL_VS_BP,
				     param->v_bp);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_14, TTL_HS_BP,
				     param->h_bp);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_18, TTL_IMG_WD,
				     param->width);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_18, TTL_IMG_HT,
				     param->height);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_1C, TTL_SYNC_0,
				     0xFFFF);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_30, VI_SEL,
				     param->vi_sel);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_30, VI_V_SEL_VS,
				     1);
		break;
	case TTL_SYNC_PAT_9B_BT656:
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_10, TTL_FMT_IN,
				     TTL_SYNC_PAT_9B_BT656);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_10, TTL_BT_FMT_OUT,
				     param->fmt_out);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_14, TTL_VS_BP,
				     param->v_bp);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_14, TTL_HS_BP,
				     param->h_bp);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_18, TTL_IMG_WD,
				     param->width);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_18, TTL_IMG_HT,
				     param->height);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_1C, TTL_SYNC_0,
				     0xFFFF);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_1C, TTL_SYNC_1,
				     0x0);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_20, TTL_SYNC_2,
				     0x0);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_24, TTL_SAV_VLD,
				     0x8000);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_24, TTL_SAV_BLK,
				     0xAB00);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_28, TTL_EAV_VLD,
				     0x9D00);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_28, TTL_EAV_BLK,
				     0xB600);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_30, VI_SEL,
				     param->vi_sel);
		break;
	case TTL_CUSTOM_0:
		/* Config TTL format */
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_10, TTL_FMT_IN,
				     TTL_VHS_19B_BT601);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_10, TTL_BT_FMT_OUT,
				     param->fmt_out);
		/* Config TTL format */
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_10, TTL_BT_FMT_OUT,
				     TTL_BT_FMT_OUT_CBYCRY);
		/* Config HV inverse */
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_10, TTL_VS_INV,
				     1);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_10, TTL_HS_INV,
				     1);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_14, TTL_VS_BP,
				     param->v_bp);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_14, TTL_HS_BP,
				     param->h_bp);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_18, TTL_IMG_WD,
				     param->width);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_18, TTL_IMG_HT,
				     param->height);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_1C, TTL_SYNC_0,
				     0xFFFF);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_1C, TTL_SYNC_1,
				     0);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_20, TTL_SYNC_2,
				     0);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_24, TTL_SAV_VLD,
				     0x8000);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_24, TTL_SAV_BLK,
				     0xab00);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_28, TTL_EAV_VLD,
				     0x9d00);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_28, TTL_EAV_BLK,
				     0xb600);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_30, VI_SEL,
				     param->vi_sel);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_30, VI_V_SEL_VS,
				     1);
		break;
	default:
		/* Config TTL format */
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_10, TTL_FMT_IN,
				     param->fmt);
		break;
	}
}

static void _cif_btdemux_config(struct cif_ctx *ctx,
				struct param_btdemux *param)
{
	uintptr_t mac_top = (ctx->mac_num != 2) ? ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP] :
				ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];

	/* Config the sensor mode */
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_00, BT_DEMUX_ENABLE,
			     1);

	/* config demux channels */
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_80, BT_DEMUX_CH,
			     param->demux);
	/* config bt fp  */
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_90, BT_VS_FP_M1,
			     param->v_fp);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_90, BT_HS_FP_M1,
			     param->h_fp);
	/* config bt bp  */
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_8C, BT_VS_BP_M1,
			     param->v_bp);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_8C, BT_HS_BP_M1,
			     param->h_bp);
	/* config bt sync code  */
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_94, BT_SYNC_0,
			     param->sync_code_part_A[0]);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_94, BT_SYNC_1,
			     param->sync_code_part_A[1]);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_94, BT_SYNC_2,
			     param->sync_code_part_A[2]);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_98, BT_SAV_VLD_0,
			     param->sync_code_part_B[0].sav_vld);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_98, BT_SAV_BLK_0,
			     param->sync_code_part_B[0].sav_blk);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_98, BT_EAV_VLD_0,
			     param->sync_code_part_B[0].eav_vld);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_98, BT_EAV_BLK_0,
			     param->sync_code_part_B[0].eav_blk);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_9C, BT_SAV_VLD_1,
			     param->sync_code_part_B[1].sav_vld);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_9C, BT_SAV_BLK_1,
			     param->sync_code_part_B[1].sav_blk);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_9C, BT_EAV_VLD_1,
			     param->sync_code_part_B[1].eav_vld);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_9C, BT_EAV_BLK_1,
			     param->sync_code_part_B[1].eav_blk);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_A0, BT_SAV_VLD_2,
			     param->sync_code_part_B[2].sav_vld);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_A0, BT_SAV_BLK_2,
			     param->sync_code_part_B[2].sav_blk);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_A0, BT_EAV_VLD_2,
			     param->sync_code_part_B[2].eav_vld);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_A0, BT_EAV_BLK_2,
			     param->sync_code_part_B[2].eav_blk);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_A4, BT_SAV_VLD_3,
			     param->sync_code_part_B[3].sav_vld);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_A4, BT_SAV_BLK_3,
			     param->sync_code_part_B[3].sav_blk);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_A4, BT_EAV_VLD_3,
			     param->sync_code_part_B[3].eav_vld);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_A4, BT_EAV_BLK_3,
			     param->sync_code_part_B[3].eav_blk);
	/* config bt clk inv  */
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_30, PAD_VI2_CLK_INV,
			     param->clk_inv);
	/* config bt yc exchange  */
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_A8, BT_YC_INV,
			     param->yc_exchg);
	/* config bt image size  */
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_88, BT_IMG_WD_M1,
			     param->width);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_88, BT_IMG_HT_M1,
			     param->height);
}

static void _cif_csi_config(struct cif_ctx *ctx,
			   struct param_csi *param)
{
	uintptr_t wrap_4l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L];
	uintptr_t wrap_2l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L];
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];

	/* Config the sensor mode */
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_00, SENSOR_MAC_MODE,
			     1);
	/* invert the HS/VS */
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_00, CSI_VS_INV,
			     1);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_00, CSI_HS_INV,
			     1);
	/* CSI controller enable */
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_00, CSI_CTRL_ENABLE,
			     1);
	/* Config the lane enable */
	CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
			     REG_00, CSI_LANE_MODE,
			     param->lane_num - 1);
	/* Config the VS gen mode */
	CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
			     REG_70, CSI_VS_GEN_MODE,
			     param->vs_gen_mode);
	if (!ctx->mac_num) {
		/* [Note] disable auto_ignore and auto sync by default. */
		CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				     REG_10, AUTO_IGNORE,
				     0);
		CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				     REG_10, AUTO_SYNC,
				     0);
		/* DPHY sensor mode select */
		CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				     REG_00, SENSOR_MODE,
				     0);
	} else {
		/* [Note] disable auto_ignore and auto sync by default. */
		CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				     REG_10, AUTO_IGNORE,
				     0);
		CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				     REG_10, AUTO_SYNC,
				     0);
		/* DPHY sensor mode select */
		CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				     REG_00, SENSOR_MODE,
				     0);
	}
	/* Config csi vc mapping */
	CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
			     REG_18, CSI_VC_MAP_CH00,
			     param->vc_mapping[0]);
	CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
			     REG_18, CSI_VC_MAP_CH01,
			     param->vc_mapping[1]);
	CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
			     REG_18, CSI_VC_MAP_CH10,
			     param->vc_mapping[2]);
	CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
			     REG_18, CSI_VC_MAP_CH11,
			     param->vc_mapping[3]);
}

void cif_crop_info_line(struct cif_ctx *ctx, uint32_t line_num, uint32_t sw_up)
{
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];

	/* Config the info line strip for HDR pattern 2 */
	if (line_num) {
		CIF_WR_BITS_GRP2(mac_top, REG_SENSOR_MAC_T,
				     REG_48,
				     SENSOR_MAC_INFO_LINE_NUM,
				     line_num,
				     SENSOR_MAC_RM_INFO_LINE,
				     1);

		if (sw_up) {
			CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
					     REG_00, SW_UP,
					     1);
			CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
					     REG_00, SW_UP,
					     1);
		}
	} else {
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_48,
				     SENSOR_MAC_RM_INFO_LINE,
				     0);
	}
}

void cif_set_bt_fmt_out(struct cif_ctx *ctx, enum ttl_bt_fmt_out fmt_out)
{
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];

	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_10, TTL_BT_FMT_OUT,
			     fmt_out);
}

void cif_set_ttl_pinmux(struct cif_ctx *ctx,
			enum ttl_vi_from_e vi, enum ttl_vi_func_e func, uint32_t pad)
{
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];
	uint32_t offset;
	uint32_t value;
	uint32_t reg, mask;

	if (vi == FROM_VI2) {
		offset = (func - VI_FUNC_D0) * 4;
		value = pad << offset;
		mask = 0x7 << offset;
		reg = CIF_RD_REG(mac_top, REG_SENSOR_MAC_T, REG_74);
		reg &= ~mask;
		reg |= value;
		CIF_WR_REG(mac_top, REG_SENSOR_MAC_T,
				    REG_74,
				    reg);
		return;
	}

	offset = (func & 0x3) * 8;
	value = (vi << 5 | pad) << offset;
	mask = 0x3f << offset;

	if (func <= VI_FUNC_HDE) {
		reg = CIF_RD_REG(mac_top, REG_SENSOR_MAC_T, REG_60);
		reg &= ~mask;
		reg |= value;
		CIF_WR_REG(mac_top, REG_SENSOR_MAC_T,
				    REG_60,
				    reg);
	} else if (func <= VI_FUNC_D3) {
		reg = CIF_RD_REG(mac_top, REG_SENSOR_MAC_T, REG_64);
		reg &= ~mask;
		reg |= value;
		CIF_WR_REG(mac_top, REG_SENSOR_MAC_T,
				    REG_64,
				    reg);
	} else if (func <= VI_FUNC_D7) {
		reg = CIF_RD_REG(mac_top, REG_SENSOR_MAC_T, REG_68);
		reg &= ~mask;
		reg |= value;
		CIF_WR_REG(mac_top, REG_SENSOR_MAC_T,
				    REG_68,
				    reg);
	} else if (func <= VI_FUNC_D11) {
		reg = CIF_RD_REG(mac_top, REG_SENSOR_MAC_T, REG_6C);
		reg &= ~mask;
		reg |= value;
		CIF_WR_REG(mac_top, REG_SENSOR_MAC_T,
				    REG_6C,
				    reg);
	} else {
		reg = CIF_RD_REG(mac_top, REG_SENSOR_MAC_T, REG_70);
		reg &= ~mask;
		reg |= value;
		CIF_WR_REG(mac_top, REG_SENSOR_MAC_T,
				    REG_70,
				    reg);
	}
}

void cif_config(struct cif_ctx *ctx, struct cif_param *param)
{
	switch (param->type) {
	case CIF_TYPE_CSI:
		_cif_csi_config(ctx, &param->cfg.csi);
		break;
	case CIF_TYPE_SUBLVDS:
		_cif_sublvds_config(ctx, &param->cfg.sublvds);
		break;
	case CIF_TYPE_HISPI:
		_cif_hispi_config(ctx, &param->cfg.hispi);
		break;
	case CIF_TYPE_TTL:
		_cif_ttl_config(ctx, &param->cfg.ttl);
		break;
	case CIF_TYPE_BT_DMUX:
		_cif_btdemux_config(ctx, &param->cfg.btdemux);
		break;
	default:
		break;
	}
}

static void _cif_hdr_sublvds_enable(struct cif_ctx *ctx,
				    struct param_sublvds *param,
				    uint32_t on)
{
	uintptr_t mac_slvds = ctx->mac_phys_regs[CIF_MAC_BLK_ID_SLVDS];

	if (!on) {
		/*[TODO] V FP recovery? info line strip? */
		CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
				       REG_00, SLVDS_HDR_MODE,
				       0);
		return;
	}

	/* Config the HDR. */
	switch (param->hdr_mode) {
	case CIF_SLVDS_HDR_PAT1:
		/* Config the sync code if raw 10, default is raw 12*/
		/* Select the HDR pattern */
		CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
				       REG_00, SLVDS_HDR_PATTERN,
				       param->hdr_mode>>1);
		break;
	case CIF_SLVDS_HDR_PAT2:
		/* Config the HSIZE and HBlank per lane */
		CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
				       REG_58, SLVDS_HDR_P2_HSIZE,
				       param->h_size/param->lane_num);
		CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
				       REG_58, SLVDS_HDR_P2_HBLANK,
				       param->hdr_hblank[0]/param->lane_num);
		/* Select the HDR pattern */
		CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
				       REG_00, SLVDS_HDR_PATTERN,
				       param->hdr_mode>>1);
		break;
	}

	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_00, SLVDS_HDR_MODE,
			       1);
}

static void _cif_hdr_csi_enable(struct cif_ctx *ctx,
				 struct param_csi *param,
				 uint32_t on)
{
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];

	if (param->hdr_mode == CSI_HDR_MODE_VC) {
		CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
				     REG_04, CSI_HDR_MODE,
				     0);
	} else if (param->hdr_mode == CSI_HDR_MODE_DT) {
		/* Enable dtat type mode. */
		CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
				     REG_74, CSI_HDR_DT_MODE,
				     1);
		/* Program lef data type. */
		CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
				     REG_74, CSI_HDR_DT_LEF,
				     param->data_type[0]);
		/* Program sef data type. */
		CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
				     REG_74, CSI_HDR_DT_SEF,
				     param->data_type[1]);
		/* Program decode data type. */
		CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
				     REG_74, CSI_HDR_DT_FORMAT,
				     param->decode_type);
	} else if (param->hdr_mode == CSI_HDR_MODE_DOL) {
		/* Enable Sony DOL mode. */
		CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
				     REG_04, CSI_HDR_MODE,
				     1);
		CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
				     REG_04, CSI_ID_RM_ELSE,
				     1);
		CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
				     REG_04, CSI_ID_RM_OB,
				     1);
	} else {
		/* [TODO] */
		CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
				     REG_04, CSI_HDR_MODE,
				     1);
	}
	/* CV181X&CV180X not support invert the HDR */
	// CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
	//		     REG_00, CSI_HDR_INV,
	//		     1);
	CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
			     REG_04, CSI_HDR_EN,
			     !!on);
}

static void _cif_hdr_hispi_enable(struct cif_ctx *ctx,
				  struct param_hispi *param,
				  uint32_t on)
{
	uintptr_t mac_slvds = ctx->mac_phys_regs[CIF_MAC_BLK_ID_SLVDS];

	if (param->mode == CIF_HISPI_MODE_PKT_SP) {
		CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
				       REG_60, HISPI_HDR_PSP_MODE,
				       !!on);

	}
}

void cif_hdr_manual_config(struct cif_ctx *ctx,
			   struct cif_param *param,
			   uint32_t sw_up)
{
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];

	if (!param->hdr_manual) {
		CIF_WR_BITS_GRP3(mac_top, REG_SENSOR_MAC_T,
				 REG_40,
				 SENSOR_MAC_HDR_EN,
				 0,
				 SENSOR_MAC_HDR_HDR0INV, // to-do
				 0,
				 SENSOR_MAC_HDR_HDR1INV, // to-do
				 0);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				 REG_40, SENSOR_MAC_HDR_MODE,
				 0);

		if (sw_up) {
			CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
					     REG_00, SW_UP,
					     1);
		}
		return;
	}

	/* Config the HDR mode V size and T2 line shift */
	CIF_WR_BITS_GRP2(mac_top, REG_SENSOR_MAC_T,
			 REG_44,
			 SENSOR_MAC_HDR_VSIZE,
			 param->hdr_vsize,
			 SENSOR_MAC_HDR_SHIFT,
			 param->hdr_shift);

	CIF_WR_BITS_GRP3(mac_top, REG_SENSOR_MAC_T,
			 REG_40,
			 SENSOR_MAC_HDR_EN,
			 1,
			 SENSOR_MAC_HDR_HDR0INV, // to-do
			 0,
			 SENSOR_MAC_HDR_HDR1INV, // to-do
			 0);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			 REG_40, SENSOR_MAC_HDR_MODE,
			 !!param->hdr_rm_padding);

	if (sw_up) {
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_00, SW_UP,
				     1);
	}
}
EXPORT_SYMBOL_GPL(cif_hdr_manual_config);

void cif_hdr_enable(struct cif_ctx *ctx, struct cif_param *param, uint32_t on)
{
	if (param->hdr_manual)
		return;

	switch (param->type) {
	case CIF_TYPE_CSI:
		_cif_hdr_csi_enable(ctx, &param->cfg.csi, on);
		break;
	case CIF_TYPE_SUBLVDS:
		_cif_hdr_sublvds_enable(ctx, &param->cfg.sublvds, on);
		break;
	case CIF_TYPE_HISPI:
		_cif_hdr_hispi_enable(ctx, &param->cfg.hispi, on);
		break;
	default:
		break;
	}

}

static void cif_stream_enable(struct cif_ctx *ctx,
			      struct cif_param *param, uint32_t on)
{
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];
	uintptr_t wrap_4l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L];
	uintptr_t wrap_2l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L];
	union cif_cfg *cfg = &param->cfg;

	/* configure the phy termination only for serdes format. */
	if (param->type <= CIF_TYPE_HISPI) {
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			    REG_00, MIPIRX_PD_IBIAS, 1);
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			    REG_00, MIPIRX_PD_RXLP, 0x3F);
		if (on) {
			CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
				    REG_00, MIPIRX_PD_IBIAS, 0);
		}
	}
	/* enable mac debug */
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_BC, SENSOR_MAC_DBG_EN,
			     1);

	switch (param->type) {
	case CIF_TYPE_CSI:
		/* clear the lane enable */
		if (!ctx->mac_num) {
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				    REG_0C, DESKEW_LANE_EN, 0);
		} else {
			CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				    REG_0C, DESKEW_LANE_EN, 0);
		}
		if (on) {
			CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
				    REG_00, MIPIRX_PD_RXLP, 0);
			udelay(20);
			/* lane enable */
			if (!ctx->mac_num) {
				CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
					    REG_0C, DESKEW_LANE_EN,
					    (1 << cfg->csi.lane_num) - 1);
			} else {
				CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
					    REG_0C, DESKEW_LANE_EN,
					    (1 << cfg->csi.lane_num) - 1);
			}
		}
		break;
	case CIF_TYPE_SUBLVDS:
		/* clear the lane enable */
		if (!ctx->mac_num) {
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				    REG_20, SLVDS_LANE_EN, 0);
		} else {
			CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				    REG_20, SLVDS_LANE_EN, 0);
		}
		if (on) {
			udelay(20);
			/* lane enable */
			if (!ctx->mac_num) {
				CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
					    REG_20, SLVDS_LANE_EN,
					    (1 << cfg->sublvds.lane_num) - 1);
			} else {
				CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
					    REG_20, SLVDS_LANE_EN,
					    (1 << cfg->sublvds.lane_num) - 1);
			}
		}
		break;
	case CIF_TYPE_HISPI:
		/* clear the lane enable */
		if (!ctx->mac_num) {
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				    REG_20, SLVDS_LANE_EN, 0);
		} else {
			CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				    REG_20, SLVDS_LANE_EN, 0);
		}
		if (on) {
			udelay(20);
			/* lane enable */
			if (!ctx->mac_num) {
				CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
					    REG_20, SLVDS_LANE_EN,
					    (1 << cfg->hispi.lane_num) - 1);
			} else {
				CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
					    REG_20, SLVDS_LANE_EN,
					    (1 << cfg->hispi.lane_num) - 1);
			}
		}
		break;
	case CIF_TYPE_TTL:
		/* Enable TTL */
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			    REG_10, TTL_IP_EN,
			    !!on);
		break;
	case CIF_TYPE_BT_DMUX:
		/* Enable BT DEMUX */
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_80, BT_IP_EN,
				     1);
		break;
	default:
		break;
	}

}

void cif_streaming(struct cif_ctx *ctx, uint32_t on, uint32_t hdr)
{
	/* CIF OFF */
	cif_stream_enable(ctx, ctx->cur_config, 0);

	if (on) {
		cif_config(ctx, ctx->cur_config);
		cif_hdr_enable(ctx, ctx->cur_config, hdr);
		/* CIF ON */
		cif_stream_enable(ctx, ctx->cur_config, on);
	}
}

void cif_set_lane_id(struct cif_ctx *ctx, enum lane_id_e lane,
			uint32_t select, uint32_t pn_swap)
{
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];
	uintptr_t wrap_4l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L];
	uintptr_t wrap_2l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L];

	switch (lane) {
	case CIF_LANE_CLK:
		if (!ctx->mac_num) {
			/* PHYA clock select */

			CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
				      REG_04, MIPIRX_SEL_CLK_CHANNEL,
				      CIF_RD_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
						REG_04, MIPIRX_SEL_CLK_CHANNEL) | 1 << select);
			/* PHYD clock select */
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				      REG_08, CSI_LANE_CK_SEL,
				      select);
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				      REG_08, CSI_LANE_CK_PNSWAP,
				      pn_swap);
		} else {
			/* Enable dual mode */
			CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
				      REG_30, SENSOR_PHY_MODE,
				      1);
			/* PHYA clock select */
			CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
				      REG_04, MIPIRX_SEL_CLK_CHANNEL,
				      CIF_RD_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
						REG_04, MIPIRX_SEL_CLK_CHANNEL) | 1 << select);
			/* PHYD clock select */
			CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				      REG_08, CSI_LANE_CK_SEL,
				      select);
			CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				      REG_08, CSI_LANE_CK_PNSWAP,
				      pn_swap);
		}
		break;
	case CIF_LANE_0:
		if (!ctx->mac_num) {
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				      REG_04, CSI_LANE_D0_SEL,
				      select);
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				      REG_08, CSI_LANE_D0_PNSWAP,
				      pn_swap);
		} else {
			CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				      REG_04, CSI_LANE_D0_SEL,
				      select % 3);
			CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				      REG_08, CSI_LANE_D0_PNSWAP,
				      pn_swap);
		}
		break;
	case CIF_LANE_1:
		if (!ctx->mac_num) {
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				      REG_04, CSI_LANE_D1_SEL,
				      select);
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				      REG_08, CSI_LANE_D1_PNSWAP,
				      pn_swap);
		} else {
			CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				      REG_04, CSI_LANE_D1_SEL,
				      select % 3);
			CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				      REG_08, CSI_LANE_D1_PNSWAP,
				      pn_swap);
		}
		break;
	case CIF_LANE_2:
		if (!ctx->mac_num) {
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				      REG_04, CSI_LANE_D2_SEL,
				      select);
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				      REG_08, CSI_LANE_D2_PNSWAP,
				      pn_swap);
		}
		break;
	case CIF_LANE_3:
		if (!ctx->mac_num) {
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				      REG_04, CSI_LANE_D3_SEL,
				      select);
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				      REG_08, CSI_LANE_D3_PNSWAP,
				      pn_swap);
		}
		break;
	default:
		break;
	}
}

// to-do: check with RD
void cif_set_clk_edge(struct cif_ctx *ctx,
		      enum phy_lane_id_e lane, enum cif_clk_edge_e edge)
{
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];

	switch (lane) {
	case CIF_PHY_LANE_0:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, AD_D0_CLK_INV,
			      edge);
		break;
	case CIF_PHY_LANE_1:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, AD_D1_CLK_INV,
			      edge);
		break;
	case CIF_PHY_LANE_2:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, AD_D2_CLK_INV,
			      edge);
		break;
	case CIF_PHY_LANE_3:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, AD_D3_CLK_INV,
			      edge);
		break;
	case CIF_PHY_LANE_4:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, AD_D4_CLK_INV,
			      edge);
		break;
	case CIF_PHY_LANE_5:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, AD_D5_CLK_INV,
			      edge);
		break;
	default:
		break;
	}
}

void cif_set_clk_dir(struct cif_ctx *ctx, enum cif_clk_dir_e dir)
{
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];

	switch (dir) {
	case CIF_CLK_P02P1:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_04, MIPIRX_SEL_CLK_P1TOP0,
			      0);
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_04, MIPIRX_SEL_CLK_P0TOP1,
			      1);
		break;
	case CIF_CLK_P12P0:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_04, MIPIRX_SEL_CLK_P0TOP1,
			      0);
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_04, MIPIRX_SEL_CLK_P1TOP0,
			      1);
		break;
	case CIF_CLK_FREERUN:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_04, MIPIRX_SEL_CLK_P0TOP1,
			      0);
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_04, MIPIRX_SEL_CLK_P1TOP0,
			      0);
		break;
	default:
		break;
	}
}

void cif_set_hs_settle(struct cif_ctx *ctx, uint8_t hs_settle)
{
	uintptr_t wrap_4l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L];
	uintptr_t wrap_2l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L];

	if (!ctx->mac_num) {
		/* disable auto_ignore and auto sync. */
		CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
			      REG_10, AUTO_IGNORE,
			      0);
		CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
			      REG_10, AUTO_SYNC,
			      0);
		CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
			      REG_10, T_HS_SETTLE,
			      hs_settle);
	} else {
		/* disable auto_ignore and auto sync. */
		CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
			      REG_10, AUTO_IGNORE,
			      0);
		CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
			      REG_10, AUTO_SYNC,
			      0);
		CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
			      REG_10, T_HS_SETTLE,
			      hs_settle);

	}
}

uint8_t cif_get_lane_data(struct cif_ctx *ctx, enum phy_lane_id_e lane)
{
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];
	uint8_t value;

	switch (lane) {
	case CIF_PHY_LANE_0:
		value = CIF_RD_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
				    REG_70, AD_D0_DATA);
		break;
	case CIF_PHY_LANE_1:
		value = CIF_RD_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
				    REG_70, AD_D1_DATA);
		break;
	case CIF_PHY_LANE_2:
		value = CIF_RD_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
				    REG_70, AD_D2_DATA);
		break;
	case CIF_PHY_LANE_3:
		value = CIF_RD_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
				    REG_70, AD_D3_DATA);
		break;
	case CIF_PHY_LANE_4:
		value = CIF_RD_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
				    REG_74, AD_D4_DATA);
		break;
	case CIF_PHY_LANE_5:
		value = CIF_RD_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
				    REG_74, AD_D5_DATA);
		break;
	default:
		value = 0;
		break;
	}

	return value;
}

void cif_set_lane_deskew(struct cif_ctx *ctx,
		      enum phy_lane_id_e lane, uint8_t phase)
{
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];

	switch (lane) {
	case CIF_PHY_LANE_0:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_84, DESKEW_CODE0,
			      phase);
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, FORCE_DESKEW_CODE0,
			      !!phase);
		break;
	case CIF_PHY_LANE_1:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_84, DESKEW_CODE1,
			      phase);
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, FORCE_DESKEW_CODE1,
			      !!phase);
		break;
	case CIF_PHY_LANE_2:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_84, DESKEW_CODE2,
			      phase);
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, FORCE_DESKEW_CODE2,
			      !!phase);
		break;
	case CIF_PHY_LANE_3:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_84, DESKEW_CODE3,
			      phase);
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, FORCE_DESKEW_CODE3,
			      !!phase);
		break;
	case CIF_PHY_LANE_4:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_88, DESKEW_CODE4,
			      phase);
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, FORCE_DESKEW_CODE4,
			      !!phase);
		break;
	case CIF_PHY_LANE_5:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_88, DESKEW_CODE5,
			      phase);
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, FORCE_DESKEW_CODE5,
			      !!phase);
		break;
	default:
		break;
	}
}

void cif_set_lvds_endian(struct cif_ctx *ctx,
			 enum cif_endian mac, enum cif_endian wrap)
{
	uintptr_t wrap_4l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L];
	uintptr_t wrap_2l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L];
	uintptr_t mac_slvds = ctx->mac_phys_regs[CIF_MAC_BLK_ID_SLVDS];

	/* Config the endian. */
	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_00, SLVDS_DATA_REVERSE,
			       mac == CIF_SLVDS_ENDIAN_LSB);
	/* DPHY endian mode select */
	if (!ctx->mac_num)
		CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				      REG_20, SLVDS_INV_EN,
				      wrap == CIF_SLVDS_ENDIAN_MSB);
	else
		CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				      REG_20, SLVDS_INV_EN,
				      wrap == CIF_SLVDS_ENDIAN_MSB);
}

void cif_set_lvds_vsync_gen(struct cif_ctx *ctx, uint32_t fp)
{
	uintptr_t mac_slvds = ctx->mac_phys_regs[CIF_MAC_BLK_ID_SLVDS];

	CIF_WR_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
			       REG_00, SLVDS_VFPORCH,
			       fp);
}

int cif_check_csi_int_sts(struct cif_ctx *ctx, uint32_t mask)
{
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];
	uint32_t reg = CIF_RD_REG(mac_csi, REG_CSI_CTRL_TOP_T, REG_60);

	return !!(reg & mask);
}

void cif_clear_csi_int_sts(struct cif_ctx *ctx)
{
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];

	CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
			     REG_04, CSI_INTR_CLR,
			     0xFF);
}

void cif_mask_csi_int_sts(struct cif_ctx *ctx, uint32_t mask)
{
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];

	CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
			     REG_04, CSI_INTR_MASK,
			     0xFF);
}

void cif_unmask_csi_int_sts(struct cif_ctx *ctx, uint32_t mask)
{
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];

	CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
			     REG_04, CSI_INTR_MASK,
			     0x00);
}

int cif_check_csi_fifo_full(struct cif_ctx *ctx)
{
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];

	return !!CIF_RD_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
				      REG_40, CSI_FIFO_FULL);
}

int cif_check_lvds_fifo_full(struct cif_ctx *ctx)
{
	uintptr_t mac_slvds = ctx->mac_phys_regs[CIF_MAC_BLK_ID_SLVDS];

	return !!CIF_RD_BITS(mac_slvds, REG_SUBLVDS_CTRL_TOP_T,
					REG_40, SLVDS_FIFO_FULL);
}

int cif_get_csi_decode_fmt(struct cif_ctx *ctx)
{
	int i;
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];
	uint32_t value = CIF_RD_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
				     REG_40, CSI_DECODE_FORMAT);

	for (i = 0; i < DEC_FMT_NUM; i++) {
		if (value & (1 << i))
			return i;
	}

	return i;
}

int cif_get_csi_phy_state(struct cif_ctx *ctx, union mipi_phy_state *state)
{
	uintptr_t wrap_4l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L];
	uintptr_t wrap_2l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L];

	if (!ctx->mac_num) {
		state->raw = CIF_RD_REG(wrap_4l, REG_SENSOR_PHY_4L_T, DBG_90) |
			     CIF_RD_REG(wrap_4l, REG_SENSOR_PHY_4L_T, DBG_94) << 16;
	} else {
		state->raw = CIF_RD_REG(wrap_2l, REG_SENSOR_PHY_2L_T, DBG_90) |
			     CIF_RD_REG(wrap_2l, REG_SENSOR_PHY_2L_T, DBG_94) << 16;
	}

	return 0;
}

uintptr_t *cif_get_mac_phys_reg_bases(uint32_t link)
{
	static uintptr_t m_cif_mac_phys_base_list[MAX_LINK_NUM]
		[CIF_MAC_BLK_ID_MAX];

	m_cif_mac_phys_base_list[link][CIF_MAC_BLK_ID_TOP] =
					(CIF_MAC_BLK_BA_TOP);
	m_cif_mac_phys_base_list[link][CIF_MAC_BLK_ID_SLVDS] =
					(CIF_MAC_BLK_BA_SLVDS);
	m_cif_mac_phys_base_list[link][CIF_MAC_BLK_ID_CSI] =
					(CIF_MAC_BLK_BA_CSI);

	return m_cif_mac_phys_base_list[link];
}

uintptr_t *cif_get_wrap_phys_reg_bases(uint32_t link)
{
	static uintptr_t m_cif_wrap_phys_base_list[MAX_LINK_NUM]
		[CIF_WRAP_BLK_ID_MAX];

	m_cif_wrap_phys_base_list[link][CIF_WRAP_BLK_ID_TOP] =
					(CIF_WRAP_BLK_BA_TOP);
	m_cif_wrap_phys_base_list[link][CIF_WRAP_BLK_ID_4L] =
					(CIF_WRAP_BLK_BA_4L);
	m_cif_wrap_phys_base_list[link][CIF_WRAP_BLK_ID_2L] =
					(CIF_WRAP_BLK_BA_2L);

	return m_cif_wrap_phys_base_list[link];
}

