#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>

#include "cvi_base.h"
#include "cvi_vi.h"
#include "cvi_sys.h"
#include "vi_ioctl.h"
#include "cvi_isp.h"

#define VDEV_CLOSED_CHK(_dev_type, _dev_id)					\
	struct vdev *d;								\
	d = get_dev_info(_dev_type, _dev_id);					\
	if (IS_VDEV_CLOSED(d->state)) {						\
		CVI_TRACE_VI(CVI_DBG_ERR, "vi dev(%d) state(%d) incorrect.",	\
					_dev_id, d->state);			\
		return CVI_ERR_VI_SYS_NOTREADY;					\
	}

#define CHECK_VI_NULL_PTR(ptr)							\
	do {									\
		if (ptr == NULL) {						\
			CVI_TRACE_VI(CVI_DBG_ERR, " Invalid null pointer\n");	\
			return CVI_ERR_VI_INVALID_NULL_PTR;			\
		}								\
	} while (0)

#define GET_BASE_VADDR(ip_info_id)				\
	do {							\
		paddr = ip_info_list[ip_info_id].str_addr;	\
		size = ip_info_list[ip_info_id].size;		\
		vaddr = CVI_SYS_Mmap(paddr, size);		\
	} while (0)

#define CLEAR_ACCESS_CNT(addr_ofs)				\
	do {							\
		val = vaddr + addr_ofs;				\
		*val = 0x1;					\
	} while (0)

#define SET_BITS(addr_ofs, val_ofs)				\
	do {							\
		val = vaddr + addr_ofs;				\
		data = *val | (0x1 << val_ofs);			\
		*val = data;					\
	} while (0)

#define CLEAR_BITS(addr_ofs, val_ofs)				\
	do {							\
		val = vaddr + addr_ofs;				\
		data = *val & (~(0x1 << val_ofs));		\
		*val = data;					\
	} while (0)

#define SET_REGISTER_COMMON(addr_ofs, val_ofs, on_off)		\
	do {							\
		if (!on_off) {/* off */				\
			CLEAR_BITS(addr_ofs, val_ofs);		\
		} else {/* on */				\
			SET_BITS(addr_ofs, val_ofs);		\
		}						\
	} while (0)

#define GET_REGISTER_COMMON(addr_ofs, val_ofs, ret)		\
	do {							\
		val = vaddr + addr_ofs;				\
		data = *val;					\
		ret = (data >> val_ofs) & 0x1;			\
	} while (0)

#define WAIT_IP_DISABLE(addr_ofs, val_ofs)			\
	do {							\
		do {						\
			usleep(1);				\
			val = vaddr + addr_ofs;			\
			data = *val;				\
		} while (((data >> val_ofs) & 0x1) != 0);	\
	} while (0)

#define SET_SW_MODE_ENABLE(addr_ofs, val_ofs, on_off)		\
	SET_REGISTER_COMMON(addr_ofs, val_ofs, on_off)

#define SET_SW_MODE_MEM_SEL(addr_ofs, val_ofs, mem_id)		\
	SET_REGISTER_COMMON(addr_ofs, val_ofs, mem_id)

#define FPRINTF_VAL()							\
	do {								\
		val = vaddr + offset;					\
		fprintf(fp, "\t\"%s\": {\n", name);			\
		fprintf(fp, "\t\t\"length\": %u,\n", length);		\
		fprintf(fp, "\t\t\"lut\": [\n");			\
		fprintf(fp, "\t\t\t");					\
		for (CVI_U32 i = 0 ; i < length; i++) {			\
			if (i == length - 1) {				\
				fprintf(fp, "%u\n", *val);		\
			} else if (i % 16 == 15) {			\
				fprintf(fp, "%u,\n\t\t\t", *val);	\
			} else {					\
				fprintf(fp, "%u,\t", *val);		\
			}						\
		}							\
		fprintf(fp, "\t\t]\n\t},\n");				\
	} while (0)

#define FPRINTF_VAL2()							\
	do {								\
		val = vaddr + offset;					\
		fprintf(fp, "\t\"%s\": {\n", name);			\
		fprintf(fp, "\t\t\"length\": %u,\n", length);		\
		fprintf(fp, "\t\t\"lut\": [\n");			\
		fprintf(fp, "\t\t\t");					\
		for (CVI_U32 i = 0 ; i < length; i++) {			\
			if (i == length - 1) {				\
				fprintf(fp, "%u\n", *(val++));		\
			} else if (i % 16 == 15) {			\
				fprintf(fp, "%u,\n\t\t\t", *(val++));	\
			} else {					\
				fprintf(fp, "%u,\t", *(val++));		\
			}						\
		}							\
		fprintf(fp, "\t\t]\n\t},\n");				\
	} while (0)

#define FPRINTF_TBL(data_tbl)						\
	do {								\
		fprintf(fp, "\t\"%s\": {\n", name);			\
		fprintf(fp, "\t\t\"length\": %u,\n", length);		\
		fprintf(fp, "\t\t\"lut\": [\n");			\
		fprintf(fp, "\t\t\t");					\
		for (CVI_U32 i = 0 ; i < length; i++) {			\
			if (i == length - 1) {				\
				fprintf(fp, "%u\n", data_tbl[i]);	\
			} else if (i % 16 == 15) {			\
				fprintf(fp, "%u,\n\t\t\t", data_tbl[i]);\
			} else {					\
				fprintf(fp, "%u,\t", data_tbl[i]);	\
			}						\
		}							\
		fprintf(fp, "\t\t]\n\t},\n");				\
	} while (0)

#define DUMP_LUT_BASE(data_tbl, data_mask, r_addr, r_trig, r_data)	\
	do {								\
		for (CVI_U32 i = 0 ; i < length; i++) {			\
			val = vaddr + r_addr;				\
			*val = i;					\
			usleep(1);					\
									\
			val = vaddr + r_trig;				\
			data = (*val | (0x1 << 31));			\
			*val = data;					\
			usleep(1);					\
									\
			val = vaddr + r_data;				\
			data = *val;					\
			data_tbl[i] = (data & data_mask);		\
		}							\
	} while (0)

#define DUMP_LUT_COMMON(data_tbl, data_mask, sw_mode, r_addr, r_trig, r_data)	\
	do {									\
		val = vaddr + sw_mode;						\
		data = 0x1;							\
		*val = data;							\
										\
		DUMP_LUT_BASE(data_tbl, data_mask, r_addr, r_trig, r_data);	\
										\
		val = vaddr + sw_mode;						\
		data = 0x0;							\
		*val = data;							\
	} while (0)

struct reg_tbl {
	int addr_ofs;
	int val_ofs;
	int data;
	int mask;
};

struct gamma_tbl {
	enum IP_INFO_GRP ip_info_id;
	char name[16];
	int length;
	struct reg_tbl enable;
	struct reg_tbl shdw_sel;
	struct reg_tbl force_clk_enable;
	struct reg_tbl prog_en;
	struct reg_tbl raddr;
	struct reg_tbl rdata_r;
	struct reg_tbl rdata_gb;
};

static void _dump_gamma_table(FILE *fp, struct ip_info *ip_info_list, struct gamma_tbl *tbl)
{
	CVI_U32 paddr;
	CVI_U32 size;
	CVI_VOID *vaddr;
	CVI_U32 *val;

	CVI_U32 length = tbl->length;
	CVI_U32 data;
	CVI_CHAR name[32];

	CVI_U32 *data_gamma_r = calloc(1, sizeof(CVI_U32) * length);
	CVI_U32 *data_gamma_g = calloc(1, sizeof(CVI_U32) * length);
	CVI_U32 *data_gamma_b = calloc(1, sizeof(CVI_U32) * length);

	GET_BASE_VADDR(tbl->ip_info_id);
	GET_REGISTER_COMMON(tbl->enable.addr_ofs, tbl->enable.val_ofs, tbl->enable.data);
	GET_REGISTER_COMMON(tbl->shdw_sel.addr_ofs, tbl->shdw_sel.val_ofs, tbl->shdw_sel.data);
	GET_REGISTER_COMMON(tbl->force_clk_enable.addr_ofs, tbl->force_clk_enable.val_ofs, tbl->force_clk_enable.data);

#ifdef __SOC_MARS__
	//for 181x need read register at postraw done
	CVI_U32 timeOut = 500;
	CVI_U32 s32Ret = CVI_SUCCESS;

	s32Ret = CVI_ISP_GetVDTimeOut(0, ISP_VD_BE_END, timeOut);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_LOG(CVI_DBG_ERR, "wait Vd time out. s32Ret: %#x\n", s32Ret);
	}
#endif

	SET_REGISTER_COMMON(tbl->enable.addr_ofs, tbl->enable.val_ofs, 0);
	// SET_REGISTER_COMMON(tbl->shdw_sel.addr_ofs, tbl->shdw_sel.val_ofs, 0);
	SET_REGISTER_COMMON(tbl->force_clk_enable.addr_ofs, tbl->force_clk_enable.val_ofs, 0);
	WAIT_IP_DISABLE(tbl->enable.addr_ofs, tbl->enable.val_ofs);
	SET_REGISTER_COMMON(tbl->prog_en.addr_ofs, tbl->prog_en.val_ofs, 1);
#ifdef __SOC_PHOBOS__
	if (tbl->ip_info_id == IP_INFO_ID_RGBGAMMA ||
		tbl->ip_info_id == IP_INFO_ID_YGAMMA ||
		tbl->ip_info_id == IP_INFO_ID_DCI) {
		CVI_U8 r_sel = 0;

		GET_REGISTER_COMMON(tbl->prog_en.addr_ofs, 4, r_sel);
		CVI_TRACE_VI(CVI_DBG_INFO, "mem[%d] work, mem[%d] IDLE\n", r_sel, r_sel ^ 0x1);
		SET_REGISTER_COMMON(tbl->raddr.addr_ofs, tbl->raddr.val_ofs, r_sel ^ 0x1);
	}
