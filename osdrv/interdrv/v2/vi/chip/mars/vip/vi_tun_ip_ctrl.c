#include <linux/slab.h>
#include <vip/vi_drv.h>

#define BE_RUNTIME_TUN(_name) \
	{\
		struct cvi_vip_isp_##_name##_config *cfg;\
		cfg = &be_tun->_name##_cfg;\
		ispblk_##_name##_tun_cfg(ctx, cfg, raw_num);\
	}

#define POST_RUNTIME_TUN(_name) \
	{\
		struct cvi_vip_isp_##_name##_config *cfg;\
		cfg = &post_tun->_name##_cfg;\
		ispblk_##_name##_tun_cfg(ctx, cfg, raw_num);\
	}

/****************************************************************************
 * Global parameters
 ****************************************************************************/
extern uint8_t g_w_bit[ISP_PRERAW_VIRT_MAX], g_h_bit[ISP_PRERAW_VIRT_MAX];
extern int tuning_dis[4];

struct isp_tuning_cfg tuning_buf_addr;
static void *vi_tuning_ptr[ISP_PRERAW_VIRT_MAX];

struct vi_clut_idx {
	u32		clut_tbl_idx;
	spinlock_t	clut_idx_lock;
};

struct vi_clut_idx gClutIdx;

/*******************************************************************************
 *	Tuning modules update
 ******************************************************************************/
void vi_tuning_gamma_ips_update(
	struct isp_ctx *ctx,
	enum cvi_isp_raw raw_num)
{
	u8 tun_idx = 0;
	static int stop_update_gamma_ip = -1;
	struct cvi_vip_isp_post_cfg     *post_cfg;
	struct cvi_vip_isp_post_tun_cfg *post_tun;

	struct cvi_vip_isp_ygamma_config  *ygamma_cfg;
	struct cvi_vip_isp_gamma_config   *gamma_cfg;
	struct cvi_vip_isp_ycur_config    *ycur_cfg;

	post_cfg = (struct cvi_vip_isp_post_cfg *)tuning_buf_addr.post_vir[raw_num];
	tun_idx  = post_cfg->tun_idx;

	if ((tun_idx >= TUNING_NODE_NUM) || (post_cfg->tun_update[tun_idx] == 0))
		return;

	post_tun = &post_cfg->tun_cfg[tun_idx];

	if (tuning_dis[3]) {
		if (stop_update_gamma_ip > 0)
			return;
		else if (tuning_dis[0] == 0) {
			stop_update_gamma_ip = 1;
			return;
		} else if ((tuning_dis[0] - 1) == raw_num)
			stop_update_gamma_ip = 1; // stop on next
	} else
		stop_update_gamma_ip = 0;

	ygamma_cfg = &post_cfg->tun_cfg[tun_idx].ygamma_cfg;
	ispblk_ygamma_tun_cfg(ctx, ygamma_cfg, raw_num);

	gamma_cfg = &post_cfg->tun_cfg[tun_idx].gamma_cfg;
	ispblk_gamma_tun_cfg(ctx, gamma_cfg, raw_num);

	ycur_cfg = &post_cfg->tun_cfg[tun_idx].ycur_cfg;
	ispblk_ycur_tun_cfg(ctx, ycur_cfg, raw_num);
}

void vi_tuning_dci_update(
	struct isp_ctx *ctx,
	enum cvi_isp_raw raw_num)
{
	u8 tun_idx = 0;
	static int stop_update_dci = -1;
	struct cvi_vip_isp_post_cfg     *post_cfg;
	struct cvi_vip_isp_post_tun_cfg *post_tun;
	struct cvi_vip_isp_dci_config   *dci_cfg;

	post_cfg = (struct cvi_vip_isp_post_cfg *)tuning_buf_addr.post_vir[raw_num];
	tun_idx  = post_cfg->tun_idx;

	if ((tun_idx >= TUNING_NODE_NUM) || (post_cfg->tun_update[tun_idx] == 0))
		return;

	post_tun = &post_cfg->tun_cfg[tun_idx];

	if (tuning_dis[3]) {
		if (stop_update_dci > 0)
			return;
		else if (tuning_dis[0] == 0) {
			stop_update_dci = 1;
			return;
		} else if ((tuning_dis[0] - 1) == raw_num)
			stop_update_dci = 1; // stop on next
	} else
		stop_update_dci = 0;

	dci_cfg = &post_cfg->tun_cfg[tun_idx].dci_cfg;
	ispblk_dci_tun_cfg(ctx, dci_cfg, raw_num);
}

void vi_tuning_drc_update(
	struct isp_ctx *ctx,
	enum cvi_isp_raw raw_num)
{
	u8 tun_idx = 0;
	static int stop_update_drc = -1;
	struct cvi_vip_isp_post_cfg     *post_cfg;
	struct cvi_vip_isp_post_tun_cfg *post_tun;
	struct cvi_vip_isp_drc_config   *drc_cfg;

	post_cfg = (struct cvi_vip_isp_post_cfg *)tuning_buf_addr.post_vir[raw_num];
	drc_cfg = &post_cfg->tun_cfg[tun_idx].drc_cfg;
	tun_idx  = post_cfg->tun_idx;

	if ((tun_idx >= TUNING_NODE_NUM) || (post_cfg->tun_update[tun_idx] == 0))
		return;

	post_tun = &post_cfg->tun_cfg[tun_idx];

	if (tuning_dis[3]) {
		if (stop_update_drc > 0)
			return;
		else if (tuning_dis[0] == 0) {
			stop_update_drc = 1;
			return;
		} else if ((tuning_dis[0] - 1) == raw_num)
			stop_update_drc = 1; // stop on next
	} else
		stop_update_drc = 0;

	ispblk_drc_tun_cfg(ctx, drc_cfg, raw_num);
}

void vi_tuning_clut_update(
	struct isp_ctx *ctx,
	enum cvi_isp_raw raw_num)
{
	u8 tun_idx = 0;
	static int stop_update_clut = -1;
	struct cvi_vip_isp_post_cfg     *post_cfg;
	struct cvi_vip_isp_post_tun_cfg *post_tun;
	struct cvi_vip_isp_clut_config  *clut_cfg;
	unsigned long flags;

	post_cfg = (struct cvi_vip_isp_post_cfg *)tuning_buf_addr.post_vir[raw_num];
	tun_idx  = post_cfg->tun_idx;

	vi_pr(VI_DBG, "Postraw_%d tuning update(%d):idx(%d)\n",
			raw_num, post_cfg->tun_update[tun_idx], tun_idx);

	if ((tun_idx >= TUNING_NODE_NUM) || (post_cfg->tun_update[tun_idx] == 0))
		return;

	post_tun = &post_cfg->tun_cfg[tun_idx];

	if (tuning_dis[3]) {
		if (stop_update_clut > 0)
			return;
		else if (tuning_dis[0] == 0) {
			stop_update_clut = 1;
			return;
		} else if ((tuning_dis[0] - 1) == raw_num)
			stop_update_clut = 1; // stop on next
	} else
		stop_update_clut = 0;

	clut_cfg = &post_cfg->tun_cfg[tun_idx].clut_cfg;
	ispblk_clut_tun_cfg(ctx, clut_cfg, raw_num);

	//Record the clut tbl idx written into HW for ISP MW.
	spin_lock_irqsave(&gClutIdx.clut_idx_lock, flags);
	gClutIdx.clut_tbl_idx = clut_cfg->tbl_idx;
	spin_unlock_irqrestore(&gClutIdx.clut_idx_lock, flags);
}

int vi_tuning_get_clut_tbl_idx(void)
{
	unsigned long flags;
	u32 tbl_idx = 0;

	//Return clut tbl idx to ISP MW that currently is written into ISP HW.
	spin_lock_irqsave(&gClutIdx.clut_idx_lock, flags);
	tbl_idx = gClutIdx.clut_tbl_idx;
	spin_unlock_irqrestore(&gClutIdx.clut_idx_lock, flags);

	return tbl_idx;
}

int vi_tuning_sw_init(void)
{
	gClutIdx.clut_tbl_idx = 0;
	spin_lock_init(&gClutIdx.clut_idx_lock);

	return 0;
}

int vi_tuning_buf_setup(void)
{
	u8 i = 0;
	static u64 fe_paddr[ISP_PRERAW_VIRT_MAX] = {0, 0};
	static u64 be_paddr[ISP_PRERAW_VIRT_MAX] = {0, 0};
	static u64 post_paddr[ISP_PRERAW_VIRT_MAX] = {0, 0};
	u32 size = 0;

	size = (VI_ALIGN(sizeof(struct cvi_vip_isp_post_cfg)) +
		VI_ALIGN(sizeof(struct cvi_vip_isp_be_cfg)) +
		VI_ALIGN(sizeof(struct cvi_vip_isp_fe_cfg)));

	for (i = 0; i < ISP_PRERAW_VIRT_MAX; i++) {
		u64 phyAddr = 0;

		if (vi_tuning_ptr[i] == NULL) {
			vi_tuning_ptr[i] = kzalloc(size, GFP_KERNEL | __GFP_RETRY_MAYFAIL);
			if (vi_tuning_ptr[i] == NULL) {
				vi_pr(VI_ERR, "tuning_buf ptr[%d] kmalloc size(%u) fail\n", i, size);
				return -ENOMEM;
			}

			phyAddr = virt_to_phys(vi_tuning_ptr[i]);
		}

		if (post_paddr[i] == 0) {
			post_paddr[i] = phyAddr;
			tuning_buf_addr.post_addr[i] = post_paddr[i];
			tuning_buf_addr.post_vir[i] = phys_to_virt(post_paddr[i]);
		}

		if (be_paddr[i] == 0) {
			be_paddr[i] = phyAddr + VI_ALIGN(sizeof(struct cvi_vip_isp_post_cfg));
			tuning_buf_addr.be_addr[i] = be_paddr[i];
			tuning_buf_addr.be_vir[i] = phys_to_virt(be_paddr[i]);
		}

		if (fe_paddr[i] == 0) {
			fe_paddr[i] = phyAddr + VI_ALIGN(sizeof(struct cvi_vip_isp_post_cfg))
					+ VI_ALIGN(sizeof(struct cvi_vip_isp_be_cfg));
			tuning_buf_addr.fe_addr[i] = fe_paddr[i];
			tuning_buf_addr.fe_vir[i] = phys_to_virt(fe_paddr[i]);
		}

		vi_pr(VI_INFO, "tuning fe_addr[%d]=0x%llx, be_addr[%d]=0x%llx, post_addr[%d]=0x%llx\n",
				i, tuning_buf_addr.fe_addr[i],
				i, tuning_buf_addr.be_addr[i],
				i, tuning_buf_addr.post_addr[i]);
	}

	return 0;
}

void vi_tuning_buf_release(void)
{
	u8 i;

	for (i = 0; i < ISP_PRERAW_VIRT_MAX; i++) {
		kfree(vi_tuning_ptr[i]);
		vi_tuning_ptr[i] = NULL;
	}
}

void *vi_get_tuning_buf_addr(u32 *size)
{
	*size = sizeof(struct isp_tuning_cfg);

	return (void *)&tuning_buf_addr;
}

void vi_tuning_buf_clear(void)
{
	struct cvi_vip_isp_post_cfg *post_cfg;
	struct cvi_vip_isp_be_cfg   *be_cfg;
	struct cvi_vip_isp_fe_cfg   *fe_cfg;
	u8 i = 0, tun_idx = 0;

	for (i = 0; i < ISP_PRERAW_VIRT_MAX; i++) {
		post_cfg = (struct cvi_vip_isp_post_cfg *)tuning_buf_addr.post_vir[i];
		be_cfg   = (struct cvi_vip_isp_be_cfg *)tuning_buf_addr.be_vir[i];
		fe_cfg   = (struct cvi_vip_isp_fe_cfg *)tuning_buf_addr.fe_vir[i];

		if (tuning_buf_addr.post_vir[i] != NULL) {
			memset((void *)tuning_buf_addr.post_vir[i], 0x0, sizeof(struct cvi_vip_isp_post_cfg));
			tun_idx = post_cfg->tun_idx;
			vi_pr(VI_INFO, "Clear post tuning tun_update(%d), tun_idx(%d)",
					post_cfg->tun_update[tun_idx], tun_idx);
		}

		if (tuning_buf_addr.be_vir[i] != NULL) {
			memset((void *)tuning_buf_addr.be_vir[i], 0x0, sizeof(struct cvi_vip_isp_be_cfg));
			tun_idx = be_cfg->tun_idx;
			vi_pr(VI_INFO, "Clear be tuning tun_update(%d), tun_idx(%d)",
					be_cfg->tun_update[tun_idx], tun_idx);
		}

		if (tuning_buf_addr.fe_vir[i] != NULL) {
			memset((void *)tuning_buf_addr.fe_vir[i], 0x0, sizeof(struct cvi_vip_isp_fe_cfg));
			tun_idx = fe_cfg->tun_idx;
			vi_pr(VI_INFO, "Clear fe tuning tun_update(%d), tun_idx(%d)",
					fe_cfg->tun_update[tun_idx], tun_idx);
		}
	}
}

void pre_fe_tuning_update(
	struct isp_ctx *ctx,
	enum cvi_isp_raw raw_num)
{
	u8 idx = 0, tun_idx = 0;
	static int stop_update = -1;
	struct cvi_vip_isp_fe_cfg *fe_cfg;
	struct cvi_vip_isp_fe_tun_cfg *fe_tun;

	fe_cfg = (struct cvi_vip_isp_fe_cfg *)tuning_buf_addr.fe_vir[raw_num];
	tun_idx = fe_cfg->tun_idx;

	vi_pr(VI_DBG, "Pre_fe_%d tuning update(%d):idx(%d)\n",
			raw_num, fe_cfg->tun_update[tun_idx], tun_idx);

	if ((tun_idx >= TUNING_NODE_NUM) || (fe_cfg->tun_update[tun_idx] == 0))
		return;

	fe_tun = &fe_cfg->tun_cfg[tun_idx];

	if (tuning_dis[1]) {
		if (stop_update > 0)
			return;
		else if (tuning_dis[0] == 0) {
			stop_update = 1;
			return;
		} else if ((tuning_dis[0] - 1) == raw_num)
			stop_update = 1; // stop on next
	} else
		stop_update = 0;

	for (idx = 0; idx < 2; idx++) {
		struct cvi_vip_isp_blc_config	*blc_cfg;
		//struct cvi_vip_isp_lscr_config	*lscr_cfg;
		struct cvi_vip_isp_wbg_config	*wbg_cfg;

		blc_cfg  = &fe_tun->blc_cfg[idx];
		ispblk_blc_tun_cfg(ctx, blc_cfg, raw_num);

		//lscr_cfg = &fe_tun->lscr_cfg[idx];
		//ispblk_lscr_tun_cfg(ctx, lscr_cfg, raw_num);

		wbg_cfg = &fe_tun->wbg_cfg[idx];
		ispblk_wbg_tun_cfg(ctx, wbg_cfg, raw_num);
	}
}

void pre_be_tuning_update(
	struct isp_ctx *ctx,
	enum cvi_isp_raw raw_num)
{
	u8 idx = 0, tun_idx = 0;
	static int stop_update = -1;
	struct cvi_vip_isp_be_cfg *be_cfg;
	struct cvi_vip_isp_be_tun_cfg *be_tun;

