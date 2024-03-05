#include <vip/vi_drv.h>

#ifdef PORTING_TEST

/****************************************************************************
 * Global parameters
 ****************************************************************************/
enum VI_TEST_CASE {
	VI_TEST_RGBGAMMA_ENABLE = 1,
	VI_TEST_RGBGAMMA_ENABLE_TBL_INVERSE,
	VI_TEST_CNR_ENABLE,
	VI_TEST_CNR_SET_LUT,
	VI_TEST_LCAC_ENABLE_MEDIUM_STRENGTH,
	VI_TEST_LCAC_ENABLE_STRONG_STRENGTH,
	VI_TEST_BNR_ENABLE_WEAK_STRENGTH,
	VI_TEST_BNR_ENABLE_STRONG_STRENGTH,
	VI_TEST_YNR_ENABLE,
	VI_TEST_DHZ_ENABLE,
	VI_TEST_BLC_GAIN_800,
	VI_TEST_BLC_GAIN_B00,
	VI_TEST_BLC_OFFSET_1FF,
	VI_TEST_BLC_GAIN_800_OFFSET_1FF,
	VI_TEST_BLC_GAIN_800_OFFSET_1FF_2ND_OFFSET_1FF,
	VI_TEST_WBG_GAIN_800,
	VI_TEST_DPC_ENABLE,
	VI_TEST_DPC_ENABLE_STATIC_ONLY,
	VI_TEST_DPC_ENABLE_DYNAMIC_ONLY,
	VI_TEST_PREEE_ENABLE,
	VI_TEST_EE_ENABLE,
	VI_TEST_CCM_ENABLE,
	VI_TEST_YCURVE_ENABLE_TBL_INVERSE,
	VI_TEST_YGAMMA_ENABLE,
	VI_TEST_YGAMMA_ENABLE_TBL_INVERSE,
	VI_TEST_YGAMMA_LUT_MEM0_CHECK,
	VI_TEST_YGAMMA_LUT_MEM1_CHECK,
	VI_TEST_3DNR_ENABLE_MOTION_MAP_OUT,
	VI_TEST_FUSION_LE_OUTPUT,
	VI_TEST_FUSION_SE_OUTPUT,
	VI_TEST_AE_HIST_ENABLE,
	VI_TEST_HIST_V_ENABLE_LUMA_MODE,
	VI_TEST_HIST_V_ENABLE,
	VI_TEST_HIST_V_ENABLE_OFFX64_OFFY32,
	VI_TEST_DCI_ENABLE,
	VI_TEST_DCI_ENABLE_DEMO_MODE,
	VI_TEST_LTM_CHECK_GLOBAL_TONE,
	VI_TEST_LTM_CHECK_DARK_TONE,
	VI_TEST_LTM_CHECK_BRIGHT_TONE,
	VI_TEST_LTM_CHECK_ALL_TONE_EE_ENABLE,
	VI_TEST_GMS_ENABLE,
	VI_TEST_AF_ENABLE,
	VI_TEST_LSC_ENABLE,
	VI_TEST_LDCI_ENABLE,
	VI_TEST_LDCI_ENABLE_TONE_CURVE_LUT_P_1023,
	VI_TEST_RGBCAC_ON_DEFAULT,
	VI_TEST_RGBCAC_ON_STRENGTH_MAX,
	VI_TEST_HIST_V_DISABLE,
	VI_TEST_HIST_V_ENABLE_ALL_FF_WHITE,
	VI_TEST_HIST_V_ENABLE_ALL_FF_BLACK,
	VI_TEST_RGBGAMMA_ENABLE_HW_AUTO_ENABLE,
	VI_TEST_RGB_DITHER_OFF,
	VI_TEST_RGB_DITHER_ON,
	VI_TEST_YUV_DITHER_OFF,
	VI_TEST_YUV_DITHER_ON,
	VI_TEST_CLUT_ON,
	VI_TEST_DCI_ENABLE_DEMO_MODE_ALL_FF_WHITE,
	VI_TEST_DCI_ENABLE_DEMO_MODE_ALL_FF_BLACK,
	VI_TEST_CACP_OFF,
	VI_TEST_CA_ON_MODE_0,
	VI_TEST_CP_ON_MODE_1,
	VI_TEST_CP_LITE_OFF,
	VI_TEST_CP_LITE_ON,
	VI_TEST_CNR_OFF,
	VI_TEST_CNR_HW_AUTO_ENABLE_ISO_2,
	VI_TEST_YNR_OFF,
	VI_TEST_YNR_HW_AUTO_ON_ISO_0,
	VI_TEST_YNR_HW_AUTO_ON_ISO_2,
	VI_TEST_CROP_YUV,
	VI_TEST_CROP_RAW,
	VI_TEST_PREEE_EE_DISABLE,
};