#endif

	for (CVI_U32 i = 0 ; i < length; i++) {
		val = vaddr + tbl->raddr.addr_ofs;
		data = (*val & (~tbl->raddr.mask)) | i;
		*val = data;

		val = vaddr + tbl->rdata_r.addr_ofs;
		data = (*val | (0x1 << tbl->rdata_r.val_ofs));
		*val = data;

		val = vaddr + tbl->rdata_r.addr_ofs;
		data = *val;
		data_gamma_r[i] = (data & tbl->rdata_r.mask);

		val = vaddr + tbl->rdata_gb.addr_ofs;
		data = *val;
		data_gamma_g[i] = (data & tbl->rdata_gb.mask);
		data_gamma_b[i] = ((data >> tbl->rdata_gb.val_ofs) & tbl->rdata_gb.mask);
	}

	SET_REGISTER_COMMON(tbl->prog_en.addr_ofs, tbl->prog_en.val_ofs, 0);
	SET_REGISTER_COMMON(tbl->force_clk_enable.addr_ofs, tbl->force_clk_enable.val_ofs, tbl->force_clk_enable.data);
	SET_REGISTER_COMMON(tbl->shdw_sel.addr_ofs, tbl->shdw_sel.val_ofs, tbl->shdw_sel.data);
	SET_REGISTER_COMMON(tbl->enable.addr_ofs, tbl->enable.val_ofs, tbl->enable.data);

	memset(name, 0, sizeof(name));
	strcat(strcat(name, tbl->name), "_r");
	FPRINTF_TBL(data_gamma_r);

	memset(name, 0, sizeof(name));
	strcat(strcat(name, tbl->name), "_g");
	FPRINTF_TBL(data_gamma_g);

	memset(name, 0, sizeof(name));
	strcat(strcat(name, tbl->name), "_b");
	FPRINTF_TBL(data_gamma_b);

	CVI_SYS_Munmap(vaddr, size);
	free(data_gamma_r);
	free(data_gamma_g);
	free(data_gamma_b);
}