	be_cfg	= (struct cvi_vip_isp_be_cfg *)tuning_buf_addr.be_vir[raw_num];
	tun_idx = be_cfg->tun_idx;

	vi_pr(VI_DBG, "Pre_be_%d tuning update(%d):idx(%d)\n",
			raw_num, be_cfg->tun_update[tun_idx], tun_idx);

	if ((tun_idx >= TUNING_NODE_NUM) || (be_cfg->tun_update[tun_idx] == 0))
		return;

	be_tun = &be_cfg->tun_cfg[tun_idx];

	if (tuning_dis[2]) {
		if (tuning_dis[0] == 0) {
			vi_pr(VI_DBG, "raw_%d stop tuning_update immediately\n", raw_num);
			return;
		} else if ((tuning_dis[0] - 1) == raw_num) {//stop on next
			if (stop_update > 0) {
				vi_pr(VI_DBG, "raw_%d stop tuning_update\n", raw_num);
				return;
			}
			stop_update = 1;
		} else {//must update tuning buf for sensor, it's will be not trrigered
			stop_update = 0;
		}
	} else
		stop_update = 0;

	for (idx = 0; idx < 2; idx++) {
		struct cvi_vip_isp_blc_config	*blc_cfg;
		struct cvi_vip_isp_dpc_config	*dpc_cfg;
		struct cvi_vip_isp_ge_config	*ge_cfg;

		blc_cfg = &be_tun->blc_cfg[idx];
		ispblk_blc_tun_cfg(ctx, blc_cfg, raw_num);

		dpc_cfg = &be_tun->dpc_cfg[idx];
		ispblk_dpc_tun_cfg(ctx, dpc_cfg, raw_num);

		ge_cfg = &be_tun->ge_cfg[idx];
		ispblk_ge_tun_cfg(ctx, ge_cfg, raw_num);
	}

	BE_RUNTIME_TUN(af);
}

void postraw_tuning_update(
	struct isp_ctx *ctx,
	enum cvi_isp_raw raw_num)
{
	u8 idx = 0, tun_idx = 0;
	static int stop_update = -1;
	struct cvi_vip_isp_post_cfg     *post_cfg;
	struct cvi_vip_isp_post_tun_cfg *post_tun;
	struct cvi_vip_isp_clut_config  *clut_cfg;
	unsigned long flags;

	post_cfg = (struct cvi_vip_isp_post_cfg *)tuning_buf_addr.post_vir[raw_num];
	tun_idx  = post_cfg->tun_idx;

	vi_pr(VI_DBG, "Postraw_%d tuning update(%d):idx(%d)\n",
			raw_num, post_cfg->tun_update[tun_idx], tun_idx);

	if ((tun_idx >= TUNING_NODE_NUM) || (post_cfg->tun_update[tun_idx] == 0))
		return;

	post_tun = &post_cfg->tun_cfg[tun_idx];

	if (tuning_dis[3]) {
		if (tuning_dis[0] == 0) {
			vi_pr(VI_DBG, "raw_%d stop tuning_update immediately\n", raw_num);
			return;
		} else if ((tuning_dis[0] - 1) == raw_num) {//stop on next
			if (stop_update > 0) {
				vi_pr(VI_DBG, "raw_%d stop tuning_update\n", raw_num);
				return;
			}
			stop_update = 1;
		} else {//must update tuning buf for sensor, it's will be not trrigered
			stop_update = 0;
		}
	} else
		stop_update = 0;

	for (idx = 0; idx < (ctx->isp_pipe_cfg[raw_num].is_hdr_on + 1); idx++) {
		struct cvi_vip_isp_wbg_config	*wbg_cfg;
		struct cvi_vip_isp_ccm_config	*ccm_cfg;
		struct cvi_vip_isp_ae_config	*ae_cfg;

		wbg_cfg = &post_tun->wbg_cfg[idx];
		ispblk_wbg_tun_cfg(ctx, wbg_cfg, raw_num);

		ccm_cfg = &post_tun->ccm_cfg[idx];
		ispblk_ccm_tun_cfg(ctx, ccm_cfg, raw_num);

		ae_cfg = &post_tun->ae_cfg[idx];
		ispblk_ae_tun_cfg(ctx, ae_cfg, raw_num);
	}

	POST_RUNTIME_TUN(bnr);
	POST_RUNTIME_TUN(lsc);
	POST_RUNTIME_TUN(gms);
	POST_RUNTIME_TUN(rgbcac);
	POST_RUNTIME_TUN(lcac);
	POST_RUNTIME_TUN(demosiac);

	POST_RUNTIME_TUN(fswdr);
	// POST_RUNTIME_TUN(drc);
	POST_RUNTIME_TUN(hist_v);
	//HW limit
	//Need to update gamma ips in postraw done
	//POST_RUNTIME_TUN(ygamma);
	//POST_RUNTIME_TUN(gamma);
	POST_RUNTIME_TUN(dhz);

	//Clut can't be writen when streaming
	//HW limit
	//Need to update gamma ips in postraw done
	if (_is_be_post_online(ctx) || (!ctx->is_slice_buf_on)) {// _be_post_online or not slice buffer mode
		if (!ctx->isp_pipe_cfg[raw_num].is_offline_preraw) {//timing of raw replay is before trigger
			POST_RUNTIME_TUN(drc);
			POST_RUNTIME_TUN(ygamma);
			POST_RUNTIME_TUN(gamma);
			POST_RUNTIME_TUN(dci);
			POST_RUNTIME_TUN(ycur);

			clut_cfg = &post_tun->clut_cfg;
			ispblk_clut_tun_cfg(ctx, clut_cfg, raw_num);

			//Record the clut tbl idx written into HW for ISP MW.
			spin_lock_irqsave(&gClutIdx.clut_idx_lock, flags);
			gClutIdx.clut_tbl_idx = clut_cfg->tbl_idx;
			spin_unlock_irqrestore(&gClutIdx.clut_idx_lock, flags);
		}
	}

	POST_RUNTIME_TUN(csc);
	//HW limit
	//To update dci tuning at postraw done because josh's ping pong sram has bug
	//POST_RUNTIME_TUN(dci);
	POST_RUNTIME_TUN(ldci);
	POST_RUNTIME_TUN(pre_ee);
	POST_RUNTIME_TUN(tnr);
	POST_RUNTIME_TUN(mono);
	POST_RUNTIME_TUN(cnr);
	POST_RUNTIME_TUN(cac);
	POST_RUNTIME_TUN(ynr);
	POST_RUNTIME_TUN(ee);
	POST_RUNTIME_TUN(cacp);
	POST_RUNTIME_TUN(ca2);
	//HW limit
	//Need to update gamma ips in postraw done
	//POST_RUNTIME_TUN(ycur);
}

/****************************************************************************
 *	Tuning Config
 ****************************************************************************/
void ispblk_blc_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_blc_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	int id = blc_find_hwid(cfg->inst);
	uintptr_t blc;

	if (!cfg->update || id < 0)
		return;

	blc = ctx->phys_regs[id];

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_0, BLC_BYPASS, cfg->bypass);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_2, BLC_ENABLE, cfg->enable);

	if (!cfg->enable)
		return;

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_3, BLC_OFFSET_R, cfg->roffset);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_3, BLC_OFFSET_GR, cfg->groffset);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_4, BLC_OFFSET_GB, cfg->gboffset);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_4, BLC_OFFSET_B, cfg->boffset);

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_9, BLC_2NDOFFSET_R, cfg->roffset_2nd);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_9, BLC_2NDOFFSET_GR, cfg->groffset_2nd);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_A, BLC_2NDOFFSET_GB, cfg->gboffset_2nd);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_A, BLC_2NDOFFSET_B, cfg->boffset_2nd);

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_5, BLC_GAIN_R, cfg->rgain);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_5, BLC_GAIN_GR, cfg->grgain);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_6, BLC_GAIN_GB, cfg->gbgain);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_6, BLC_GAIN_B, cfg->bgain);
}

void ispblk_wbg_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_wbg_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba;
	int id = wbg_find_hwid(cfg->inst);

	if (!cfg->update || id < 0)
		return;

	ba = ctx->phys_regs[id];

	ISP_WR_BITS(ba, REG_ISP_WBG_T, WBG_0, WBG_BYPASS, cfg->bypass);
	ISP_WR_BITS(ba, REG_ISP_WBG_T, WBG_2, WBG_ENABLE, cfg->enable);

	if (!cfg->enable)
		return;

	ISP_WR_BITS(ba, REG_ISP_WBG_T, WBG_4, WBG_RGAIN, cfg->rgain);
	ISP_WR_BITS(ba, REG_ISP_WBG_T, WBG_4, WBG_GGAIN, cfg->ggain);
	ISP_WR_BITS(ba, REG_ISP_WBG_T, WBG_5, WBG_BGAIN, cfg->bgain);
	ISP_WR_REG(ba, REG_ISP_WBG_T, WBG_34, cfg->rgain_fraction);
	ISP_WR_REG(ba, REG_ISP_WBG_T, WBG_38, cfg->ggain_fraction);
	ISP_WR_REG(ba, REG_ISP_WBG_T, WBG_3C, cfg->bgain_fraction);
}

/****************************************************************************
 *	Postraw Tuning Config
 ****************************************************************************/
void ispblk_ccm_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_ccm_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	int id = ccm_find_hwid(cfg->inst);
	uintptr_t ccm;

	if (!cfg->update || id < 0)
		return;

	ccm = ctx->phys_regs[id];

	ISP_WR_BITS(ccm, REG_ISP_CCM_T, CCM_CTRL, CCM_ENABLE, cfg->enable);

	if (!cfg->enable)
		return;

	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_00, cfg->coef[0][0]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_01, cfg->coef[0][1]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_02, cfg->coef[0][2]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_10, cfg->coef[1][0]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_11, cfg->coef[1][1]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_12, cfg->coef[1][2]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_20, cfg->coef[2][0]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_21, cfg->coef[2][1]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_22, cfg->coef[2][2]);
}

void ispblk_cacp_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_cacp_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t cacp = ctx->phys_regs[ISP_BLK_ID_CA];
	uint16_t i;
	union REG_CA_00 ca_00;
	union REG_CA_04 wdata;

	if (!cfg->update)
		return;

	ca_00.raw = ISP_RD_REG(cacp, REG_CA_T, REG_00);
	ca_00.bits.CACP_ENABLE		= cfg->enable;
	ca_00.bits.CACP_MODE		= cfg->mode; // 0 CA mode, 1 CP mode
	ca_00.bits.CACP_ISO_RATIO	= cfg->iso_ratio;
	ISP_WR_REG(cacp, REG_CA_T, REG_00, ca_00.raw);

	if (!cfg->enable)
		return;

	ISP_WR_BITS(cacp, REG_CA_T, REG_00, CACP_MEM_SW_MODE, 1);
	if (cfg->mode == 0) {
		for (i = 0; i < 256; i++) {
			wdata.raw = 0;
			wdata.bits.CACP_MEM_D = cfg->ca_y_ratio_lut[i];
			wdata.bits.CACP_MEM_W = 1;
			ISP_WR_REG(cacp, REG_CA_T, REG_04, wdata.raw);
		}
	} else { //cp mode
		for (i = 0; i < 256; i++) {
			wdata.raw = 0;
			wdata.bits.CACP_MEM_D = ((cfg->cp_y_lut[i] << 16) |
					(cfg->cp_u_lut[i] << 8) | (cfg->cp_v_lut[i]));
			wdata.bits.CACP_MEM_W = 1;
			ISP_WR_REG(cacp, REG_CA_T, REG_04, wdata.raw);
		}
	}

	ISP_WR_BITS(cacp, REG_CA_T, REG_00, CACP_MEM_SW_MODE, 0);
}

void ispblk_ca2_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_ca2_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ca_lite = ctx->phys_regs[ISP_BLK_ID_CA_LITE];

	if (!cfg->update)
		return;

	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_00, CA_LITE_ENABLE, cfg->enable);

	if (!cfg->enable)
		return;

	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_04, CA_LITE_LUT_IN_0, cfg->lut_in[0]);
	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_04, CA_LITE_LUT_IN_1, cfg->lut_in[1]);

	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_08, CA_LITE_LUT_IN_2, cfg->lut_in[2]);
	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_08, CA_LITE_LUT_IN_3, cfg->lut_in[3]);

	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_0C, CA_LITE_LUT_IN_4, cfg->lut_in[4]);
	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_0C, CA_LITE_LUT_IN_5, cfg->lut_in[5]);

	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_10, CA_LITE_LUT_OUT_0, cfg->lut_out[0]);
	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_10, CA_LITE_LUT_OUT_1, cfg->lut_out[1]);

	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_14, CA_LITE_LUT_OUT_2, cfg->lut_out[2]);
	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_14, CA_LITE_LUT_OUT_3, cfg->lut_out[3]);

	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_18, CA_LITE_LUT_OUT_4, cfg->lut_out[4]);
	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_18, CA_LITE_LUT_OUT_5, cfg->lut_out[5]);

	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_1C, CA_LITE_LUT_SLP_0, cfg->lut_slp[0]);
	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_1C, CA_LITE_LUT_SLP_1, cfg->lut_slp[1]);

	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_20, CA_LITE_LUT_SLP_2, cfg->lut_slp[2]);
	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_20, CA_LITE_LUT_SLP_3, cfg->lut_slp[3]);

	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_24, CA_LITE_LUT_SLP_4, cfg->lut_slp[4]);
}

void ispblk_ygamma_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_ygamma_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t gamma = ctx->phys_regs[ISP_BLK_ID_YGAMMA];
	int16_t i;

	union REG_YGAMMA_GAMMA_PROG_DATA reg_data;
	union REG_YGAMMA_GAMMA_PROG_CTRL prog_ctrl;

	if (!cfg->update)
		return;

	ISP_WR_BITS(gamma, REG_YGAMMA_T, GAMMA_CTRL, YGAMMA_ENABLE, cfg->enable);

	if (!cfg->enable)
		return;

	prog_ctrl.raw = ISP_RD_REG(gamma, REG_YGAMMA_T, GAMMA_PROG_CTRL);
#ifndef  __SOC_PHOBOS__
	prog_ctrl.bits.GAMMA_WSEL		= 0;
#else
	prog_ctrl.bits.GAMMA_WSEL		= prog_ctrl.bits.GAMMA_WSEL ^ 1;
#endif
	prog_ctrl.bits.GAMMA_PROG_EN		= 1;
	prog_ctrl.bits.GAMMA_PROG_1TO3_EN	= 1;
	ISP_WR_REG(gamma, REG_YGAMMA_T, GAMMA_PROG_CTRL, prog_ctrl.raw);

	ISP_WR_BITS(gamma, REG_YGAMMA_T, GAMMA_PROG_ST_ADDR, GAMMA_ST_ADDR, 0);
	ISP_WR_BITS(gamma, REG_YGAMMA_T, GAMMA_PROG_ST_ADDR, GAMMA_ST_W, 1);
	ISP_WR_REG(gamma, REG_YGAMMA_T, GAMMA_PROG_MAX, cfg->max);

	for (i = 0; i < 256; i += 2) {
		reg_data.raw = 0;
		reg_data.bits.GAMMA_DATA_E = cfg->lut[i];
		reg_data.bits.GAMMA_DATA_O = cfg->lut[i + 1];
		ISP_WR_REG(gamma, REG_YGAMMA_T, GAMMA_PROG_DATA, reg_data.raw);
		ISP_WR_BITS(gamma, REG_YGAMMA_T, GAMMA_PROG_CTRL, GAMMA_W, 1);
	}

	ISP_WR_BITS(gamma, REG_YGAMMA_T, GAMMA_PROG_CTRL, GAMMA_RSEL, prog_ctrl.bits.GAMMA_WSEL);
	ISP_WR_BITS(gamma, REG_YGAMMA_T, GAMMA_PROG_CTRL, GAMMA_PROG_EN, 0);
}

