#include <linux/types.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include <linux/uaccess.h>
#include <linux/cvi_vip.h>
#include <linux/vpss_uapi.h>
#include <linux/clk.h>
#include <linux/random.h>

#include <base_cb.h>
#include <vpss_cb.h>
#include "vpss_common.h"
#include "scaler.h"
#include "vpss_core.h"
#include "sclr_test.h"
#include "sys.h"
#include "scaler_reg.h"
#include "cmdq.h"


#define PROC_NAME	"cvitek/sclr_test"

#define NV12_FILE_800_Y "/mnt/nfs/res/800-16.ppm_NV12_Y"
#define NV12_FILE_800_C "/mnt/nfs/res/800-16.ppm_NV12_C"
#define NV12_FILE_800   "/mnt/nfs/res/800-16.ppm_nv12p"


// rgb packed
#define TEST_SIZE_PER_PLANE	(ALIGN(SCL_MAX_HEIGHT*SCL_MAX_WIDTH, 1024))

static u64 sclr_timer_tick;
static volatile int sclr_mark;
static volatile union sclr_intr intr_mask;
static struct cvi_vip_dev *pdev;


static u64 src_addr_phy[1][3];
static u64 dst_addr_phy[1][3];
static void *src_addr_vir[1][3];
static void *dst_addr_vir[1][3];

uint8_t sclr_test_enabled;
static bool enable_cmdq;