CVI_S32 dump_register_182x(VI_PIPE ViPipe, FILE *fp, VI_DUMP_REGISTER_TABLE_S *pstRegTbl)
{
	VDEV_CLOSED_CHK(VDEV_TYPE_ISP, ViPipe);
	CHECK_VI_NULL_PTR(fp);
	CHECK_VI_NULL_PTR(pstRegTbl);

#ifdef ARCH_CV182X
	CVI_S32 s32Ret = CVI_SUCCESS;
	struct ip_info *ip_info_list;
	struct isp_tuning_cfg *tun_buf_info;

	ip_info_list = calloc(1, sizeof(struct ip_info) * IP_INFO_ID_MAX);
	tun_buf_info = calloc(1, sizeof(struct isp_tuning_cfg));

	CVI_U32 paddr;
	CVI_U32 size;
	CVI_VOID *vaddr;
	CVI_U32 *val;

	CVI_U32 ip_info_id;
	CVI_U32 offset;
	CVI_U32 length;
	CVI_U32 data;
	CVI_CHAR name[32];

	s32Ret = vi_get_ip_dump_list(d->fd, ip_info_list);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VI(CVI_DBG_ERR, "vi_get_ip_dump_list ioctl\n");
		free(tun_buf_info);
		free(ip_info_list);
		return s32Ret;
	}

	s32Ret = vi_get_tun_addr(d->fd, tun_buf_info);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VI(CVI_DBG_ERR, "vi_get_tun_addr ioctl\n");
		free(tun_buf_info);
		free(ip_info_list);
		return s32Ret;
	}

	/* stop tuning update */
	if (ViPipe == 0) {
		system("echo 1,1,1,1 > /sys/module/cv182x_vip/parameters/tuning_dis");
	} else if (ViPipe == 1) {
		system("echo 2,1,1,1 > /sys/module/cv182x_vip/parameters/tuning_dis");
	}
	/* In the worst case, have to wait two frames to stop tuning update. */
	usleep(80 * 1000);
	/* start of file */
	fprintf(fp, "{\n");
	/* dump isp moudle register */
	for (CVI_U32 i = 0; i < IP_INFO_ID_MAX; i++) {
		GET_BASE_VADDR(i);
		val = vaddr;

		fprintf(fp, "\t\"0x%08X\": {\n", paddr);
		for (CVI_U32 j = 0 ; j < (size / 0x4); j++) {
			fprintf(fp, "\t\t\"h%02x\": %u,\n", (j * 4), *(val++));
		}

		// if (i != IP_INFO_ID_MAX - 1)
			fprintf(fp, "\t\t\"size\": %u\n\t},\n", size);
		// else
			// fprintf(fp, "\t\t\"size\": %u\n\t}\n", size);

		CVI_SYS_Munmap(vaddr, size);
	}

	/* dump look up table */
	struct cvi_vip_isp_fe_cfg *fe_addr;
	struct cvi_vip_isp_be_cfg *be_addr;
	struct cvi_vip_isp_post_cfg *post_addr;

	tun_buf_info->fe_vir[ViPipe] =
			CVI_SYS_Mmap(tun_buf_info->fe_addr[ViPipe], sizeof(struct cvi_vip_isp_fe_cfg));
	tun_buf_info->be_vir[ViPipe] =
			CVI_SYS_Mmap(tun_buf_info->be_addr[ViPipe], sizeof(struct cvi_vip_isp_be_cfg));
	tun_buf_info->post_vir[ViPipe] =
			CVI_SYS_Mmap(tun_buf_info->post_addr[ViPipe], sizeof(struct cvi_vip_isp_post_cfg));
	fe_addr = (struct cvi_vip_isp_fe_cfg *)(tun_buf_info->fe_vir[ViPipe]);
	be_addr = (struct cvi_vip_isp_be_cfg *)(tun_buf_info->be_vir[ViPipe]);
	post_addr = (struct cvi_vip_isp_post_cfg *)(tun_buf_info->post_vir[ViPipe]);

	// LSCR_FE
	// 182x lscr hardware limitation: only read even index
	// use the isp tun buffer
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "LSCR_FE\n");
		struct cvi_vip_isp_lscr_config *pre_fe_rlsc_cfg[2] = {
			(struct cvi_vip_isp_lscr_config *)&(fe_addr->tun_cfg[0].lscr_cfg[0]),
			(struct cvi_vip_isp_lscr_config *)&(fe_addr->tun_cfg[0].lscr_cfg[1])
		};

		length = 32;
		snprintf(name, sizeof(name), "lscr_fe_le_gain_lut");
		FPRINTF_TBL(pre_fe_rlsc_cfg[0]->gain_lut);

		snprintf(name, sizeof(name), "lscr_fe_le_gain_lut_ir");
		FPRINTF_TBL(pre_fe_rlsc_cfg[0]->gain_lut_ir);

		if (vi_ctx.devAttr[ViPipe].stWDRAttr.enWDRMode == WDR_MODE_2To1_LINE) {
			snprintf(name, sizeof(name), "lscr_fe_se_gain_lut");
			FPRINTF_TBL(pre_fe_rlsc_cfg[1]->gain_lut);

			snprintf(name, sizeof(name), "lscr_fe_se_gain_lut_ir");
			FPRINTF_TBL(pre_fe_rlsc_cfg[1]->gain_lut_ir);
		}
	}

	// FPN
	// register is write only
	// use the fixed value
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "FPN\n");
		length = 256;
		CVI_U32 *data_offset = calloc(1, sizeof(CVI_U32) * length);
		CVI_U32 *data_gain = calloc(1, sizeof(CVI_U32) * length);

		for (CVI_U32 i = 0 ; i < length; i++) {
			data = 200 + i * 4;
			data_offset[i] = data;
			data_gain[i] = data;
		}

		snprintf(name, sizeof(name), "fpn_data_offset");
		FPRINTF_TBL(data_offset);

		snprintf(name, sizeof(name), "fpn_data_gain");
		FPRINTF_TBL(data_gain);

		free(data_offset);
		free(data_gain);
	}

	// IR_PREPROC
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "IR_PRE_PROC_LE\n");
		ip_info_id = IP_INFO_ID_IR_PRE_PROC_LE;
		GET_BASE_VADDR(ip_info_id);

		length = 128;
		CVI_U32 *data_lut_r = calloc(1, sizeof(CVI_U32) * length);
		CVI_U32 *data_lut_g = calloc(1, sizeof(CVI_U32) * length);
		CVI_U32 *data_lut_b = calloc(1, sizeof(CVI_U32) * length);
		CVI_U32 *data_lut_w = calloc(1, sizeof(CVI_U32) * length);

		SET_SW_MODE_ENABLE(0x04, 8, 1);
		SET_SW_MODE_MEM_SEL(0x10, 8, 1);
		// 0: ratio lut R, 1: ratio lut G, 2: ratio lut B, 3: w lut
		for (CVI_U32 j = 0 ; j < 4; j++) {
			CVI_U32 *data_lut;

			if (j == 0) {
				data_lut = data_lut_r;
			} else if (j == 1) {
				data_lut = data_lut_g;
			} else if (j == 2) {
				data_lut = data_lut_b;
			} else if (j == 3) {
				data_lut = data_lut_w;
			}

			val = vaddr + 0x04;
			data = (*val & ~(0x3 << 16));
			*val = (data | (j << 16));

			for (CVI_U32 i = 0 ; i < length; i++) {
				val = vaddr + 0x10;
				data = (*val & (~0x3F)) | i;
				*val = data;
				usleep(1);

				val = vaddr + 0x14;
				data = (*val | (0x1 << 31));
				*val = data;
				usleep(1);

				val = vaddr + 0x14;
				data = *val;
				data_lut[i] = (data & 0x3FFF);
			}
		}
		SET_SW_MODE_ENABLE(0x04, 8, 0);

		snprintf(name, sizeof(name), "ir_preproc_le_r");
		FPRINTF_TBL(data_lut_r);

		snprintf(name, sizeof(name), "ir_preproc_le_g");
		FPRINTF_TBL(data_lut_g);

		snprintf(name, sizeof(name), "ir_preproc_le_b");
		FPRINTF_TBL(data_lut_b);

		snprintf(name, sizeof(name), "ir_preproc_le_w");
		FPRINTF_TBL(data_lut_w);

		if (vi_ctx.devAttr[ViPipe].stWDRAttr.enWDRMode == WDR_MODE_2To1_LINE) {
			CVI_TRACE_VI(CVI_DBG_INFO, "IR_PRE_PROC_SE\n");
			ip_info_id = IP_INFO_ID_IR_PRE_PROC_SE;
			GET_BASE_VADDR(ip_info_id);

			SET_SW_MODE_ENABLE(0x04, 8, 1);
			SET_SW_MODE_MEM_SEL(0x10, 8, 1);
			for (CVI_U32 j = 0 ; j < 4; j++) {
				CVI_U32 *data_lut;

				if (j == 0) {
					data_lut = data_lut_r;
				} else if (j == 1) {
					data_lut = data_lut_g;
				} else if (j == 2) {
					data_lut = data_lut_b;
				} else if (j == 3) {
					data_lut = data_lut_w;
				}

				val = vaddr + 0x04;
				data = (*val & ~(0x3 << 16));
				*val = (data | (j << 16));

				for (CVI_U32 i = 0 ; i < length; i++) {
					val = vaddr + 0x10;
					data = (*val & (~0x3F)) | i;
					*val = data;
					usleep(1);

					val = vaddr + 0x14;
					data = (*val | (0x1 << 31));
					*val = data;
					usleep(1);

					val = vaddr + 0x14;
					data = *val;
					data_lut[i] = (data & 0x3FFF);
				}
			}
			SET_SW_MODE_ENABLE(0x04, 8, 0);

			snprintf(name, sizeof(name), "ir_preproc_se_r");
			FPRINTF_TBL(data_lut_r);

			snprintf(name, sizeof(name), "ir_preproc_se_g");
			FPRINTF_TBL(data_lut_g);

			snprintf(name, sizeof(name), "ir_preproc_se_b");
			FPRINTF_TBL(data_lut_b);

			snprintf(name, sizeof(name), "ir_preproc_se_w");
			FPRINTF_TBL(data_lut_w);
		}

		free(data_lut_r);
		free(data_lut_g);
		free(data_lut_b);
		free(data_lut_w);
	}

	// IR_WDMA_PROC
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "IR_PROC\n");
		ip_info_id = IP_INFO_ID_IR_PROC;
		GET_BASE_VADDR(ip_info_id);

		length = 256;
		CVI_U32 *data_gamma_curve = calloc(1, sizeof(CVI_U32) * length);

		DUMP_LUT_COMMON(data_gamma_curve, 0xFF, 0x14, 0x18, 0x1C, 0x1C);
		snprintf(name, sizeof(name), "ir_wdma_proc");
		FPRINTF_TBL(data_gamma_curve);

		free(data_gamma_curve);
	}

	// AF_GAMMA
	// 182x has no af gamma

	// DPC
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "DPC_LE\n");
		ip_info_id = IP_INFO_ID_DPC0;
		GET_BASE_VADDR(ip_info_id);

		length = 2047;
		CVI_U32 *data_dpc = calloc(1, sizeof(CVI_U32) * length);

		val = vaddr + 0x08;
		data = *val & (~(0x3 << 2));
		*val = data;

		SET_SW_MODE_ENABLE(0x44, 31, 1);
		DUMP_LUT_BASE(data_dpc, 0xFFFFFF, 0x48, 0x4C, 0x4C);
		SET_SW_MODE_ENABLE(0x44, 31, 0);

		snprintf(name, sizeof(name), "dpc_le_bp_tbl");
		FPRINTF_TBL(data_dpc);

		if (vi_ctx.devAttr[ViPipe].stWDRAttr.enWDRMode == WDR_MODE_2To1_LINE) {
			CVI_TRACE_VI(CVI_DBG_INFO, "DPC_SE\n");
			ip_info_id = IP_INFO_ID_DPC1;
			GET_BASE_VADDR(ip_info_id);

			val = vaddr + 0x08;
			data = *val & (~(0x3 << 2));
			*val = data;

			SET_SW_MODE_ENABLE(0x44, 31, 1);
			DUMP_LUT_BASE(data_dpc, 0xFFFFFF, 0x48, 0x4C, 0x4C);
			SET_SW_MODE_ENABLE(0x44, 31, 0);

			snprintf(name, sizeof(name), "dpc_se_bp_tbl");
			FPRINTF_TBL(data_dpc);
		}

		free(data_dpc);
	}

	// LSCR_BE
	// 182x lscr hardware limitation: only read even index
	// use the isp tun buffer
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "LSCR_BE_LE\n");
		struct cvi_vip_isp_lscr_config *pre_be_rlsc_cfg[2] = {
			(struct cvi_vip_isp_lscr_config *)&(be_addr->tun_cfg[0].lscr_cfg[0]),
			(struct cvi_vip_isp_lscr_config *)&(be_addr->tun_cfg[0].lscr_cfg[1])
		};

		length = 32;
		snprintf(name, sizeof(name), "lscr_be_le_gain_lut_r");
		FPRINTF_TBL(pre_be_rlsc_cfg[0]->gain_lut);

		snprintf(name, sizeof(name), "lscr_be_le_gain_lut_g");
		FPRINTF_TBL(pre_be_rlsc_cfg[0]->gain_lut1);

		snprintf(name, sizeof(name), "lscr_be_le_gain_lut_b");
		FPRINTF_TBL(pre_be_rlsc_cfg[0]->gain_lut2);

		snprintf(name, sizeof(name), "lscr_be_le_gain_lut_ir");
		FPRINTF_TBL(pre_be_rlsc_cfg[0]->gain_lut_ir);


		if (vi_ctx.devAttr[ViPipe].stWDRAttr.enWDRMode == WDR_MODE_2To1_LINE) {
			CVI_TRACE_VI(CVI_DBG_INFO, "LSCR_BE_SE\n");
			snprintf(name, sizeof(name), "lscr_be_se_gain_lut_r");
			FPRINTF_TBL(pre_be_rlsc_cfg[1]->gain_lut);

			snprintf(name, sizeof(name), "lscr_be_se_gain_lut_g");
			FPRINTF_TBL(pre_be_rlsc_cfg[1]->gain_lut1);

			snprintf(name, sizeof(name), "lscr_be_se_gain_lut_b");
			FPRINTF_TBL(pre_be_rlsc_cfg[1]->gain_lut2);

			snprintf(name, sizeof(name), "lscr_be_se_gain_lut_ir");
			FPRINTF_TBL(pre_be_rlsc_cfg[1]->gain_lut_ir);
		}
	}

	// BNR
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "BNR\n");
		ip_info_id = IP_INFO_ID_BNR;
		GET_BASE_VADDR(ip_info_id);
		CLEAR_ACCESS_CNT(0x8);

		length = 8;
		offset = 0x148;
		snprintf(name, sizeof(name), "bnr_intensity_sel");
		FPRINTF_VAL();

		length = 256;
		offset = 0x228;
		snprintf(name, sizeof(name), "bnr_weight_lut");
		FPRINTF_VAL();

		length = 32;
		offset = 0x424;
		snprintf(name, sizeof(name), "bnr_lsc_gain_lut");
		FPRINTF_VAL();
	}

	// CA_CP
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "CA_CP\n");
		ip_info_id = IP_INFO_ID_RGBTOP;
		GET_BASE_VADDR(ip_info_id);

		length = 256;
		CVI_U32 *data_ca_cp = calloc(1, sizeof(CVI_U32) * length);

		SET_SW_MODE_ENABLE(0x60, 3, 1);
		for (CVI_U32 i = 0 ; i < length; i++) {
			val = vaddr + 0x58;
			data = (*val & ~0xFF) | i;
			*val = data;
			usleep(1);

			val = vaddr + 0x58;
			data = (*val | (0x1 << 31));
			*val = data;
			usleep(1);

			val = vaddr + 0x5C;
			data = *val;
			data_ca_cp[i] = (data & 0xFFFFFF);
		}
		SET_SW_MODE_ENABLE(0x60, 3, 0);

		snprintf(name, sizeof(name), "ca_cp");
		FPRINTF_TBL(data_ca_cp);

		free(data_ca_cp);
	}

	// MLSC
	// NO INDIRECT LUT
	// use the isp tun buffer
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "MLSC\n");
		MLSC_GAIN_LUT_S *MlscGainLut = &pstRegTbl->MlscGainLut;

		length = 1369;
		if (MlscGainLut->RGain && MlscGainLut->GGain && MlscGainLut->BGain) {
			snprintf(name, sizeof(name), "mlsc_gain_r");
			FPRINTF_TBL(MlscGainLut->RGain);

			snprintf(name, sizeof(name), "mlsc_gain_g");
			FPRINTF_TBL(MlscGainLut->GGain);

			snprintf(name, sizeof(name), "mlsc_gain_b");
			FPRINTF_TBL(MlscGainLut->BGain);
		} else {
			CVI_TRACE_VI(CVI_DBG_INFO, "MLSC is no data\n");
		}
	}

	// MANR
	// register is write only

	// RGB_GAMMA
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "GAMMA\n");
		ip_info_id = IP_INFO_ID_GAMMA;
		GET_BASE_VADDR(ip_info_id);

		length = 256;
		CVI_U32 *data_gamma_r = calloc(1, sizeof(CVI_U32) * length);
		CVI_U32 *data_gamma_g = calloc(1, sizeof(CVI_U32) * length);
		CVI_U32 *data_gamma_b = calloc(1, sizeof(CVI_U32) * length);

		SET_SW_MODE_ENABLE(0x4, 8, 1);
		SET_SW_MODE_MEM_SEL(0x14, 12, 1);
		for (CVI_U32 i = 0 ; i < length; i++) {
			val = vaddr + 0x14;
			data = (*val & (~0xFF)) | i;
			*val = data;
			usleep(1);

			val = vaddr + 0x18;
			data = (*val | (0x1 << 31));
			*val = data;
			usleep(1);

			val = vaddr + 0x18;
			data = *val;
			data_gamma_r[i] = (data & 0xFFF);

			val = vaddr + 0x1C;
			data = *val;
			data_gamma_g[i] = (data & 0xFFF);
			data_gamma_b[i] = ((data >> 16) & 0xFFF);
		}
		SET_SW_MODE_ENABLE(0x4, 8, 0);

		snprintf(name, sizeof(name), "gamma_r");
		FPRINTF_TBL(data_gamma_r);

		snprintf(name, sizeof(name), "gamma_g");
		FPRINTF_TBL(data_gamma_g);

		snprintf(name, sizeof(name), "gamma_b");
		FPRINTF_TBL(data_gamma_b);

		free(data_gamma_r);
		free(data_gamma_g);
		free(data_gamma_b);
	}

	// CLUT
	// There is a probability that the clut register will not be read
	// used tuning buffer
	{
		#if 1
		CVI_TRACE_VI(CVI_DBG_INFO, "CLUT\n");
		struct cvi_vip_isp_clut_config *clut_cfg =
			(struct cvi_vip_isp_clut_config *)&(post_addr->tun_cfg[0].clut_cfg);

		length = 4913;
		snprintf(name, sizeof(name), "clut_r");
		FPRINTF_TBL(clut_cfg->r_lut);

		snprintf(name, sizeof(name), "clut_g");
		FPRINTF_TBL(clut_cfg->g_lut);

		snprintf(name, sizeof(name), "clut_b");
		FPRINTF_TBL(clut_cfg->b_lut);
		#else
		CVI_TRACE_VI(CVI_DBG_INFO, "CLUT\n");
		ip_info_id = IP_INFO_ID_CLUT;
		GET_BASE_VADDR(ip_info_id);

		CVI_U32 r_idx = 17;
		CVI_U32 g_idx = 17;
		CVI_U32 b_idx = 17;
		CVI_U32 rgb_idx = 0;

		length = r_idx * g_idx * b_idx;
		CVI_U32 data_clut_r[length];
		CVI_U32 data_clut_g[length];
		CVI_U32 data_clut_b[length];

		SET_SW_MODE_ENABLE(0x0, 0, 0); // reg_clut_enable
		SET_SW_MODE_ENABLE(0x0, 1, 0); // reg_clut_shdw_sel

		do {
			usleep(10);
			val = vaddr + 0x00;
			data = *val;
		} while ((data & 0x1) != 0);

		SET_SW_MODE_ENABLE(0x0, 3, 1); // reg_prog_en

		for (CVI_U32 i = 0 ; i < b_idx; i++) {
			for (CVI_U32 j = 0 ; j < g_idx; j++) {
				for (CVI_U32 k = 0 ; k < r_idx; k++) {
					rgb_idx = i * g_idx * r_idx + j * r_idx + k;

					val = vaddr + 0x04; // reg_sram_r_idx/reg_sram_g_idx/reg_sram_b_idx
					data = (i << 16) | (j << 8) | k;
					*val = data;
					usleep(1);

					val = vaddr + 0x0C; // reg_sram_rd
					data = (0x1 << 31);
					*val = data;
					usleep(1);

					val = vaddr + 0x0C; // reg_sram_rdata
					data = *val;
					usleep(1);

					data_clut_r[rgb_idx] = (data >> 20) & 0x3FF;
					data_clut_g[rgb_idx] = (data >> 10) & 0x3FF;
					data_clut_b[rgb_idx] = data & 0x3FF;
				}
			}
		}

		SET_SW_MODE_ENABLE(0x0, 3, 0); // reg_prog_en
		SET_SW_MODE_ENABLE(0x0, 1, 1); // reg_clut_shdw_sel

		snprintf(name, sizeof(name), "clut_r");
		FPRINTF_TBL(data_clut_r);

		snprintf(name, sizeof(name), "clut_g");
		FPRINTF_TBL(data_clut_g);

		snprintf(name, sizeof(name), "clut_b");
		FPRINTF_TBL(data_clut_b);
		#endif
	}

	// DCI_GAMMA
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "DCI_GAMMA\n");
		ip_info_id = IP_INFO_ID_DCI;
		GET_BASE_VADDR(ip_info_id);

		length = 256;
		CVI_U32 *data_gamma_r = calloc(1, sizeof(CVI_U32) * length);
		CVI_U32 *data_gamma_g = calloc(1, sizeof(CVI_U32) * length);
		CVI_U32 *data_gamma_b = calloc(1, sizeof(CVI_U32) * length);

		SET_SW_MODE_ENABLE(0x204, 8, 1);
		SET_SW_MODE_MEM_SEL(0x214, 12, 1);
		for (CVI_U32 i = 0 ; i < length; i++) {
			val = vaddr + 0x214;
			data = (*val & (~0xFF)) | i;
			*val = data;
			usleep(1);

			val = vaddr + 0x218;
			data = (*val | (0x1 << 31));
			*val = data;
			usleep(1);

			val = vaddr + 0x218;
			data = *val;
			data_gamma_r[i] = (data & 0xFFF);

			val = vaddr + 0x21C;
			data = *val;
			data_gamma_g[i] = (data & 0xFFF);
			data_gamma_b[i] = ((data >> 16) & 0xFFF);
		}
		SET_SW_MODE_ENABLE(0x204, 8, 0);

		snprintf(name, sizeof(name), "dci_gamma_r");
		FPRINTF_TBL(data_gamma_r);

		snprintf(name, sizeof(name), "dci_gamma_g");
		FPRINTF_TBL(data_gamma_g);

		snprintf(name, sizeof(name), "dci_gamma_b");
		FPRINTF_TBL(data_gamma_b);

		free(data_gamma_r);
		free(data_gamma_g);
		free(data_gamma_b);
	}

	// LTM
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "LTM\n");
		ip_info_id = IP_INFO_ID_HDRLTM;
		GET_BASE_VADDR(ip_info_id);

		length = 257;
		CVI_U32 *data_dtone_curve = calloc(1, sizeof(CVI_U32) * length);

		DUMP_LUT_COMMON(data_dtone_curve, 0xFFF, 0xF0, 0xF4, 0xF8, 0xF8);
		val = vaddr + 0xe8; //reg_dtone_curve_max
		data_dtone_curve[length - 1] = *val;
		snprintf(name, sizeof(name), "ltm_dtone_curve");
		FPRINTF_TBL(data_dtone_curve);

		length = 513;
		CVI_U32 *data_btone_curve = calloc(1, sizeof(CVI_U32) * length);

		DUMP_LUT_COMMON(data_btone_curve, 0xFFF, 0x110, 0x114, 0x118, 0x118);
		val = vaddr + 0x108; //reg_btone_curve_max
		data_btone_curve[length - 1] = *val;
		snprintf(name, sizeof(name), "ltm_btone_curve");
		FPRINTF_TBL(data_btone_curve);

		length = 769;
		CVI_U32 *data_global_curve = calloc(1, sizeof(CVI_U32) * length);

		DUMP_LUT_COMMON(data_global_curve, 0xFFF, 0x130, 0x134, 0x138, 0x138);
		val = vaddr + 0x128; //reg_global_curve_max
		data_global_curve[length - 1] = *val;
		snprintf(name, sizeof(name), "ltm_global_curve");
		FPRINTF_TBL(data_global_curve);

		free(data_dtone_curve);
		free(data_btone_curve);
		free(data_global_curve);
	}

	// YNR
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "YNR\n");
		ip_info_id = IP_INFO_ID_YNR;
		GET_BASE_VADDR(ip_info_id);
		CLEAR_ACCESS_CNT(0x8);

		length = 6;
		offset = 0x100;
		snprintf(name, sizeof(name), "ynr_ns0_luma_th");
		FPRINTF_VAL();

		length = 5;
		offset = 0x104;
		snprintf(name, sizeof(name), "ynr_ns0_slope");
		FPRINTF_VAL();

		length = 6;
		offset = 0x108;
		snprintf(name, sizeof(name), "ynr_ns0_offset");
		FPRINTF_VAL();

		length = 6;
		offset = 0x110;
		snprintf(name, sizeof(name), "ynr_ns1_luma_th");
		FPRINTF_VAL();

		length = 5;
		offset = 0x114;
		snprintf(name, sizeof(name), "ynr_ns1_slope");
		FPRINTF_VAL();

		length = 6;
		offset = 0x118;
		snprintf(name, sizeof(name), "ynr_ns1_offset");
		FPRINTF_VAL();

		length = 8;
		offset = 0x134;
		snprintf(name, sizeof(name), "ynr_intensity_sel");
		FPRINTF_VAL();

		length = 16;
		offset = 0x138;
		snprintf(name, sizeof(name), "ynr_motion_lut");
		FPRINTF_VAL();

		length = 64;
		offset = 0x228;
		snprintf(name, sizeof(name), "ynr_weight_lut");
		FPRINTF_VAL();

		length = 16;
		offset = 0x260;
		snprintf(name, sizeof(name), "ynr_res_mot_lut");
		FPRINTF_VAL();
	}

	// YCURVE
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "YCURVE\n");
		ip_info_id = IP_INFO_ID_YCURVE;
		GET_BASE_VADDR(ip_info_id);

		length = 64;
		CVI_U32 *data_ycurve = calloc(1, sizeof(CVI_U32) * length);

		SET_SW_MODE_ENABLE(0x4, 8, 1);
		SET_SW_MODE_MEM_SEL(0x14, 8, 1);
		for (CVI_U32 i = 0 ; i < length; i++) {
			val = vaddr + 0x14;
			data = (*val & ~(0x3F)) | i;
			*val = data;
			usleep(1);

			val = vaddr + 0x18;
			data = (*val | (0x1 << 31));
			*val = data;
			usleep(1);

			val = vaddr + 0x18;
			data = *val;
			data_ycurve[i] = (data & 0xFF);
		}
		SET_SW_MODE_ENABLE(0x4, 8, 0);

		snprintf(name, sizeof(name), "ycurve");
		FPRINTF_TBL(data_ycurve);

		free(data_ycurve);
	}

	/* end of file */
	fprintf(fp, "\t\"end\": {}\n");
	fprintf(fp, "}");
	/* start tuning update */
	system("echo 0,0,0,0 > /sys/module/cv182x_vip/parameters/tuning_dis");

	free(tun_buf_info);
	free(ip_info_list);