void ispblk_gamma_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_gamma_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t gamma = ctx->phys_regs[ISP_BLK_ID_RGBGAMMA];
	int16_t i;

	union REG_ISP_GAMMA_PROG_DATA reg_data;
	union REG_ISP_GAMMA_PROG_CTRL prog_ctrl;

	if (!cfg->update)
		return;

	ISP_WR_BITS(gamma, REG_ISP_GAMMA_T, GAMMA_CTRL, GAMMA_ENABLE, cfg->enable);

	if (!cfg->enable)
		return;

	prog_ctrl.raw = ISP_RD_REG(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_CTRL);
#ifndef  __SOC_PHOBOS__
	prog_ctrl.bits.GAMMA_WSEL		= 0;
#else
	prog_ctrl.bits.GAMMA_WSEL		= prog_ctrl.bits.GAMMA_WSEL ^ 1;
#endif
	prog_ctrl.bits.GAMMA_PROG_EN		= 1;
	prog_ctrl.bits.GAMMA_PROG_1TO3_EN	= 1;
	ISP_WR_REG(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_CTRL, prog_ctrl.raw);

	ISP_WR_BITS(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_ST_ADDR, GAMMA_ST_ADDR, 0);
	ISP_WR_BITS(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_ST_ADDR, GAMMA_ST_W, 1);
	ISP_WR_REG(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_MAX, cfg->max);

	for (i = 0; i < 256; i += 2) {
		reg_data.raw = 0;
		reg_data.bits.GAMMA_DATA_E = cfg->lut[i];
		reg_data.bits.GAMMA_DATA_O = cfg->lut[i + 1];
		reg_data.bits.GAMMA_W = 1;
		ISP_WR_REG(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_DATA, reg_data.raw);
	}

	ISP_WR_BITS(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_CTRL, GAMMA_RSEL, prog_ctrl.bits.GAMMA_WSEL);
	ISP_WR_BITS(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_CTRL, GAMMA_PROG_EN, 0);
}

void ispblk_demosiac_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_demosiac_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t cfa = ctx->phys_regs[ISP_BLK_ID_CFA];
	uintptr_t rgbcac = ctx->phys_regs[ISP_BLK_ID_RGBCAC];
	union REG_ISP_CFA_00 reg_00;
	union REG_ISP_CFA_04 reg_04;
	union REG_ISP_CFA_110 reg_110;

	if (!cfg->update)
		return;

	if (ISP_RD_BITS(rgbcac, REG_ISP_RGBCAC_T, RGBCAC_CTRL, RGBCAC_ENABLE) && !cfg->cfa_enable) {
		vi_pr(VI_WARN, "[WARN] not support cfa disable && rgbcac enable\n");
		return;
	}

	reg_00.raw = ISP_RD_REG(cfa, REG_ISP_CFA_T, REG_00);
	reg_00.bits.CFA_ENABLE			= cfg->cfa_enable;
	reg_00.bits.CFA_FORCE_DIR_ENABLE	= cfg->cfa_force_dir_enable;
	reg_00.bits.CFA_FORCE_DIR_SEL		= cfg->cfa_force_dir_sel;
	reg_00.bits.CFA_YMOIRE_ENABLE		= cfg->cfa_ymoire_enable;
	ISP_WR_REG(cfa, REG_ISP_CFA_T, REG_00, reg_00.raw);

	if (!cfg->cfa_enable)
		return;

	reg_04.raw = ISP_RD_REG(cfa, REG_ISP_CFA_T, REG_04);
	reg_04.bits.CFA_OUT_SEL		= cfg->cfa_out_sel;
	reg_04.bits.CFA_EDGEE_THD2	= cfg->cfa_edgee_thd2;
	ISP_WR_REG(cfa, REG_ISP_CFA_T, REG_04, reg_04.raw);

	ISP_WR_BITS(cfa, REG_ISP_CFA_T, REG_20, CFA_RBSIG_LUMA_THD, cfg->cfa_rbsig_luma_thd);

	reg_110.raw = ISP_RD_REG(cfa, REG_ISP_CFA_T, REG_110);
	reg_110.bits.CFA_YMOIRE_LPF_W	= cfg->cfa_ymoire_lpf_w;
	reg_110.bits.CFA_YMOIRE_DC_W	= cfg->cfa_ymoire_dc_w;
	ISP_WR_REG(cfa, REG_ISP_CFA_T, REG_110, reg_110.raw);

	ISP_WR_REG_LOOP_SHFT(cfa, REG_ISP_CFA_T, REG_30, 32, 4, cfg->cfa_ghp_lut, 8);

	ISP_WR_REGS_BURST(cfa, REG_ISP_CFA_T, REG_0C, cfg->demosiac_cfg, cfg->demosiac_cfg.REG_0C);

	ISP_WR_REGS_BURST(cfa, REG_ISP_CFA_T, REG_120, cfg->demosiac_1_cfg, cfg->demosiac_1_cfg.REG_120);

	ISP_WR_REGS_BURST(cfa, REG_ISP_CFA_T, REG_90, cfg->demosiac_2_cfg, cfg->demosiac_2_cfg.REG_90);
}

void ispblk_lsc_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_lsc_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t lsc = ctx->phys_regs[ISP_BLK_ID_LSC];
	union REG_ISP_LSC_ENABLE lsc_enable;
	union REG_ISP_LSC_INTERPOLATION inter_p;
	union REG_ISP_LSC_BLD lsc_bld;

	if (!cfg->update)
		return;

	ISP_WR_BITS(lsc, REG_ISP_LSC_T, DMI_ENABLE, DMI_ENABLE, cfg->enable);
	ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_ENABLE, LSC_ENABLE, cfg->enable);

	if (!cfg->enable)
		return;

	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_STRENGTH, cfg->strength);
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_GAIN_BASE, cfg->gain_base);
	ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_DUMMY, LSC_DEBUG, cfg->debug);

	inter_p.raw = ISP_RD_REG(lsc, REG_ISP_LSC_T, INTERPOLATION);
	inter_p.bits.LSC_BOUNDARY_INTERPOLATION_LF_RANGE = cfg->boundary_interpolation_lf_range;
	inter_p.bits.LSC_BOUNDARY_INTERPOLATION_UP_RANGE = cfg->boundary_interpolation_up_range;
	inter_p.bits.LSC_BOUNDARY_INTERPOLATION_RT_RANGE = cfg->boundary_interpolation_rt_range;
	inter_p.bits.LSC_BOUNDARY_INTERPOLATION_DN_RANGE = cfg->boundary_interpolation_dn_range;
	ISP_WR_REG(lsc, REG_ISP_LSC_T, INTERPOLATION, inter_p.raw);

	lsc_enable.raw = ISP_RD_REG(lsc, REG_ISP_LSC_T, LSC_ENABLE);
	lsc_enable.bits.LSC_GAIN_3P9_0_4P8_1 = cfg->gain_3p9_0_4p8_1;
	lsc_enable.bits.LSC_GAIN_BICUBIC_0_BILINEAR_1 = cfg->gain_bicubic_0_bilinear_1;
	lsc_enable.bits.LSC_BOUNDARY_INTERPOLATION_MODE = cfg->boundary_interpolation_mode;
	lsc_enable.bits.LSC_RENORMALIZE_ENABLE = cfg->renormalize_enable;
	lsc_enable.bits.LSC_HDR_ENABLE = ctx->isp_pipe_cfg[raw_num].is_hdr_on ? 1 : 0;
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_ENABLE, lsc_enable.raw);

	lsc_bld.raw = ISP_RD_REG(lsc, REG_ISP_LSC_T, LSC_BLD);
	lsc_bld.bits.LSC_BLDRATIO_ENABLE = cfg->bldratio_enable;
	lsc_bld.bits.LSC_BLDRATIO = cfg->bldratio;
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_BLD, lsc_bld.raw);

	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_INTP_GAIN_MAX, cfg->intp_gain_max);
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_INTP_GAIN_MIN, cfg->intp_gain_min);
}

void ispblk_bnr_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_bnr_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t bnr = ctx->phys_regs[ISP_BLK_ID_BNR];
	uint16_t i = 0;

	if (!cfg->update)
		return;

	if (cfg->enable) {
		if ((cfg->out_sel == 8) || ((cfg->out_sel >= 11) && (cfg->out_sel <= 15)))
			ISP_WO_BITS(bnr, REG_ISP_BNR_T, OUT_SEL, BNR_OUT_SEL, cfg->out_sel);
		else
			vi_pr(VI_ERR, "[ERR] BNR out_sel(%d) should be 8 and 11~15\n", cfg->out_sel);

		ISP_WR_REG(bnr, REG_ISP_BNR_T, WEIGHT_INTRA_0, cfg->weight_intra_0);
		ISP_WR_REG(bnr, REG_ISP_BNR_T, WEIGHT_INTRA_1, cfg->weight_intra_1);
		ISP_WR_REG(bnr, REG_ISP_BNR_T, WEIGHT_INTRA_2, cfg->weight_intra_2);
		ISP_WR_REG(bnr, REG_ISP_BNR_T, WEIGHT_NORM_1, cfg->weight_norm_1);
		ISP_WR_REG(bnr, REG_ISP_BNR_T, WEIGHT_NORM_2, cfg->weight_norm_2);
		ISP_WR_REG(bnr, REG_ISP_BNR_T, RES_K_SMOOTH, cfg->k_smooth);
		ISP_WR_REG(bnr, REG_ISP_BNR_T, RES_K_TEXTURE, cfg->k_texture);

		ISP_WR_REGS_BURST(bnr, REG_ISP_BNR_T, NS_LUMA_TH_R,
					cfg->bnr_1_cfg, cfg->bnr_1_cfg.NS_LUMA_TH_R);

		ISP_WR_REGS_BURST(bnr, REG_ISP_BNR_T, VAR_TH,
					cfg->bnr_2_cfg, cfg->bnr_2_cfg.VAR_TH);

		ISP_WO_BITS(bnr, REG_ISP_BNR_T, INDEX_CLR, BNR_INDEX_CLR, 1);
		for (i = 0; i < 8; i++)
			ISP_WR_REG(bnr, REG_ISP_BNR_T, INTENSITY_SEL, cfg->intensity_sel[i]);

		for (i = 0; i < 256; i++)
			ISP_WR_REG(bnr, REG_ISP_BNR_T, WEIGHT_LUT, cfg->weight_lut[i]);
	} else {
		ISP_WO_BITS(bnr, REG_ISP_BNR_T, OUT_SEL, BNR_OUT_SEL, 1);
	}

	ISP_WO_BITS(bnr, REG_ISP_BNR_T, SHADOW_RD_SEL, SHADOW_RD_SEL, 1);
}

void ispblk_clut_partial_update(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_clut_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t clut = ctx->phys_regs[ISP_BLK_ID_CLUT];
	union REG_ISP_CLUT_CTRL      ctrl;
	union REG_ISP_CLUT_PROG_DATA prog_data;
	u32 i = 0;

	if ((_is_all_online(ctx) || (_is_fe_be_online(ctx) && ctx->is_slice_buf_on)) && cfg->update_length >= 256)
		cfg->update_length = 256;
	else if (cfg->update_length >= 1024)
		cfg->update_length = 1024;

	ctrl.raw = ISP_RD_REG(clut, REG_ISP_CLUT_T, CLUT_CTRL);
	ctrl.bits.PROG_EN = 1;
	ISP_WR_REG(clut, REG_ISP_CLUT_T, CLUT_CTRL, ctrl.raw);

	for (; i < cfg->update_length; i++) {
		ISP_WR_REG(clut, REG_ISP_CLUT_T, CLUT_PROG_ADDR, cfg->lut[i][0]);

		prog_data.raw			= 0;
		prog_data.bits.SRAM_WDATA	= cfg->lut[i][1];
		prog_data.bits.SRAM_WR		= 1;
		ISP_WR_REG(clut, REG_ISP_CLUT_T, CLUT_PROG_DATA, prog_data.raw);
	}

	ctrl.bits.CLUT_ENABLE = cfg->enable;
	ctrl.bits.PROG_EN = 0;
	ISP_WR_REG(clut, REG_ISP_CLUT_T, CLUT_CTRL, ctrl.raw);
}

void ispblk_clut_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_clut_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	if (!cfg->update)
		return;

	if (cfg->is_update_partial) { //partail update table
		ispblk_clut_partial_update(ctx, cfg, raw_num);
	} else if (!(_is_all_online(ctx) || (_is_fe_be_online(ctx) && ctx->is_slice_buf_on))) {
		ispblk_clut_config(ctx, cfg->enable, cfg->r_lut, cfg->g_lut, cfg->b_lut);
	}
}

void ispblk_drc_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_drc_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_HDRLTM];
	uintptr_t rawtop = ctx->phys_regs[ISP_BLK_ID_RAWTOP];
	uintptr_t lmap0 = ctx->phys_regs[ISP_BLK_ID_LMAP0];
	uintptr_t lmap1 = ctx->phys_regs[ISP_BLK_ID_LMAP1];
	union REG_LTM_H00 reg_00;
	union REG_LTM_H08 reg_08;
	union REG_LTM_H0C reg_0C;
	union REG_ISP_LMAP_LMP_0 lmp_0;

	if (!cfg->update)
		return;

#if (defined( __SOC_MARS__) && !defined(PORTING_TEST))
	if (!(ctx->isp_pipe_cfg[raw_num].is_hdr_on)
	    && (_is_fe_be_online(ctx) && ctx->is_slice_buf_on))
		isp_runtime_hdr_patgen(ctx, raw_num, cfg->hdr_pattern);