static const struct cvi_vip_fmt cvi_vip_formats[] = {
	{
	.fmt         = SCL_FMT_YUV420,
	.bit_depth   = { 8, 2, 2 },
	.buffers     = 3,
	.plane_sub_h = 2,
	.plane_sub_v = 2,
	},
	{
	.fmt         = SCL_FMT_YUV422,
	.bit_depth   = { 8, 4, 4 },
	.buffers     = 3,
	.plane_sub_h = 2,
	.plane_sub_v = 1,
	},
	{
	.fmt         = SCL_FMT_NV12,
	.bit_depth   = { 8, 4, 0 },
	.buffers     = 2,
	.plane_sub_h = 2,
	.plane_sub_v = 2,
	},
	{
	.fmt         = SCL_FMT_NV21,
	.bit_depth   = { 8, 4, 0 },
	.buffers     = 2,
	.plane_sub_h = 2,
	.plane_sub_v = 2,
	},
	{
	.fmt         = SCL_FMT_YUV422SP2,
	.bit_depth   = { 8, 8, 0 },
	.buffers     = 2,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fmt         = SCL_FMT_YUV422SP1,
	.bit_depth   = { 8, 8, 0 },
	.buffers     = 2,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fmt         = SCL_FMT_YUYV,
	.bit_depth   = { 16 },
	.buffers     = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fmt         = SCL_FMT_YVYU,
	.bit_depth   = { 16 },
	.buffers     = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fmt         = SCL_FMT_UYVY,
	.bit_depth   = { 16 },
	.buffers     = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fmt         = SCL_FMT_VYUY,
	.bit_depth   = { 16 },
	.buffers     = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fmt         = SCL_FMT_RGB_PLANAR,
	.bit_depth   = { 8, 8, 8 },
	.buffers     = 3,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fmt         = SCL_FMT_RGB_PACKED,
	.bit_depth   = { 24 },
	.buffers     = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fmt         = SCL_FMT_BGR_PACKED,
	.bit_depth   = { 24 },
	.buffers     = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fmt         = SCL_FMT_Y_ONLY,
	.bit_depth   = { 8 },
	.buffers     = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fmt         = SCL_FMT_BGR_PACKED,
	.bit_depth   = { 24 },
	.buffers     = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fmt         = SCL_FMT_RGB_PLANAR,
	.bit_depth   = { 8, 8, 8 },
	.buffers     = 3,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
};

static void sclr_test_free_ion(void)
{
	if (src_addr_phy[0][0]) {
		sys_ion_free(src_addr_phy[0][0]);
	}
	src_addr_phy[0][0] = 0;
}

static int sclr_test_alloc_ion(void)
{
	size_t len =  2 * TEST_SIZE_PER_PLANE * 3; // in+out, h*w, planes
	int i;
	uint64_t u64Addr;
	void *vaddr = NULL;

	sclr_test_free_ion();

	if (sys_ion_alloc(&u64Addr, &vaddr, "sclr_test", len, false)) {
		pr_err("sys_ion_alloc fail, len:%ld\n", len);
		return -1;
	}

	for (i = 0; i < 3; i++) {
		src_addr_phy[0][i] = u64Addr + i * TEST_SIZE_PER_PLANE;
		dst_addr_phy[0][i] = u64Addr + (i + 3) * TEST_SIZE_PER_PLANE;

		src_addr_vir[0][i] = vaddr + i * TEST_SIZE_PER_PLANE;
		dst_addr_vir[0][i] = vaddr + (i + 3) * TEST_SIZE_PER_PLANE;
	}

	pr_err("sclr src buf[%d] -- 0x%llx, 0x%llx, 0x%llx\n", 0,
		src_addr_phy[0][0], src_addr_phy[0][1], src_addr_phy[0][2]);
	pr_err("sclr dst buf[%d] -- 0x%llx, 0x%llx, 0x%llx\n\n", 0,
		dst_addr_phy[0][0], dst_addr_phy[0][1], dst_addr_phy[0][2]);


	return 0;
}

static void _get_bytesperline(enum sclr_format sc_fmt, u16 width, u16 bytesperline[2])
{
	int k, p;
	const struct cvi_vip_fmt *fmt;

	memset(bytesperline, 0, sizeof(bytesperline[0] * 2));
	for (k = 0; k < ARRAY_SIZE(cvi_vip_formats); k++) {
		fmt = &cvi_vip_formats[k];
		if (fmt->fmt == sc_fmt)
			break;
	}

	for (p = 0; p < fmt->buffers; p++) {
		u8 plane_sub_h = (p == 0) ? 1 : fmt->plane_sub_h;
		/* Calculate the minimum supported bytesperline value */
		bytesperline[p] = VIP_ALIGN((width * fmt->bit_depth[p] * plane_sub_h) >> 3);
	}
	pr_err("fmt(%d) num_planes(%d) pitch(%d %d)\n", sc_fmt, fmt->buffers, bytesperline[0], bytesperline[1]);
}

static u64 timer_get_tick(void)
{
	return 0;
}

static void _intr_setup(u8 inst, enum sclr_img_in img_inst)
{
	union sclr_intr mask;

	mask.raw = 0;

	if (img_inst == SCL_IMG_V)
		mask.b.img_in_v_frame_end = true;
	else
		mask.b.img_in_d_frame_end = true;

	if (inst == 0)
		mask.b.scl0_frame_end = true;
	else if (inst == 1)
		mask.b.scl1_frame_end = true;
	else if (inst == 2)
		mask.b.scl2_frame_end = true;
	else if (inst == 3)
		mask.b.scl3_frame_end = true;

	mask.b.cmdq = true;
	mask.b.prog_too_late = true;
	mask.b.osd_cmp_frame_end = true;
	sclr_set_intr_mask(mask);

	intr_mask.raw = BIT(6 + inst);

	sclr_timer_tick = timer_get_tick();
}

static void sclr_trig_start_ext(int img_inst, int sc_inst)
{
	struct sclr_top_cfg *cfg = sclr_top_get_cfg();

	sclr_mark = 0;
	sclr_timer_tick = timer_get_tick();
	if (img_inst == SCL_IMG_D) {
		cfg->sclr_enable[0] = true;
		//cfg->disp_enable = true;
	} else if (sc_inst > 0 && sc_inst < SCL_MAX_INST) {
		cfg->sclr_enable[sc_inst] = true;
	} else {
		pr_err("%s: img(%d), sc(%d), wrong path\n", __func__, img_inst, sc_inst);
		return;
	}

	sclr_top_set_cfg(cfg);

	sclr_img_start(img_inst);
}

void sclr_test_irq_handler(uint32_t intr_raw_status)
{
	union sclr_intr intr_status;
	u8 status;

	pr_err("sclr_test_irq_handler....\n");

	if (enable_cmdq) {
		status = sclr_cmdq_intr_status();
		if (status) {
			sclr_cmdq_intr_clr(status);
			pr_err("cmdq-status(0x%x)\n", status);

			//if ((status & 0x04)&& cmdQ_is_sw_restart(REG_SCL_CMDQ_BASE)) {
			//	pr_err("cmdq-sw restart\n");
			//	cmdQ_sw_restart(REG_SCL_CMDQ_BASE);
			//}

			if (status & 0x02) {
				sclr_mark++;  // cmdq end
				enable_cmdq = false;
			}
		} else {
			pr_err("cmdq-status not found\n");
		}
	} else {
		intr_status.raw = intr_raw_status;

		sclr_intr_clr(intr_status);

		pr_err("%s: status(%#x)\n", __func__, intr_status.raw);

		if (intr_mask.raw & intr_status.raw) {
			++sclr_mark;
		}
	}
}

static int sclr_check_irq_handler(unsigned int timeout)
{
	unsigned int i;
	const unsigned int step = 100;

	/* OS tick(10 ms) -> usec */
	timeout *= 10000;
	timeout = (timeout >= step) ? timeout : step;

	for (i = 0; i < timeout; i+= step) {
		if (sclr_mark > 0) {
			sclr_mark = 0;
			printk("%s: check tick:%d\n", __func__, i);
			return 0;
		}
		udelay(step);
	}

	return -1;
}

static void fill_pattern(void)
{
	unsigned long *buf;
	CVI_U32 i;

	buf = (unsigned long *)src_addr_vir[0][0];
	for (i = 0; i < TEST_SIZE_PER_PLANE/4; i++)
		buf[i] = i;
}

static void sclr_reset(void)
{
	struct sclr_top_cfg cfg;

	sclr_img_reset(0);	// img_in_v
	sclr_img_reset(1);	// img_in_d
	sclr_sc_reset(0);	// sc_d
	sclr_sc_reset(1);	// sc_v1
	sclr_sc_reset(2);	// sc_v2
	sclr_sc_reset(3);	// sc_v3

	memset(&cfg, 0, sizeof(cfg));
	sclr_top_set_cfg(&cfg);
}

static void test_tile_cal_size(struct sclr_scale_cfg *cfg)
{
	struct sclr_size in_size = cfg->src;
	struct sclr_size out_size = cfg->dst;
	u32 out_l_width = (out_size.w >> 1);
	u32 h_sc_fac = (((in_size.w - 1) << 13) + (out_size.w >> 1))
		       / (out_size.w - 1);
	u32 L_last_phase, R_first_phase;
	u16 L_last_pixel, R_first_pixel;

	L_last_phase = (out_l_width - 1) * h_sc_fac;
	L_last_pixel = (L_last_phase >> 13) + ((cfg->mir_enable) ? 0 : 1);
	cfg->tile.src_l_width = L_last_pixel + 2
				+ ((L_last_phase & 0x1fff) ? 1 : 0);
	cfg->tile.out_l_width = out_l_width;

	// right tile no mirror
	R_first_phase = L_last_phase + h_sc_fac;
	R_first_pixel = (R_first_phase >> 13) + ((cfg->mir_enable) ? 0 : 1);
	cfg->tile.r_ini_phase = R_first_phase & 0x1fff;
	cfg->tile.src_r_offset = R_first_pixel - 1;
	cfg->tile.src_r_width = in_size.w - cfg->tile.src_r_offset;

	// TODO: crop?
	cfg->tile.src = in_size;
	cfg->tile.out = out_size;

	pr_err("h_sc_fac:%d\n", h_sc_fac);
	pr_err("L last phase(%d) last pixel(%d)\n", L_last_phase, L_last_pixel);
	pr_err("R first phase(%d) first pixel(%d)\n", R_first_phase, R_first_pixel);
	pr_err("tile cfg: Left width src(%d) out(%d)\n", cfg->tile.src_l_width, cfg->tile.out_l_width);
	pr_err("tile cfg: Right src offset(%d) src(%d) phase(%d)\n",
		cfg->tile.src_r_offset, cfg->tile.src_r_width, cfg->tile.r_ini_phase);
}

/**
 * sclr_ctrl_set_scale - setup scaling
 *
 * @param inst: (0~3), the instance of sc
 * @param cfg: scaling settings, include in/crop/out size.
 */
void test_sclr_ctrl_set_scale(u8 inst, struct sclr_scale_cfg *cfg)
{
	struct sclr_core_cfg *sc_cfg;

	if (inst >= SCL_MAX_INST) {
		pr_err("[bm-vip][sc] %s: no enough sclr-instance ", __func__);
		pr_err("for the requirement(%d)\n", inst);
		return;
	}
	sc_cfg = sclr_get_cfg(inst);
	if (memcmp(cfg, &sc_cfg->sc, sizeof(*cfg)) == 0)
		return;

	sclr_set_input_size(inst, cfg->src, true);

	// if crop invalid, use src-size
	if (cfg->crop.w + cfg->crop.x > cfg->src.w) {
		cfg->crop.x = 0;
		cfg->crop.w = cfg->src.w;
	}
	if (cfg->crop.h + cfg->crop.y > cfg->src.h) {
		cfg->crop.y = 0;
		cfg->crop.h = cfg->src.h;
	}
	sclr_set_crop(inst, cfg->crop, true);
	sclr_set_output_size(inst, cfg->dst);

	if (cfg->tile_enable)
		test_tile_cal_size(cfg);
	sc_cfg->sc.mir_enable = cfg->mir_enable;
	sc_cfg->sc.tile_enable = cfg->tile_enable;
	sc_cfg->sc.tile = cfg->tile;
	sclr_set_scale(inst);
}

void test_sclr_left_tile(u8 inst)
{
	struct sclr_scale_cfg *sc;
	struct sclr_odma_cfg *odma_cfg;
	struct sclr_rect crop;
	struct sclr_size dst;

	if (inst >= SCL_MAX_INST) {
		pr_err("[bm-vip][sc] %s: no enough sclr-instance ", __func__);
		pr_err("for the requirement(%d)\n", inst);
		return;
	}
	sc = &(sclr_get_cfg(inst)->sc);
	odma_cfg = sclr_odma_get_cfg(inst);

	crop.x = 0;
	crop.y = 0;
	crop.w = sc->tile.src_l_width;
	crop.h = sc->tile.src.h;
	dst.w = sc->tile.out_l_width;
	dst.h = sc->tile.out.h;

	sclr_set_crop(inst, crop, true);
	sclr_set_output_size(inst, dst);
	sclr_set_scale_mir(inst, sc->mir_enable);
	sclr_set_scale_phase(inst, 0, 0);

	odma_cfg->mem.start_x = 0;
	odma_cfg->mem.width = dst.w;
	sclr_odma_set_mem(inst, &odma_cfg->mem);
}

void test_sclr_right_tile(u8 inst)
{
	struct sclr_scale_cfg *sc;
	struct sclr_odma_cfg *odma_cfg;
	struct sclr_rect crop;
	struct sclr_size dst;

	if (inst >= SCL_MAX_INST) {
		pr_err("[bm-vip][sc] %s: no enough sclr-instance ", __func__);
		pr_err("for the requirement(%d)\n", inst);
		return;
	}
	sc = &(sclr_get_cfg(inst)->sc);
	odma_cfg = sclr_odma_get_cfg(inst);

	crop.x = sc->tile.src_r_offset;
	crop.y = 0;
	crop.w = sc->tile.src_r_width;
	crop.h = sc->src.h;
	dst.w = sc->tile.out.w - sc->tile.out_l_width;
	dst.h = sc->tile.out.h;

	sclr_set_crop(inst, crop, true);
	sclr_set_output_size(inst, dst);
	sclr_set_scale_mir(inst, 0);
	sclr_set_scale_phase(inst, sc->tile.r_ini_phase, 0);

	odma_cfg->mem.start_x = sc->tile.out_l_width;
	odma_cfg->mem.width = dst.w;
	sclr_odma_set_mem(inst, &odma_cfg->mem);
}

static int sclr_size_test(void)
{
	int ret = 0;
	int i;
	struct sclr_scale_cfg cfg;
	u8 inst;
#ifdef  __SOC_MARS__
	int max_size[SCL_MAX_INST] = {1920, 2880, 1920, 1280};
#else
	int max_size[SCL_MAX_INST] = {1920, 2880, 1920};
#endif
	enum sclr_img_in img_inst;
	enum sclr_input in = SCL_INPUT_MEM;
	enum sclr_format img_in_fmt = SCL_FMT_NV21;
	enum sclr_csc img_in_csc = SCL_CSC_601_LIMIT_YUV2RGB;
	enum sclr_format odma_fmt = SCL_FMT_NV21;
	enum sclr_csc odma_csc = SCL_CSC_601_LIMIT_RGB2YUV;
	struct sclr_mem mem;
	u16 pitch[3] = {0, 0, 0};
	struct sclr_size src;
	struct sclr_size dst;
	struct sclr_rect crop;
	struct sclr_odma_cfg *odma_cfg;
	struct sclr_img_checksum_status img_chksum_status;
	struct sclr_core_checksum_status sc_chksum_status;

	fill_pattern();

	for (inst = 0; inst < SCL_MAX_INST; inst++) {
		for (i = 64; i <= 2880; i += 32) {
			if (i > max_size[inst])
				continue;

			memset(&cfg, 0, sizeof(cfg));

			cfg.src.w = i;
			cfg.src.h = i;
			cfg.crop.w = i;
			cfg.crop.h = i;
			cfg.dst.w = i;
			cfg.dst.h = i;

			sclr_ctrl_set_scale(inst, &cfg);
			sclr_set_scale_phase(inst, 0, 0);

			// CFG_OP_IMG_CFG
			if (inst == 0) {
				img_inst = SCL_IMG_D;
			} else {
				img_inst = SCL_IMG_V;
			}

			// one line: 8192 byte
			//   rgb packed: max height/width 2730, 2730 * 3 = 8190 < 8192
			sclr_ctrl_set_input(img_inst, in, img_in_fmt, img_in_csc, false);

			// CFG_OP_IMG_SET_MEM
			memset(&mem, 0, sizeof(mem));
			mem.width = i;
			mem.height = i;
			mem.start_x = 0;
			mem.start_y = 0;
			mem.addr0 = src_addr_phy[0][R_IDX];
			mem.addr1 = src_addr_phy[0][G_IDX];
			mem.addr2 = src_addr_phy[0][B_IDX];

			_get_bytesperline(img_in_fmt, i, pitch);
			mem.pitch_y = pitch[0];
			mem.pitch_c = pitch[1];

			sclr_img_set_mem(img_inst, &mem, true);

			sclr_img_checksum_en(img_inst, true);

			// CFG_OP_SC_CFG
			sclr_set_cfg(inst, /*sc_bypass*/false, /*gop_bypass*/true, /*cir_bypass*/true, /*odma_bypass*/false);

			// CFG_OP_SC_SET_INPUT
			src.w = i;
			src.h = i;
			sclr_set_input_size(inst, src, true);

			// CFG_OP_SC_SET_CROP
			crop.x = 0;
			crop.y = 0;
			crop.w = src.w;
			crop.h = src.h;
			sclr_set_crop(inst, crop, true);
			sclr_set_scale(inst);

			sclr_core_checksum_en(inst, true);

			// CFG_OP_SC_SET_OUTPUT
			dst.w = crop.w;
			dst.h = crop.h;
			sclr_set_output_size(inst, dst);
			sclr_set_scale(inst);

			// CFG_OP_ODMA_CFG
			odma_cfg = sclr_odma_get_cfg(inst);
			memset(odma_cfg, 0, sizeof(*odma_cfg));
			odma_cfg->csc_cfg.mode = SCL_OUT_CSC;
			odma_cfg->fmt = odma_fmt;
			sclr_odma_set_cfg(inst, odma_cfg);

			odma_cfg->csc_cfg.csc_type = odma_csc;

			sclr_ctrl_set_output(inst, &odma_cfg->csc_cfg, odma_cfg->fmt);

			// CFG_OP_ODMA_SET_MEM
			mem.width = dst.w;
			mem.height = dst.h;
			mem.start_x = 0;
			mem.start_y = 0;
			mem.addr0 = dst_addr_phy[0][R_IDX];
			mem.addr1 = dst_addr_phy[0][G_IDX];
			mem.addr2 = dst_addr_phy[0][B_IDX];

			_get_bytesperline(odma_cfg->fmt, dst.w, pitch);
			mem.pitch_y = pitch[0];
			mem.pitch_c = pitch[1];
			sclr_odma_set_mem(inst, &mem);

			sclr_top_reg_done();
			_intr_setup(inst, img_inst);

			// CFG_OP_IMG_START
			sclr_trig_start_ext(img_inst, inst);

			if (sclr_check_irq_handler(10000) != 0) {
				pr_err("  img(%d), sc(%d), (w=h=%d), end intr doesn't happens after 10000 tick\n", img_inst, inst, src.w);
				ret = -1;
				break;
			} else {
				sclr_img_get_checksum_status(img_inst, &img_chksum_status);
				sclr_core_get_checksum_status(inst, &sc_chksum_status);

				pr_err("  img(%d), sc(%d), (w=h=%d), sclr done\n", img_inst, inst, src.w);
				pr_err("  img(%d) checksum: enable=%d, in=0x%x, out=0x%x\n",
					img_inst,
					img_chksum_status.checksum_base.b.enable,
					img_chksum_status.axi_read_data,
					img_chksum_status.checksum_base.b.output_data);
				pr_err("  sc(%d) odma status(%#x)\n", inst, sclr_odma_get_dbg_status(inst).raw);
				pr_err("  sc(%d): checksum (raw=0x%x), enable=%d, in=0x%x, out=0x%x\n",
					inst,
					sc_chksum_status.checksum_base.raw,
					sc_chksum_status.checksum_base.b.enable,
					sc_chksum_status.checksum_base.b.data_in_from_img_in,
					sc_chksum_status.checksum_base.b.data_out);
			}
		}

		if (ret)
			break;
	}

	sclr_reset();

	pr_err("%s %s\n", __func__, ret ? "fail":"pass");

	return ret;
}

int read_file(int num, char **file_names, void **addrs, int *len)
{
	struct file *filep = NULL;
	loff_t pos = 0;
	int i;

	//read file
	for (i = 0; i < num; i++) {
		filep = filp_open(file_names[i], O_RDONLY, 0644);
		if (IS_ERR(filep)) {
			pr_err("Open file %s error\n", file_names[i]);
			return -1;
		}
		pos = 0;
		kernel_read(filep, addrs[i], len[i], &pos);
		filp_close(filep, NULL);
		filep = NULL;
	}

	return 0;
}

int save_file(int num, char *file_name, void **addrs, int *len)
{
	struct file *filep = NULL;
	loff_t pos = 0;
	int i, ret;

	filep = filp_open(file_name, O_RDWR | O_CREAT, 0644);
	if (IS_ERR(filep)) {
		pr_err("Open file %s error\n", file_name);
		return -1;
	}

	//write file
	for (i = 0; i < num; i++) {
		ret = kernel_write(filep, addrs[i], len[i], &pos);
		if (ret != len[i])
			pr_err("pos=%lld ret=%d, len[%d]=%d\n", pos, ret, i, len[i]);
	}
	filp_close(filep, NULL);

	return 0;
}

static int sclr_send_file_test(void)
{
	int ret = 0;
	u8 sc_inst = 1;
	u16 width_in = 800;
	u16 height_in = 600;
	u16 width_out = 640;
	u16 height_out = 320;
	struct sclr_scale_cfg cfg;
	enum sclr_img_in img_inst;
	enum sclr_input in = SCL_INPUT_MEM;
	enum sclr_format img_in_fmt = SCL_FMT_NV12;
	enum sclr_csc img_in_csc = SCL_CSC_601_LIMIT_YUV2RGB;
	enum sclr_format odma_fmt = SCL_FMT_NV12;
	enum sclr_csc odma_csc = SCL_CSC_601_LIMIT_RGB2YUV;
	struct sclr_mem mem;
	u16 pitch[3] = {0, 0, 0};
	struct sclr_odma_cfg *odma_cfg;
	struct sclr_img_checksum_status img_chksum_status;
	struct sclr_core_checksum_status sc_chksum_status;
	char *file_in[2] = {NV12_FILE_800_Y, NV12_FILE_800_C};
	char *file_out = "/mnt/nfs/640_320_nv12.yuv";
	int lens[2] = {width_in * height_in, width_in * height_in / 2};

	//fill image
	if (read_file(2, file_in, &src_addr_vir[0][0], lens)) {
		pr_err("read file error\n");
		return -1;
	}

	// CFG_OP_IMG_CFG
	//dual mode
	if (sc_inst == 0) {
		img_inst = SCL_IMG_D;
	} else {
		img_inst = SCL_IMG_V;
	}

	/********img config*********/
	sclr_ctrl_set_input(img_inst, in, img_in_fmt, img_in_csc, false);

	// CFG_OP_IMG_SET_MEM
	memset(&mem, 0, sizeof(mem));
	mem.width = width_in;
	mem.height = height_in;
	mem.start_x = 0;
	mem.start_y = 0;
	mem.addr0 = src_addr_phy[0][R_IDX];
	mem.addr1 = src_addr_phy[0][G_IDX];
	mem.addr2 = src_addr_phy[0][B_IDX];

	_get_bytesperline(img_in_fmt, width_in, pitch);
	mem.pitch_y = pitch[0];
	mem.pitch_c = pitch[1];

	sclr_img_set_mem(img_inst, &mem, true);
	sclr_img_checksum_en(img_inst, true);

	/********sc config*********/
	memset(&cfg, 0, sizeof(cfg));
	cfg.src.w = width_in;
	cfg.src.h = height_in;
	cfg.crop.w = width_in;
	cfg.crop.h = height_in;
	cfg.dst.w = width_out;
	cfg.dst.h = height_out;

	sclr_ctrl_set_scale(sc_inst, &cfg);
	sclr_set_scale_phase(sc_inst, 0, 0);

	// CFG_OP_SC_CFG
	sclr_set_cfg(sc_inst, /*sc_bypass*/false, /*gop_bypass*/false, /*cir_bypass*/true, /*odma_bypass*/false);

	sclr_core_checksum_en(sc_inst, true);

	// CFG_OP_ODMA_CFG
	odma_cfg = sclr_odma_get_cfg(sc_inst);
	memset(odma_cfg, 0, sizeof(*odma_cfg));
	odma_cfg->csc_cfg.mode = SCL_OUT_CSC;
	odma_cfg->fmt = odma_fmt;
	sclr_odma_set_cfg(sc_inst, odma_cfg);

	odma_cfg->csc_cfg.csc_type = odma_csc;

	sclr_ctrl_set_output(sc_inst, &odma_cfg->csc_cfg, odma_cfg->fmt);

	// CFG_OP_ODMA_SET_MEM
	mem.width = width_out;
	mem.height = height_out;
	mem.start_x = 0;
	mem.start_y = 0;
	mem.addr0 = dst_addr_phy[0][R_IDX];
	mem.addr1 = dst_addr_phy[0][G_IDX];
	mem.addr2 = dst_addr_phy[0][B_IDX];

	_get_bytesperline(odma_cfg->fmt, width_out, pitch);
	mem.pitch_y = pitch[0];
	mem.pitch_c = pitch[1];
	sclr_odma_set_mem(sc_inst, &mem);

	sclr_top_reg_done();
	_intr_setup(sc_inst, img_inst);

	// CFG_OP_IMG_START
	sclr_trig_start_ext(img_inst, sc_inst);
	pr_err("  img(%d), sc(%d) start...\n", img_inst, sc_inst);

	if (sclr_check_irq_handler(1000) != 0) {
		pr_err("  img(%d), sc(%d), end intr doesn't happens after 1000 tick\n", img_inst, sc_inst);
		ret = -1;
	} else {
		lens[0] = pitch[0] * height_out;
		lens[1] = pitch[1] * height_out / 2;
		save_file(2, file_out, &dst_addr_vir[0][0], lens);
		sclr_img_get_checksum_status(img_inst, &img_chksum_status);
		sclr_core_get_checksum_status(sc_inst, &sc_chksum_status);

		pr_err("  img(%d), sc(%d), sclr done\n", img_inst, sc_inst);
		pr_err("  img(%d) checksum: enable=%d, in=0x%x, out=0x%x\n",
			img_inst,
			img_chksum_status.checksum_base.b.enable,
			img_chksum_status.axi_read_data,
			img_chksum_status.checksum_base.b.output_data);
		pr_err("  sc(%d): checksum (raw=0x%x), enable=%d, in=0x%x, out=0x%x\n",
			sc_inst,
			sc_chksum_status.checksum_base.raw,
			sc_chksum_status.checksum_base.b.enable,
			sc_chksum_status.checksum_base.b.data_in_from_img_in,
			sc_chksum_status.checksum_base.b.data_out);
	}

	//sclr_reset();

	pr_err("%s %s\n", __func__, ret ? "fail":"pass");
	return ret;
}

static int sclr_tile_test(bool bMirror)
{
	int ret = 0;
	u8 sc_inst = 1;
	u16 width_in = 800;
	u16 height_in = 600;
	u16 width_out = 800;
	u16 height_out = 600;
	struct sclr_scale_cfg cfg;
	enum sclr_img_in img_inst;
	enum sclr_input in = SCL_INPUT_MEM;
	enum sclr_format img_in_fmt = SCL_FMT_NV12;
	enum sclr_csc img_in_csc = SCL_CSC_601_LIMIT_YUV2RGB;
	enum sclr_format odma_fmt = SCL_FMT_NV12;
	enum sclr_csc odma_csc = SCL_CSC_601_LIMIT_RGB2YUV;
	struct sclr_mem mem;
	u16 pitch[3] = {0, 0, 0};
	struct sclr_odma_cfg *odma_cfg;
	struct sclr_img_checksum_status img_chksum_status;
	struct sclr_core_checksum_status sc_chksum_status;
	char *file_in[2] = {NV12_FILE_800_Y, NV12_FILE_800_C};
	char *file_out = "/mnt/nfs/800_600_nv12_tile.yuv";
	int lens[2] = {width_in * height_in, width_in * height_in / 2};

	//fill image
	if (read_file(2, file_in, &src_addr_vir[0][0], lens)) {
		pr_err("read file error\n");
		return -1;
	}

	// CFG_OP_IMG_CFG
	//dual mode
	if (sc_inst == 0) {
		img_inst = SCL_IMG_D;
	} else {
		img_inst = SCL_IMG_V;
	}

	/********img config*********/
	sclr_ctrl_set_input(img_inst, in, img_in_fmt, img_in_csc, false);

	// CFG_OP_IMG_SET_MEM
	memset(&mem, 0, sizeof(mem));
	mem.width = width_in;
	mem.height = height_in;
	mem.start_x = 0;
	mem.start_y = 0;
	mem.addr0 = src_addr_phy[0][R_IDX];
	mem.addr1 = src_addr_phy[0][G_IDX];
	mem.addr2 = src_addr_phy[0][B_IDX];

	_get_bytesperline(img_in_fmt, width_in, pitch);
	mem.pitch_y = pitch[0];
	mem.pitch_c = pitch[1];

	sclr_img_set_mem(img_inst, &mem, true);
	sclr_img_checksum_en(img_inst, true);

	/********sc config*********/
	memset(&cfg, 0, sizeof(cfg));
	cfg.src.w = width_in;
	cfg.src.h = height_in;
	cfg.crop.w = width_in;
	cfg.crop.h = height_in;
	cfg.dst.w = width_out;
	cfg.dst.h = height_out;
	cfg.tile_enable = 1;
	cfg.mir_enable = bMirror;

	test_sclr_ctrl_set_scale(sc_inst, &cfg);
	sclr_set_scale_phase(sc_inst, 0, 0);

	// CFG_OP_SC_CFG
	sclr_set_cfg(sc_inst, /*sc_bypass*/false, /*gop_bypass*/false, /*cir_bypass*/true, /*odma_bypass*/false);

	sclr_core_checksum_en(sc_inst, true);

	// CFG_OP_ODMA_CFG
	odma_cfg = sclr_odma_get_cfg(sc_inst);
	memset(odma_cfg, 0, sizeof(*odma_cfg));
	odma_cfg->csc_cfg.mode = SCL_OUT_CSC;
	odma_cfg->fmt = odma_fmt;
	sclr_odma_set_cfg(sc_inst, odma_cfg);

	odma_cfg->csc_cfg.csc_type = odma_csc;

	sclr_ctrl_set_output(sc_inst, &odma_cfg->csc_cfg, odma_cfg->fmt);

	// CFG_OP_ODMA_SET_MEM
	mem.width = width_out;
	mem.height = height_out;
	mem.start_x = 0;
	mem.start_y = 0;
	mem.addr0 = dst_addr_phy[0][R_IDX];
	mem.addr1 = dst_addr_phy[0][G_IDX];
	mem.addr2 = dst_addr_phy[0][B_IDX];

	_get_bytesperline(odma_cfg->fmt, width_out, pitch);
	mem.pitch_y = pitch[0];
	mem.pitch_c = pitch[1];
	sclr_odma_set_mem(sc_inst, &mem);

	test_sclr_left_tile(sc_inst);

	sclr_top_reg_done();
	_intr_setup(sc_inst, img_inst);

	// CFG_OP_IMG_START
	sclr_trig_start_ext(img_inst, sc_inst);
	pr_err("  tile mode + mir_enable=%d\n", cfg.mir_enable);
	pr_err("  img(%d), sc(%d) start...\n", img_inst, sc_inst);

	if (sclr_check_irq_handler(1000) != 0) {
		pr_err("  img(%d), sc(%d), end intr doesn't happens after 1000 tick\n", img_inst, sc_inst);
		ret = -1;
	} else {
		pr_err("sclr done, left tile\n");
		test_sclr_right_tile(sc_inst);
		sclr_top_reg_done();
		_intr_setup(sc_inst, img_inst);
		sclr_trig_start_ext(img_inst, sc_inst);
		pr_err("  img(%d), sc(%d) start...\n", img_inst, sc_inst);

		if (sclr_check_irq_handler(1000) != 0) {
			pr_err("  img(%d), sc(%d), end intr doesn't happens after 1000 tick\n", img_inst, sc_inst);
			ret = -1;
		} else {
			lens[0] = pitch[0] * height_out;
			lens[1] = pitch[1] * height_out / 2;
			save_file(2, file_out, &dst_addr_vir[0][0], lens);
			sclr_img_get_checksum_status(img_inst, &img_chksum_status);
			sclr_core_get_checksum_status(sc_inst, &sc_chksum_status);

			pr_err("sclr done, right tile\n");
			pr_err("  img(%d) checksum: enable=%d, in=0x%x, out=0x%x\n",
				img_inst,
				img_chksum_status.checksum_base.b.enable,
				img_chksum_status.axi_read_data,
				img_chksum_status.checksum_base.b.output_data);
			pr_err("  sc(%d): checksum (raw=0x%x), enable=%d, in=0x%x, out=0x%x\n",
				sc_inst,
				sc_chksum_status.checksum_base.raw,
				sc_chksum_status.checksum_base.b.enable,
				sc_chksum_status.checksum_base.b.data_in_from_img_in,
				sc_chksum_status.checksum_base.b.data_out);
		}
	}

	//sclr_reset();

	pr_err("%s %s\n", __func__, ret ? "fail":"pass");
	return ret;
}

static int sclr_cmdq_test(void)
{
	int ret = 0;
	int len = 0x2000;
	uint64_t u64Addr;
	void *vaddr = NULL;
	struct sclr_ctrl_cfg cfgs[2];
	union sclr_intr mask;
	u16 width_in = 800;
	u16 height_in = 600;
	u16 width_out = 800;
	u16 height_out = 600;
	u16 pitch[3] = {0, 0, 0};
	char *file_in[2] = {NV12_FILE_800_Y, NV12_FILE_800_C};
	char *file_out = "/mnt/nfs/800_600_nv12_cmdq.yuv";
	int lens[2] = {width_in * height_in, width_in * height_in / 2};

	//fill image
	if (read_file(2, file_in, &src_addr_vir[0][0], lens)) {
		pr_err("read file error\n");
		return -1;
	}

	if (sys_ion_alloc(&u64Addr, &vaddr, "cmdq_addr", len, false)) {
		pr_err("sys_ion_alloc fail, len:%d\n", len);
		return -1;
	}
	pr_err("cmdq addr:%llx\n", u64Addr);

	memset(vaddr, 0, len);
	memset(cfgs, 0, sizeof(cfgs));
	memset(&mask, 0, sizeof(mask));

	cfgs[0].img_inst = SCL_IMG_V;
	cfgs[0].input = SCL_INPUT_MEM;
	cfgs[0].src_fmt = SCL_FMT_NV12;
	cfgs[0].src_csc = SCL_CSC_601_LIMIT_YUV2RGB;
	cfgs[0].src.w = width_in;
	cfgs[0].src.h = height_in;
	cfgs[0].src_crop.x = 0;
	cfgs[0].src_crop.y = 0;
	cfgs[0].src_crop.w = width_in;
	cfgs[0].src_crop.h = height_in;
	cfgs[0].src_addr0 = src_addr_phy[0][R_IDX];
	cfgs[0].src_addr1 = src_addr_phy[0][G_IDX];
	cfgs[0].src_addr2 = src_addr_phy[0][B_IDX];
	cfgs[0].dst[0].inst = 1;
	cfgs[0].dst[0].fmt = SCL_FMT_NV12;
	cfgs[0].dst[0].csc = SCL_CSC_601_LIMIT_RGB2YUV;
	cfgs[0].dst[0].frame.w = width_out;
	cfgs[0].dst[0].frame.h = height_out;
	cfgs[0].dst[0].offset.x = 0;
	cfgs[0].dst[0].offset.y = 0;
	cfgs[0].dst[0].window.w = width_out;
	cfgs[0].dst[0].window.h = height_out;
	cfgs[0].dst[0].addr0 = dst_addr_phy[0][R_IDX];
	cfgs[0].dst[0].addr1 = dst_addr_phy[0][G_IDX];
	cfgs[0].dst[0].addr2 = dst_addr_phy[0][B_IDX];
	cfgs[0].dst[1].inst = SCL_MAX_INST;
	cfgs[0].dst[2].inst = SCL_MAX_INST;
	cfgs[0].dst[3].inst = SCL_MAX_INST;

	enable_cmdq = true;
	// disable scl's intr to have no scl's irq during process of cmdq
	mask.b.cmdq = true;
	sclr_set_intr_mask(mask);
	sclr_engine_cmdq(cfgs, 1, vaddr, u64Addr);

	if (sclr_check_irq_handler(1000) != 0) {
		pr_err("cmdq fail, end intr doesn't happens after 1000 tick\n");
		ret = -1;
	} else {
		pitch[0] = IS_PACKED_FMT(cfgs[0].dst[0].fmt)
				? VIP_ALIGN(3 * width_out)
				: VIP_ALIGN(width_out);
		if ((cfgs[0].dst[0].fmt == SCL_FMT_YUV420) ||
			(cfgs[0].dst[0].fmt == SCL_FMT_YUV422))
			pitch[1] = VIP_ALIGN(width_out >> 1);
		else
			pitch[1] = VIP_ALIGN(width_out);
		lens[0] = pitch[0] * height_out;
		lens[1] = pitch[1] * height_out / 2;
		pr_err("%d %d %d %d.\n", pitch[0], pitch[1], lens[0], lens[1]);
		save_file(2, file_out, &dst_addr_vir[0][0], lens);
		pr_err("cmdq ok.\n");
	}
	sys_ion_free(u64Addr);
	enable_cmdq = false;
	pr_err("%s %s\n", __func__, ret ? "fail":"pass");
	return ret;
}

static int sclr_blend_privacy_mask_test(struct sclr_bld_cfg *bld_cfg,
					struct sclr_privacy_cfg *prm_cfg)
{
	int ret = 0;
	struct sclr_scale_cfg cfg;
	u16 width_in = 800;
	u16 height_in = 600;
	u16 width_out = 800;
	u16 height_out = 600;
	u8 inst_d = 0; // sc_d
	u8 inst_v = 1; // sc_v1
	enum sclr_img_in img_inst_d = SCL_IMG_D;
	enum sclr_img_in img_inst_v = SCL_IMG_V;
	enum sclr_input in = SCL_INPUT_MEM;
	enum sclr_format src_fmt_d = SCL_FMT_Y_ONLY;
	enum sclr_format src_fmt_v = SCL_FMT_NV12;
	enum sclr_format dst_fmt = SCL_FMT_NV12;
	struct sclr_mem mem;
	enum sclr_csc csc;
	u16 pitch[3];
	struct sclr_odma_cfg *odma_cfg;
	u8 *ptr = (u8 *)src_addr_vir[0][R_IDX];
	struct sclr_top_cfg *top_cfg;
	char *file_in[1] = {NV12_FILE_800};
	char *file_out = "/mnt/nfs/800_600_nv12_blend_privacy.yuv";
	int lens = width_in * height_in * 3 / 2;

	//fill image
	if (read_file(1, file_in, &src_addr_vir[0][G_IDX], &lens)) {
		pr_err("read file error\n");
		return -1;
	}

	//src_addr_phy[0][0] img_d in
	//src_addr_scl[0][1] img_v in
	//src_addr_scl[0][2] privacy mask data
	//dst_addr_phy[0][0] sc1 out
	pr_err("0x%08llx: 800x600, Y only, top half white, bottom half black, img_d in\n",
		src_addr_phy[0][R_IDX]);
	pr_err("0x%08llx: 800x600 NV12 packed, 800-16.ppm_nv12p, img_v in\n",
		src_addr_phy[0][G_IDX]);
	pr_err("0x%08x: privacy mask data, in\n", prm_cfg->map_cfg.base);
	pr_err("0x%08llx: 800x600 NV12 packed, out\n", dst_addr_phy[0][R_IDX]);

	memset(&cfg, 0, sizeof(cfg));

	cfg.src.w = width_in;
	cfg.src.h = height_in;
	cfg.crop.w = cfg.src.w;
	cfg.crop.h = cfg.src.h;
	cfg.dst.w = width_out;
	cfg.dst.h = height_out;

	// Y only, top half white, bottom half black
	// Top half: 0xff; Bottom half: 0x00
	pr_err("Fill img_d dram content ...\n");
	memset(ptr, 0xff, cfg.src.w*cfg.src.h/2);
	memset(ptr+cfg.src.w*cfg.src.h/2, 0, cfg.src.w*cfg.src.h/2);
	pr_err("Fill img_d dram content done\n");

	//========================================
	sclr_ctrl_set_scale(inst_d, &cfg);
	sclr_set_scale_phase(inst_d, 0, 0);

	// CFG_OP_IMG_CFG
	// dram -> sclr rgb, img_in_d
	sclr_ctrl_set_input(img_inst_d, in, src_fmt_d, SCL_CSC_601_LIMIT_YUV2RGB, false);

	// CFG_OP_IMG_SET_MEM, Y only
	memset(&mem, 0, sizeof(mem));
	mem.width = cfg.src.w;
	mem.height = cfg.src.h;
	mem.start_x = 0;
	mem.start_y = 0;
	mem.addr0 = src_addr_phy[0][R_IDX];

	_get_bytesperline(src_fmt_d, cfg.src.w, pitch);
	mem.pitch_y = pitch[0];
	mem.pitch_c = pitch[1];
	sclr_img_set_mem(img_inst_d, &mem, true);

	// CFG_OP_SC_CFG
	sclr_set_cfg(inst_d, /*sc_bypass*/true, /*gop_bypass*/true, /*cir_bypass*/true, /*odma_bypass*/true);

	//========================================
	sclr_ctrl_set_scale(inst_v, &cfg);
	sclr_set_scale_phase(inst_v, 0, 0);

	// CFG_OP_IMG_CFG
	// dram -> sclr rgb, img_in_v
	sclr_ctrl_set_input(img_inst_v, in, src_fmt_v, SCL_CSC_601_LIMIT_YUV2RGB, false);

	// CFG_OP_IMG_SET_MEM, NV21 packed
	memset(&mem, 0, sizeof(mem));
	mem.width = cfg.src.w;
	mem.height = cfg.src.h;
	mem.start_x = 0;
	mem.start_y = 0;
	mem.addr0 = src_addr_phy[0][G_IDX];
	mem.addr1 = mem.addr0 + cfg.src.w * cfg.src.h;

	_get_bytesperline(src_fmt_v, cfg.src.w, pitch);
	mem.pitch_y = pitch[0];
	mem.pitch_c = pitch[1];
	sclr_img_set_mem(img_inst_v, &mem, true);

	// CFG_OP_SC_CFG
	sclr_set_cfg(inst_v, /*sc_bypass*/false, /*gop_bypass*/true, /*cir_bypass*/true, /*odma_bypass*/false);

	// CFG_OP_SC_SET_INPUT
	sclr_set_input_size(inst_v, cfg.src, true);

	// CFG_OP_SC_SET_CROP
	sclr_set_crop(inst_v, cfg.crop, true);
	sclr_set_scale(inst_v);

	// CFG_OP_SC_SET_OUTPUT
	sclr_set_output_size(inst_v, cfg.dst);
	sclr_set_scale(inst_v);

	// CFG_OP_ODMA_CFG, sclr rgb -> odma
	odma_cfg = sclr_odma_get_cfg(inst_v);
	memset(odma_cfg, 0, sizeof(*odma_cfg));
	odma_cfg->csc_cfg.mode = SCL_OUT_CSC;
	odma_cfg->fmt = dst_fmt;
	odma_cfg->csc_cfg.csc_type = SCL_CSC_NONE;

	if (odma_cfg->csc_cfg.mode == SCL_OUT_CSC) {
		csc = SCL_CSC_NONE;

		if (IS_YUV_FMT(odma_cfg->fmt))
			csc = SCL_CSC_601_LIMIT_RGB2YUV;

		odma_cfg->csc_cfg.csc_type = csc;
	}

	sclr_odma_set_cfg(inst_v, odma_cfg);
	sclr_ctrl_set_output(inst_v, &odma_cfg->csc_cfg, odma_cfg->fmt);

	// CFG_OP_ODMA_SET_MEM
	mem.width = cfg.dst.w;
	mem.height = cfg.dst.h;
	mem.start_x = 0;
	mem.start_y = 0;
	mem.addr0 = dst_addr_phy[0][R_IDX];
	mem.addr1 = mem.addr0 + mem.width * mem.height;

	_get_bytesperline(odma_cfg->fmt, cfg.dst.w, pitch);
	mem.pitch_y = pitch[0];
	mem.pitch_c = pitch[1];
	sclr_odma_set_mem(inst_v, &mem);

	// CFG_OP_TOP_SET_BLEND
	bld_cfg->width = mem.width;
	sclr_top_bld_set_cfg(bld_cfg);

	// CFG_OP_SC_SET_PRIVACY
	sclr_pri_set_cfg(SCL_MAX_INST, prm_cfg);

	top_cfg = sclr_top_get_cfg();
	top_cfg->sclr_enable[inst_d] = 0;
	top_cfg->sclr_enable[inst_v] = 1;
	top_cfg->img_in_v_trig_src = SCL_IMG_TRIG_SRC_SW;
	top_cfg->img_in_d_trig_src = SCL_IMG_TRIG_SRC_SW;
	sclr_top_set_cfg(top_cfg);

	sclr_top_reg_done();
	_intr_setup(inst_v, img_inst_v);

	// CFG_OP_IMG_START
	sclr_img_start(img_inst_d);
	sclr_img_start(img_inst_v);

	if (sclr_check_irq_handler(10000) != 0) {
		pr_err("img_d(%d), img_v(%d), sc(%d), sc(%d), (w=%d, h=%d), end intr doesn't happens after 10000 tick\n",
			img_inst_d, img_inst_v, inst_d, inst_v, cfg.dst.w, cfg.dst.h);
		ret = -1;
	} else {
		lens = (pitch[0] * height_out) + (pitch[1] * height_out / 2);
		save_file(1, file_out, &dst_addr_vir[0][R_IDX], &lens);
		pr_err("img_d(%d), img_v(%d), sc(%d), sc(%d), (w=%d, h=%d), sclr done\n",
			img_inst_d, img_inst_v, inst_d, inst_v, cfg.dst.w, cfg.dst.h);
		pr_err("sc_d(%d) odma status(%#x)\n", inst_d, sclr_odma_get_dbg_status(inst_d).raw);
		pr_err("sc_v(%d) odma status(%#x)\n", inst_v, sclr_odma_get_dbg_status(inst_v).raw);
	}

	pr_err("%s %s\n", __func__, ret ? "fail":"pass");

	return ret;
}

// blend alpha test
//   IMG-D, Y-Only. Top half, 0xff; Bottom half, 0x00
//   IMG-V, NV12

//   1. enable blending test, with alpha factor, 256
//   2. alpha factor, 128

//   1. cover with top half white, bottom half black
//   2. transparent blending"
//
//   img_in_d + img_in_v + sc_v1 + sc_v1_odma
static int sclr_blend_test_1(void)
{
	struct sclr_bld_cfg bld_cfg;
	struct sclr_privacy_cfg prm_cfg;

	bld_cfg.ctrl.raw = 0;
	bld_cfg.ctrl.b.enable = 1;
	bld_cfg.ctrl.b.fix_alpha = 1;	// fix alpha value
	bld_cfg.ctrl.b.blend_y = 1;	// y channel only
	bld_cfg.ctrl.b.y2r_enable = 1;	// translate blending output from YUV to RGB
	bld_cfg.ctrl.b.alpha_factor = 256;

	prm_cfg.cfg.raw = 0;

	return sclr_blend_privacy_mask_test(&bld_cfg, &prm_cfg);
}

// blend stitch test
//   IMG-D, Y-Only. Top half, 0xff; Bottom half, 0x00
//   IMG-V, NV12

//   1. enable blending test, with alpha factor, 256
//   2. alpha factor, 128

//   1. cover with top half white, bottom half black
//   2. transparent blending"
//
//   img_in_d + img_in_v + sc_v1 + sc_v1_odma
static int sclr_blend_test_2(void)
{
	struct sclr_bld_cfg bld_cfg;
	struct sclr_privacy_cfg prm_cfg;

	bld_cfg.ctrl.raw = 0;
	bld_cfg.ctrl.b.enable = 1;
	bld_cfg.ctrl.b.fix_alpha = 0;	// gradient, stitching
	bld_cfg.ctrl.b.blend_y = 1;	// y channel only
	bld_cfg.ctrl.b.y2r_enable = 1;	// translate blending output from YUV to RGB
	bld_cfg.ctrl.b.alpha_factor = 256;

	prm_cfg.cfg.raw = 0;

	return sclr_blend_privacy_mask_test(&bld_cfg, &prm_cfg);
}

//   1. enable blending test, with alpha factor, 256
//   2. alpha factor, 128

//   1. cover with top half white, bottom half black
//   2. transparent blending"
//
//   img_in_d + img_in_v + sc_v1 + sc_v1_odma
//
//   Privacy mask(rgb332)
static int sclr_blend_test_3(void)
{
	struct sclr_bld_cfg bld_cfg;
	struct sclr_privacy_cfg prm_cfg;
	u8 *ptr = (u8 *)src_addr_vir[0][B_IDX];
	int i, j;

	bld_cfg.ctrl.raw = 0;
	bld_cfg.ctrl.b.enable = 1;
	bld_cfg.ctrl.b.fix_alpha = 1;	// fix alpha value
	bld_cfg.ctrl.b.blend_y = 1;	// y channel only
	bld_cfg.ctrl.b.y2r_enable = 1;	// translate blending output from YUV to RGB
	bld_cfg.ctrl.b.alpha_factor = 256;

	// red(0xe0) green(0x1c) blue(0x03)
	prm_cfg.cfg.raw = 0;
	prm_cfg.cfg.b.enable = 1;
	prm_cfg.cfg.b.mode = 0;	// grid mode
	prm_cfg.cfg.b.grid_size = 0;	// 8x8
	prm_cfg.cfg.b.fit_picture = 0;	// no full image
	prm_cfg.cfg.b.force_alpha = 0;
	prm_cfg.map_cfg.alpha_factor = 81;
	prm_cfg.cfg.b.mask_rgb332 = 1;
	prm_cfg.map_cfg.no_mask_idx = 0;
	prm_cfg.map_cfg.base = src_addr_phy[0][B_IDX];
	prm_cfg.map_cfg.axi_burst = 7;
	prm_cfg.start.x = 512;
	prm_cfg.end.x = 640 - 1;	// x + 128
	prm_cfg.start.y = 64;
	prm_cfg.end.y = 192 - 1;	// y + 128

	// 8x8 grid => 128x128
	for (i = 0; i < 16; i++)
		for (j = 0; j < 16; j++)
			ptr[i * 16 + j] = (u8)get_random_u32();

	return sclr_blend_privacy_mask_test(&bld_cfg, &prm_cfg);
}

//   1. enable blending test, with alpha factor, 256
//   2. alpha factor, 128

//   1. cover with top half white, bottom half black
//   2. transparent blending"
//
//   img_in_d + img_in_v + sc_v1 + sc_v1_odma
//
//   Privacy mask(y)
static int sclr_blend_test_4(void)
{
	struct sclr_bld_cfg bld_cfg;
	struct sclr_privacy_cfg prm_cfg;
	u8 *ptr = (u8 *)src_addr_vir[0][B_IDX];
	int i, j;

	bld_cfg.ctrl.raw = 0;
	bld_cfg.ctrl.b.enable = 1;
	bld_cfg.ctrl.b.fix_alpha = 1;	// fix alpha value
	bld_cfg.ctrl.b.blend_y = 1;	// y channel only
	bld_cfg.ctrl.b.y2r_enable = 1;	// translate blending output from YUV to RGB
	bld_cfg.ctrl.b.alpha_factor = 256;

	// red(0xe0) green(0x1c) blue(0x03)
	prm_cfg.cfg.raw = 0;
	prm_cfg.cfg.b.enable = 1;
	prm_cfg.cfg.b.mode = 0;	// grid mode
	prm_cfg.cfg.b.grid_size = 0;	// 8x8
	prm_cfg.cfg.b.fit_picture = 0;	// no full image
	prm_cfg.cfg.b.force_alpha = 0;
	prm_cfg.map_cfg.alpha_factor = 81;
	prm_cfg.cfg.b.mask_rgb332 = 0;
	prm_cfg.cfg.b.blend_y = 1;
	prm_cfg.cfg.b.y2r_enable = 1;
	prm_cfg.map_cfg.no_mask_idx = 0;
	prm_cfg.map_cfg.base = src_addr_phy[0][B_IDX];
	prm_cfg.map_cfg.axi_burst = 7;
	prm_cfg.start.x = 512;
	prm_cfg.end.x = 640 - 1;	// x + 128
	prm_cfg.start.y = 64;
	prm_cfg.end.y = 192 - 1;	// y + 128

	// 8x8 grid => 128x128
	for (i = 0; i < 16; i++)
		for (j = 0; j < 16; j++)
			ptr[i * 16 + j] = (u8)get_random_u32();

	return sclr_blend_privacy_mask_test(&bld_cfg, &prm_cfg);
}

int sclr_disp_test(void)
{
	int ret = 0;
	u8 inst = 0; // sc_v0
	//u32 tmp;
	u16 pitch[3] = {0, 0, 0};
	enum sclr_img_in img_inst = SCL_IMG_D;
	struct sclr_scale_cfg cfg;
	enum sclr_input in = SCL_INPUT_MEM;
	enum sclr_format src_fmt = SCL_FMT_RGB_PACKED;
	enum sclr_format dst_fmt = SCL_FMT_RGB_PACKED;
	u64 img_in_src_addr = src_addr_phy[0][R_IDX];
	struct sclr_mem mem;
	struct sclr_odma_cfg *odma_cfg;
	struct sclr_top_cfg *top_cfg;

	pr_err("Please run 42) disp_tgen, 43) lvdstx first\n");
	pr_err("0x90000000: 800-16.ppm_rgbp, 800x600 rgb packed\n");

#if 0
	// CFG_OP_DISP_TGEN
	sclr_disp_tgen_enable(1);

	// CFG_OP_LVDSTX
	//union sclr_lvdstx lvdstx_cfg;
	sclr_lvdstx_get(&lvdstx_cfg);
	lvdstx_cfg.b.en = 1;
	if (lvdstx_cfg.b.en) {
		lvdstx_cfg.b.out_bit = 0;
		lvdstx_cfg.b.vesa_mode = 1;
		lvdstx_cfg.b.dual_ch = 0;
		lvdstx_cfg.b.vs_out_en = 1;
		lvdstx_cfg.b.hs_out_en = 1;
		lvdstx_cfg.b.hs_blk_en = 1;
		lvdstx_cfg.b.ml_swap = 1;
		lvdstx_cfg.b.ctrl_rev = 0;
		lvdstx_cfg.b.oe_swap = 0;
		_reg_write(REG_DSI_PHY_PD, 0x1f1f);
		dphy_lvds_enable(true);
	}
	sclr_lvdstx_set(lvdstx_cfg);
#endif

	// Scaler
	memset(&cfg, 0, sizeof(cfg));
	cfg.src.w = 800;
	cfg.src.h = 600;
	cfg.crop.w = cfg.src.w;
	cfg.crop.h = cfg.src.h;
	cfg.dst.w = cfg.src.w;
	cfg.dst.h = cfg.src.h;

	sclr_ctrl_set_scale(inst, &cfg);
	sclr_set_scale_phase(inst, 0, 0);

	// CFG_OP_IMG_CFG
	// dram rgbp -> sclr rgb
	sclr_ctrl_set_input(img_inst, in, src_fmt, SCL_CSC_NONE, false);

	// CFG_OP_IMG_SET_MEM
	memset(&mem, 0, sizeof(mem));
	mem.width = cfg.src.w;
	mem.height = cfg.src.h;
	mem.start_x = 0;
	mem.start_y = 0;
	mem.addr0 = img_in_src_addr;
	mem.addr1 = mem.addr0 + cfg.src.w * cfg.src.h;

	_get_bytesperline(src_fmt, cfg.src.w, pitch);
	mem.pitch_y = pitch[0];
	mem.pitch_c = pitch[1];
	sclr_img_set_mem(img_inst, &mem, true);

	// CFG_OP_SC_CFG, odma disabled
	sclr_set_cfg(inst, /*sc_bypass*/false, /*gop_bypass*/true, /*cir_bypass*/true, /*odma_bypass*/true);

	// CFG_OP_SC_SET_INPUT
	sclr_set_input_size(inst, cfg.src, true);

	// CFG_OP_SC_SET_CROP
	sclr_set_crop(inst, cfg.crop, true);
	sclr_set_scale(inst);

	// CFG_OP_SC_SET_OUTPUT
	sclr_set_output_size(inst, cfg.dst);
	sclr_set_scale(inst);

	// CFG_OP_ODMA_CFG, odma disabled in when display used
	odma_cfg = sclr_odma_get_cfg(inst);
	memset(odma_cfg, 0, sizeof(*odma_cfg));
	odma_cfg->csc_cfg.mode = SCL_OUT_DISABLE;
	odma_cfg->fmt = dst_fmt;
	odma_cfg->csc_cfg.csc_type = SCL_CSC_NONE;
	sclr_odma_set_cfg(inst, odma_cfg);
	sclr_ctrl_set_output(inst, &odma_cfg->csc_cfg, odma_cfg->fmt);

	// CFG_OP_ODMA_SET_MEM
	mem.width = cfg.dst.w;
	mem.height = cfg.dst.h;
	mem.start_x = 0;
	mem.start_y = 0;
	mem.addr0 = dst_addr_phy[0][R_IDX];

	_get_bytesperline(odma_cfg->fmt, cfg.dst.w, pitch);
	mem.pitch_y = pitch[0];
	mem.pitch_c = pitch[1];
	sclr_odma_set_mem(inst, &mem);

	if (0 == inst) {
		struct sclr_disp_cfg *disp_cfg = sclr_disp_get_cfg();
		disp_cfg->fmt = odma_cfg->fmt;
		if (odma_cfg->csc_cfg.csc_type != SCL_CSC_NONE) disp_cfg->in_csc = odma_cfg->csc_cfg.csc_type - SCL_CSC_601_LIMIT_RGB2YUV + 1;
		disp_cfg->mem = mem;
		sclr_disp_set_cfg(disp_cfg);

		sclr_ctrl_set_disp_src(/*disp_from_sc*/true);
	}

	sclr_top_reg_done();
	_intr_setup(inst, img_inst);

	// No interrupt log
	//sclr_enable_intr_log(0);

	// CFG_OP_IMG_START
	sclr_trig_start_ext(img_inst, inst);

	if (sclr_check_irq_handler(10000) != 0) {
		pr_err("img(%d), sc(%d), (w=%d, h=%d), end intr doesn't happens after 10000 tick\n", img_inst, inst, cfg.dst.w, cfg.dst.h);
		ret = -1;

		sclr_dump_top_register();
		sclr_dump_img_in_register(img_inst);
		sclr_dump_core_register(inst);
		sclr_dump_odma_register(inst);
		sclr_dump_gop_register(inst);
		sclr_dump_disp_register();
	} else {
		pr_err("img(%d), sc(%d), (w=%d, h=%d), sclr done\n", img_inst, inst, cfg.dst.w, cfg.dst.h);
	}

	//bm_get_num("Press any key to stop", tmp);

	// Disable scaler
	top_cfg = sclr_top_get_cfg();
	top_cfg->sclr_enable[0] = 0;
	sclr_top_set_cfg(top_cfg);

	//sclr_enable_intr_log(1);

	return ret;
}

int sclr_to_vc_sb_test_start_sclr(u8 inst, u8 img_inst)
{
	int ret = 0;
	enum sclr_input in;
	enum sclr_format src_fmt = SCL_FMT_NV21;
	enum sclr_format dst_fmt = SCL_FMT_NV21;
	u64 img_in_src_addr = src_addr_phy[0][R_IDX];
	u64 odma_out_addr_y = dst_addr_phy[0][R_IDX];
	u64 odma_out_addr_uv = dst_addr_phy[0][G_IDX];
	struct sclr_mem mem;
	u16 pitch[3] = {0, 0, 0};
	struct sclr_scale_cfg sc_cfg;
	struct sclr_odma_cfg *odma_cfg;

	printk("0x%08x: 320_256.ppm_NV21, 320x256 NV12 packed\n", (u32)img_in_src_addr);
	printk("0x%08x: 320x256 NV21 packed, Y\n", (u32)odma_out_addr_y);
	printk("0x%08x: 320x256 NV21 packed, UV\n", (u32)odma_out_addr_uv);

	memset(&sc_cfg, 0, sizeof(sc_cfg));

	sc_cfg.src.w = 320;
	sc_cfg.src.h = 256;
	sc_cfg.crop.w = sc_cfg.src.w;
	sc_cfg.crop.h = sc_cfg.src.h;
	sc_cfg.dst.w = 320;
	sc_cfg.dst.h = 256;

	sclr_ctrl_set_scale(inst, &sc_cfg);
	sclr_set_scale_phase(inst, 0, 0);

	// CFG_OP_IMG_CFG
	in = SCL_INPUT_MEM;	// still from dram

	// dram -> sclr rgb
	sclr_ctrl_set_input(img_inst, in, src_fmt, SCL_CSC_601_LIMIT_YUV2RGB, false);

	// CFG_OP_IMG_SET_MEM
	memset(&mem, 0, sizeof(mem));
	mem.width = sc_cfg.src.w;
	mem.height = sc_cfg.src.h;
	mem.start_x = 0;
	mem.start_y = 0;
	mem.addr0 = img_in_src_addr;
	mem.addr1 = mem.addr0 + sc_cfg.src.w * sc_cfg.src.h;

	_get_bytesperline(src_fmt, sc_cfg.src.w, pitch);
	mem.pitch_y = pitch[0];
	mem.pitch_c = pitch[1];
	sclr_img_set_mem(img_inst, &mem, true);

	// CFG_OP_SC_CFG
	sclr_set_cfg(inst, /*sc_bypass*/false, /*gop_bypass*/true, /*cir_bypass*/true, /*odma_bypass*/false);

	// CFG_OP_SC_SET_INPUT
	sclr_set_input_size(inst, sc_cfg.src, true);

	// CFG_OP_SC_SET_CROP
	sclr_set_crop(inst, sc_cfg.crop, true);
	sclr_set_scale(inst);

	// CFG_OP_SC_SET_OUTPUT
	sclr_set_output_size(inst, sc_cfg.dst);
	sclr_set_scale(inst);

	// CFG_OP_ODMA_CFG, sclr rgb -> odma
	odma_cfg = sclr_odma_get_cfg(inst);
	memset(odma_cfg, 0, sizeof(*odma_cfg));
	odma_cfg->csc_cfg.mode = SCL_OUT_CSC;
	odma_cfg->fmt = dst_fmt;
	odma_cfg->csc_cfg.csc_type = SCL_CSC_NONE;

	if (odma_cfg->csc_cfg.mode == SCL_OUT_CSC) {
		enum sclr_csc csc = SCL_CSC_NONE;
		if (IS_YUV_FMT(odma_cfg->fmt))
			csc = SCL_CSC_601_LIMIT_RGB2YUV;

		odma_cfg->csc_cfg.csc_type = csc;
	}

	sclr_odma_set_cfg(inst, odma_cfg);
	sclr_ctrl_set_output(inst, &odma_cfg->csc_cfg, odma_cfg->fmt);

	// CFG_OP_ODMA_SET_MEM
	mem.width = sc_cfg.dst.w;
	mem.height = sc_cfg.dst.h;
	mem.start_x = 0;
	mem.start_y = 0;
	mem.addr0 = odma_out_addr_y;
	mem.addr1 = odma_out_addr_uv;

	_get_bytesperline(odma_cfg->fmt, sc_cfg.dst.w, pitch);
	mem.pitch_y = pitch[0];
	mem.pitch_c = pitch[1];
	sclr_odma_set_mem(inst, &mem);

	sclr_top_reg_done();
	_intr_setup(inst, img_inst);

	// CFG_OP_IMG_START
	sclr_trig_start_ext(img_inst, inst);

	if (sclr_check_irq_handler(10000) != 0) {
		printk("img(%d), sc(%d), (w=%d, h=%d), end intr doesn't happens after 10000 tick\n",
			img_inst, inst, sc_cfg.dst.w, sc_cfg.dst.h);
		ret = -1;
	} else {
		printk("img(%d), sc(%d), (w=%d, h=%d), sclr done\n", img_inst, inst, sc_cfg.dst.w, sc_cfg.dst.h);
		printk("sc(%d) odma status(%#x)\n", inst, sclr_odma_get_dbg_status(inst).raw);
	}

	return ret;
}

int sclr_to_vc_sb_test(void)
{
	int ret = 0;
	u8 inst = 1; // sc_v1
	enum sclr_img_in img_inst = SCL_IMG_V;

	sclr_set_sclr_to_vc_sb(inst, 2, 0, 0, 0);
	ret = sclr_to_vc_sb_test_start_sclr(inst, img_inst);

	pr_err("%s %s\n", __func__, ret ? "fail":"pass");

	return ret;
}

int sclr_to_vc_sb_ktest(void *data)
{
	int ret = 0;
	struct cvi_vpss_vc_sb_cfg *sb_cfg = (struct cvi_vpss_vc_sb_cfg *)data;
	u8 inst = sb_cfg->sc_inst;
	enum sclr_img_in img_inst = sb_cfg->img_inst;
	enum sclr_input in;
	enum sclr_format src_fmt = sb_cfg->img_in_fmt;
	enum sclr_format dst_fmt = sb_cfg->odma_fmt;
	struct sclr_mem mem;
	u16 pitch[3] = {0, 0, 0};
	struct sclr_scale_cfg sc_cfg;
	struct sclr_odma_cfg *odma_cfg;

	sclr_set_sclr_to_vc_sb(inst, 2, 0, 0, 0);

	printk("  img_in(%d), sc(%d)\n", sb_cfg->img_inst, sb_cfg->sc_inst);
	printk("  0x%08x 0x%08x 0x%08x: %dx%d fmt %d\n",
		(u32)sb_cfg->img_in_address[0], (u32)sb_cfg->img_in_address[1],
		(u32)sb_cfg->img_in_address[2],
		sb_cfg->img_in_width, sb_cfg->img_in_height,
		sb_cfg->img_in_fmt);
	printk("  0x%08x 0x%08x 0x%08x: %dx%d fmt %d\n",
		(u32)sb_cfg->odma_address[0], (u32)sb_cfg->odma_address[1],
		(u32)sb_cfg->odma_address[2],
		sb_cfg->odma_width, sb_cfg->odma_height,
		sb_cfg->odma_fmt);

	memset(&sc_cfg, 0, sizeof(sc_cfg));

	sc_cfg.src.w = sb_cfg->img_in_width;
	sc_cfg.src.h = sb_cfg->img_in_height;
	sc_cfg.crop.w = sc_cfg.src.w;
	sc_cfg.crop.h = sc_cfg.src.h;
	sc_cfg.dst.w = sb_cfg->odma_width;
	sc_cfg.dst.h = sb_cfg->odma_height;

	sclr_ctrl_set_scale(inst, &sc_cfg);
	sclr_set_scale_phase(inst, 0, 0);

	// CFG_OP_IMG_CFG
	in = SCL_INPUT_MEM;	// still from dram

	// dram -> sclr rgb
	sclr_ctrl_set_input(img_inst, in, src_fmt, SCL_CSC_601_LIMIT_YUV2RGB, false);

	// CFG_OP_IMG_SET_MEM
	memset(&mem, 0, sizeof(mem));
	mem.width = sc_cfg.src.w;
	mem.height = sc_cfg.src.h;
	mem.start_x = 0;
	mem.start_y = 0;
	mem.addr0 = sb_cfg->img_in_address[0];
	mem.addr1 = sb_cfg->img_in_address[1];
	mem.addr2 = sb_cfg->img_in_address[2];

	_get_bytesperline(src_fmt, sc_cfg.src.w, pitch);
	mem.pitch_y = pitch[0];
	mem.pitch_c = pitch[1];
	sclr_img_set_mem(img_inst, &mem, true);

	// CFG_OP_SC_CFG
	sclr_set_cfg(inst, /*sc_bypass*/false, /*gop_bypass*/true, /*cir_bypass*/true, /*odma_bypass*/false);

	// CFG_OP_SC_SET_INPUT
	sclr_set_input_size(inst, sc_cfg.src, true);

	// CFG_OP_SC_SET_CROP
	sclr_set_crop(inst, sc_cfg.crop, true);
	sclr_set_scale(inst);

	// CFG_OP_SC_SET_OUTPUT
	sclr_set_output_size(inst, sc_cfg.dst);
	sclr_set_scale(inst);

	// CFG_OP_ODMA_CFG, sclr rgb -> odma
	odma_cfg = sclr_odma_get_cfg(inst);
	memset(odma_cfg, 0, sizeof(*odma_cfg));
	odma_cfg->csc_cfg.mode = SCL_OUT_CSC;
	odma_cfg->fmt = dst_fmt;
	odma_cfg->csc_cfg.csc_type = SCL_CSC_NONE;

	if (odma_cfg->csc_cfg.mode == SCL_OUT_CSC) {
		enum sclr_csc csc = SCL_CSC_NONE;
		if (IS_YUV_FMT(odma_cfg->fmt))
			csc = SCL_CSC_601_LIMIT_RGB2YUV;

		odma_cfg->csc_cfg.csc_type = csc;
	}

	sclr_odma_set_cfg(inst, odma_cfg);
	sclr_ctrl_set_output(inst, &odma_cfg->csc_cfg, odma_cfg->fmt);

	// CFG_OP_ODMA_SET_MEM
	mem.width = sc_cfg.dst.w;
	mem.height = sc_cfg.dst.h;
	mem.start_x = 0;
	mem.start_y = 0;
	mem.addr0 = sb_cfg->odma_address[0];
	mem.addr1 = sb_cfg->odma_address[1];
	mem.addr2 = sb_cfg->odma_address[2];

	_get_bytesperline(odma_cfg->fmt, sc_cfg.dst.w, pitch);
	mem.pitch_y = pitch[0];
	mem.pitch_c = pitch[1];
	sclr_odma_set_mem(inst, &mem);

	sclr_top_reg_done();
	_intr_setup(inst, img_inst);

	// CFG_OP_IMG_START
	sclr_trig_start_ext(img_inst, inst);

	if (sclr_check_irq_handler(10000) != 0) {
		printk("  img(%d), sc(%d), (w=%d, h=%d), end intr doesn't happens after 10000 tick\n",
			img_inst, inst, sc_cfg.dst.w, sc_cfg.dst.h);
		ret = -1;
	} else {
		printk("  img(%d), sc(%d), (w=%d, h=%d), sclr done\n", img_inst, inst, sc_cfg.dst.w, sc_cfg.dst.h);
		printk("  sc(%d) odma status(%#x)\n", inst, sclr_odma_get_dbg_status(inst).raw);
	}

	pr_err("%s %s\n", __func__, ret ? "fail":"pass");

	return ret;
}

int sclr_force_img_in_trigger(void)
{
	enum sclr_img_in img_inst = SCL_IMG_V;
	struct sclr_img_cfg *img_cfg = sclr_img_get_cfg(img_inst);

	img_cfg->src = SCL_INPUT_MEM;
	img_cfg->trig_src = SCL_IMG_TRIG_SRC_SW;
	sclr_img_set_cfg(img_inst, img_cfg);

	sclr_top_reg_done();

	sclr_img_start(img_inst);

	return 0;
}

static void sclr_test_usage(struct seq_file *m)
{
	seq_puts(m, "  0: check img/scl size\n");
	seq_puts(m, "  1: Send File test\n");
	seq_puts(m, "  2: tile mode test, mirror off\n");
	seq_puts(m, "  3: tile mode test, mirror on\n");
	seq_puts(m, "  4: cmdQ test\n");
	seq_puts(m, "  5: blend alpha\n");
	seq_puts(m, "  6: blend stitch\n");
	seq_puts(m, "  7: blend alpha + privacy mask(rgb332)\n");
	seq_puts(m, "  8: blend alpha + privacy mask(y)\n");
	seq_puts(m, " 10: DISP test 800x600 rgb packed\n");
	seq_puts(m, " 23: Slice buffer: ldc to scaler, 320x240 nv21 -> 320x240 nv21\n");
	seq_puts(m, " 24: Slice buffer: scaler to vc, 320x256 nv21 -> 320x240 nv21\n");
	seq_puts(m, " 51: force img_in trigger\n");
	seq_puts(m, "100: dump top register\n");
	seq_puts(m, "11?: dump img_in register, 0:v, 1:d\n");
	seq_puts(m, "12?: dump sc core register, 0:sc_d, 1:sc_v1, 2:sc_v2, 3:sc_v3\n");
	seq_puts(m, "13?: dump sc odma register, 0:sc_d, 1:sc_v1, 2:sc_v2, 3:sc_v3\n");
	seq_puts(m, "140: dump display register\n");
	seq_puts(m, "15?: dump sc gop register, 0:sc_d, 1:sc_v1, 2:sc_v2, 3:sc_v3\n");
}

static int sclr_test_proc_show(struct seq_file *m, void *v)
{
	sclr_test_usage(m);

	return 0;
}

static int sclr_test_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sclr_test_proc_show, PDE_DATA(inode));
}