#endif
	return CVI_SUCCESS;
}

CVI_S32 dump_register_183x(VI_PIPE ViPipe, FILE *fp, VI_DUMP_REGISTER_TABLE_S *pstRegTbl)
{
	VDEV_CLOSED_CHK(VDEV_TYPE_ISP, ViPipe);
	CHECK_VI_NULL_PTR(fp);
	CHECK_VI_NULL_PTR(pstRegTbl);

#ifdef ARCH_CV183X
	CVI_S32 s32Ret = CVI_SUCCESS;
	struct ip_info ip_info_list[IP_INFO_ID_MAX];
	struct isp_tuning_cfg tun_buf_info;

	CVI_U32 paddr;
	CVI_U32 size;
	CVI_VOID *vaddr;
	CVI_U32 *val;

	CVI_U32 ip_info_id;
	CVI_U32 offset;
	CVI_U32 length;
	CVI_U32 data;
	CVI_CHAR name[32];

	s32Ret = vi_get_ip_dump_list(d->fd, ip_info_list);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VI(CVI_DBG_ERR, "vi_get_ip_dump_list ioctl\n");
		return CVI_FAILURE;
	}

	memset(&tun_buf_info, 0, sizeof(struct isp_tuning_cfg));
	s32Ret = vi_get_tun_addr(d->fd, &tun_buf_info);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VI(CVI_DBG_ERR, "vi_get_tun_addr ioctl\n");
		return CVI_FAILURE;
	}

	/* stop tuning update */
	if (ViPipe == 0) {
		system("echo 1,1,1 > /sys/module/cv183x_vip/parameters/tuning_dis");
	} else if (ViPipe == 1) {
		system("echo 2,1,1 > /sys/module/cv183x_vip/parameters/tuning_dis");
	}
	/* In the worst case, have to wait two frames to stop tuning update. */
	usleep(80 * 1000);
	/* start of file */
	fprintf(fp, "{\n");
	/* dump isp moudle register */
	for (CVI_U32 i = 0; i < IP_INFO_ID_MAX; i++) {
		GET_BASE_VADDR(i);
		val = vaddr;

		fprintf(fp, "\t\"0x%08X\": {\n", paddr);
		for (CVI_U32 j = 0 ; j < (size / 0x4); j++) {
			fprintf(fp, "\t\t\"h%02x\": %u,\n", (j * 4), *(val++));
		}

		// if (i != IP_INFO_ID_MAX - 1)
			fprintf(fp, "\t\t\"size\": %u\n\t},\n", size);
		// else
			// fprintf(fp, "\t\t\"size\": %u\n\t}\n", size);

		CVI_SYS_Munmap(vaddr, size);
	}

	/* dump look up table */
	// struct cvi_vip_isp_pre_cfg *pre_addr;
	struct cvi_vip_isp_post_cfg *post_addr;

	tun_buf_info.pre_vir[ViPipe] =
			CVI_SYS_Mmap(tun_buf_info.pre_addr[ViPipe], sizeof(struct cvi_vip_isp_pre_cfg));
	tun_buf_info.post_vir[ViPipe] =
			CVI_SYS_Mmap(tun_buf_info.post_addr[ViPipe], sizeof(struct cvi_vip_isp_post_cfg));
	// pre_addr = (struct cvi_vip_isp_pre_cfg *)(tun_buf_info.pre_vir[ViPipe]);
	post_addr = (struct cvi_vip_isp_post_cfg *)(tun_buf_info.post_vir[ViPipe]);

	// LSCR
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "LSCR_LE\n");
		ip_info_id = (ViPipe == 0) ? IP_INFO_ID_LSCR0 : IP_INFO_ID_LSCR2_R1;
		GET_BASE_VADDR(ip_info_id);
		CLEAR_ACCESS_CNT(0x8);

		length = 32;
		offset = 0x114;
		snprintf(name, sizeof(name), "lscr_le_lsc_lut");
		FPRINTF_VAL();

		if (vi_ctx.devAttr[ViPipe].stWDRAttr.enWDRMode == WDR_MODE_2To1_LINE) {
			CVI_TRACE_VI(CVI_DBG_INFO, "LSCR_SE\n");
			ip_info_id = (ViPipe == 0) ? IP_INFO_ID_LSCR1 : IP_INFO_ID_LSCR3_R1;
			GET_BASE_VADDR(ip_info_id);
			CLEAR_ACCESS_CNT(0x8);

			length = 32;
			offset = 0x114;
			snprintf(name, sizeof(name), "lscr_se_lsc_lut");
			FPRINTF_VAL();
		}
	}

	// AF_GAMMA
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "AF_GAMMA\n");
		ip_info_id = (ViPipe == 0) ? IP_INFO_ID_AF_GAMMA : IP_INFO_ID_AF_GAMMA_R1;
		GET_BASE_VADDR(ip_info_id);

		length = 256;
		CVI_U32 data_af_gamma[length];

		SET_SW_MODE_ENABLE(0x10, 0, 1);
		SET_SW_MODE_MEM_SEL(0x10, 4, 1);
		DUMP_LUT_BASE(data_af_gamma, 0xFF, 0x14, 0x18, 0x18);
		SET_SW_MODE_ENABLE(0x10, 0, 0);

		snprintf(name, sizeof(name), "af_gamma");
		FPRINTF_TBL(data_af_gamma);
	}

	// DPC
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "DPC_LE\n");
		ip_info_id = IP_INFO_ID_DPC0;
		GET_BASE_VADDR(ip_info_id);

		length = 4096;
		CVI_U32 data_dpc[length];

		val = vaddr + 0x08; // reg_dpc_staticbpc_enable
		data = *val & (~(0x3 << 2));
		*val = data;

		SET_SW_MODE_ENABLE(0x44, 31, 1);
		DUMP_LUT_BASE(data_dpc, 0xFFFFFF, 0x48, 0x4C, 0x4C);
		SET_SW_MODE_ENABLE(0x44, 31, 0);

		val = vaddr + 0x08;
		data = *val | (0x3 << 2);
		*val = data;

		snprintf(name, sizeof(name), "dpc_le_bp_tbl");
		FPRINTF_TBL(data_dpc);

		if (vi_ctx.devAttr[ViPipe].stWDRAttr.enWDRMode == WDR_MODE_2To1_LINE) {
			CVI_TRACE_VI(CVI_DBG_INFO, "DPC_SE\n");
			ip_info_id = IP_INFO_ID_DPC1;
			GET_BASE_VADDR(ip_info_id);

			val = vaddr + 0x08;
			data = *val & (~(0x3 << 2));
			*val = data;

			SET_SW_MODE_ENABLE(0x44, 31, 1);
			DUMP_LUT_BASE(data_dpc, 0xFFFFFF, 0x48, 0x4C, 0x4C);
			SET_SW_MODE_ENABLE(0x44, 31, 0);

			val = vaddr + 0x08;
			data = *val | (0x3 << 2);
			*val = data;

			snprintf(name, sizeof(name), "dpc_se_bp_tbl");
			FPRINTF_TBL(data_dpc);
		}
	}

	// MLSC
	// NO INDIRECT LUT
	// use the isp tun buffer
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "MLSC\n");
		MLSC_GAIN_LUT_S *MlscGainLut = &pstRegTbl->MlscGainLut;

		length = 1369;
		if (MlscGainLut->RGain && MlscGainLut->GGain && MlscGainLut->BGain) {
			snprintf(name, sizeof(name), "mlsc_gain_r");
			FPRINTF_TBL(MlscGainLut->RGain);

			snprintf(name, sizeof(name), "mlsc_gain_g");
			FPRINTF_TBL(MlscGainLut->GGain);

			snprintf(name, sizeof(name), "mlsc_gain_b");
			FPRINTF_TBL(MlscGainLut->BGain);
		} else {
			CVI_TRACE_VI(CVI_DBG_INFO, "MLSC is no data\n");
		}
	}

	// LTM
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "LTM\n");
		ip_info_id = IP_INFO_ID_HDRLTM;
		GET_BASE_VADDR(ip_info_id);

		length = 257;
		CVI_U32 data_dtone_curve[length];

		DUMP_LUT_COMMON(data_dtone_curve, 0xFFF, 0xF0, 0xF4, 0xF8, 0xF8);
		val = vaddr + 0xe8; //reg_dtone_curve_max
		data_dtone_curve[length - 1] = *val;
		snprintf(name, sizeof(name), "ltm_dtone_curve");
		FPRINTF_TBL(data_dtone_curve);

		length = 513;
		CVI_U32 data_btone_curve[length];

		DUMP_LUT_COMMON(data_btone_curve, 0xFFF, 0x110, 0x114, 0x118, 0x118);
		val = vaddr + 0x108; //reg_btone_curve_max
		data_btone_curve[length - 1] = *val;
		snprintf(name, sizeof(name), "ltm_btone_curve");
		FPRINTF_TBL(data_btone_curve);

		length = 769;
		CVI_U32 data_global_curve[length];

		DUMP_LUT_COMMON(data_global_curve, 0xFFF, 0x130, 0x134, 0x138, 0x138);
		val = vaddr + 0x128; //reg_global_curve_max
		data_global_curve[length - 1] = *val;
		snprintf(name, sizeof(name), "ltm_global_curve");
		FPRINTF_TBL(data_global_curve);
	}

	// BNR
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "BNR\n");
		ip_info_id = IP_INFO_ID_BNR;
		GET_BASE_VADDR(ip_info_id);
		CLEAR_ACCESS_CNT(0x8);

		length = 8;
		offset = 0x148;
		snprintf(name, sizeof(name), "bnr_intensity_sel");
		FPRINTF_VAL();

		length = 256;
		offset = 0x228;
		snprintf(name, sizeof(name), "bnr_weight_lut");
		FPRINTF_VAL();

		length = 32;
		offset = 0x424;
		snprintf(name, sizeof(name), "bnr_lsc_gain_lut");
		FPRINTF_VAL();
	}
	// MANR
	// register is write only

	// FPN
	// register is write only
	// use the fixed value
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "FPN\n");
		length = 256;
		CVI_U32 data_offset[length];
		CVI_U32 data_gain[length];

		for (CVI_U32 i = 0 ; i < length; i++) {
			data_offset[i] = i * 4;
			data_gain[i] = 1024;
		}

		snprintf(name, sizeof(name), "fpn_data_offset");
		FPRINTF_TBL(data_offset);

		snprintf(name, sizeof(name), "fpn_data_gain");
		FPRINTF_TBL(data_gain);
	}

	// RGB_GAMMA
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "GAMMA\n");
		ip_info_id = IP_INFO_ID_GAMMA;
		GET_BASE_VADDR(ip_info_id);

		length = 256;
		CVI_U32 data_gamma_r[length];
		CVI_U32 data_gamma_g[length];
		CVI_U32 data_gamma_b[length];

		SET_SW_MODE_ENABLE(0x10, 0, 1);
		SET_SW_MODE_MEM_SEL(0x10, 4, 1);
		for (CVI_U32 i = 0 ; i < length; i++) {
			val = vaddr + 0x14;
			*val = i;

			val = vaddr + 0x18;
			data = (*val | (0x1 << 31));
			*val = data;
			usleep(1);

			val = vaddr + 0x18;
			data = *val;
			data_gamma_r[i] = (data & 0xFFF);

			val = vaddr + 0x1C;
			data = *val;
			data_gamma_g[i] = (data & 0xFFF);
			data_gamma_b[i] = ((data >> 16) & 0xFFF);
		}
		SET_SW_MODE_ENABLE(0x10, 0, 0);

		snprintf(name, sizeof(name), "gamma_r");
		FPRINTF_TBL(data_gamma_r);

		snprintf(name, sizeof(name), "gamma_g");
		FPRINTF_TBL(data_gamma_g);

		snprintf(name, sizeof(name), "gamma_b");
		FPRINTF_TBL(data_gamma_b);
	}

	// HSV
	// register is write only
	// use the isp tun buffer
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "HSV\n");
		struct cvi_vip_isp_hsv_config *hsv_cfg =
			(struct cvi_vip_isp_hsv_config *)&(post_addr->tun_cfg[0].hsv_cfg);

		length = 769;
		snprintf(name, sizeof(name), "hsv_h_lut");
		FPRINTF_TBL(hsv_cfg->h_lut);

		length = 513;
		snprintf(name, sizeof(name), "hsv_s_lut");
		FPRINTF_TBL(hsv_cfg->s_lut);

		length = 769;
		snprintf(name, sizeof(name), "hsv_sgain_lut");
		FPRINTF_TBL(hsv_cfg->sgain_lut);

		length = 769;
		snprintf(name, sizeof(name), "hsv_vgain_lut");
		FPRINTF_TBL(hsv_cfg->vgain_lut);
	}

	// CLUT
	// There is a probability that the clut register will not be read
	// used tuning buffer
	{
		#if 1
		CVI_TRACE_VI(CVI_DBG_INFO, "CLUT\n");
		struct cvi_vip_isp_3dlut_config *thrdlut_cfg =
			(struct cvi_vip_isp_3dlut_config *)&(post_addr->tun_cfg[0].thrdlut_cfg);

		length = 3276;
		snprintf(name, sizeof(name), "clut_h");
		FPRINTF_TBL(thrdlut_cfg->h_lut);

		snprintf(name, sizeof(name), "clut_s");
		FPRINTF_TBL(thrdlut_cfg->s_lut);

		snprintf(name, sizeof(name), "clut_v");
		FPRINTF_TBL(thrdlut_cfg->v_lut);
		#else
		CVI_TRACE_VI(CVI_DBG_INFO, "CLUT\n");
		ip_info_id = IP_INFO_ID_YUVTOP;
		GET_BASE_VADDR(ip_info_id);

		CVI_U32 h_idx = 28;
		CVI_U32 s_idx = 13;
		CVI_U32 v_idx = 9;
		CVI_U32 hsv_idx = 0;

		length = h_idx * s_idx * v_idx;
		CVI_U64 data_clut[length];

		SET_SW_MODE_ENABLE(0x50, 0, 0);

		for (CVI_U32 i = 0 ; i < v_idx; i++) {
			for (CVI_U32 j = 0 ; j < s_idx; j++) {
				for (CVI_U32 k = 0 ; k < h_idx; k++) {
					hsv_idx = i * s_idx * h_idx + j * h_idx + k;

					val = vaddr + 0x44;
					data = (k << 2) | (j << 7) | (i << 11);
					*val = data;
					usleep(1);

					val = vaddr + 0x44;
					data = (data | (0x1 << 16));
					*val = data;
					usleep(1);

					val = vaddr + 0x48; // LSB
					data = *val;

					val = vaddr + 0x4C; // MSB
					data_clut[hsv_idx] = ((CVI_U64)(*val & 0x3) << 32) | data;
				}
			}
		}

		SET_SW_MODE_ENABLE(0x50, 0, 1);

		fprintf(fp, "\t\"clut_hsv\": {\n");
		fprintf(fp, "\t\t\"length\": %u,\n", length);
		fprintf(fp, "\t\t\"lut\": [\n");
		fprintf(fp, "\t\t\t");
		for (CVI_U32 i = 0 ; i < length; i++) {
			if (i == length - 1) {
				fprintf(fp, "%"PRIu64"\n", data_clut[i]);
			} else if (i % 16 == 15) {
				fprintf(fp, "%"PRIu64",\n\t\t\t", data_clut[i]);
			} else {
				fprintf(fp, "%"PRIu64",\t", data_clut[i]);
			}
		}
		fprintf(fp, "\t\t]\n\t},\n");
		#endif
	}

	// YNR
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "YNR\n");
		ip_info_id = IP_INFO_ID_YNR;
		GET_BASE_VADDR(ip_info_id);
		CLEAR_ACCESS_CNT(0x8);

		length = 6;
		offset = 0x100;
		snprintf(name, sizeof(name), "ynr_ns0_luma_th");
		FPRINTF_VAL();

		length = 5;
		offset = 0x104;
		snprintf(name, sizeof(name), "ynr_ns0_slope");
		FPRINTF_VAL();

		length = 6;
		offset = 0x108;
		snprintf(name, sizeof(name), "ynr_ns0_offset_th");
		FPRINTF_VAL();

		length = 6;
		offset = 0x110;
		snprintf(name, sizeof(name), "ynr_ns1_luma_th");
		FPRINTF_VAL();

		length = 5;
		offset = 0x114;
		snprintf(name, sizeof(name), "ynr_ns1_slope");
		FPRINTF_VAL();

		length = 6;
		offset = 0x118;
		snprintf(name, sizeof(name), "ynr_ns1_offset_th");
		FPRINTF_VAL();

		length = 8;
		offset = 0x134;
		snprintf(name, sizeof(name), "ynr_intensity_sel");
		FPRINTF_VAL();

		length = 64;
		offset = 0x228;
		snprintf(name, sizeof(name), "ynr_weight_lut_h");
		FPRINTF_VAL();
	}

	// YCURVE
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "YCURVE\n");
		ip_info_id = IP_INFO_ID_YCURVE;
		GET_BASE_VADDR(ip_info_id);

		length = 64;
		CVI_U32 data_ycurve[length];

		SET_SW_MODE_ENABLE(0x10, 0, 1);
		SET_SW_MODE_MEM_SEL(0x10, 4, 1);
		DUMP_LUT_BASE(data_ycurve, 0xFF, 0x14, 0x18, 0x18);
		SET_SW_MODE_ENABLE(0x10, 0, 0);

		snprintf(name, sizeof(name), "ycurve");
		FPRINTF_TBL(data_ycurve);
	}

	// DCI
	// NO INDIRECT LUT
	// use the isp tun buffer
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "DCI\n");
		struct cvi_vip_isp_dci_config *dci_cfg =
			(struct cvi_vip_isp_dci_config *)&(post_addr->tun_cfg[0].dci_cfg);

		length = 256;
		snprintf(name, sizeof(name), "dci_map_lut");
		FPRINTF_TBL(dci_cfg->map_lut);
	}

	/* end of file */
	fprintf(fp, "\t\"end\": {}\n");
	fprintf(fp, "}");
	/* start tuning update */
	system("echo 0,0,0 > /sys/module/cv183x_vip/parameters/tuning_dis");