#endif

	reg_00.raw = ISP_RD_REG(ba, REG_LTM_T, REG_H00);
	reg_00.bits.LTM_ENABLE			= cfg->ltm_enable;
	reg_00.bits.LTM_DARK_ENH_ENABLE		= cfg->dark_enh_en;
	reg_00.bits.LTM_BRIT_ENH_ENABLE		= cfg->brit_enh_en;
	reg_00.bits.LTM_DBG_MODE		= cfg->dbg_mode;
	reg_00.bits.FORCE_DMA_DISABLE		= ((!cfg->dark_enh_en) | (!cfg->brit_enh_en << 1));
	reg_00.bits.DARK_TONE_WGT_REFINE_ENABLE	= cfg->dark_tone_wgt_refine_en;
	reg_00.bits.BRIT_TONE_WGT_REFINE_ENABLE	= cfg->brit_tone_wgt_refine_en;
	ISP_WR_REG(ba, REG_LTM_T, REG_H00, reg_00.raw);

	if (!cfg->ltm_enable)
		return;

	lmp_0.raw = ISP_RD_REG(lmap0, REG_ISP_LMAP_T, LMP_0);
	lmp_0.bits.LMAP_ENABLE	= cfg->lmap_enable;
	lmp_0.bits.LMAP_Y_MODE	= cfg->lmap_y_mode;
	lmp_0.bits.LMAP_THD_L	= cfg->lmap_thd_l;
	lmp_0.bits.LMAP_THD_H	= cfg->lmap_thd_h;
	ISP_WR_REG(lmap0, REG_ISP_LMAP_T, LMP_0, lmp_0.raw);
	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
		ISP_WR_REG(lmap1, REG_ISP_LMAP_T, LMP_0, lmp_0.raw);
	else {
		lmp_0.bits.LMAP_ENABLE = 0;
		ISP_WR_REG(lmap1, REG_ISP_LMAP_T, LMP_0, lmp_0.raw);
	}

	reg_08.raw = ISP_RD_REG(ba, REG_LTM_T, REG_H08);
	reg_08.bits.LTM_BE_STRTH_DSHFT	= cfg->be_strth_dshft;
	reg_08.bits.LTM_BE_STRTH_GAIN	= cfg->be_strth_gain;
	ISP_WR_REG(ba, REG_LTM_T, REG_H08, reg_08.raw);

	reg_0C.raw = ISP_RD_REG(ba, REG_LTM_T, REG_H0C);
	reg_0C.bits.LTM_DE_STRTH_DSHFT	= cfg->de_strth_dshft;
	reg_0C.bits.LTM_DE_STRTH_GAIN	= cfg->de_strth_gain;
	ISP_WR_REG(ba, REG_LTM_T, REG_H0C, reg_0C.raw);

	//Update ltm w_h_bit and lmap w_h_bit in rawtop
	if ((g_lmp_cfg[raw_num].post_w_bit != cfg->lmap_w_bit ||
		g_lmp_cfg[raw_num].post_h_bit != cfg->lmap_h_bit) &&
		((cfg->lmap_w_bit > 2) && (cfg->lmap_h_bit > 2))) {
		union REG_LTM_H8C reg_8c;
		union REG_RAW_TOP_LE_LMAP_GRID_NUMBER	le_lmap_size;
		union REG_RAW_TOP_SE_LMAP_GRID_NUMBER	se_lmap_size;

		g_lmp_cfg[raw_num].post_w_bit = cfg->lmap_w_bit;
		g_lmp_cfg[raw_num].post_h_bit = cfg->lmap_h_bit;

		reg_8c.raw = ISP_RD_REG(ba, REG_LTM_T, REG_H8C);
		reg_8c.bits.LMAP_W_BIT = g_lmp_cfg[raw_num].post_w_bit;
		reg_8c.bits.LMAP_H_BIT = g_lmp_cfg[raw_num].post_h_bit;
		ISP_WR_REG(ba, REG_LTM_T, REG_H8C, reg_8c.raw);

		le_lmap_size.raw = ISP_RD_REG(rawtop, REG_RAW_TOP_T, LE_LMAP_GRID_NUMBER);
		le_lmap_size.bits.LE_LMP_H_GRID_SIZE = g_lmp_cfg[raw_num].post_w_bit;
		le_lmap_size.bits.LE_LMP_V_GRID_SIZE = g_lmp_cfg[raw_num].post_h_bit;
		ISP_WR_REG(rawtop, REG_RAW_TOP_T, LE_LMAP_GRID_NUMBER, le_lmap_size.raw);

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			se_lmap_size.raw = ISP_RD_REG(rawtop, REG_RAW_TOP_T, SE_LMAP_GRID_NUMBER);
			se_lmap_size.bits.SE_LMP_H_GRID_SIZE = g_lmp_cfg[raw_num].post_w_bit;
			se_lmap_size.bits.SE_LMP_V_GRID_SIZE = g_lmp_cfg[raw_num].post_h_bit;
			ISP_WR_REG(rawtop, REG_RAW_TOP_T, SE_LMAP_GRID_NUMBER, se_lmap_size.raw);
		}
	}

	ispblk_ltm_g_lut(ctx, 0, cfg->global_lut);
	ispblk_ltm_b_lut(ctx, 0, cfg->brit_lut);
	ispblk_ltm_d_lut(ctx, 0, cfg->dark_lut);

	ISP_WR_REGS_BURST(ba, REG_LTM_T, REG_H90, cfg->drc_1_cfg, cfg->drc_1_cfg.REG_H90);
	ISP_WR_REGS_BURST(ba, REG_LTM_T, REG_H14, cfg->drc_2_cfg, cfg->drc_2_cfg.REG_H14);
	ISP_WR_REGS_BURST(ba, REG_LTM_T, REG_H64, cfg->drc_3_cfg, cfg->drc_3_cfg.REG_H64);
}

void ispblk_ynr_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_ynr_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ynr = ctx->phys_regs[ISP_BLK_ID_YNR];
	uint8_t i = 0;

	if (!cfg->update)
		return;

	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_INTRA_0, cfg->weight_intra_0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_INTRA_1, cfg->weight_intra_1);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_INTRA_2, cfg->weight_intra_2);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_NORM_1, cfg->weight_norm_1);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_NORM_2, cfg->weight_norm_2);

	if (cfg->enable) {
		if ((cfg->out_sel == 8) || ((cfg->out_sel >= 11) && (cfg->out_sel <= 15)))
			ISP_WR_REG(ynr, REG_ISP_YNR_T, OUT_SEL, cfg->out_sel);
		else
			vi_pr(VI_ERR, "[ERR] YNR out_sel(%d) should be 8 and 11~15\n", cfg->out_sel);

		ISP_WR_REG(ynr, REG_ISP_YNR_T, MOTION_NS_CLIP_MAX, cfg->motion_ns_clip_max);
		ISP_WR_REG(ynr, REG_ISP_YNR_T, RES_MAX, cfg->res_max);
		ISP_WR_REG(ynr, REG_ISP_YNR_T, RES_MOTION_MAX, cfg->res_motion_max);

		ISP_WR_REGS_BURST(ynr, REG_ISP_YNR_T, NS0_LUMA_TH_00,
					cfg->ynr_1_cfg, cfg->ynr_1_cfg.NS0_LUMA_TH_00);

		ISP_WR_REGS_BURST(ynr, REG_ISP_YNR_T, MOTION_LUT_00,
					cfg->ynr_2_cfg, cfg->ynr_2_cfg.MOTION_LUT_00);

		ISP_WR_REGS_BURST(ynr, REG_ISP_YNR_T, ALPHA_GAIN,
					cfg->ynr_3_cfg, cfg->ynr_3_cfg.ALPHA_GAIN);

		ISP_WR_REGS_BURST(ynr, REG_ISP_YNR_T, RES_MOT_LUT_00,
					cfg->ynr_4_cfg, cfg->ynr_4_cfg.RES_MOT_LUT_00);

		ISP_WO_BITS(ynr, REG_ISP_YNR_T, INDEX_CLR, YNR_INDEX_CLR, 1);
		for (i = 0; i < 64; i++)
			ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_LUT, cfg->weight_lut_h[i]);

	} else {
		ISP_WR_REG(ynr, REG_ISP_YNR_T, OUT_SEL, 1);
	}
}

void ispblk_cnr_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_cnr_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t cnr = ctx->phys_regs[ISP_BLK_ID_CNR];

	union REG_ISP_CNR_ENABLE reg_enable;
	union REG_ISP_CNR_STRENGTH_MODE reg_strength_mode;
	union REG_ISP_CNR_WEIGHT_LUT_INTER_CNR_00 reg_weight_lut_00;
	union REG_ISP_CNR_WEIGHT_LUT_INTER_CNR_04 reg_weight_lut_04;
	union REG_ISP_CNR_WEIGHT_LUT_INTER_CNR_08 reg_weight_lut_08;
	union REG_ISP_CNR_WEIGHT_LUT_INTER_CNR_12 reg_weight_lut_12;
	union REG_ISP_CNR_CORING_MOTION_LUT_0 reg_coring_motion_lut_00;
	union REG_ISP_CNR_CORING_MOTION_LUT_4 reg_coring_motion_lut_04;
	union REG_ISP_CNR_CORING_MOTION_LUT_8 reg_coring_motion_lut_08;
	union REG_ISP_CNR_CORING_MOTION_LUT_12 reg_coring_motion_lut_12;
	union REG_ISP_CNR_MOTION_LUT_0 reg_motion_lut_00;
	union REG_ISP_CNR_MOTION_LUT_4 reg_motion_lut_04;
	union REG_ISP_CNR_MOTION_LUT_8 reg_motion_lut_08;
	union REG_ISP_CNR_MOTION_LUT_12 reg_motion_lut_12;

	if (!cfg->update)
		return;

	ISP_WR_BITS(cnr, REG_ISP_CNR_T, CNR_ENABLE, CNR_ENABLE, cfg->enable);

	if (!cfg->enable)
		return;

	reg_enable.raw = ISP_RD_REG(cnr, REG_ISP_CNR_T, CNR_ENABLE);
	reg_enable.bits.CNR_ENABLE = cfg->enable;
	reg_enable.bits.CNR_DIFF_SHIFT_VAL = cfg->diff_shift_val;
	reg_enable.bits.CNR_RATIO = cfg->ratio;
	ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_ENABLE, reg_enable.raw);

	reg_strength_mode.raw = ISP_RD_REG(cnr, REG_ISP_CNR_T, CNR_STRENGTH_MODE);
	reg_strength_mode.bits.CNR_STRENGTH_MODE = cfg->strength_mode;
	reg_strength_mode.bits.CNR_FUSION_INTENSITY_WEIGHT = cfg->fusion_intensity_weight;
	reg_strength_mode.bits.CNR_FLAG_NEIGHBOR_MAX_WEIGHT = cfg->flag_neighbor_max_weight;
	ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_STRENGTH_MODE, reg_strength_mode.raw);

	ISP_WR_BITS(cnr, REG_ISP_CNR_T, CNR_PURPLE_TH, CNR_DIFF_GAIN, cfg->diff_gain);
	ISP_WR_BITS(cnr, REG_ISP_CNR_T, CNR_PURPLE_TH, CNR_MOTION_ENABLE, cfg->motion_enable);

	reg_weight_lut_00.raw = (u32)((cfg->weight_lut_inter[0] & 0x1F) |
				((cfg->weight_lut_inter[1] & 0x1F) << 8) |
				((cfg->weight_lut_inter[2] & 0x1F) << 16) |
				((cfg->weight_lut_inter[3] & 0x1F) << 24));
	reg_weight_lut_04.raw = (u32)((cfg->weight_lut_inter[4] & 0x1F) |
				((cfg->weight_lut_inter[5] & 0x1F) << 8) |
				((cfg->weight_lut_inter[6] & 0x1F) << 16) |
				((cfg->weight_lut_inter[7] & 0x1F) << 24));
	reg_weight_lut_08.raw = (u32)((cfg->weight_lut_inter[8] & 0x1F) |
				((cfg->weight_lut_inter[9] & 0x1F) << 8) |
				((cfg->weight_lut_inter[10] & 0x1F) << 16) |
				((cfg->weight_lut_inter[11] & 0x1F) << 24));
	reg_weight_lut_12.raw = (u32)((cfg->weight_lut_inter[12] & 0x1F) |
				((cfg->weight_lut_inter[13] & 0x1F) << 8) |
				((cfg->weight_lut_inter[14] & 0x1F) << 16) |
				((cfg->weight_lut_inter[15] & 0x1F) << 24));
	ISP_WR_REG(cnr, REG_ISP_CNR_T, WEIGHT_LUT_INTER_CNR_00, reg_weight_lut_00.raw);
	ISP_WR_REG(cnr, REG_ISP_CNR_T, WEIGHT_LUT_INTER_CNR_04, reg_weight_lut_04.raw);
	ISP_WR_REG(cnr, REG_ISP_CNR_T, WEIGHT_LUT_INTER_CNR_08, reg_weight_lut_08.raw);
	ISP_WR_REG(cnr, REG_ISP_CNR_T, WEIGHT_LUT_INTER_CNR_12, reg_weight_lut_12.raw);

	reg_coring_motion_lut_00.raw = (u32)((cfg->coring_motion_lut[0] & 0xFF) |
				((cfg->coring_motion_lut[1] & 0xFF) << 8) |
				((cfg->coring_motion_lut[2] & 0xFF) << 16) |
				((cfg->coring_motion_lut[3] & 0xFF) << 24));
	reg_coring_motion_lut_04.raw = (u32)((cfg->coring_motion_lut[4] & 0xFF) |
				((cfg->coring_motion_lut[5] & 0xFF) << 8) |
				((cfg->coring_motion_lut[6] & 0xFF) << 16) |
				((cfg->coring_motion_lut[7] & 0xFF) << 24));
	reg_coring_motion_lut_08.raw = (u32)((cfg->coring_motion_lut[8] & 0xFF) |
				((cfg->coring_motion_lut[9] & 0xFF) << 8) |
				((cfg->coring_motion_lut[10] & 0xFF) << 16) |
				((cfg->coring_motion_lut[11] & 0xFF) << 24));
	reg_coring_motion_lut_12.raw = (u32)((cfg->coring_motion_lut[12] & 0xFF) |
				((cfg->coring_motion_lut[13] & 0xFF) << 8) |
				((cfg->coring_motion_lut[14] & 0xFF) << 16) |
				((cfg->coring_motion_lut[15] & 0xFF) << 24));
	ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_CORING_MOTION_LUT_0, reg_coring_motion_lut_00.raw);
	ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_CORING_MOTION_LUT_4, reg_coring_motion_lut_04.raw);
	ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_CORING_MOTION_LUT_8, reg_coring_motion_lut_08.raw);
	ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_CORING_MOTION_LUT_12, reg_coring_motion_lut_12.raw);

	reg_motion_lut_00.raw = (u32)((cfg->motion_lut[0] & 0xFF) |
				((cfg->motion_lut[1] & 0xFF) << 8) |
				((cfg->motion_lut[2] & 0xFF) << 16) |
				((cfg->motion_lut[3] & 0xFF) << 24));
	reg_motion_lut_04.raw = (u32)((cfg->motion_lut[4] & 0xFF) |
				((cfg->motion_lut[5] & 0xFF) << 8) |
				((cfg->motion_lut[6] & 0xFF) << 16) |
				((cfg->motion_lut[7] & 0xFF) << 24));
	reg_motion_lut_08.raw = (u32)((cfg->motion_lut[8] & 0xFF) |
				((cfg->motion_lut[9] & 0xFF) << 8) |
				((cfg->motion_lut[10] & 0xFF) << 16) |
				((cfg->motion_lut[11] & 0xFF) << 24));
	reg_motion_lut_12.raw = (u32)((cfg->motion_lut[12] & 0xFF) |
				((cfg->motion_lut[13] & 0xFF) << 8) |
				((cfg->motion_lut[14] & 0xFF) << 16) |
				((cfg->motion_lut[15] & 0xFF) << 24));
	ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_MOTION_LUT_0, reg_motion_lut_00.raw);
	ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_MOTION_LUT_4, reg_motion_lut_04.raw);
	ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_MOTION_LUT_8, reg_motion_lut_08.raw);
	ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_MOTION_LUT_12, reg_motion_lut_12.raw);
}