static uint32_t sclr_test_init(void)
{
	uint32_t old_shd_val;
	int i;
	union sclr_intr mask;

	mask.raw = 0xffffffff;
	sclr_intr_ctrl(mask);

	sclr_img_reg_shadow_sel(0, false);
	sclr_img_reg_shadow_sel(1, false);
	//sclr_disp_reg_shadow_sel(false);
	for (i = 0; i < SCL_MAX_INST; ++i) {
		sclr_reg_shadow_sel(i, false);
		sclr_init(i);
		sclr_set_cfg(i, false, false, true, false);
		sclr_reg_force_up(i);
	}

	old_shd_val = sclr_top_get_shd_reg();
	pr_err("  sc top REG04=0x%08x\n", old_shd_val);
	sclr_top_set_shd_reg(0x200);
	sclr_top_reg_force_up();

	return old_shd_val;
}

#if 0
static void sclr_test_reset_vpss(void)
{
	union vip_sys_reset mask;

	mask.raw = 0;
	mask.img_d = 1;
	mask.img_v = 1;
	mask.sc_top = 1;
	mask.sc_v1 = 1;
	mask.sc_v2 = 1;
	vip_toggle_reset(mask);
}
#endif

void open_clk(void)
{
	int i;

	if (pdev->clk_sys[1])
		clk_prepare_enable(pdev->clk_sys[1]);
	if (pdev->clk_sc_top)
		clk_prepare_enable(pdev->clk_sc_top);
	for (i = 0; i < SCL_IMG_MAX; ++i)
		clk_prepare_enable(pdev->img_vdev[i].clk);
	for (i = 0; i < SCL_MAX_INST; ++i)
		clk_prepare_enable(pdev->sc_vdev[i].clk);
}