#endif
	return CVI_SUCCESS;
}

CVI_S32 dump_hw_register(VI_PIPE ViPipe, FILE *fp, VI_DUMP_REGISTER_TABLE_S *pstRegTbl)
{
	VDEV_CLOSED_CHK(VDEV_TYPE_ISP, ViPipe);
	CHECK_VI_NULL_PTR(fp);
	CHECK_VI_NULL_PTR(pstRegTbl);

#if defined(__SOC_MARS__) || defined(__SOC_PHOBOS__)
	CVI_S32 s32Ret = CVI_SUCCESS;
	VI_DEV_ATTR_S stDevAttr;
	struct ip_info *ip_info_list;

	ip_info_list = calloc(1, sizeof(struct ip_info) * IP_INFO_ID_MAX);

	CVI_U32 paddr;
	CVI_U32 size;
	CVI_VOID *vaddr;
	CVI_U32 *val;

	CVI_U32 ip_info_id;
	CVI_U32 offset;
	CVI_U32 length;
	CVI_U32 data;
	CVI_CHAR name[32];

	s32Ret = vi_sdk_get_dev_attr(d->fd, (int)ViPipe, &stDevAttr);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VI(CVI_DBG_ERR, "vi_sdk_get_dev_attr ioctl\n");
		free(ip_info_list);
		return s32Ret;
	}

	s32Ret = vi_get_ip_dump_list(d->fd, ip_info_list);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VI(CVI_DBG_ERR, "vi_get_ip_dump_list ioctl\n");
		free(ip_info_list);
		return s32Ret;
	}