void ispblk_tnr_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_tnr_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t manr = ctx->phys_regs[ISP_BLK_ID_MMAP];
	uintptr_t tnr = ctx->phys_regs[ISP_BLK_ID_TNR];

	//union REG_ISP_MMAP_00 mm_00;
	union REG_ISP_MMAP_04 mm_04;
	union REG_ISP_MMAP_08 mm_08;
	union REG_ISP_MMAP_44 mm_44;
	union REG_ISP_444_422_8 reg_8;
	union REG_ISP_444_422_9 reg_9;

	if (!ctx->is_3dnr_on || !cfg->update)
		return;
#if 0
	mm_00.raw = ISP_RD_REG(manr, REG_ISP_MMAP_T, REG_00);
	if (!cfg->manr_enable) {
		mm_00.bits.MMAP_0_ENABLE = 0;
		mm_00.bits.MMAP_1_ENABLE = 0;
		mm_00.bits.BYPASS = 1;
	} else {
		mm_00.bits.MMAP_0_ENABLE = 1;
		mm_00.bits.MMAP_1_ENABLE = (ctx->isp_pipe_cfg[raw_num].is_hdr_on) ? 1 : 0;
		mm_00.bits.BYPASS = (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw) ? 1 : 0;
	}
	ISP_WR_REG(manr, REG_ISP_MMAP_T, REG_00, mm_00.raw);
#endif
	mm_04.raw = ISP_RD_REG(manr, REG_ISP_MMAP_T, REG_04);
	mm_04.bits.MMAP_0_LPF_00 = cfg->lpf[0][0];
	mm_04.bits.MMAP_0_LPF_01 = cfg->lpf[0][1];
	mm_04.bits.MMAP_0_LPF_02 = cfg->lpf[0][2];
	mm_04.bits.MMAP_0_LPF_10 = cfg->lpf[1][0];
	mm_04.bits.MMAP_0_LPF_11 = cfg->lpf[1][1];
	mm_04.bits.MMAP_0_LPF_12 = cfg->lpf[1][2];
	mm_04.bits.MMAP_0_LPF_20 = cfg->lpf[2][0];
	mm_04.bits.MMAP_0_LPF_21 = cfg->lpf[2][1];
	mm_04.bits.MMAP_0_LPF_22 = cfg->lpf[2][2];
	ISP_WR_REG(manr, REG_ISP_MMAP_T, REG_04, mm_04.raw);

	mm_08.raw = ISP_RD_REG(manr, REG_ISP_MMAP_T, REG_08);
	mm_08.bits.MMAP_0_MAP_GAIN = cfg->map_gain;
	mm_08.bits.MMAP_0_MAP_THD_L = cfg->map_thd_l;
	mm_08.bits.MMAP_0_MAP_THD_H = cfg->map_thd_h;
	ISP_WR_REG(manr, REG_ISP_MMAP_T, REG_08, mm_08.raw);

	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_1C, MMAP_0_LUMA_ADAPT_LUT_SLOPE_2,
		cfg->luma_adapt_lut_slope_2);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_F8, HISTORY_SEL_0, cfg->history_sel_0);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_F8, HISTORY_SEL_1, cfg->history_sel_1);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_F8, HISTORY_SEL_3, cfg->history_sel_3);

	if (_is_all_online(ctx) && cfg->rgbmap_w_bit > 3) {
		vi_pr(VI_WARN, "RGBMAP_w_bit(%d) need <= 3 under on the fly mode\n", cfg->rgbmap_w_bit);
		cfg->rgbmap_w_bit = cfg->rgbmap_h_bit = 3;
	} else if (cfg->rgbmap_w_bit > 5) {
		vi_pr(VI_WARN, "RGBMAP_w_bit(%d) need <= 5\n", cfg->rgbmap_w_bit);
		cfg->rgbmap_w_bit = cfg->rgbmap_h_bit = 5;
	} else if (cfg->rgbmap_w_bit < 3) {
		vi_pr(VI_WARN, "RGBMAP_w_bit(%d) need >= 3\n", cfg->rgbmap_w_bit);
		cfg->rgbmap_w_bit = cfg->rgbmap_h_bit = 3;
	}

	if (ctx->is_slice_buf_on && ctx->is_rgbmap_sbm_on) {
		uint32_t grid_size = (1 << cfg->rgbmap_h_bit);
		uint32_t w = ctx->isp_pipe_cfg[raw_num].crop.w;
		uint32_t seglen = ((w + grid_size - 1) / grid_size) * 6;
		uint32_t stride = ((((w + grid_size - 1) / grid_size) * 6 + 15) / 16) * 16;

		if (seglen != stride) {
			vi_pr(VI_ERR, "rgbmap is not correct in this scenario, must use rgbmap frame mode!!!\n");
		}
	}

	reg_8.raw = ISP_RD_REG(tnr, REG_ISP_444_422_T, REG_8);
	//reg_8.bits.FORCE_DMA_DISABLE = (cfg->manr_enable) ? 0 : 0x3f;
	reg_8.bits.UV_ROUNDING_TYPE_SEL = cfg->uv_rounding_type_sel;
	reg_8.bits.TDNR_PIXEL_LP = cfg->tdnr_pixel_lp;
	reg_8.bits.TDNR_DEBUG_SEL = cfg->tdnr_debug_sel;
	ISP_WR_REG(tnr, REG_ISP_444_422_T, REG_8, reg_8.raw);

	ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_5, TDNR_ENABLE, cfg->manr_enable);
	if (!cfg->manr_enable)
		return;

	reg_9.raw = ISP_RD_REG(tnr, REG_ISP_444_422_T, REG_9);
	reg_9.bits.AVG_MODE_WRITE  = cfg->avg_mode_write;
	reg_9.bits.DROP_MODE_WRITE = cfg->drop_mode_write;
	ISP_WR_REG(tnr, REG_ISP_444_422_T, REG_9, reg_9.raw);

	mm_44.raw = 0;
	mm_44.bits.MMAP_MED_ENABLE	= cfg->med_enable;
	mm_44.bits.MMAP_MED_WGT		= cfg->med_wgt;
	ISP_WR_REG(manr, REG_ISP_MMAP_T, REG_44, mm_44.raw);

	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_3C, MOTION_YV_LS_MODE, cfg->mtluma_mode);
	ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_5, REG_3DNR_COMP_GAIN_ENABLE,
		cfg->tdnr_comp_gain_enable);
	ISP_WR_REG(tnr, REG_ISP_444_422_T, REG_80, cfg->tdnr_ee_comp_gain);

	ISP_WR_REGS_BURST(manr, REG_ISP_MMAP_T, REG_0C, cfg->tnr_cfg, cfg->tnr_cfg.REG_0C);
	ISP_WR_REGS_BURST(manr, REG_ISP_MMAP_T, REG_20, cfg->tnr_5_cfg, cfg->tnr_5_cfg.REG_20);
	ISP_WR_REGS_BURST(manr, REG_ISP_MMAP_T, REG_4C, cfg->tnr_1_cfg, cfg->tnr_1_cfg.REG_4C);
	ISP_WR_REGS_BURST(manr, REG_ISP_MMAP_T, REG_70, cfg->tnr_2_cfg, cfg->tnr_2_cfg.REG_70);
	ISP_WR_REGS_BURST(manr, REG_ISP_MMAP_T, REG_A0, cfg->tnr_3_cfg, cfg->tnr_3_cfg.REG_A0);
	ISP_WR_REGS_BURST(tnr, REG_ISP_444_422_T, REG_13, cfg->tnr_4_cfg, cfg->tnr_4_cfg.REG_13);
	ISP_WR_REGS_BURST(tnr, REG_ISP_444_422_T, REG_84, cfg->tnr_6_cfg, cfg->tnr_6_cfg.REG_84);
	ISP_WR_REGS_BURST(manr, REG_ISP_MMAP_T, REG_100, cfg->tnr_7_cfg, cfg->tnr_7_cfg.REG_100);

	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_2C, MMAP_0_MH_WGT, cfg->mh_wgt);

	if (g_w_bit[raw_num] != cfg->rgbmap_w_bit) {
		g_w_bit[raw_num] = cfg->rgbmap_w_bit;
		g_h_bit[raw_num] = cfg->rgbmap_h_bit;

		g_rgbmap_chg_pre[raw_num][0] = true;
		g_rgbmap_chg_pre[raw_num][1] = true;

		if (_is_all_online(ctx) || (_is_fe_be_online(ctx) && ctx->is_slice_buf_on) ||
			ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw) {
			ispblk_tnr_rgbmap_chg(ctx, raw_num, ISP_FE_CH0);
			if (ctx->is_hdr_on)
				ispblk_tnr_rgbmap_chg(ctx, raw_num, ISP_FE_CH1);

			ctx->isp_pipe_cfg[raw_num].rgbmap_i.w_bit = g_w_bit[raw_num];
			ctx->isp_pipe_cfg[raw_num].rgbmap_i.h_bit = g_h_bit[raw_num];
			ispblk_tnr_post_chg(ctx, raw_num);
		}
	}
}

void ispblk_ee_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_ee_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_EE];
	union REG_ISP_EE_00  reg_0;
	union REG_ISP_EE_04  reg_4;
	union REG_ISP_EE_0C  reg_c;
	union REG_ISP_EE_10  reg_10;
	union REG_ISP_EE_1FC reg_1fc;
	uint32_t i, raw;

	if (!cfg->update)
		return;

	reg_0.raw = ISP_RD_REG(ba, REG_ISP_EE_T, REG_00);
	reg_0.bits.EE_ENABLE = cfg->enable;
	reg_0.bits.EE_DEBUG_MODE = cfg->dbg_mode;
	reg_0.bits.EE_TOTAL_CORING = cfg->total_coring;
	reg_0.bits.EE_TOTAL_MOTION_CORING = cfg->total_motion_coring;
	reg_0.bits.EE_TOTAL_GAIN = cfg->total_gain;
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_00, reg_0.raw);

	if (!cfg->enable)
		return;

	reg_4.raw = ISP_RD_REG(ba, REG_ISP_EE_T, REG_04);
	reg_4.bits.EE_TOTAL_OSHTTHRD = cfg->total_oshtthrd;
	reg_4.bits.EE_TOTAL_USHTTHRD = cfg->total_ushtthrd;
	reg_4.bits.EE_PRE_PROC_ENABLE = cfg->pre_proc_enable;
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_04, reg_4.raw);

	reg_c.raw = ISP_RD_REG(ba, REG_ISP_EE_T, REG_0C);
	reg_c.bits.EE_LUMAREF_LPF_EN = cfg->lumaref_lpf_en;
	reg_c.bits.EE_LUMA_CORING_EN = cfg->luma_coring_en;
	reg_c.bits.EE_LUMA_ADPTCTRL_EN = cfg->luma_adptctrl_en;
	reg_c.bits.EE_DELTA_ADPTCTRL_EN = cfg->delta_adptctrl_en;
	reg_c.bits.EE_DELTA_ADPTCTRL_SHIFT = cfg->delta_adptctrl_shift;
	reg_c.bits.EE_CHROMAREF_LPF_EN = cfg->chromaref_lpf_en;
	reg_c.bits.EE_CHROMA_ADPTCTRL_EN = cfg->chroma_adptctrl_en;
	reg_c.bits.EE_MF_CORE_GAIN = cfg->mf_core_gain;
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_0C, reg_c.raw);

	reg_10.raw = ISP_RD_REG(ba, REG_ISP_EE_T, REG_10);
	reg_10.bits.HF_BLEND_WGT = cfg->hf_blend_wgt;
	reg_10.bits.MF_BLEND_WGT = cfg->mf_blend_wgt;
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_10, reg_10.raw);

	reg_1fc.raw = ISP_RD_REG(ba, REG_ISP_EE_T, REG_1FC);
	reg_1fc.bits.EE_SOFT_CLAMP_ENABLE = cfg->soft_clamp_enable;
	reg_1fc.bits.EE_UPPER_BOUND_LEFT_DIFF = cfg->upper_bound_left_diff;
	reg_1fc.bits.EE_LOWER_BOUND_RIGHT_DIFF = cfg->lower_bound_right_diff;
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_1FC, reg_1fc.raw);

	for (i = 0; i < 32; i += 4) {
		raw = cfg->luma_adptctrl_lut[i] + (cfg->luma_adptctrl_lut[i + 1] << 8) +
			(cfg->luma_adptctrl_lut[i + 2] << 16) + (cfg->luma_adptctrl_lut[i + 3] << 24);
		ISP_WR_REG_OFT(ba, REG_ISP_EE_T, REG_130, i, raw);

		raw = cfg->delta_adptctrl_lut[i] + (cfg->delta_adptctrl_lut[i + 1] << 8) +
			(cfg->delta_adptctrl_lut[i + 2] << 16) + (cfg->delta_adptctrl_lut[i + 3] << 24);
		ISP_WR_REG_OFT(ba, REG_ISP_EE_T, REG_154, i, raw);

		raw = cfg->chroma_adptctrl_lut[i] + (cfg->chroma_adptctrl_lut[i + 1] << 8) +
			(cfg->chroma_adptctrl_lut[i + 2] << 16) + (cfg->chroma_adptctrl_lut[i + 3] << 24);
		ISP_WR_REG_OFT(ba, REG_ISP_EE_T, REG_178, i, raw);
	}

	ISP_WR_REG(ba, REG_ISP_EE_T, REG_150, cfg->luma_adptctrl_lut[32]);
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_174, cfg->delta_adptctrl_lut[32]);
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_198, cfg->chroma_adptctrl_lut[32]);

	ISP_WR_REGS_BURST(ba, REG_ISP_EE_T, REG_A4, cfg->ee_1_cfg, cfg->ee_1_cfg.REG_A4);
	ISP_WR_REGS_BURST(ba, REG_ISP_EE_T, REG_19C, cfg->ee_2_cfg, cfg->ee_2_cfg.REG_19C);
	ISP_WR_REGS_BURST(ba, REG_ISP_EE_T, REG_1C4, cfg->ee_3_cfg, cfg->ee_3_cfg.REG_1C4);
}