extern int vi_ip_test_case;
extern struct isp_ccm_cfg ccm_hw_cfg;
extern uint16_t ygamma_data[];
extern uint16_t gamma_data[];
extern uint16_t ycur_data[];
extern u16 dci_map_lut_50[];
extern uint16_t ltm_d_lut[];
extern uint16_t ltm_b_lut[];
extern uint16_t ltm_g_lut[];

extern uint16_t c_lut_r_lut[];
extern uint16_t c_lut_g_lut[];
extern uint16_t c_lut_b_lut[];

/*******************************************************************************
 *	IPs test case config
 ******************************************************************************/
void vi_ip_test_cases_init(struct isp_ctx *ctx)
{
	switch (vi_ip_test_case) {
	case VI_TEST_RGBGAMMA_ENABLE: //rgbgamma enable
	{
		vi_pr(VI_INFO, "RGBgamma enable\n");

		ispblk_gamma_config(ctx, false, 0, gamma_data, 0);
		ispblk_gamma_enable(ctx, true);
		break;
	}
	case VI_TEST_RGBGAMMA_ENABLE_TBL_INVERSE: //2
	{
		vi_pr(VI_INFO, "RGBgamma enable, tbl inverse\n");

		ispblk_gamma_config(ctx, false, 0, gamma_data, 1);
		ispblk_gamma_enable(ctx, true);
		break;
	}
	case VI_TEST_CNR_ENABLE: //3
	{
		vi_pr(VI_INFO, "CNR enable\n");

		ispblk_cnr_config(ctx, true, true, 255, 0);
		break;
	}
	case VI_TEST_CNR_SET_LUT: //4
	{
		vi_pr(VI_INFO, "CNR set lut\n");

		ispblk_cnr_config(ctx, true, true, 255, 1);
		break;
	}
	case VI_TEST_LCAC_ENABLE_MEDIUM_STRENGTH: //5
	{
		vi_pr(VI_INFO, "LCAC enable, default medium strength setting\n");

		ispblk_lcac_config(ctx, true, 0);
		break;
	}
	case VI_TEST_LCAC_ENABLE_STRONG_STRENGTH: //6
	{
		vi_pr(VI_INFO, "LCAC enable, strong strength setting\n");

		ispblk_lcac_config(ctx, true, 1);
		break;
	}
	case VI_TEST_BNR_ENABLE_WEAK_STRENGTH: //7
	{
		vi_pr(VI_INFO, "BNR enable, weak strength\n");

		ispblk_bnr_config(ctx, ISP_BNR_OUT_B_OUT, false, 0, 0);
		break;
	}
	case VI_TEST_BNR_ENABLE_STRONG_STRENGTH: //8
	{
		vi_pr(VI_INFO, "BNR enable, strong strength\n");

		ispblk_bnr_config(ctx, ISP_BNR_OUT_B_OUT, false, 0, 255);
		break;
	}
	case VI_TEST_YNR_ENABLE: //9
	{
		vi_pr(VI_INFO, "YNR enable\n");

		ispblk_ynr_config(ctx, ISP_YNR_OUT_Y_OUT, 128);
		break;
	}
	case VI_TEST_DHZ_ENABLE: //10
	{
		vi_pr(VI_INFO, "DHZ enable\n");

		ispblk_dhz_config(ctx, true);
		break;
	}
	case VI_TEST_BLC_GAIN_800: //11
	{
		vi_pr(VI_INFO, "BLC be_le enable, gain 0x800\n");

		ispblk_blc_set_gain(ctx, ISP_BLC_ID_BE_LE, 0x800, 0x800, 0x800, 0x800);
		ispblk_blc_enable(ctx, ISP_BLC_ID_BE_LE, true, false);
		break;
	}
	case VI_TEST_BLC_GAIN_B00: //12
	{
		vi_pr(VI_INFO, "BLC be_le enable, gain 0xB00\n");

		ispblk_blc_set_gain(ctx, ISP_BLC_ID_BE_LE, 0xB00, 0xB00, 0xB00, 0xB00);
		ispblk_blc_enable(ctx, ISP_BLC_ID_BE_LE, true, false);
		break;
	}
	case VI_TEST_BLC_OFFSET_1FF: //13
	{
		vi_pr(VI_INFO, "BLC be_le enable, offset 0x1ff\n");

		ispblk_blc_set_offset(ctx, ISP_BLC_ID_BE_LE, 0x1FF, 0x1FF, 0x1FF, 0x1FF);
		ispblk_blc_enable(ctx, ISP_BLC_ID_BE_LE, true, false);
		break;
	}
	case VI_TEST_BLC_GAIN_800_OFFSET_1FF: //14
	{
		vi_pr(VI_INFO, "BLC be_le enable, gain 0x800 offset 0x1ff\n");

		ispblk_blc_set_offset(ctx, ISP_BLC_ID_BE_LE, 0x1FF, 0x1FF, 0x1FF, 0x1FF);
		ispblk_blc_set_gain(ctx, ISP_BLC_ID_BE_LE, 0x800, 0x800, 0x800, 0x800);
		ispblk_blc_enable(ctx, ISP_BLC_ID_BE_LE, true, false);
		break;
	}
	case VI_TEST_BLC_GAIN_800_OFFSET_1FF_2ND_OFFSET_1FF: //15
	{
		vi_pr(VI_INFO, "BLC be_le enable, gain 0x800 offset 0x1ff\n");

		ispblk_blc_set_offset(ctx, ISP_BLC_ID_BE_LE, 0x1FF, 0x1FF, 0x1FF, 0x1FF);
		ispblk_blc_set_gain(ctx, ISP_BLC_ID_BE_LE, 0x800, 0x800, 0x800, 0x800);
		ispblk_blc_set_2ndoffset(ctx, ISP_BLC_ID_BE_LE, 0x1FF, 0x1FF, 0x1FF, 0x1FF);
		ispblk_blc_enable(ctx, ISP_BLC_ID_BE_LE, true, false);
		break;
	}
	case VI_TEST_WBG_GAIN_800: //16
	{
		vi_pr(VI_INFO, "WBG rawtop_le enable, gain 0x800\n");

		ispblk_wbg_config(ctx, ISP_WBG_ID_RAW_TOP_LE, 0x800, 0x800, 0x800);
		ispblk_wbg_enable(ctx, ISP_WBG_ID_RAW_TOP_LE, true, false);
		break;
	}
	case VI_TEST_DPC_ENABLE: //17
	{
		vi_pr(VI_INFO, "DPC enable\n");
		ispblk_dpc_config(ctx, ISP_RAW_PATH_LE, true, 0);
		break;
	}
	case VI_TEST_DPC_ENABLE_STATIC_ONLY: //18
	{
		vi_pr(VI_INFO, "DPC enable static only\n");
		ispblk_dpc_config(ctx, ISP_RAW_PATH_LE, true, 1);
		break;
	}
	case VI_TEST_DPC_ENABLE_DYNAMIC_ONLY: //19
	{
		vi_pr(VI_INFO, "DPC enable dynamic only\n");
		ispblk_dpc_config(ctx, ISP_RAW_PATH_LE, true, 2);
		break;
	}
	case VI_TEST_PREEE_ENABLE: //20
	{
		vi_pr(VI_INFO, "PREEE enable\n");
		ispblk_ee_config(ctx, false);
		ispblk_pre_ee_config(ctx, true);
		break;
	}
	case VI_TEST_EE_ENABLE: //21
	{
		vi_pr(VI_INFO, "EE enable\n");
		ispblk_pre_ee_config(ctx, false);
		ispblk_ee_config(ctx, true);
		break;
	}
	case VI_TEST_PREEE_EE_DISABLE: //71
	{
		vi_pr(VI_INFO, "PREEE EE disable\n");
		ispblk_ee_config(ctx, false);
		ispblk_pre_ee_config(ctx, false);
		break;
	}
	case VI_TEST_CCM_ENABLE: //22
	{
		vi_pr(VI_INFO, "CCM enable\n");
		ispblk_ccm_config(ctx, ISP_BLK_ID_CCM0, true, &ccm_hw_cfg);
		break;
	}
	case VI_TEST_YCURVE_ENABLE_TBL_INVERSE: //23
	{
		vi_pr(VI_INFO, "YCURVE enable, tbl inverse\n");
		ispblk_ycur_config(ctx, true, 0, ycur_data);
		ispblk_ycur_enable(ctx, true, 0);
		break;
	}
	case VI_TEST_YGAMMA_ENABLE: //24
	{
		vi_pr(VI_INFO, "Ygamma enable\n");
		ispblk_ygamma_config(ctx, false, 0, ygamma_data, 0, 0);
		ispblk_ygamma_enable(ctx, true);
		break;
	}
	case VI_TEST_YGAMMA_ENABLE_TBL_INVERSE: //25
	{
		vi_pr(VI_INFO, "Ygamma enable, tbl inverse\n");
		ispblk_ygamma_config(ctx, false, 0, ygamma_data, 1, 0);
		ispblk_ygamma_enable(ctx, true);
		break;
	}
	case VI_TEST_YGAMMA_LUT_MEM0_CHECK: //26
	{
		vi_pr(VI_INFO, "Ygamma LUT MEM0 Check\n");
		ispblk_ygamma_config(ctx, false, 0, ygamma_data, 0, 1);
		break;
	}
	case VI_TEST_YGAMMA_LUT_MEM1_CHECK: //27
	{
		vi_pr(VI_INFO, "Ygamma LUT MEM1 Check\n");
		ispblk_ygamma_config(ctx, false, 1, ygamma_data, 0, 1);
		break;
	}
	case VI_TEST_3DNR_ENABLE_MOTION_MAP_OUT: //28
	{
		vi_pr(VI_INFO, "TNR enable, motion map output\n");
		ispblk_tnr_config(ctx, ctx->is_3dnr_on, 1);
		break;
	}
	case VI_TEST_FUSION_LE_OUTPUT: //29
	{
		vi_pr(VI_INFO, "ltm disable, le frame output\n");
		ispblk_ltm_config(ctx, false, false, false, false);
		ispblk_fusion_config(ctx, true, true, ISP_FS_OUT_LONG);
		break;
	}
	case VI_TEST_FUSION_SE_OUTPUT: //30
	{
		vi_pr(VI_INFO, "ltm disable, se frame output\n");
		ispblk_ltm_config(ctx, false, false, false, false);
		ispblk_fusion_config(ctx, true, true, ISP_FS_OUT_SHORT);
		break;
	}
	case VI_TEST_AE_HIST_ENABLE: //31
	{
		vi_pr(VI_INFO, "AE_HIST enable\n");

		ispblk_aehist_config(ctx, ISP_BLK_ID_AEHIST0, true);
		if (ctx->is_hdr_on)
			ispblk_aehist_config(ctx, ISP_BLK_ID_AEHIST1, true);
		else
			ispblk_aehist_config(ctx, ISP_BLK_ID_AEHIST1, false);

		break;
	}
	case VI_TEST_HIST_V_ENABLE_LUMA_MODE: //32
	{
		vi_pr(VI_INFO, "HIST_V enable, luma mode\n");
		ispblk_hist_v_config(ctx, true, 0);
		break;
	}
	case VI_TEST_HIST_V_ENABLE: //33
	{
		vi_pr(VI_INFO, "HIST_V enable\n");
		ispblk_hist_v_config(ctx, true, 1);
		break;
	}
	case VI_TEST_HIST_V_ENABLE_OFFX64_OFFY32: //34
	{
		vi_pr(VI_INFO, "HIST_V enable, Offx64_Offy32\n");
		ispblk_hist_v_config(ctx, true, 2);
		break;
	}
	case VI_TEST_DCI_ENABLE: //35
	{
		vi_pr(VI_INFO, "DCI enable\n");
		ispblk_dci_config(ctx, true, ctx->gamma_tbl_idx, dci_map_lut_50, 0);
		break;
	}
	case VI_TEST_DCI_ENABLE_DEMO_MODE: //36
	{
		vi_pr(VI_INFO, "DCI enable, demo_mode\n");
		ispblk_dci_config(ctx, true, ctx->gamma_tbl_idx, dci_map_lut_50, 1);
		break;
	}
	case VI_TEST_LTM_CHECK_GLOBAL_TONE: //37
	{
		vi_pr(VI_INFO, "ltm enable, dark_tone/bright_tone/ee_en disable\n");
		ispblk_ltm_b_lut(ctx, 0, ltm_b_lut);
		ispblk_ltm_d_lut(ctx, 0, ltm_d_lut);
		ispblk_ltm_g_lut(ctx, 0, ltm_g_lut);
		ispblk_ltm_config(ctx, true, false, false, false);
		break;
	}
	case VI_TEST_LTM_CHECK_DARK_TONE: //38
	{
		vi_pr(VI_INFO, "ltm enable, bright_tone/ee_en disable\n");
		ispblk_ltm_b_lut(ctx, 0, ltm_b_lut);
		ispblk_ltm_d_lut(ctx, 0, ltm_d_lut);
		ispblk_ltm_g_lut(ctx, 0, ltm_g_lut);
		ispblk_ltm_config(ctx, true, true, false, false);
		break;
	}
	case VI_TEST_LTM_CHECK_BRIGHT_TONE: //39
	{
		vi_pr(VI_INFO, "ltm enable, bright_tone/ee_en disable\n");
		ispblk_ltm_b_lut(ctx, 0, ltm_b_lut);
		ispblk_ltm_d_lut(ctx, 0, ltm_d_lut);
		ispblk_ltm_g_lut(ctx, 0, ltm_g_lut);
		ispblk_ltm_config(ctx, true, false, true, false);
		break;
	}
	case VI_TEST_LTM_CHECK_ALL_TONE_EE_ENABLE: //40
	{
		vi_pr(VI_INFO, "ltm enable, all enable\n");
		ispblk_ltm_b_lut(ctx, 0, ltm_b_lut);
		ispblk_ltm_d_lut(ctx, 0, ltm_d_lut);
		ispblk_ltm_g_lut(ctx, 0, ltm_g_lut);
		ispblk_ltm_config(ctx, true, true, true, true);
		break;
	}
	case VI_TEST_GMS_ENABLE: //41
	{
		vi_pr(VI_INFO, "gms enable\n");
		ispblk_gms_config(ctx, true);
		break;
	}
	case VI_TEST_AF_ENABLE: //42
	{
		vi_pr(VI_INFO, "af enable\n");
		ispblk_af_config(ctx, true);
		break;
	}
	case VI_TEST_LSC_ENABLE: //43
	{
		vi_pr(VI_INFO, "lsc enable\n");
		ispblk_lsc_config(ctx, true);
		break;
	}
	case VI_TEST_LDCI_ENABLE: //44
	{
		vi_pr(VI_INFO, "ldci enable\n");
		ispblk_ldci_config(ctx, true, 0);
		break;
	}
	case VI_TEST_LDCI_ENABLE_TONE_CURVE_LUT_P_1023: //45
	{
		vi_pr(VI_INFO, "ldci enable, TONE_CURVE_LUT_P_1023\n");
		ispblk_ldci_config(ctx, true, 1);
		break;
	}
	case VI_TEST_RGBCAC_ON_DEFAULT: //46
	{
		vi_pr(VI_INFO, "rgbcac enable, default\n");
		ispblk_rgbcac_config(ctx, true, 0);
		break;
	}
	case VI_TEST_RGBCAC_ON_STRENGTH_MAX: //47
	{
		vi_pr(VI_INFO, "rgbcac enable, PURPLE_TH_0xFF_STRENGTH_0xFF\n");
		ispblk_rgbcac_config(ctx, true, 1);
		break;
	}
	case VI_TEST_HIST_V_DISABLE: //48
	{
		vi_pr(VI_INFO, "hist_v disable\n");
		ispblk_hist_v_config(ctx, false, 0);
		break;
	}
	case VI_TEST_HIST_V_ENABLE_ALL_FF_WHITE: //49
	{
		vi_pr(VI_INFO, "hist_v enable all ff white\n");
		ispblk_patgen_config_pat(ctx, ISP_PRERAW_A, 0);
		ispblk_hist_v_config(ctx, true, 1);
		break;
	}
	case VI_TEST_HIST_V_ENABLE_ALL_FF_BLACK: //50
	{
		vi_pr(VI_INFO, "hist_v enable all ff black\n");
		ispblk_patgen_config_pat(ctx, ISP_PRERAW_A, 1);
		ispblk_hist_v_config(ctx, true, 1);
		break;
	}
	case VI_TEST_RGBGAMMA_ENABLE_HW_AUTO_ENABLE: //51
	{
		vi_pr(VI_INFO, "RGBgamma enable, hw auto enable\n");
		ispblk_isptop_fpga_config(ctx, 1);
		ispblk_gamma_config(ctx, false, 0, gamma_data, 0);
		ispblk_gamma_enable(ctx, true);
		break;
	}
	case VI_TEST_RGB_DITHER_OFF: //52
	{
		vi_pr(VI_INFO, "RGB DITHER disable\n");
		ispblk_rgbdither_config(ctx, false, false, false, false);
		break;
	}
	case VI_TEST_RGB_DITHER_ON: //53
	{
		vi_pr(VI_INFO, "RGB DITHER enable\n");
		ispblk_rgbdither_config(ctx, true, true, true, true);
		break;
	}
	case VI_TEST_YUV_DITHER_OFF: //54
	{
		vi_pr(VI_INFO, "YUV DITHER disable\n");
		ispblk_yuvdither_config(ctx, 0, false, false, false, false);
		ispblk_yuvdither_config(ctx, 1, false, false, false, false);
		break;
	}
	case VI_TEST_YUV_DITHER_ON: //55
	{
		vi_pr(VI_INFO, "YUV DITHER enable\n");
		ispblk_yuvdither_config(ctx, 0, true, true, true, true);
		ispblk_yuvdither_config(ctx, 1, true, true, true, true);
		break;
	}
	case VI_TEST_CLUT_ON: //56
	{
		vi_pr(VI_INFO, "CLUT enable\n");
		ispblk_clut_config(ctx, true, c_lut_r_lut, c_lut_g_lut, c_lut_b_lut);
		break;
	}
	case VI_TEST_DCI_ENABLE_DEMO_MODE_ALL_FF_WHITE: //57
	{
		vi_pr(VI_INFO, "DCI enable, demo_mode all white\n");
		ispblk_patgen_config_pat(ctx, ISP_PRERAW_A, 0);
		ispblk_dci_config(ctx, true, ctx->gamma_tbl_idx, dci_map_lut_50, 0);
		break;
	}
	case VI_TEST_DCI_ENABLE_DEMO_MODE_ALL_FF_BLACK: //58
	{
		vi_pr(VI_INFO, "DCI enable, demo_mode all black\n");
		ispblk_patgen_config_pat(ctx, ISP_PRERAW_A, 1);
		ispblk_dci_config(ctx, true, ctx->gamma_tbl_idx, dci_map_lut_50, 0);
		break;
	}
	case VI_TEST_CACP_OFF: //59
	{
		vi_pr(VI_INFO, "CACP OFF\n");
		ispblk_ca_config(ctx, false, 1);
		break;
	}
	case VI_TEST_CA_ON_MODE_0: //60
	{
		vi_pr(VI_INFO, "CA ON, mode 0\n");
		ispblk_ca_config(ctx, true, 0);
		break;
	}
	case VI_TEST_CP_ON_MODE_1: //61
	{
		vi_pr(VI_INFO, "CP ON, mode 1\n");
		ispblk_ca_config(ctx, true, 1);
		break;
	}
	case VI_TEST_CP_LITE_OFF: //62
	{
		vi_pr(VI_INFO, "CP LITE OFF\n");
		ispblk_patgen_config_pat(ctx, ISP_PRERAW_A, 3);
		ispblk_ca_lite_config(ctx, false);
		break;
	}
	case VI_TEST_CP_LITE_ON: //63
	{
		vi_pr(VI_INFO, "CP LITE ON\n");
		ispblk_patgen_config_pat(ctx, ISP_PRERAW_A, 3);
		ispblk_ca_lite_config(ctx, true);
		break;
	}
	case VI_TEST_CNR_OFF: //64
	{
		vi_pr(VI_INFO, "CNR ALL OFF\n");
		ispblk_cnr_config(ctx, false, false, 255, 0);
		break;
	}
	case VI_TEST_CNR_HW_AUTO_ENABLE_ISO_2: //65
	{
		vi_pr(VI_INFO, " CNR HW AUTO enable iso 2\n");
		ispblk_isptop_fpga_config(ctx, 2);
		ispblk_cnr_config(ctx, true, true, 255, 0);
		break;
	}
	case VI_TEST_YNR_OFF: //66
	{
		vi_pr(VI_INFO, " YNR OFF\n");
		ispblk_isptop_fpga_config(ctx, 0);
		break;
	}
	case VI_TEST_YNR_HW_AUTO_ON_ISO_0:
	{
		vi_pr(VI_INFO, " YNR ON iso 0\n");
		ispblk_isptop_fpga_config(ctx, 1);
		ispblk_ynr_config(ctx, ISP_YNR_OUT_Y_OUT, 128);
		break;
	}
	case VI_TEST_YNR_HW_AUTO_ON_ISO_2:
	{
		vi_pr(VI_INFO, " YNR ON iso 2\n");
		ispblk_isptop_fpga_config(ctx, 2);
		ispblk_ynr_config(ctx, ISP_YNR_OUT_Y_OUT, 128);
		break;
	}
	default:
		break;
	}
}

