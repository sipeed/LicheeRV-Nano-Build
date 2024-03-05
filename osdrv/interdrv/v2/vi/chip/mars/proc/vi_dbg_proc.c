#include <linux/version.h>
#include <proc/vi_dbg_proc.h>

#define VI_DBG_PROC_NAME	"cvitek/vi_dbg"

/* Switch the output of proc.
 *
 * 0: VI debug info
 * 1: preraw0 reg-dump
 * 2: preraw1 reg-dump
 * 3: postraw reg-dump
 */
int proc_isp_mode;

/*************************************************************************
 *	Proc functions
 *************************************************************************/
static inline void _vi_dbg_proc_show(struct seq_file *m, void *v)
{
	struct cvi_vi_dev *vdev = m->private;
	struct isp_ctx *ctx = &vdev->ctx;
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;
	enum cvi_isp_raw raw_max = ISP_PRERAW_MAX - 1;
	struct timespec64 ts1, ts2;
	u32 frmCnt_start[ISP_PRERAW_VIRT_MAX], sofCnt_start[ISP_PRERAW_VIRT_MAX];
	u32 frmCnt_end[ISP_PRERAW_VIRT_MAX], sofCnt_end[ISP_PRERAW_VIRT_MAX];
	u64 t2 = 0, t1 = 0;
	char raw_char;

	raw_max = gViCtx->total_dev_num;

	for (raw_num = ISP_PRERAW_A; raw_num < ISP_PRERAW_VIRT_MAX; raw_num++) {
		if (!ctx->isp_pipe_enable[raw_num])
			continue;
		sofCnt_start[raw_num] = vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0];
		frmCnt_start[raw_num] = vdev->ctx.isp_pipe_cfg[raw_num].is_yuv_bypass_path
					? vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0]
					: vdev->postraw_frame_number[raw_num];
	}

	ktime_get_real_ts64(&ts1);
	t1 = ts1.tv_sec * 1000000 + ts1.tv_nsec / 1000;

	msleep(940);
	do {
		for (raw_num = ISP_PRERAW_A; raw_num < ISP_PRERAW_VIRT_MAX; raw_num++) {
			if (!ctx->isp_pipe_enable[raw_num])
				continue;
			sofCnt_end[raw_num] = vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0];
			frmCnt_end[raw_num] = vdev->ctx.isp_pipe_cfg[raw_num].is_yuv_bypass_path
						? vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0]
						: vdev->postraw_frame_number[raw_num];
		}
		ktime_get_real_ts64(&ts2);
		t2 = ts2.tv_sec * 1000000 + ts2.tv_nsec / 1000;
	} while ((t2 - t1) < 1000000);

	for (raw_num = ISP_PRERAW_A; raw_num < ISP_PRERAW_VIRT_MAX; raw_num++) {
		if (!ctx->isp_pipe_enable[raw_num])
			continue;
		if (raw_num == ISP_PRERAW_A) {
			seq_puts(m, "[VI BE_Dbg_Info]\n");
			seq_printf(m, "VIPreBEDoneSts\t\t:0x%x\t\tVIPreBEDmaIdleStatus\t:0x%x\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.be_sts.be_done_sts,
					ctx->isp_pipe_cfg[raw_num].dg_info.be_sts.be_dma_idle_sts);
			seq_puts(m, "[VI Post_Dbg_Info]\n");
			seq_printf(m, "VIIspTopStatus\t\t:0x%x\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.post_sts.top_sts);
			seq_puts(m, "[VI DMA_Dbg_Info]\n");
			seq_printf(m, "VIWdma0ErrStatus\t:0x%x\tVIWdma0IdleStatus\t:0x%x\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_0_err_sts,
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_0_idle);
			seq_printf(m, "VIWdma1ErrStatus\t:0x%x\tVIWdma1IdleStatus\t:0x%x\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_1_err_sts,
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_1_idle);
			seq_printf(m, "VIRdmaErrStatus\t\t:0x%x\tVIRdmaIdleStatus\t:0x%x\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.rdma_err_sts,
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.rdma_idle);
		}

		raw_char = 'A' + raw_num;

		seq_printf(m, "[VI ISP_PIPE_%c FE_Dbg_Info]\n", raw_char);
		seq_printf(m, "VIPreFERawDbgSts\t:0x%x\t\tVIPreFEDbgInfo\t\t:0x%x\n",
				ctx->isp_pipe_cfg[raw_num].dg_info.fe_sts.fe_idle_sts,
				ctx->isp_pipe_cfg[raw_num].dg_info.fe_sts.fe_done_sts);

		seq_printf(m, "[VI ISP_PIPE_%c]\n", raw_char);
		seq_printf(m, "VIOutImgWidth\t\t:%4d\n", ctx->isp_pipe_cfg[raw_num].post_img_w);
		seq_printf(m, "VIOutImgHeight\t\t:%4d\n", ctx->isp_pipe_cfg[raw_num].post_img_h);
		seq_printf(m, "VIInImgWidth\t\t:%4d\n", ctx->isp_pipe_cfg[raw_num].csibdg_width);
		seq_printf(m, "VIInImgHeight\t\t:%4d\n", ctx->isp_pipe_cfg[raw_num].csibdg_height);

		seq_printf(m, "VIDevFPS\t\t:%4d\n", sofCnt_end[raw_num] - sofCnt_start[raw_num]);
		seq_printf(m, "VIFPS\t\t\t:%4d\n", frmCnt_end[raw_num] - frmCnt_start[raw_num]);

		seq_printf(m, "VISofCh0Cnt\t\t:%4d\n", vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0]);
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
			seq_printf(m, "VISofCh1Cnt\t\t:%4d\n", vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH1]);

		seq_printf(m, "VIPreFECh0Cnt\t\t:%4d\n", vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0]);
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
			seq_printf(m, "VIPreFECh1Cnt\t\t:%4d\n", vdev->pre_fe_frm_num[raw_num][ISP_FE_CH1]);

		seq_printf(m, "VIPreBECh0Cnt\t\t:%4d\n", vdev->pre_be_frm_num[raw_num][ISP_BE_CH0]);
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
			seq_printf(m, "VIPreBECh1Cnt\t\t:%4d\n", vdev->pre_be_frm_num[raw_num][ISP_BE_CH1]);
		seq_printf(m, "VIPostCnt\t\t:%4d\n", vdev->postraw_frame_number[raw_num]);
		seq_printf(m, "VIDropCnt\t\t:%4d\n", vdev->drop_frame_number[raw_num]);
		seq_printf(m, "VIDumpCnt\t\t:%4d\n", vdev->dump_frame_number[raw_num]);

		seq_printf(m, "[VI ISP_PIPE_%c Csi_Dbg_Info]\n", raw_char);
		seq_printf(m, "VICsiIntStatus0\t\t:0x%x\n", ctx->isp_pipe_cfg[raw_num].dg_info.bdg_int_sts_0);
		seq_printf(m, "VICsiIntStatus1\t\t:0x%x\n", ctx->isp_pipe_cfg[raw_num].dg_info.bdg_int_sts_1);
		seq_printf(m, "VICsiCh0Dbg\t\t:0x%x\n", ctx->isp_pipe_cfg[raw_num].dg_info.bdg_chn_debug[ISP_FE_CH0]);
		seq_printf(m, "VICsiCh1Dbg\t\t:0x%x\n", ctx->isp_pipe_cfg[raw_num].dg_info.bdg_chn_debug[ISP_FE_CH1]);
		seq_printf(m, "VICsiOverFlowCnt\t:%4d\n", ctx->isp_pipe_cfg[raw_num].dg_info.bdg_fifo_of_cnt);

		seq_printf(m, "VICsiCh0WidthGTCnt\t:%4d\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_gt_cnt[ISP_FE_CH0]);
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			seq_printf(m, "VICsiCh1WidthGTCnt\t:%4d\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_gt_cnt[ISP_FE_CH1]);
		}

		seq_printf(m, "VICsiCh0WidthLSCnt\t:%4d\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_ls_cnt[ISP_FE_CH0]);
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			seq_printf(m, "VICsiCh1WidthLSCnt\t:%4d\n",
						ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_ls_cnt[ISP_FE_CH1]);
		}

		seq_printf(m, "VICsiCh0HeightGTCnt\t:%4d\n",
						ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_gt_cnt[ISP_FE_CH0]);
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			seq_printf(m, "VICsiCh1HeightGTCnt\t:%4d\n",
						ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_gt_cnt[ISP_FE_CH1]);
		}

		seq_printf(m, "VICsiCh0HeightLSCnt\t:%4d\n",
						ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_ls_cnt[ISP_FE_CH0]);
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			seq_printf(m, "VICsiCh1HeightLSCnt\t:%4d\n",
							ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_ls_cnt[ISP_FE_CH1]);
		}
	}
}