void ispblk_pre_ee_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_pre_ee_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_PRE_EE];
	union REG_ISP_EE_00  reg_0;
	union REG_ISP_EE_04  reg_4;
	union REG_ISP_EE_0C  reg_c;
	union REG_ISP_EE_10  reg_10;
	union REG_ISP_EE_1FC reg_1fc;
	uint32_t i, raw;

	if (!cfg->update)
		return;

	reg_0.raw = ISP_RD_REG(ba, REG_ISP_EE_T, REG_00);
	reg_0.bits.EE_ENABLE = cfg->enable;
	reg_0.bits.EE_DEBUG_MODE = cfg->dbg_mode;
	reg_0.bits.EE_TOTAL_CORING = cfg->total_coring;
	reg_0.bits.EE_TOTAL_MOTION_CORING = cfg->total_motion_coring;
	reg_0.bits.EE_TOTAL_GAIN = cfg->total_gain;
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_00, reg_0.raw);

	if (!cfg->enable)
		return;

	reg_4.raw = ISP_RD_REG(ba, REG_ISP_EE_T, REG_04);
	reg_4.bits.EE_TOTAL_OSHTTHRD = cfg->total_oshtthrd;
	reg_4.bits.EE_TOTAL_USHTTHRD = cfg->total_ushtthrd;
	reg_4.bits.EE_PRE_PROC_ENABLE = cfg->pre_proc_enable;
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_04, reg_4.raw);

	reg_c.raw = ISP_RD_REG(ba, REG_ISP_EE_T, REG_0C);
	reg_c.bits.EE_LUMAREF_LPF_EN = cfg->lumaref_lpf_en;
	reg_c.bits.EE_LUMA_CORING_EN = cfg->luma_coring_en;
	reg_c.bits.EE_LUMA_ADPTCTRL_EN = cfg->luma_adptctrl_en;
	reg_c.bits.EE_DELTA_ADPTCTRL_EN = cfg->delta_adptctrl_en;
	reg_c.bits.EE_DELTA_ADPTCTRL_SHIFT = cfg->delta_adptctrl_shift;
	reg_c.bits.EE_CHROMAREF_LPF_EN = cfg->chromaref_lpf_en;
	reg_c.bits.EE_CHROMA_ADPTCTRL_EN = cfg->chroma_adptctrl_en;
	reg_c.bits.EE_MF_CORE_GAIN = cfg->mf_core_gain;
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_0C, reg_c.raw);

	reg_10.raw = ISP_RD_REG(ba, REG_ISP_EE_T, REG_10);
	reg_10.bits.HF_BLEND_WGT = cfg->hf_blend_wgt;
	reg_10.bits.MF_BLEND_WGT = cfg->mf_blend_wgt;
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_10, reg_10.raw);

	reg_1fc.raw = ISP_RD_REG(ba, REG_ISP_EE_T, REG_1FC);
	reg_1fc.bits.EE_SOFT_CLAMP_ENABLE = cfg->soft_clamp_enable;
	reg_1fc.bits.EE_UPPER_BOUND_LEFT_DIFF = cfg->upper_bound_left_diff;
	reg_1fc.bits.EE_LOWER_BOUND_RIGHT_DIFF = cfg->lower_bound_right_diff;
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_1FC, reg_1fc.raw);

	for (i = 0; i < 32; i += 4) {
		raw = cfg->luma_adptctrl_lut[i] + (cfg->luma_adptctrl_lut[i + 1] << 8) +
			(cfg->luma_adptctrl_lut[i + 2] << 16) + (cfg->luma_adptctrl_lut[i + 3] << 24);
		ISP_WR_REG_OFT(ba, REG_ISP_EE_T, REG_130, i, raw);

		raw = cfg->delta_adptctrl_lut[i] + (cfg->delta_adptctrl_lut[i + 1] << 8) +
			(cfg->delta_adptctrl_lut[i + 2] << 16) + (cfg->delta_adptctrl_lut[i + 3] << 24);
		ISP_WR_REG_OFT(ba, REG_ISP_EE_T, REG_154, i, raw);

		raw = cfg->chroma_adptctrl_lut[i] + (cfg->chroma_adptctrl_lut[i + 1] << 8) +
			(cfg->chroma_adptctrl_lut[i + 2] << 16) + (cfg->chroma_adptctrl_lut[i + 3] << 24);
		ISP_WR_REG_OFT(ba, REG_ISP_EE_T, REG_178, i, raw);
	}

	ISP_WR_REG(ba, REG_ISP_EE_T, REG_150, cfg->luma_adptctrl_lut[32]);
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_174, cfg->delta_adptctrl_lut[32]);
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_198, cfg->chroma_adptctrl_lut[32]);

	ISP_WR_REGS_BURST(ba, REG_ISP_EE_T, REG_A4, cfg->pre_ee_1_cfg, cfg->pre_ee_1_cfg.REG_A4);
	ISP_WR_REGS_BURST(ba, REG_ISP_EE_T, REG_19C, cfg->pre_ee_2_cfg, cfg->pre_ee_2_cfg.REG_19C);
	ISP_WR_REGS_BURST(ba, REG_ISP_EE_T, REG_1C4, cfg->pre_ee_3_cfg, cfg->pre_ee_3_cfg.REG_1C4);
}

void ispblk_fswdr_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_fswdr_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_HDRFUSION];
	uintptr_t manr_ba = ctx->phys_regs[ISP_BLK_ID_MMAP];
	union REG_FUSION_FS_CTRL_0 fs_ctrl;
	union REG_FUSION_FS_CTRL_1 fs_ctrl_1;
	union REG_ISP_MMAP_00 mm_00;
	union REG_ISP_MMAP_34 mm_34;
	union REG_ISP_MMAP_3C mm_3c;
	union REG_ISP_MMAP_40 mm_40;

	if (!cfg->update)
		return;

	fs_ctrl.raw = ISP_RD_REG(ba, REG_FUSION_T, FS_CTRL_0);
	fs_ctrl.bits.FS_ENABLE			= cfg->enable;
	fs_ctrl.bits.SE_IN_SEL			= cfg->se_in_sel;
	fs_ctrl.bits.FS_MC_ENABLE		= cfg->mc_enable;
	fs_ctrl.bits.FS_DC_MODE			= cfg->dc_mode;
	fs_ctrl.bits.FS_LUMA_MODE		= cfg->luma_mode;
	fs_ctrl.bits.FS_LMAP_GUIDE_DC_MODE	= cfg->lmap_guide_dc_mode;
	fs_ctrl.bits.FS_LMAP_GUIDE_LUMA_MODE	= cfg->lmap_guide_luma_mode;
	fs_ctrl.bits.FS_S_MAX			= cfg->s_max;
	ISP_WR_REG(ba, REG_FUSION_T, FS_CTRL_0, fs_ctrl.raw);

	if (!cfg->enable)
		return;

	fs_ctrl_1.raw = ISP_RD_REG(ba, REG_FUSION_T, FS_CTRL_1);
	fs_ctrl_1.bits.LE_IN_SEL      = cfg->le_in_sel;
	fs_ctrl_1.bits.FS_FUSION_TYPE = cfg->fusion_type;
	fs_ctrl_1.bits.FS_FUSION_LWGT = cfg->fusion_lwgt;
	ISP_WR_REG(ba, REG_FUSION_T, FS_CTRL_1, fs_ctrl_1.raw);

	mm_00.raw = ISP_RD_REG(manr_ba, REG_ISP_MMAP_T, REG_00);
	mm_00.bits.MMAP_1_ENABLE = cfg->mmap_1_enable;
	mm_00.bits.MMAP_MRG_MODE = cfg->mmap_mrg_mode;
	mm_00.bits.MMAP_MRG_ALPH = cfg->mmap_mrg_alph;
	ISP_WR_REG(manr_ba, REG_ISP_MMAP_T, REG_00, mm_00.raw);

	mm_34.raw = 0;
	mm_34.bits.V_THD_L = cfg->mmap_v_thd_l;
	mm_34.bits.V_THD_H = cfg->mmap_v_thd_h;
	ISP_WR_REG(manr_ba, REG_ISP_MMAP_T, REG_34, mm_34.raw);

	mm_40.raw = 0;
	mm_40.bits.V_WGT_MIN = cfg->mmap_v_wgt_min;
	mm_40.bits.V_WGT_MAX = cfg->mmap_v_wgt_max;
	ISP_WR_REG(manr_ba, REG_ISP_MMAP_T, REG_40, mm_40.raw);

	mm_3c.raw = ISP_RD_REG(manr_ba, REG_ISP_MMAP_T, REG_3C);
	mm_3c.bits.V_WGT_SLP		= (cfg->mmap_v_wgt_slp & 0x7FFFF);
	mm_3c.bits.MOTION_LS_MODE	= cfg->motion_ls_mode;
	mm_3c.bits.MOTION_LS_SEL	= cfg->motion_ls_sel;
	ISP_WR_REG(manr_ba, REG_ISP_MMAP_T, REG_3C, mm_3c.raw);

	ISP_WR_BITS(manr_ba, REG_ISP_MMAP_T, REG_F8, HISTORY_SEL_2, cfg->history_sel_2);

	ISP_WR_REGS_BURST(ba, REG_FUSION_T, FS_SE_GAIN, cfg->fswdr_cfg, cfg->fswdr_cfg.FS_SE_GAIN);
	ISP_WR_REGS_BURST(ba, REG_FUSION_T, FS_MOTION_LUT_IN, cfg->fswdr_2_cfg, cfg->fswdr_2_cfg.FS_MOTION_LUT_IN);
	ISP_WR_REGS_BURST(ba, REG_FUSION_T, FS_CALIB_CTRL_0, cfg->fswdr_3_cfg, cfg->fswdr_3_cfg.FS_CALIB_CTRL_0);
}

void ispblk_fswdr_update_rpt(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_fswdr_report *cfg)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_HDRFUSION];

	cfg->cal_pix_num = ISP_RD_REG(ba, REG_FUSION_T, FS_CALIB_OUT_0);
	cfg->diff_sum_r = ISP_RD_REG(ba, REG_FUSION_T, FS_CALIB_OUT_1);
	cfg->diff_sum_g = ISP_RD_REG(ba, REG_FUSION_T, FS_CALIB_OUT_2);
	cfg->diff_sum_b = ISP_RD_REG(ba, REG_FUSION_T, FS_CALIB_OUT_3);
}

void ispblk_ldci_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_ldci_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_LDCI];
	union REG_ISP_LDCI_ENABLE reg_enable;

	if (!cfg->update)
		return;

	reg_enable.raw = ISP_RD_REG(ba, REG_ISP_LDCI_T, LDCI_ENABLE);
	reg_enable.bits.LDCI_ENABLE = cfg->enable;
	reg_enable.bits.LDCI_STATS_ENABLE = cfg->stats_enable;
	reg_enable.bits.LDCI_MAP_ENABLE = cfg->map_enable;
	reg_enable.bits.LDCI_UV_GAIN_ENABLE = cfg->uv_gain_enable;
	reg_enable.bits.LDCI_FIRST_FRAME_ENABLE = cfg->first_frame_enable;
	reg_enable.bits.LDCI_IMAGE_SIZE_DIV_BY_16X12 = cfg->image_size_div_by_16x12;
	ISP_WR_REG(ba, REG_ISP_LDCI_T, LDCI_ENABLE, reg_enable.raw);
	ISP_WR_BITS(ba, REG_ISP_LDCI_T, DMI_ENABLE, DMI_ENABLE, cfg->enable ? 3 : 0);

	if (!cfg->enable)
		return;

	ISP_WR_BITS(ba, REG_ISP_LDCI_T, LDCI_STRENGTH, LDCI_STRENGTH, cfg->strength);

	ISP_WR_REGS_BURST(ba, REG_ISP_LDCI_T, LDCI_LUMA_WGT_MAX,
		cfg->ldci_1_cfg, cfg->ldci_1_cfg.LDCI_LUMA_WGT_MAX);
	ISP_WR_REGS_BURST(ba, REG_ISP_LDCI_T, LDCI_BLK_SIZE_X,
		cfg->ldci_2_cfg, cfg->ldci_2_cfg.LDCI_BLK_SIZE_X);
	ISP_WR_REGS_BURST(ba, REG_ISP_LDCI_T, LDCI_IDX_FILTER_LUT_00,
		cfg->ldci_3_cfg, cfg->ldci_3_cfg.LDCI_IDX_FILTER_LUT_00);
	ISP_WR_REGS_BURST(ba, REG_ISP_LDCI_T, LDCI_LUMA_WGT_LUT_00,
		cfg->ldci_4_cfg, cfg->ldci_4_cfg.LDCI_LUMA_WGT_LUT_00);
	ISP_WR_REGS_BURST(ba, REG_ISP_LDCI_T, LDCI_VAR_FILTER_LUT_00,
		cfg->ldci_5_cfg, cfg->ldci_5_cfg.LDCI_VAR_FILTER_LUT_00);
}

void ispblk_ycur_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_ycur_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ycur = ctx->phys_regs[ISP_BLK_ID_YCURVE];
	uint16_t i = 0;

	union REG_ISP_YCURV_YCUR_PROG_DATA reg_data;
	union REG_ISP_YCURV_YCUR_PROG_CTRL prog_ctrl;

	if (!cfg->update)
		return;

	ISP_WR_BITS(ycur, REG_ISP_YCURV_T, YCUR_CTRL, YCUR_ENABLE, cfg->enable);

	if (cfg->enable) {
		prog_ctrl.raw = ISP_RD_REG(ycur, REG_ISP_YCURV_T, YCUR_PROG_CTRL);
#ifndef  __SOC_PHOBOS__
		prog_ctrl.bits.YCUR_WSEL = 0;
#else
		prog_ctrl.bits.YCUR_WSEL = prog_ctrl.bits.YCUR_WSEL ^ 1;
#endif
		prog_ctrl.bits.YCUR_PROG_EN = 1;
		ISP_WR_REG(ycur, REG_ISP_YCURV_T, YCUR_PROG_CTRL, prog_ctrl.raw);

		ISP_WR_BITS(ycur, REG_ISP_YCURV_T, YCUR_PROG_ST_ADDR, YCUR_ST_ADDR, 0);
		ISP_WR_BITS(ycur, REG_ISP_YCURV_T, YCUR_PROG_ST_ADDR, YCUR_ST_W, 1);
		ISP_WR_REG(ycur, REG_ISP_YCURV_T, YCUR_PROG_MAX, cfg->lut_256);

		for (i = 0; i < 64; i += 2) {
			reg_data.raw = 0;
			reg_data.bits.YCUR_DATA_E = cfg->lut[i];
			reg_data.bits.YCUR_DATA_O = cfg->lut[i + 1];
			reg_data.bits.YCUR_W = 1;
			ISP_WR_REG(ycur, REG_ISP_YCURV_T, YCUR_PROG_DATA, reg_data.raw);
		}

		ISP_WR_BITS(ycur, REG_ISP_YCURV_T, YCUR_PROG_CTRL, YCUR_RSEL, prog_ctrl.bits.YCUR_WSEL);
		ISP_WR_BITS(ycur, REG_ISP_YCURV_T, YCUR_PROG_CTRL, YCUR_PROG_EN, 0);
	}
}

void ispblk_dci_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_dci_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t dci = ctx->phys_regs[ISP_BLK_ID_DCI];
	union REG_ISP_DCI_GAMMA_PROG_CTRL dci_gamma_ctrl;
	union REG_ISP_DCI_GAMMA_PROG_DATA dci_gamma_data;
	uint16_t i = 0;

	if (!cfg->update)
		return;

	ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_ENABLE, DCI_ENABLE, cfg->enable);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, DMI_ENABLE, DMI_ENABLE, cfg->enable);

	if (!cfg->enable)
		return;

	ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_MAP_ENABLE, DCI_MAP_ENABLE, cfg->map_enable);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_ENABLE, DCI_HIST_ENABLE, cfg->hist_enable);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_MAP_ENABLE, DCI_PER1SAMPLE_ENABLE, cfg->per1sample_enable);
	ISP_WR_REG(dci, REG_ISP_DCI_T, DCI_DEMO_MODE, cfg->demo_mode);

	dci_gamma_ctrl.raw = ISP_RD_REG(dci, REG_ISP_DCI_T, GAMMA_PROG_CTRL);
#ifndef  __SOC_PHOBOS__
	dci_gamma_ctrl.bits.GAMMA_WSEL = 0;
#else
	dci_gamma_ctrl.bits.GAMMA_WSEL = dci_gamma_ctrl.bits.GAMMA_WSEL ^ 1;