void vi_ip_test_cases_uninit(struct isp_ctx *ctx) //need restore default status
{
	switch (vi_ip_test_case) {
	case VI_TEST_RGBGAMMA_ENABLE: //rgbgamma disable
	case VI_TEST_RGBGAMMA_ENABLE_TBL_INVERSE: //2
	{
		vi_pr(VI_INFO, "RGBgamma disable, tbl inverse disable\n");

		ispblk_gamma_config(ctx, false, 0, gamma_data, 0);
		ispblk_gamma_enable(ctx, false);
		break;
	}
	case VI_TEST_CNR_ENABLE: //3
	case VI_TEST_CNR_SET_LUT: //4
	case VI_TEST_CNR_OFF: //64
	case VI_TEST_CNR_HW_AUTO_ENABLE_ISO_2: //65
	{
		vi_pr(VI_INFO, "CNR disable\n");

		ispblk_cnr_config(ctx, false, false, 255, 0);
		ispblk_isptop_fpga_config(ctx, 0xFF);
		break;
	}
	case VI_TEST_LCAC_ENABLE_MEDIUM_STRENGTH: //5
	case VI_TEST_LCAC_ENABLE_STRONG_STRENGTH: //6
	{
		vi_pr(VI_INFO, "LCAC disable\n");

		ispblk_lcac_config(ctx, false, 0);
		break;
	}
	case VI_TEST_BNR_ENABLE_WEAK_STRENGTH: //7
	case VI_TEST_BNR_ENABLE_STRONG_STRENGTH: //8
	{
		vi_pr(VI_INFO, "BNR disable\n");

		ispblk_bnr_config(ctx, ISP_BNR_OUT_B_DELAY, false, 0, 0);
		break;
	}
	case VI_TEST_YNR_ENABLE: //9
	case VI_TEST_YNR_OFF: //66
	case VI_TEST_YNR_HW_AUTO_ON_ISO_0: //67
	case VI_TEST_YNR_HW_AUTO_ON_ISO_2: //68
	{
		vi_pr(VI_INFO, "YNR disable\n");
		ispblk_isptop_fpga_config(ctx, 0);
		ispblk_ynr_config(ctx, ISP_YNR_OUT_Y_DELAY, 128);
		break;
	}
	case VI_TEST_DHZ_ENABLE: //10
	{
		vi_pr(VI_INFO, "DHZ disable\n");

		ispblk_dhz_config(ctx, false);
		break;
	}
	case VI_TEST_BLC_GAIN_800: //11
	case VI_TEST_BLC_GAIN_B00: //12
	case VI_TEST_BLC_OFFSET_1FF: //13
	case VI_TEST_BLC_GAIN_800_OFFSET_1FF: //14
	case VI_TEST_BLC_GAIN_800_OFFSET_1FF_2ND_OFFSET_1FF: //15
	{
		vi_pr(VI_INFO, "BLC be_le disable\n");

		ispblk_blc_set_gain(ctx, ISP_BLC_ID_BE_LE, 0x40f, 0x419, 0x419, 0x405);
		ispblk_blc_enable(ctx, ISP_BLC_ID_BE_LE, false, false);
		break;
	}
	case VI_TEST_WBG_GAIN_800: //16
	{
		vi_pr(VI_INFO, "WBG rawtop_le disable\n");

		ispblk_wbg_config(ctx, ISP_WBG_ID_RAW_TOP_LE, 0x400, 0x400, 0x400);
		ispblk_wbg_enable(ctx, ISP_WBG_ID_RAW_TOP_LE, false, false);
		break;
	}
	case VI_TEST_DPC_ENABLE: //17
	case VI_TEST_DPC_ENABLE_STATIC_ONLY: //18
	case VI_TEST_DPC_ENABLE_DYNAMIC_ONLY: //19
	{
		vi_pr(VI_INFO, "DPC disable\n");
		ispblk_dpc_config(ctx, ISP_RAW_PATH_LE, false, 0);
		break;
	}
	case VI_TEST_PREEE_ENABLE: //20
	case VI_TEST_EE_ENABLE: //21
	case VI_TEST_PREEE_EE_DISABLE: //71
	{
		vi_pr(VI_INFO, "PREEE EE disable\n");
		ispblk_pre_ee_config(ctx, false);
		ispblk_ee_config(ctx, false);
		break;
	}
	case VI_TEST_CCM_ENABLE: //22
	{
		vi_pr(VI_INFO, "CCM disable\n");
		ispblk_ccm_config(ctx, ISP_BLK_ID_CCM0, false, &ccm_hw_cfg);
		break;
	}
	case VI_TEST_YCURVE_ENABLE_TBL_INVERSE: //23
	{
		vi_pr(VI_INFO, "YCURVE disable, tbl inverse\n");
		ispblk_ycur_config(ctx, false, 0, ycur_data);
		ispblk_ycur_enable(ctx, false, 0);
		break;
	}
	case VI_TEST_YGAMMA_ENABLE: //24
	case VI_TEST_YGAMMA_ENABLE_TBL_INVERSE: //25
	case VI_TEST_YGAMMA_LUT_MEM0_CHECK: //26
	case VI_TEST_YGAMMA_LUT_MEM1_CHECK: //27
	{
		vi_pr(VI_INFO, "Ygamma disable\n");
		ispblk_ygamma_config(ctx, false, ctx->gamma_tbl_idx, ygamma_data, 0, 0);
		ispblk_ygamma_enable(ctx, false);
		break;
	}
	case VI_TEST_3DNR_ENABLE_MOTION_MAP_OUT: //28
	{
		vi_pr(VI_INFO, "TNR disable\n");
		ispblk_tnr_config(ctx, ctx->is_3dnr_on, 0);
		break;
	}
	case VI_TEST_FUSION_LE_OUTPUT: //29
	case VI_TEST_FUSION_SE_OUTPUT: //30
	{
		vi_pr(VI_INFO, "ltm disable, se frame output\n");
		ispblk_fusion_config(ctx, false, false, ISP_FS_OUT_LONG);
		ispblk_ltm_config(ctx, false, false, false, false);
		break;
	}
	case VI_TEST_AE_HIST_ENABLE: //31
	{
		vi_pr(VI_INFO, "AE_HIST disable\n");

		ispblk_aehist_config(ctx, ISP_BLK_ID_AEHIST0, false);
		if (ctx->is_hdr_on)
			ispblk_aehist_config(ctx, ISP_BLK_ID_AEHIST1, false);
		else
			ispblk_aehist_config(ctx, ISP_BLK_ID_AEHIST1, false);

		break;
	}
	case VI_TEST_HIST_V_ENABLE_LUMA_MODE: //32
	case VI_TEST_HIST_V_ENABLE: //33
	case VI_TEST_HIST_V_ENABLE_OFFX64_OFFY32: //34
	{
		vi_pr(VI_INFO, "HIST_V disable, Offx64_Offy32\n");
		ispblk_hist_v_config(ctx, false, 0);
		break;
	}
	case VI_TEST_DCI_ENABLE: //35
	case VI_TEST_DCI_ENABLE_DEMO_MODE: //36
	case VI_TEST_DCI_ENABLE_DEMO_MODE_ALL_FF_WHITE:
	case VI_TEST_DCI_ENABLE_DEMO_MODE_ALL_FF_BLACK:
	{
		vi_pr(VI_INFO, "DCI disable\n");
		ispblk_dci_restore_default_config(ctx, false);
		break;
	}
	case VI_TEST_LTM_CHECK_GLOBAL_TONE: //37
	case VI_TEST_LTM_CHECK_DARK_TONE: //38
	case VI_TEST_LTM_CHECK_BRIGHT_TONE: //39
	case VI_TEST_LTM_CHECK_ALL_TONE_EE_ENABLE: //40
	{
		vi_pr(VI_INFO, "ltm disable, all disable\n");
		ispblk_ltm_b_lut(ctx, 0, ltm_b_lut);
		ispblk_ltm_d_lut(ctx, 0, ltm_d_lut);
		ispblk_ltm_g_lut(ctx, 0, ltm_g_lut);
		ispblk_ltm_config(ctx, false, false, false, false);
		break;
	}
	case VI_TEST_GMS_ENABLE: //41
	{
		vi_pr(VI_INFO, "gms disable\n");
		ispblk_gms_config(ctx, false);
		break;
	}
	case VI_TEST_AF_ENABLE: //42
	{
		vi_pr(VI_INFO, "af disable\n");
		ispblk_af_config(ctx, false);
		break;
	}
	case VI_TEST_LSC_ENABLE: //43
	case VI_TEST_LDCI_ENABLE: //44
	case VI_TEST_LDCI_ENABLE_TONE_CURVE_LUT_P_1023: //45
	{
		vi_pr(VI_INFO, "ldci disable\n");
		ispblk_ldci_config(ctx, false, 0);
		break;
	}
	case VI_TEST_RGBCAC_ON_DEFAULT:
	case VI_TEST_RGBCAC_ON_STRENGTH_MAX:
	{
		vi_pr(VI_INFO, "rgbcac disable\n");
		ispblk_rgbcac_config(ctx, true, 1);
		break;
	}
	case VI_TEST_CLUT_ON:
	{
		vi_pr(VI_INFO, "clut disable\n");
		ispblk_clut_config(ctx, false, c_lut_r_lut, c_lut_g_lut, c_lut_b_lut);
		break;
	}
	default:
		break;
	}
}

#endif