#ifdef __SOC_MARS__
	/* stop tuning update */
	if (ViPipe == 0) {
		system("echo 1,1,1,1 > /sys/module/cv181x_vi/parameters/tuning_dis");
	} else if (ViPipe == 1) {
		system("echo 2,1,1,1 > /sys/module/cv181x_vi/parameters/tuning_dis");
	}
#else
	/* stop tuning update */
	if (ViPipe == 0) {
		system("echo 1,1,1,1 > /sys/module/cv180x_vi/parameters/tuning_dis");
	}
#endif
	/* In the worst case, have to wait two frames to stop tuning update. */
	usleep(80 * 1000);
	/* start of file */
	fprintf(fp, "{\n");
	/* dump isp moudle register */
	for (CVI_U32 i = 0; i < IP_INFO_ID_MAX; i++) {
		CVI_TRACE_VI(CVI_DBG_INFO, "%d\n", i);

		GET_BASE_VADDR(i);
		val = vaddr;

		fprintf(fp, "\t\"0x%08X\": {\n", paddr);
		for (CVI_U32 j = 0 ; j < (size / 0x4); j++) {
			fprintf(fp, "\t\t\"h%02x\": %u,\n", (j * 4), *(val++));
		}
		fprintf(fp, "\t\t\"size\": %u\n\t},\n", size);

		CVI_SYS_Munmap(vaddr, size);
	}

	/* dump look up table */
	// BNR
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "BNR\n");
		ip_info_id = IP_INFO_ID_BNR;

		CVI_U8 shdw_sel = 0;

		GET_BASE_VADDR(ip_info_id);
		GET_REGISTER_COMMON(0x0, 0, shdw_sel);
		SET_REGISTER_COMMON(0x0, 0, 0); // reg_shadow_rd_sel

		CLEAR_ACCESS_CNT(0x8);

		length = 8;
		offset = 0x148;
		snprintf(name, sizeof(name), "bnr_intensity_sel");
		FPRINTF_VAL();

		length = 256;
		offset = 0x228;
		snprintf(name, sizeof(name), "bnr_weight_lut");
		FPRINTF_VAL();

		SET_REGISTER_COMMON(0x0, 0, shdw_sel); // reg_shadow_rd_sel
	}

	// DPC
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "DPC\n");

		CVI_U8 enable = 0;
		// CVI_U8 shdw_sel = 0;
		CVI_U8 force_clk_enable = 0;
#ifdef __SOC_MARS__
		CVI_U8 idx = (stDevAttr.stWDRAttr.enWDRMode == WDR_MODE_2To1_LINE) ? 2 : 1;
		CVI_U32 *data_dpc = NULL;
		CVI_CHAR name_list[2][16] = {"dpc_le_bp_tbl", "dpc_se_bp_tbl"};
		enum IP_INFO_GRP ip_info_id_list[2] = {IP_INFO_ID_DPC0, IP_INFO_ID_DPC1};
#else
		CVI_U8 idx = 1;
		CVI_U32 *data_dpc = NULL;
		CVI_CHAR name_list[1][16] = {"dpc_le_bp_tbl"};
		enum IP_INFO_GRP ip_info_id_list[1] = {IP_INFO_ID_DPC0};