#endif
	dci_gamma_ctrl.bits.GAMMA_PROG_EN = 1;
	dci_gamma_ctrl.bits.GAMMA_PROG_1TO3_EN = 1;
	ISP_WR_REG(dci, REG_ISP_DCI_T, GAMMA_PROG_CTRL, dci_gamma_ctrl.raw);

	for (i = 0; i < 256; i += 2) {
		dci_gamma_data.raw = 0;
		dci_gamma_data.bits.GAMMA_DATA_E = cfg->map_lut[i];
		dci_gamma_data.bits.GAMMA_DATA_O = cfg->map_lut[i + 1];
		dci_gamma_data.bits.GAMMA_W = 1;
		ISP_WR_REG(dci, REG_ISP_DCI_T, GAMMA_PROG_DATA, dci_gamma_data.raw);
	}

	ISP_WR_BITS(dci, REG_ISP_DCI_T, GAMMA_PROG_CTRL, GAMMA_RSEL, dci_gamma_ctrl.bits.GAMMA_WSEL);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, GAMMA_PROG_CTRL, GAMMA_PROG_EN, 0);
}

void ispblk_dhz_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_dhz_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t dhz = ctx->phys_regs[ISP_BLK_ID_DHZ];
	union REG_ISP_DEHAZE_DHZ_WGT dhz_wgt;

	if (!cfg->update)
		return;

	ISP_WR_BITS(dhz, REG_ISP_DEHAZE_T, DHZ_BYPASS, DEHAZE_ENABLE, cfg->enable);

	if (!cfg->enable)
		return;

	ISP_WR_BITS(dhz, REG_ISP_DEHAZE_T, DHZ_SMOOTH, DEHAZE_W, cfg->strength);
	ISP_WR_BITS(dhz, REG_ISP_DEHAZE_T, DHZ_SMOOTH, DEHAZE_TH_SMOOTH, cfg->th_smooth);
	ISP_WR_BITS(dhz, REG_ISP_DEHAZE_T, REG_1, DEHAZE_CUM_TH, cfg->cum_th);
	ISP_WR_BITS(dhz, REG_ISP_DEHAZE_T, REG_1, DEHAZE_HIST_TH, cfg->hist_th);
	ISP_WR_BITS(dhz, REG_ISP_DEHAZE_T, REG_3, DEHAZE_TMAP_MIN, cfg->tmap_min);
	ISP_WR_BITS(dhz, REG_ISP_DEHAZE_T, REG_3, DEHAZE_TMAP_MAX, cfg->tmap_max);
	ISP_WR_BITS(dhz, REG_ISP_DEHAZE_T, DHZ_BYPASS, DEHAZE_LUMA_LUT_ENABLE, cfg->luma_lut_enable);
	ISP_WR_BITS(dhz, REG_ISP_DEHAZE_T, DHZ_BYPASS, DEHAZE_SKIN_LUT_ENABLE, cfg->skin_lut_enable);

	dhz_wgt.raw = ISP_RD_REG(dhz, REG_ISP_DEHAZE_T, DHZ_WGT);
	dhz_wgt.bits.DEHAZE_A_LUMA_WGT = cfg->a_luma_wgt;
	dhz_wgt.bits.DEHAZE_BLEND_WGT = cfg->blend_wgt;
	dhz_wgt.bits.DEHAZE_TMAP_SCALE = cfg->tmap_scale;
	dhz_wgt.bits.DEHAZE_D_WGT = cfg->d_wgt;
	ISP_WR_REG(dhz, REG_ISP_DEHAZE_T, DHZ_WGT, dhz_wgt.raw);

	ISP_WR_BITS(dhz, REG_ISP_DEHAZE_T, REG_2, DEHAZE_SW_DC_TH, cfg->sw_dc_th);
	ISP_WR_BITS(dhz, REG_ISP_DEHAZE_T, REG_2, DEHAZE_SW_AGLOBAL_R, cfg->sw_aglobal_r);

	ISP_WR_BITS(dhz, REG_ISP_DEHAZE_T, REG_28, DEHAZE_SW_AGLOBAL_G, cfg->sw_aglobal_g);
	ISP_WR_BITS(dhz, REG_ISP_DEHAZE_T, REG_28, DEHAZE_SW_AGLOBAL_B, cfg->sw_aglobal_b);

	ISP_WR_BITS(dhz, REG_ISP_DEHAZE_T, REG_2C, DEHAZE_AGLOBAL_MAX, cfg->aglobal_max);
	ISP_WR_BITS(dhz, REG_ISP_DEHAZE_T, REG_2C, DEHAZE_AGLOBAL_MIN, cfg->aglobal_min);

	ISP_WR_BITS(dhz, REG_ISP_DEHAZE_T, DHZ_SKIN, DEHAZE_SKIN_CB, cfg->skin_cb);
	ISP_WR_BITS(dhz, REG_ISP_DEHAZE_T, DHZ_SKIN, DEHAZE_SKIN_CR, cfg->skin_cr);

	ISP_WR_REGS_BURST(dhz, REG_ISP_DEHAZE_T, REG_9, cfg->luma_cfg, cfg->luma_cfg.LUMA_00);
	ISP_WR_REGS_BURST(dhz, REG_ISP_DEHAZE_T, REG_17, cfg->skin_cfg, cfg->skin_cfg.SKIN_00);
	ISP_WR_REGS_BURST(dhz, REG_ISP_DEHAZE_T, TMAP_00, cfg->tmap_cfg, cfg->tmap_cfg.TMAP_00);
}

void ispblk_rgbcac_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_rgbcac_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t cfa = ctx->phys_regs[ISP_BLK_ID_CFA];
	uintptr_t rgbcac = ctx->phys_regs[ISP_BLK_ID_RGBCAC];

	if (!cfg->update)
		return;

	if (!ISP_RD_BITS(cfa, REG_ISP_CFA_T, REG_00, CFA_ENABLE) && cfg->enable) {
		vi_pr(VI_WARN, "[WARN] not support cfa disable && rgbcac enable\n");
		return;
	}

	ISP_WR_BITS(rgbcac, REG_ISP_RGBCAC_T, RGBCAC_CTRL, RGBCAC_ENABLE, cfg->enable);
	if (!cfg->enable)
		return;

	ISP_WR_BITS(rgbcac, REG_ISP_RGBCAC_T, RGBCAC_CTRL, RGBCAC_OUT_SEL, cfg->out_sel);
	ISP_WR_REGS_BURST(rgbcac, REG_ISP_RGBCAC_T, RGBCAC_PURPLE_TH,
		cfg->rgbcac_cfg, cfg->rgbcac_cfg.RGBCAC_PURPLE_TH);
}

void ispblk_cac_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_cac_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t cac = ctx->phys_regs[ISP_BLK_ID_CNR];
	union REG_ISP_CNR_PURPLE_CB2 cnr_purple_cb2;

	if (!cfg->update)
		return;

	ISP_WR_BITS(cac, REG_ISP_CNR_T, CNR_ENABLE, PFC_ENABLE, cfg->enable);
	if (!cfg->enable)
		return;

	ISP_WR_BITS(cac, REG_ISP_CNR_T, CNR_ENABLE, CNR_OUT_SEL, cfg->out_sel);
	ISP_WR_BITS(cac, REG_ISP_CNR_T, CNR_PURPLE_TH, CNR_PURPLE_TH, cfg->purple_th);
	ISP_WR_BITS(cac, REG_ISP_CNR_T, CNR_PURPLE_TH, CNR_CORRECT_STRENGTH, cfg->correct_strength);

	cnr_purple_cb2.raw = ISP_RD_REG(cac, REG_ISP_CNR_T, CNR_PURPLE_CB2);
	cnr_purple_cb2.bits.CNR_PURPLE_CB2 = cfg->purple_cb2;
	cnr_purple_cb2.bits.CNR_PURPLE_CR2 = cfg->purple_cr2;
	cnr_purple_cb2.bits.CNR_PURPLE_CB3 = cfg->purple_cb3;
	cnr_purple_cb2.bits.CNR_PURPLE_CR3 = cfg->purple_cr3;
	ISP_WR_REG(cac, REG_ISP_CNR_T, CNR_PURPLE_CB2, cnr_purple_cb2.raw);

	ISP_WR_REGS_BURST(cac, REG_ISP_CNR_T, CNR_PURPLE_CB, cfg->cac_cfg,
		cfg->cac_cfg.CNR_PURPLE_CB);
	ISP_WR_REGS_BURST(cac, REG_ISP_CNR_T, CNR_EDGE_SCALE, cfg->cac_2_cfg,
		cfg->cac_2_cfg.CNR_EDGE_SCALE);
	ISP_WR_REGS_BURST(cac, REG_ISP_CNR_T, CNR_EDGE_SCALE_LUT_0, cfg->cac_3_cfg,
		cfg->cac_3_cfg.CNR_EDGE_SCALE_LUT_0);
}

void ispblk_lcac_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_lcac_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t lcac = ctx->phys_regs[ISP_BLK_ID_LCAC];

	if (!cfg->update)
		return;

	ISP_WR_BITS(lcac, REG_ISP_LCAC_T, REG00, LCAC_ENABLE, cfg->enable);
	if (!cfg->enable)
		return;

	ISP_WR_BITS(lcac, REG_ISP_LCAC_T, REG00, LCAC_OUT_SEL, cfg->out_sel);
	ISP_WR_BITS(lcac, REG_ISP_LCAC_T, REG90, LCAC_LTI_LUMA_LUT_32, cfg->lti_luma_lut_32);
	ISP_WR_BITS(lcac, REG_ISP_LCAC_T, REG90, LCAC_FCF_LUMA_LUT_32, cfg->fcf_luma_lut_32);

	ISP_WR_REGS_BURST(lcac, REG_ISP_LCAC_T, REG04, cfg->lcac_cfg, cfg->lcac_cfg.REG04);
	ISP_WR_REGS_BURST(lcac, REG_ISP_LCAC_T, REG50, cfg->lcac_2_cfg, cfg->lcac_2_cfg.REG50);
	ISP_WR_REGS_BURST(lcac, REG_ISP_LCAC_T, REG70, cfg->lcac_3_cfg, cfg->lcac_3_cfg.REG70);
}

void ispblk_csc_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_csc_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t csc = ctx->phys_regs[ISP_BLK_ID_CSC];
	union REG_ISP_CSC_4 csc_4;
	union REG_ISP_CSC_5 csc_5;
	union REG_ISP_CSC_6 csc_6;
	union REG_ISP_CSC_7 csc_7;
	union REG_ISP_CSC_8 csc_8;
	union REG_ISP_CSC_9 csc_9;

	if (!cfg->update)
		return;

	ISP_WR_BITS(csc, REG_ISP_CSC_T, REG_0, CSC_ENABLE, cfg->enable);
	if (!cfg->enable)
		return;

	csc_4.raw = 0;
	csc_4.bits.COEFF_00 = cfg->coeff[0] & 0x3FFF;
	csc_4.bits.COEFF_01 = cfg->coeff[1] & 0x3FFF;
	ISP_WR_REG(csc, REG_ISP_CSC_T, REG_4, csc_4.raw);

	csc_5.raw = 0;
	csc_5.bits.COEFF_02 = cfg->coeff[2] & 0x3FFF;
	csc_5.bits.COEFF_10 = cfg->coeff[3] & 0x3FFF;
	ISP_WR_REG(csc, REG_ISP_CSC_T, REG_5, csc_5.raw);

	csc_6.raw = 0;
	csc_6.bits.COEFF_11 = cfg->coeff[4] & 0x3FFF;
	csc_6.bits.COEFF_12 = cfg->coeff[5] & 0x3FFF;
	ISP_WR_REG(csc, REG_ISP_CSC_T, REG_6, csc_6.raw);

	csc_7.raw = 0;
	csc_7.bits.COEFF_20 = cfg->coeff[6] & 0x3FFF;
	csc_7.bits.COEFF_21 = cfg->coeff[7] & 0x3FFF;
	ISP_WR_REG(csc, REG_ISP_CSC_T, REG_7, csc_7.raw);

	csc_8.raw = 0;
	csc_8.bits.COEFF_22 = cfg->coeff[8] & 0x3FFF;
	csc_8.bits.OFFSET_0 = cfg->offset[0] & 0x7FF;
	ISP_WR_REG(csc, REG_ISP_CSC_T, REG_8, csc_8.raw);

	csc_9.raw = 0;
	csc_9.bits.OFFSET_1 = cfg->offset[1] & 0x7FF;
	csc_9.bits.OFFSET_2 = cfg->offset[2] & 0x7FF;
	ISP_WR_REG(csc, REG_ISP_CSC_T, REG_9, csc_9.raw);
}

void ispblk_dpc_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_dpc_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t dpc = (cfg->inst == 0) ?
			ctx->phys_regs[ISP_BLK_ID_DPC0] : ctx->phys_regs[ISP_BLK_ID_DPC1];
	union REG_ISP_DPC_2 dpc_2;
	uint16_t i;

	if (!cfg->update)
		return;

	ISP_WR_BITS(dpc, REG_ISP_DPC_T, DPC_2, DPC_ENABLE, cfg->enable);
	if (!cfg->enable)
		return;

	dpc_2.raw = ISP_RD_REG(dpc, REG_ISP_DPC_T, DPC_2);
	dpc_2.bits.DPC_ENABLE = cfg->enable;
	dpc_2.bits.DPC_DYNAMICBPC_ENABLE = cfg->enable ? cfg->dynamicbpc_enable : 0;
	dpc_2.bits.DPC_STATICBPC_ENABLE = cfg->enable ? cfg->staticbpc_enable : 0;
	dpc_2.bits.DPC_CLUSTER_SIZE = cfg->cluster_size;
	ISP_WR_REG(dpc, REG_ISP_DPC_T, DPC_2, dpc_2.raw);

	if (cfg->staticbpc_enable && (cfg->bp_cnt > 0) && (cfg->bp_cnt < 2048)) {
		ISP_WR_BITS(dpc, REG_ISP_DPC_T, DPC_17, DPC_MEM_PROG_MODE, 1);
		ISP_WR_REG(dpc, REG_ISP_DPC_T, DPC_MEM_ST_ADDR, 0x80000000);

		for (i = 0; i < cfg->bp_cnt; i++)
			ISP_WR_REG(dpc, REG_ISP_DPC_T, DPC_MEM_W0,
				0x80000000 | cfg->bp_tbl[i]);

		// write 1 fff-fff to end
		ISP_WR_REG(dpc, REG_ISP_DPC_T, DPC_MEM_W0, 0x80ffffff);
		ISP_WR_BITS(dpc, REG_ISP_DPC_T, DPC_17, DPC_MEM_PROG_MODE, 0);
	}
	ISP_WR_REGS_BURST(dpc, REG_ISP_DPC_T, DPC_3,
				cfg->dpc_cfg, cfg->dpc_cfg.DPC_3);
}