static int vi_dbg_proc_show(struct seq_file *m, void *v)
{
	//struct cvi_isp_vdev *isp_vdev = m->private;

	if (proc_isp_mode == 0)
		_vi_dbg_proc_show(m, v);
#if 0
	else
		isp_register_dump(&isp_vdev->ctx, m, proc_isp_mode);
#endif
	return 0;
}

static ssize_t vi_dbg_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	if (kstrtoint(user_buf, 10, &proc_isp_mode))
		proc_isp_mode = 0;
	return count;
}

static int vi_dbg_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, vi_dbg_proc_show, PDE_DATA(inode));
}

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
static const struct proc_ops vi_dbg_proc_fops = {
	.proc_open = vi_dbg_proc_open,
	.proc_read = seq_read,
	.proc_write = vi_dbg_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations vi_dbg_proc_fops = {
	.owner = THIS_MODULE,
	.open = vi_dbg_proc_open,
	.read = seq_read,
	.write = vi_dbg_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int vi_dbg_proc_init(struct cvi_vi_dev *_vdev)
{
	int rc = 0;

	/* create the /proc file */
	if (proc_create_data(VI_DBG_PROC_NAME, 0644, NULL, &vi_dbg_proc_fops, _vdev) == NULL) {
		pr_err("vi_dbg proc creation failed\n");
		rc = -EAGAIN;
	}

	return rc;
}

int vi_dbg_proc_remove(void)
{
	remove_proc_entry(VI_DBG_PROC_NAME, NULL);

	return 0;
}