#endif

		length = 2047;

		for (CVI_U8 i = 0; i < idx; i++) {
			GET_BASE_VADDR(ip_info_id_list[i]);
			data_dpc = calloc(1, sizeof(CVI_U32) * length);

			GET_REGISTER_COMMON(0x8, 0, enable);
			// GET_REGISTER_COMMON(0x4, 0, shdw_sel);
			GET_REGISTER_COMMON(0x8, 8, force_clk_enable);

			SET_REGISTER_COMMON(0x8, 0, 0); // reg_dpc_enable
			// SET_REGISTER_COMMON(0x4, 0, 0); // reg_shdw_read_sel
			SET_REGISTER_COMMON(0x8, 8, 0); // reg_force_clk_enable
			WAIT_IP_DISABLE(0x8, 0);
			SET_REGISTER_COMMON(0x44, 31, 1); // reg_dpc_mem_prog_mode

			DUMP_LUT_BASE(data_dpc, 0xFFFFFF, 0x48, 0x4C, 0x4C);

			SET_REGISTER_COMMON(0x44, 31, 0); // reg_dpc_mem_prog_mode
			SET_REGISTER_COMMON(0x8, 8, force_clk_enable); // reg_force_clk_enable
			// SET_REGISTER_COMMON(0x4, 0, shdw_sel); // reg_shdw_read_sel
			SET_REGISTER_COMMON(0x8, 0, enable); // reg_dpc_enable

			snprintf(name, sizeof(name), name_list[i]);
			FPRINTF_TBL(data_dpc);

			free(data_dpc);
			CVI_SYS_Munmap(vaddr, size);
		}
	}

	// MLSC
	// NO INDIRECT LUT
	// use the isp tun buffer
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "MLSC\n");
		MLSC_GAIN_LUT_S *MlscGainLut = &pstRegTbl->MlscGainLut;

		length = 1369;
		if (MlscGainLut->RGain && MlscGainLut->GGain && MlscGainLut->BGain) {
			snprintf(name, sizeof(name), "mlsc_gain_r");
			FPRINTF_TBL(MlscGainLut->RGain);

			snprintf(name, sizeof(name), "mlsc_gain_g");
			FPRINTF_TBL(MlscGainLut->GGain);

			snprintf(name, sizeof(name), "mlsc_gain_b");
			FPRINTF_TBL(MlscGainLut->BGain);
		} else {
			CVI_TRACE_VI(CVI_DBG_INFO, "MLSC is no data\n");
		}
	}

	// RGB_GAMMA
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "RGB_GAMMA\n");

		struct gamma_tbl rgb_gamma = {
			.ip_info_id = IP_INFO_ID_RGBGAMMA,
			.name = "rgb_gamma",
			.length = 256,
			.enable = {
				.addr_ofs = 0x0,
				.val_ofs = 0,
			},
			.shdw_sel = {
				.addr_ofs = 0x0,
				.val_ofs = 1,
			},
			.force_clk_enable = {
				.addr_ofs = 0x0,
				.val_ofs = 2,
			},
			.prog_en = {
				.addr_ofs = 0x4,
				.val_ofs = 8,
			},
			.raddr = {
				.addr_ofs = 0x14,
				.mask = 0xFF,
				.val_ofs = 12,
			},
			.rdata_r = {
				.addr_ofs = 0x18,
				.val_ofs = 31,
				.mask = 0xFFF,
			},
			.rdata_gb = {
				.addr_ofs = 0x1C,
				.val_ofs = 16,
				.mask = 0xFFF,
			},
		};

		_dump_gamma_table(fp, ip_info_list, &rgb_gamma);
	}

	//Y_GAMMA
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "Y_GAMMA\n");

		struct gamma_tbl y_gamma = {
			.ip_info_id = IP_INFO_ID_YGAMMA,
			.name = "y_gamma",
			.length = 256,
			.enable = {
				.addr_ofs = 0x0,
				.val_ofs = 0,
			},
			.shdw_sel = {
				.addr_ofs = 0x0,
				.val_ofs = 1,
			},
			.force_clk_enable = {
				.addr_ofs = 0x0,
				.val_ofs = 2,
			},
			.prog_en = {
				.addr_ofs = 0x4,
				.val_ofs = 8,
			},
			.raddr = {
				.addr_ofs = 0x14,
				.mask = 0xFF,
				.val_ofs = 12,
			},
			.rdata_r = {
				.addr_ofs = 0x18,
				.val_ofs = 31,
				.mask = 0xFFFF,
			},
			.rdata_gb = {
				.addr_ofs = 0x1C,
				.val_ofs = 16,
				.mask = 0xFFF,
			},
		};

		_dump_gamma_table(fp, ip_info_list, &y_gamma);
	}

	// CLUT
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "CLUT\n");
		ip_info_id = IP_INFO_ID_CLUT;

		CVI_U32 r_idx = 17;
		CVI_U32 g_idx = 17;
		CVI_U32 b_idx = 17;
		CVI_U32 rgb_idx = 0;
		CVI_U32 *data_clut_r = NULL;
		CVI_U32 *data_clut_g = NULL;
		CVI_U32 *data_clut_b = NULL;
		CVI_U8 enable = 0;
		CVI_U8 shdw_sel = 0;
		// CVI_U8 force_clk_enable = 0;

		length = r_idx * g_idx * b_idx;
		data_clut_r = calloc(1, sizeof(CVI_U32) * length);
		data_clut_g = calloc(1, sizeof(CVI_U32) * length);
		data_clut_b = calloc(1, sizeof(CVI_U32) * length);

		GET_BASE_VADDR(ip_info_id);
		GET_REGISTER_COMMON(0x0, 0, enable);
		GET_REGISTER_COMMON(0x0, 1, shdw_sel);
		// GET_REGISTER_COMMON(0x0, 2, force_clk_enable);

		SET_REGISTER_COMMON(0x0, 0, 0); // reg_clut_enable
		SET_REGISTER_COMMON(0x0, 1, 0); // reg_clut_shdw_sel
		// SET_REGISTER_COMMON(0x0, 2, 0); // reg_force_clk_enable
		WAIT_IP_DISABLE(0x0, 0);
		SET_REGISTER_COMMON(0x0, 3, 1); // reg_prog_en

		for (CVI_U32 i = 0 ; i < b_idx; i++) {
			for (CVI_U32 j = 0 ; j < g_idx; j++) {
				for (CVI_U32 k = 0 ; k < r_idx; k++) {
					rgb_idx = i * g_idx * r_idx + j * r_idx + k;

					val = vaddr + 0x04; // reg_sram_r_idx/reg_sram_g_idx/reg_sram_b_idx
					data = (i << 16) | (j << 8) | k;
					*val = data;
					usleep(1);

					val = vaddr + 0x0C; // reg_sram_rd
					data = (0x1 << 31);
					*val = data;
					usleep(1);

					val = vaddr + 0x0C; // reg_sram_rdata
					data = *val;
					usleep(1);

					data_clut_r[rgb_idx] = (data >> 20) & 0x3FF;
					data_clut_g[rgb_idx] = (data >> 10) & 0x3FF;
					data_clut_b[rgb_idx] = data & 0x3FF;
				}
			}
		}

		SET_REGISTER_COMMON(0x0, 3, 0); // reg_prog_en
		// SET_REGISTER_COMMON(0x0, 2, force_clk_enable); // reg_force_clk_enable
		SET_REGISTER_COMMON(0x0, 1, shdw_sel); // reg_clut_shdw_sel
		SET_REGISTER_COMMON(0x0, 0, enable); // reg_clut_enable

		snprintf(name, sizeof(name), "clut_r");
		FPRINTF_TBL(data_clut_r);

		snprintf(name, sizeof(name), "clut_g");
		FPRINTF_TBL(data_clut_g);

		snprintf(name, sizeof(name), "clut_b");
		FPRINTF_TBL(data_clut_b);

		CVI_SYS_Munmap(vaddr, size);
		free(data_clut_r);
		free(data_clut_g);
		free(data_clut_b);
	}

	// LTM
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "LTM\n");
		ip_info_id = IP_INFO_ID_HDRLTM;

		CVI_U8 enable = 0;
		CVI_U8 shdw_sel = 0;
		CVI_U8 force_clk_enable = 0;

		GET_BASE_VADDR(ip_info_id);
		GET_REGISTER_COMMON(0x0, 0, enable);
		GET_REGISTER_COMMON(0x0, 5, shdw_sel);
		GET_REGISTER_COMMON(0x0, 31, force_clk_enable);

		SET_REGISTER_COMMON(0x0, 0, 0); // reg_ltm_enable
		SET_REGISTER_COMMON(0x0, 5, 0); // reg_shdw_read_sel
		SET_REGISTER_COMMON(0x0, 31, 0); // reg_force_clk_enable
		WAIT_IP_DISABLE(0x0, 0);

		// dark tone
		length = 257;
		CVI_U32 *data_dtone_curve = calloc(1, sizeof(CVI_U32) * length);

		SET_REGISTER_COMMON(0x34, 17, 1); // reg_lut_prog_en_dark
		for (CVI_U32 i = 0 ; i < length; i++) {
			val = vaddr + 0x34;
			data = (*val & ~(0x3FF)) | i; // reg_lut_dbg_raddr[0,9]
			data = (data | (0x1 << 15)); // reg_lut_dbg_read_en_1t
			*val = data;
			usleep(1);

			val = vaddr + 0x4C; // reg_lut_dbg_rdata
			data = *val;
			data_dtone_curve[i] = data;
		}
		SET_REGISTER_COMMON(0x34, 17, 0); // reg_lut_prog_en_dark
		val = vaddr + 0x44; //reg_dark_lut_max
		data_dtone_curve[length - 1] = *val;
		snprintf(name, sizeof(name), "ltm_dtone_curve");
		FPRINTF_TBL(data_dtone_curve);
		free(data_dtone_curve);

		// bright tone
#ifdef __SOC_MARS__
		length = 513;
#else
		length = 257;
#endif
		CVI_U32 *data_btone_curve = calloc(1, sizeof(CVI_U32) * length);

		SET_REGISTER_COMMON(0x34, 16, 1); // reg_lut_prog_en_bright
		for (CVI_U32 i = 0 ; i < length; i++) {
			val = vaddr + 0x34;
			data = (*val & ~(0x3FF)) | i; // reg_lut_dbg_raddr[0,9]
			data = (data | (0x1 << 15)); // reg_lut_dbg_read_en_1t
			*val = data;
			usleep(1);

			val = vaddr + 0x4C;
			data = *val;
			data_btone_curve[i] = data;
		}

		SET_REGISTER_COMMON(0x34, 16, 0); // reg_lut_prog_en_bright
		val = vaddr + 0x40; //reg_bright_lut_max
		data_btone_curve[length - 1] = *val;
		snprintf(name, sizeof(name), "ltm_btone_curve");
		FPRINTF_TBL(data_btone_curve);
		free(data_btone_curve);

		// global tone