void ispblk_ae_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_ae_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = 0;
	uint32_t raw;

	if (!cfg->update)
		return;

	switch (cfg->inst) {
	case 0: // LE
		ba = ctx->phys_regs[ISP_BLK_ID_AEHIST0];
		break;
	case 1: // SE
		ba = ctx->phys_regs[ISP_BLK_ID_AEHIST1];
		break;
	default:
		vi_pr(VI_ERR, "Wrong ae inst\n");
		return;
	}

	ISP_WR_BITS(ba, REG_ISP_AE_HIST_T, STS_AE0_HIST_ENABLE, STS_AE0_HIST_ENABLE, cfg->ae_enable);
	ISP_WR_BITS(ba, REG_ISP_AE_HIST_T, DMI_ENABLE, DMI_ENABLE, cfg->ae_enable);

	if (!cfg->ae_enable)
		return;

	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, STS_AE_OFFSETX, cfg->ae_offsetx);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, STS_AE_OFFSETY, cfg->ae_offsety);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, STS_AE_NUMXM1, cfg->ae_numx - 1);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, STS_AE_NUMYM1, cfg->ae_numy - 1);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, STS_AE_WIDTH, cfg->ae_width);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, STS_AE_HEIGHT, cfg->ae_height);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, STS_AE_STS_DIV, cfg->ae_sts_div);

	raw = cfg->ae_face_offset_x[0] + (cfg->ae_face_offset_y[0] << 16);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE0_LOCATION, raw);

	raw = cfg->ae_face_size_minus1_x[0] + (cfg->ae_face_size_minus1_y[0] << 16);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE0_SIZE, raw);

	raw = cfg->ae_face_offset_x[1] + (cfg->ae_face_offset_y[1] << 16);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE1_LOCATION, raw);

	raw = cfg->ae_face_size_minus1_x[1] + (cfg->ae_face_size_minus1_y[1] << 16);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE1_SIZE, raw);

	raw = cfg->ae_face_offset_x[2] + (cfg->ae_face_offset_y[2] << 16);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE2_LOCATION, raw);

	raw = cfg->ae_face_size_minus1_x[2] + (cfg->ae_face_size_minus1_y[2] << 16);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE2_SIZE, raw);

	raw = cfg->ae_face_offset_x[3] + (cfg->ae_face_offset_y[3] << 16);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE3_LOCATION, raw);

	raw = cfg->ae_face_size_minus1_x[3] + (cfg->ae_face_size_minus1_y[3] << 16);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE3_SIZE, raw);

	ISP_WR_REGS_BURST(ba, REG_ISP_AE_HIST_T, AE_FACE0_ENABLE, cfg->ae_cfg, cfg->ae_cfg.AE_FACE0_ENABLE);
	ISP_WR_REGS_BURST(ba, REG_ISP_AE_HIST_T, AE_WGT_00, cfg->ae_2_cfg, cfg->ae_2_cfg.AE_WGT_00);
}

void ispblk_ge_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_ge_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t dpc = (cfg->inst == 0) ?
			ctx->phys_regs[ISP_BLK_ID_DPC0] : ctx->phys_regs[ISP_BLK_ID_DPC1];
	if (!cfg->update)
		return;

	ISP_WR_BITS(dpc, REG_ISP_DPC_T, DPC_2, GE_ENABLE, cfg->enable);
	if (!cfg->enable)
		return;

	ISP_WR_REGS_BURST(dpc, REG_ISP_DPC_T, DPC_10, cfg->ge_cfg, cfg->ge_cfg.DPC_10);
}

void ispblk_af_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_af_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_AF];

	union REG_ISP_AF_ENABLES af_enables;
	union REG_ISP_AF_LOW_PASS_HORIZON low_pass_horizon;
	union REG_ISP_AF_HIGH_PASS_HORIZON_0 high_pass_horizon_0;
	union REG_ISP_AF_HIGH_PASS_HORIZON_1 high_pass_horizon_1;
	union REG_ISP_AF_HIGH_PASS_VERTICAL_0 high_pass_vertical_0;

	if (!cfg->update)
		return;

	ISP_WR_BITS(ba, REG_ISP_AF_T, KICKOFF, AF_ENABLE, cfg->enable);
	if (!cfg->enable)
		return;

	ISP_WR_BITS(ba, REG_ISP_AF_T, DMI_ENABLE, DMI_ENABLE, cfg->enable);
	ISP_WR_REG(ba, REG_ISP_AF_T, BYPASS, !cfg->enable);

	af_enables.raw = ISP_RD_REG(ba, REG_ISP_AF_T, ENABLES);
	af_enables.bits.AF_DPC_ENABLE = cfg->dpc_enable;
	af_enables.bits.AF_HLC_ENABLE = cfg->hlc_enable;
	ISP_WR_REG(ba, REG_ISP_AF_T, ENABLES, af_enables.raw);

	ISP_WR_BITS(ba, REG_ISP_AF_T, SQUARE_ENABLE, AF_SQUARE_ENABLE, cfg->square_enable);
	ISP_WR_BITS(ba, REG_ISP_AF_T, OUTSHIFT, AF_OUTSHIFT, cfg->outshift);
	ISP_WR_BITS(ba, REG_ISP_AF_T, NUM_GAPLINE, AF_NUM_GAPLINE, cfg->num_gapline);

	// 8 <= offset_x <= img_width - 8
	ISP_WR_BITS(ba, REG_ISP_AF_T, OFFSET_X, AF_OFFSET_X, cfg->offsetx);
	// 2 <= offset_y <= img_height - 2
	ISP_WR_BITS(ba, REG_ISP_AF_T, OFFSET_X, AF_OFFSET_Y, cfg->offsety);

	ISP_WR_REG(ba, REG_ISP_AF_T, BLOCK_WIDTH, cfg->block_width);	// (ctx->img_width - 16) / numx
	ISP_WR_REG(ba, REG_ISP_AF_T, BLOCK_HEIGHT, cfg->block_height);	// (ctx->img_height - 4) / numy
	ISP_WR_REG(ba, REG_ISP_AF_T, BLOCK_NUM_X, cfg->block_numx);		// should fixed to 17
	ISP_WR_REG(ba, REG_ISP_AF_T, BLOCK_NUM_Y, cfg->block_numy);		// should fixed to 15

	ISP_WR_REG(ba, REG_ISP_AF_T, HOR_LOW_PASS_VALUE_SHIFT, cfg->h_low_pass_value_shift);
	ISP_WR_REG(ba, REG_ISP_AF_T, OFFSET_HORIZONTAL_0, cfg->h_corning_offset_0);
	ISP_WR_REG(ba, REG_ISP_AF_T, OFFSET_HORIZONTAL_1, cfg->h_corning_offset_1);
	ISP_WR_REG(ba, REG_ISP_AF_T, OFFSET_VERTICAL, cfg->v_corning_offset);
	ISP_WR_REG(ba, REG_ISP_AF_T, HIGH_Y_THRE, cfg->high_luma_threshold);

	low_pass_horizon.raw = 0;
	low_pass_horizon.bits.AF_LOW_PASS_HORIZON_0 = cfg->h_low_pass_coef[0];
	low_pass_horizon.bits.AF_LOW_PASS_HORIZON_1 = cfg->h_low_pass_coef[1];
	low_pass_horizon.bits.AF_LOW_PASS_HORIZON_2 = cfg->h_low_pass_coef[2];
	low_pass_horizon.bits.AF_LOW_PASS_HORIZON_3 = cfg->h_low_pass_coef[3];
	low_pass_horizon.bits.AF_LOW_PASS_HORIZON_4 = cfg->h_low_pass_coef[4];
	ISP_WR_REG(ba, REG_ISP_AF_T, LOW_PASS_HORIZON, low_pass_horizon.raw);

	high_pass_horizon_0.raw = 0;
	high_pass_horizon_0.bits.AF_HIGH_PASS_HORIZON_0_0 = cfg->h_high_pass_coef_0[0];
	high_pass_horizon_0.bits.AF_HIGH_PASS_HORIZON_0_1 = cfg->h_high_pass_coef_0[1];
	high_pass_horizon_0.bits.AF_HIGH_PASS_HORIZON_0_2 = cfg->h_high_pass_coef_0[2];
	high_pass_horizon_0.bits.AF_HIGH_PASS_HORIZON_0_3 = cfg->h_high_pass_coef_0[3];
	high_pass_horizon_0.bits.AF_HIGH_PASS_HORIZON_0_4 = cfg->h_high_pass_coef_0[4];
	ISP_WR_REG(ba, REG_ISP_AF_T, HIGH_PASS_HORIZON_0, high_pass_horizon_0.raw);

	high_pass_horizon_1.raw = 0;
	high_pass_horizon_1.bits.AF_HIGH_PASS_HORIZON_1_0 = cfg->h_high_pass_coef_1[0];
	high_pass_horizon_1.bits.AF_HIGH_PASS_HORIZON_1_1 = cfg->h_high_pass_coef_1[1];
	high_pass_horizon_1.bits.AF_HIGH_PASS_HORIZON_1_2 = cfg->h_high_pass_coef_1[2];
	high_pass_horizon_1.bits.AF_HIGH_PASS_HORIZON_1_3 = cfg->h_high_pass_coef_1[3];
	high_pass_horizon_1.bits.AF_HIGH_PASS_HORIZON_1_4 = cfg->h_high_pass_coef_1[4];
	ISP_WR_REG(ba, REG_ISP_AF_T, HIGH_PASS_HORIZON_1, high_pass_horizon_1.raw);

	high_pass_vertical_0.raw = 0;
	high_pass_vertical_0.bits.AF_HIGH_PASS_VERTICAL_0_0 = cfg->v_high_pass_coef[0];
	high_pass_vertical_0.bits.AF_HIGH_PASS_VERTICAL_0_1 = cfg->v_high_pass_coef[1];
	high_pass_vertical_0.bits.AF_HIGH_PASS_VERTICAL_0_2 = cfg->v_high_pass_coef[2];
	ISP_WR_REG(ba, REG_ISP_AF_T, HIGH_PASS_VERTICAL_0, high_pass_vertical_0.raw);

	ISP_WR_BITS(ba, REG_ISP_AF_T, TH_LOW, AF_TH_LOW, cfg->th_low);
	ISP_WR_BITS(ba, REG_ISP_AF_T, TH_LOW, AF_TH_HIGH, cfg->th_high);
	ISP_WR_BITS(ba, REG_ISP_AF_T, GAIN_LOW, AF_GAIN_LOW, cfg->gain_low);
	ISP_WR_BITS(ba, REG_ISP_AF_T, GAIN_LOW, AF_GAIN_HIGH, cfg->gain_high);
	ISP_WR_BITS(ba, REG_ISP_AF_T, SLOP_LOW, AF_SLOP_LOW, cfg->slop_low);
	ISP_WR_BITS(ba, REG_ISP_AF_T, SLOP_LOW, AF_SLOP_HIGH, cfg->slop_high);
}

void ispblk_hist_v_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_hist_v_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t hist_v = ctx->phys_regs[ISP_BLK_ID_HIST_V];

	if (!cfg->update)
		return;

	ISP_WR_BITS(hist_v, REG_ISP_HIST_EDGE_V_T, IP_CONFIG, HIST_EDGE_V_ENABLE, cfg->enable);
	if (!cfg->enable)
		return;

	ISP_WR_BITS(hist_v, REG_ISP_HIST_EDGE_V_T, IP_CONFIG, HIST_EDGE_V_LUMA_MODE, cfg->luma_mode);
	ISP_WR_BITS(hist_v, REG_ISP_HIST_EDGE_V_T, DMI_ENABLE, DMI_ENABLE, cfg->enable);
	ISP_WR_BITS(hist_v, REG_ISP_HIST_EDGE_V_T, HIST_EDGE_V_OFFSETX, HIST_EDGE_V_OFFSETX, cfg->offset_x);
	ISP_WR_BITS(hist_v, REG_ISP_HIST_EDGE_V_T, HIST_EDGE_V_OFFSETY, HIST_EDGE_V_OFFSETY, cfg->offset_y);
}

void ispblk_gms_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_gms_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_GMS];
	u32 img_width = ctx->isp_pipe_cfg[raw_num].crop.w;
	u32 img_height = ctx->isp_pipe_cfg[raw_num].crop.h;

	if (!cfg->update)
		return;

	ISP_WR_BITS(ba, REG_ISP_GMS_T, GMS_ENABLE, GMS_ENABLE, cfg->enable);
	ISP_WR_BITS(ba, REG_ISP_GMS_T, DMI_ENABLE, DMI_ENABLE, cfg->enable);

	if (!cfg->enable)
		return;

	if (cfg->x_section_size > 1024 || cfg->y_section_size > 512 ||
		cfg->x_gap < 10 || cfg->y_gap < 10) {
		vi_pr(VI_WARN, "[WARN] GMS tuning x_gap(%d), y_gap(%d), x_sec_size(%d), y_sec_size(%d)\n",
			cfg->x_gap, cfg->y_gap, cfg->x_section_size, cfg->y_section_size);
		return;
	}

	if (((cfg->x_section_size & 1) != 0) || ((cfg->y_section_size & 1) != 0) ||
		((cfg->x_section_size & 3) != 2) || ((cfg->y_section_size & 3) != 2)) {
		vi_pr(VI_WARN, "[WARN] GMS tuning x_sec_size(%d) and y_sec_size(%d) should be even and 4n+2\n",
			cfg->x_section_size, cfg->y_section_size);
		return;
	}

	if (((cfg->x_section_size + 1) * 3 + cfg->offset_x + cfg->x_gap * 2 + 4) >= img_width) {
		vi_pr(VI_WARN, "[WARN] GMS tuning x_sec_size(%d), ofst_x(%d), x_gap(%d), img_size(%d)\n",
				cfg->x_section_size, cfg->offset_x, cfg->x_gap, img_width);
		return;
	}

	if (((cfg->y_section_size + 1) * 3 + cfg->offset_y + cfg->y_gap * 2) > img_height) {
		vi_pr(VI_WARN, "[WARN] GMS tuning y_sec_size(%d), ofst_y(%d), y_gap(%d), img_size(%d)\n",
				cfg->y_section_size, cfg->offset_y, cfg->y_gap, img_height);
		return;
	}

	ISP_WR_REG(ba, REG_ISP_GMS_T, GMS_START_X, cfg->offset_x);
	ISP_WR_REG(ba, REG_ISP_GMS_T, GMS_START_Y, cfg->offset_y);

	ISP_WR_REG(ba, REG_ISP_GMS_T, GMS_X_SIZEM1, cfg->x_section_size - 1);
	ISP_WR_REG(ba, REG_ISP_GMS_T, GMS_Y_SIZEM1, cfg->y_section_size - 1);

	ISP_WR_REG(ba, REG_ISP_GMS_T, GMS_X_GAP, cfg->x_gap);
	ISP_WR_REG(ba, REG_ISP_GMS_T, GMS_Y_GAP, cfg->y_gap);

	ispblk_dma_config(ctx, ISP_BLK_ID_DMA_CTL25, raw_num, 0);
}

void ispblk_mono_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_mono_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t tnr = ctx->phys_regs[ISP_BLK_ID_TNR];
	static uint8_t mono_enable[ISP_PRERAW_VIRT_MAX] = {0};

	if (!cfg->update)
		return;

	ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_5, FORCE_MONO_ENABLE, cfg->force_mono_enable);

	if (mono_enable[raw_num] != cfg->force_mono_enable) {
		mono_enable[raw_num] = cfg->force_mono_enable;

		if (mono_enable[raw_num] == 0) {
			ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_31, REG_3DNR_MOTION_C_LUT_OUT_0, 0xFF);
			ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_31, REG_3DNR_MOTION_C_LUT_OUT_1, 0xFF);
			ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_31, REG_3DNR_MOTION_C_LUT_OUT_2, 0xFF);
			ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_31, REG_3DNR_MOTION_C_LUT_OUT_3, 0xFF);
		}
	}
}

#if 0
void ispblk_lscr_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_lscr_config *cfg,
	const enum cvi_isp_raw raw_num)
{
}
void ispblk_preproc_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_preproc_config *cfg,
	const enum cvi_isp_raw raw_num)
{
}
#endif