void close_clk(void)
{
	int i;

	if (pdev->clk_sys[1])
		clk_disable_unprepare(pdev->clk_sys[1]);
	if (pdev->clk_sc_top)
		clk_disable_unprepare(pdev->clk_sc_top);
	for (i = 0; i < SCL_IMG_MAX; ++i)
		clk_disable_unprepare(pdev->img_vdev[i].clk);
	for (i = 0; i < SCL_MAX_INST; ++i)
		clk_disable_unprepare(pdev->sc_vdev[i].clk);
}

static ssize_t sclr_test_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	uint32_t input_param = 0;
	u32 old_shd_val = 0;

	if (kstrtouint_from_user(user_buf, count, 0, &input_param)) {
		pr_err("input parameter incorrect\n");
		return count;
	}

	//reset related info
	pr_err("\ninput_param=%d\n", input_param);

	// Same as bmtest
	if (input_param < 100) {
		open_clk();
		old_shd_val = sclr_test_init();
		sclr_test_enabled = 1;
	}

	switch (input_param) {
	case 0:
		sclr_test_alloc_ion();
		sclr_size_test();
		sclr_test_free_ion();
		break;

	case 1:
		sclr_test_alloc_ion();
		sclr_send_file_test();
		sclr_test_free_ion();
		break;

	case 2:
		sclr_test_alloc_ion();
		sclr_tile_test(false);
		sclr_test_free_ion();
		break;

	case 3:
		sclr_test_alloc_ion();
		sclr_tile_test(true);
		sclr_test_free_ion();
		break;

	case 4:
		sclr_test_alloc_ion();
		sclr_cmdq_test();
		sclr_test_free_ion();
		break;

	case 5:
		sclr_test_alloc_ion();
		sclr_blend_test_1();
		sclr_test_free_ion();
		break;

	case 6:
		sclr_test_alloc_ion();
		sclr_blend_test_2();
		sclr_test_free_ion();
		break;

	case 7:
		sclr_test_alloc_ion();
		sclr_blend_test_3();
		sclr_test_free_ion();
		break;

	case 8:
		sclr_test_alloc_ion();
		sclr_blend_test_4();
		sclr_test_free_ion();
		break;

	case 10:
		sclr_test_alloc_ion();
		sclr_disp_test();
		sclr_test_free_ion();
		break;

	case 24:
		sclr_to_vc_sb_test();
		break;

	case 51:
		sclr_force_img_in_trigger();
		break;

	case 100:
		sclr_dump_top_register();
		break;

	case 110:
	case 111:
		sclr_dump_img_in_register(input_param - 110);
		break;

	case 120:
	case 121:
	case 122:
	case 123:
		sclr_dump_core_register(input_param - 120);
		break;

	case 130:
	case 131:
	case 132:
	case 133:
		sclr_dump_odma_register(input_param - 130);
		break;

	case 140:
		sclr_dump_disp_register();
		break;

	case 150:
	case 151:
	case 152:
	case 153:
		sclr_dump_gop_register(input_param - 150);
		break;

	//case 200:
	//	sclr_test_reset_vpss();
	//	break;

	default:
		break;
	}

	// Restore original setting
	if (input_param < 100) {
		sclr_top_set_shd_reg(old_shd_val);
		sclr_top_reg_force_up();
		close_clk();

		sclr_test_enabled = 0;
	}

	return count;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops sclr_test_proc_ops = {
	.proc_open = sclr_test_proc_open,
	.proc_read = seq_read,
	.proc_write = sclr_test_proc_write,
	.proc_release = single_release,
};
#else
static const struct file_operations sclr_test_proc_ops = {
	.owner = THIS_MODULE,
	.open = sclr_test_proc_open,
	.read = seq_read,
	.write = sclr_test_proc_write,
	.release = single_release,
};
#endif

int32_t sclr_test_proc_init(struct cvi_vip_dev *dev)
{
	int ret = 0;

	if (proc_create(PROC_NAME, 0644, NULL, &sclr_test_proc_ops) == NULL) {
		pr_err("sclr_test: : proc_init() failed\n");
		ret = -1;
	}
	pdev = dev;
	return ret;
}

int32_t sclr_test_proc_deinit(void)
{
	remove_proc_entry(PROC_NAME, NULL);
	pdev = NULL;
	return 0;
}