#ifdef __SOC_MARS__
		length = 769;
#else
		length = 257;
#endif
		CVI_U32 *data_global_curve = calloc(1, sizeof(CVI_U32) * length);

		SET_REGISTER_COMMON(0x34, 18, 1); // reg_lut_prog_en_global
		for (CVI_U32 i = 0 ; i < length; i++) {
			val = vaddr + 0x34;
			data = (*val & ~(0x3FF)) | i; // reg_lut_dbg_raddr[0,9]
			data = (data | (0x1 << 15)); // reg_lut_dbg_read_en_1t
			*val = data;
			usleep(1);

			val = vaddr + 0x4C;
			data = *val;
			data_global_curve[i] = data;
		}
		SET_REGISTER_COMMON(0x34, 18, 0); // reg_lut_prog_en_global
		val = vaddr + 0x48; //reg_global_lut_max
		data_global_curve[length - 1] = *val;
		snprintf(name, sizeof(name), "ltm_global_curve");
		FPRINTF_TBL(data_global_curve);
		free(data_global_curve);

		SET_REGISTER_COMMON(0x0, 31, force_clk_enable); // reg_force_clk_enable
		SET_REGISTER_COMMON(0x0, 5, shdw_sel); // reg_shdw_read_sel
		SET_REGISTER_COMMON(0x0, 0, enable); // reg_ltm_enable

		CVI_SYS_Munmap(vaddr, size);
	}

	// CA_CP
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "CA_CP\n");
		length = 256;
		ip_info_id = IP_INFO_ID_CA;

		CVI_U8 enable = 0;
		CVI_U8 shdw_sel = 0;
		CVI_U8 ca_cp_mode = 0;
		CVI_U32 *data_cacp_y = calloc(1, sizeof(CVI_U32) * length);
		CVI_U32 *data_cacp_u = calloc(1, sizeof(CVI_U32) * length);
		CVI_U32 *data_cacp_v = calloc(1, sizeof(CVI_U32) * length);

		GET_BASE_VADDR(ip_info_id);
		GET_REGISTER_COMMON(0x0, 0, enable);
		GET_REGISTER_COMMON(0x0, 4, shdw_sel);
		GET_REGISTER_COMMON(0x0, 1, ca_cp_mode);

		SET_REGISTER_COMMON(0x0, 0, 0); // reg_cacp_enable
		SET_REGISTER_COMMON(0x0, 4, 0); // reg_cacp_shdw_read_sel
		WAIT_IP_DISABLE(0x0, 0);
		SET_REGISTER_COMMON(0x0, 3, 1); // reg_cacp_mem_sw_mode

		for (CVI_U32 i = 0 ; i < length; i++) {
			val = vaddr + 0x0C;
			data = (*val & ~0xFF) | i;
			*val = data;
			usleep(1);

			val = vaddr + 0x0C;
			data = (*val | (0x1 << 31));
			*val = data;
			usleep(1);

			val = vaddr + 0x10;
			data = *val;

			if (ca_cp_mode) {
				data_cacp_y[i] = ((data >> 16) & 0xFF);
				data_cacp_u[i] = ((data >> 8) & 0xFF);
				data_cacp_v[i] = (data & 0xFF);
			} else {
				data_cacp_y[i] = (data & 0x7FF);
			}
		}

		SET_REGISTER_COMMON(0x0, 3, 0);  // reg_cacp_mem_sw_mode
		SET_REGISTER_COMMON(0x0, 4, shdw_sel); // reg_cacp_shdw_read_sel
		SET_REGISTER_COMMON(0x0, 0, enable); // reg_cacp_enable

		if (ca_cp_mode) {
			snprintf(name, sizeof(name), "ca_cp_y");
			FPRINTF_TBL(data_cacp_y);
			snprintf(name, sizeof(name), "ca_cp_u");
			FPRINTF_TBL(data_cacp_u);
			snprintf(name, sizeof(name), "ca_cp_v");
			FPRINTF_TBL(data_cacp_v);
		} else {
			snprintf(name, sizeof(name), "ca_y_ratio");
			FPRINTF_TBL(data_cacp_y);
		}

		CVI_SYS_Munmap(vaddr, size);
		free(data_cacp_y);
		free(data_cacp_u);
		free(data_cacp_v);
	}

	// YNR
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "YNR\n");
		ip_info_id = IP_INFO_ID_YNR;

		CVI_U8 shdw_sel = 0;

		GET_BASE_VADDR(ip_info_id);
		GET_REGISTER_COMMON(0x0, 0, shdw_sel);

		SET_REGISTER_COMMON(0x0, 0, 0); // reg_shadow_rd_sel
		CLEAR_ACCESS_CNT(0x8);

		length = 6;
		offset = 0x00C;
		snprintf(name, sizeof(name), "ynr_ns0_luma_th");
		FPRINTF_VAL2();

		length = 5;
		offset = 0x024;
		snprintf(name, sizeof(name), "ynr_ns0_slope");
		FPRINTF_VAL2();

		length = 6;
		offset = 0x038;
		snprintf(name, sizeof(name), "ynr_ns0_offset");
		FPRINTF_VAL2();

		length = 6;
		offset = 0x050;
		snprintf(name, sizeof(name), "ynr_ns1_luma_th");
		FPRINTF_VAL2();

		length = 5;
		offset = 0x068;
		snprintf(name, sizeof(name), "ynr_ns1_slope");
		FPRINTF_VAL2();

		length = 6;
		offset = 0x07C;
		snprintf(name, sizeof(name), "ynr_ns1_offset");
		FPRINTF_VAL2();

		length = 16;
		offset = 0x098;
		snprintf(name, sizeof(name), "ynr_motion_lut");
		FPRINTF_VAL2();

		length = 16;
		offset = 0x260;
		snprintf(name, sizeof(name), "ynr_res_mot_lut");
		FPRINTF_VAL2();

		length = 64;
		offset = 0x200;
		snprintf(name, sizeof(name), "ynr_weight_lut");
		FPRINTF_VAL();

		SET_REGISTER_COMMON(0x0, 0, shdw_sel); // reg_shadow_rd_sel

		CVI_SYS_Munmap(vaddr, size);
	}

	// YCURVE
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "YCURVE\n");
		length = 64;
		ip_info_id = IP_INFO_ID_YCURVE;

		CVI_U8 enable = 0;
		CVI_U8 shdw_sel = 0;
		CVI_U8 force_clk_enable = 0;
		CVI_U32 *data_ycurve = calloc(1, sizeof(CVI_U32) * length);

		GET_BASE_VADDR(ip_info_id);
		GET_REGISTER_COMMON(0x0, 0, enable);
		GET_REGISTER_COMMON(0x0, 1, shdw_sel);
		GET_REGISTER_COMMON(0x0, 2, force_clk_enable);

		SET_REGISTER_COMMON(0x0, 0, 0); // reg_ycur_enable
		SET_REGISTER_COMMON(0x0, 1, 0); // reg_ycur_shdw_sel
		SET_REGISTER_COMMON(0x0, 2, 0); // reg_force_clk_enable
		WAIT_IP_DISABLE(0x0, 0);
		SET_REGISTER_COMMON(0x4, 8, 1); // reg_ycur_prog_en
#ifdef __SOC_PHOBOS__
		CVI_U8 r_sel = 0;

		GET_REGISTER_COMMON(0x4, 4, r_sel);
		CVI_TRACE_VI(CVI_DBG_INFO, "mem[%d] work, mem[%d] IDLE\n", r_sel, r_sel ^ 0x1);
		SET_REGISTER_COMMON(0x14, 12, r_sel ^ 0x1);
#endif
		for (CVI_U32 i = 0 ; i < length; i++) {
			val = vaddr + 0x14;
			data = (*val & ~(0x3F)) | i;
			*val = data;
			usleep(1);

			val = vaddr + 0x18;
			data = (*val | (0x1 << 31));
			*val = data;
			usleep(1);

			val = vaddr + 0x18;
			data = *val;
			data_ycurve[i] = (data & 0xFF);
		}

		SET_REGISTER_COMMON(0x4, 8, 0); // reg_ycur_prog_en
		SET_REGISTER_COMMON(0x0, 2, force_clk_enable); // reg_force_clk_enable
		SET_REGISTER_COMMON(0x0, 1, shdw_sel); // reg_ycur_shdw_sel
		SET_REGISTER_COMMON(0x0, 0, enable); // reg_ycur_enable

		snprintf(name, sizeof(name), "ycurve");
		FPRINTF_TBL(data_ycurve);

		free(data_ycurve);
		CVI_SYS_Munmap(vaddr, size);
	}

	// DCI_GAMMA
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "DCI_GAMMA\n");

		struct gamma_tbl dci_gamma = {
			.ip_info_id = IP_INFO_ID_DCI,
			.name = "dci_gamma",
			.length = 256,
			.enable = {
				.addr_ofs = 0x0C,
				.val_ofs = 0,
			},
			.shdw_sel = {
				.addr_ofs = 0x14,
				.val_ofs = 4,
			},
			.force_clk_enable = {
				.addr_ofs = 0x0C,
				.val_ofs = 8,
			},
			.prog_en = {
				.addr_ofs = 0x204,
				.val_ofs = 8,
			},
			.raddr = {
				.addr_ofs = 0x214,
				.mask = 0xFF,
				.val_ofs = 12,
			},
			.rdata_r = {
				.addr_ofs = 0x218,
				.val_ofs = 31,
				.mask = 0xFFF,
			},
			.rdata_gb = {
				.addr_ofs = 0x21C,
				.val_ofs = 16,
				.mask = 0xFFF,
			},
		};

		_dump_gamma_table(fp, ip_info_list, &dci_gamma);
	}

	/* end of file */
	fprintf(fp, "\t\"end\": {}\n");
	fprintf(fp, "}");
	/* start tuning update */
#ifdef __SOC_MARS__
	system("echo 0,0,0,0 > /sys/module/cv181x_vi/parameters/tuning_dis");
#else
	system("echo 0,0,0,0 > /sys/module/cv180x_vi/parameters/tuning_dis");
#endif
	free(ip_info_list);
#endif
	return CVI_SUCCESS;
}
