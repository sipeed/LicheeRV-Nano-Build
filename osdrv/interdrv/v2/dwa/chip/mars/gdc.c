
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <uapi/linux/sched/types.h>

#include <linux/cvi_comm_video.h>
#include <linux/cvi_comm_gdc.h>
#include <linux/dwa_uapi.h>

#include <base_cb.h>
#include <vb.h>
#include <sys.h>
#include "dwa_debug.h"
#include "dwa_platform.h"
#include "cvi_vip_dwa.h"
#include "gdc.h"
#include "ldc.h"
#include "mesh.h"
#include "vpss_cb.h"
#include "sys_context.h"

#define DWA_YUV_BLACK 0x808000
#define DWA_HW_TIMEOUT_MS 100

static inline CVI_S32 MOD_CHECK_NULL_PTR(MOD_ID_E mod, const void *ptr)
{
	if (mod >= CVI_ID_BUTT)
		return CVI_FAILURE;
	if (!ptr) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "NULL pointer\n");
		return CVI_ERR_GDC_NULL_PTR;
	}
	return CVI_SUCCESS;
}

static inline CVI_S32 CHECK_GDC_FORMAT(VIDEO_FRAME_INFO_S imgIn, VIDEO_FRAME_INFO_S imgOut)
{
	if (imgIn.stVFrame.enPixelFormat !=
	    imgOut.stVFrame.enPixelFormat) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "in/out pixelformat(%d-%d) mismatch\n",
		       imgIn.stVFrame.enPixelFormat,
		       imgOut.stVFrame.enPixelFormat);
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}
	if (!GDC_SUPPORT_FMT(imgIn.stVFrame.enPixelFormat)) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "pixelformat(%d) unsupported\n",
		       imgIn.stVFrame.enPixelFormat);
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}
	return CVI_SUCCESS;
}

struct cvi_gdc_proc_ctx *gdc_proc_ctx;

int gdc_handle_to_procIdx(struct cvi_dwa_job *job)
{
	int idx = 0;

	if (!gdc_proc_ctx)
		return -1;

	for (idx = 0; idx < GDC_PROC_JOB_INFO_NUM; ++idx) {
		if (gdc_proc_ctx->stJobInfo[idx].hHandle ==
			    (u64)(uintptr_t)job &&
		    gdc_proc_ctx->stJobInfo[idx].eState == GDC_JOB_WORKING)
			return idx;

		if (gdc_proc_ctx->stJobInfo[idx].eState == GDC_JOB_WORKING) {
			CVI_TRACE_DWA(CVI_DBG_WARN, "  [%d] hHandle=0x%llx, job=0x%llx\n", idx,
			       gdc_proc_ctx->stJobInfo[idx].hHandle,
			       (u64)(uintptr_t)job);
		}
	}

	return -1;
}

void gdc_proc_record_hw_start(struct cvi_dwa_job *job)
{
	struct timespec64 curTime;

	ktime_get_ts64(&curTime);
	job->hw_start_time = (u64)curTime.tv_sec * USEC_PER_SEC +
			     curTime.tv_nsec / NSEC_PER_USEC;
}

void gdc_proc_record_hw_end(struct cvi_dwa_job *job)
{
	int idx;
	u64 end_time;
	struct timespec64 curTime;

	if (!job)
		return;

	idx = gdc_handle_to_procIdx(job);
	if (idx < 0)
		return;

	/* accumulate every task execution time of one job */
	ktime_get_ts64(&curTime);
	end_time = (u64)curTime.tv_sec * USEC_PER_SEC +
		   curTime.tv_nsec / NSEC_PER_USEC;
	gdc_proc_ctx->stJobInfo[idx].u32HwTime += end_time - job->hw_start_time;
}

void gdc_proc_record_job_start(struct cvi_dwa_job *job)
{
	int idx;
	struct timespec64 curTime;
	u64 curTimeUs;

	if (!job)
		return;
	if (job->hw_start_time)
		return;

	idx = gdc_handle_to_procIdx(job);
	if (idx < 0)
		return;

	ktime_get_ts64(&curTime);
	curTimeUs = (u64)curTime.tv_sec * USEC_PER_SEC +
		    curTime.tv_nsec / NSEC_PER_USEC;

	gdc_proc_ctx->stJobInfo[idx].u32BusyTime =
		(u32)(curTimeUs - gdc_proc_ctx->stJobInfo[idx].u64SubmitTime);
	gdc_proc_ctx->stJobStatus.u32ProcingNum = 1;
}

void gdc_proc_record_job_end(struct cvi_dwa_job *job)
{
	int idx;
	struct timespec64 curTime;
	u64 curTimeUs;

	if (!job)
		return;

	idx = gdc_handle_to_procIdx(job);
	if (idx < 0)
		return;

	ktime_get_ts64(&curTime);
	curTimeUs = (u64)curTime.tv_sec * USEC_PER_SEC +
		    curTime.tv_nsec / NSEC_PER_USEC;

	gdc_proc_ctx->stJobStatus.u32Success++;
	gdc_proc_ctx->stTaskStatus.u32Success +=
		gdc_proc_ctx->stJobInfo[idx].u32TaskNum;

	gdc_proc_ctx->stJobInfo[idx].eState = GDC_JOB_SUCCESS;
	gdc_proc_ctx->stJobInfo[idx].u32CostTime =
		(u32)(curTimeUs - gdc_proc_ctx->stJobInfo[idx].u64SubmitTime);
}

void gdcq_init(void)
{
#if 0
	pthread_mutexattr_t ma;

	pthread_mutexattr_init(&ma);
	pthread_mutexattr_setpshared(&ma, PTHREAD_PROCESS_SHARED);
	pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST);

	STAILQ_INIT(&gdc_ctx.jobq);
	sem_init(&gdc_ctx.jobs_sem, 0, 0);
	pthread_mutex_init(&gdc_ctx.jobs_mutex, &ma);
	pthread_mutex_init(&gdc_ctx.lock, &ma);

	gdc_ctx.thread_created = ATOMIC_VAR_INIT(false);
#endif

	//INIT_LIST_HEAD(&gdc_ctx.jobq);
}

static s32 gdc_rotation_check_size(ROTATION_E enRotation,
				   const struct gdc_task_attr *pstTask)
{
	if (enRotation >= ROTATION_MAX) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "invalid rotation(%d).\n", enRotation);
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}

	if (enRotation == ROTATION_90 || enRotation == ROTATION_270 || enRotation == ROTATION_XY_FLIP) {
		if (pstTask->stImgOut.stVFrame.u32Width <
		    pstTask->stImgIn.stVFrame.u32Height) {
			CVI_TRACE_DWA(CVI_DBG_ERR, "rotation(%d) invalid: 'output width(%d) < input height(%d)'\n",
			       enRotation, pstTask->stImgOut.stVFrame.u32Width,
			       pstTask->stImgIn.stVFrame.u32Height);
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
		if (pstTask->stImgOut.stVFrame.u32Height <
		    pstTask->stImgIn.stVFrame.u32Width) {
			CVI_TRACE_DWA(CVI_DBG_ERR, "rotation(%d) invalid: 'output height(%d) < input width(%d)'\n",
			       enRotation, pstTask->stImgOut.stVFrame.u32Height,
			       pstTask->stImgIn.stVFrame.u32Width);
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
	} else {
		if (pstTask->stImgOut.stVFrame.u32Width <
		    pstTask->stImgIn.stVFrame.u32Width) {
			CVI_TRACE_DWA(CVI_DBG_ERR, "rotation(%d) invalid: 'output width(%d) < input width(%d)'\n",
			       enRotation, pstTask->stImgOut.stVFrame.u32Width,
			       pstTask->stImgIn.stVFrame.u32Width);
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
		if (pstTask->stImgOut.stVFrame.u32Height <
		    pstTask->stImgIn.stVFrame.u32Height) {
			CVI_TRACE_DWA(CVI_DBG_ERR, "rotation(%d) invalid: 'output height(%d) < input height(%d)'\n",
			       enRotation, pstTask->stImgOut.stVFrame.u32Height,
			       pstTask->stImgIn.stVFrame.u32Height);
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
	}

	return CVI_SUCCESS;
}

static void cvi_dwa_submit_hw(struct cvi_dwa_vdev *wdev,
			      struct cvi_dwa_job *job, struct gdc_task *tsk)
{
	struct ldc_cfg cfg;
	u8 num_of_plane;
	VIDEO_FRAME_S *in_frame, *out_frame;
	PIXEL_FORMAT_E enPixFormat;
	ROTATION_E enRotation;
	u64 mesh_addr = tsk->stTask.au64privateData[0];
	struct _DWA_BUF_WRAP_S *pstBufWrap = &tsk->stTask.stBufWrap;
	CVI_U32 bufWrapDepth = tsk->stTask.bufWrapDepth;
	u32 bufWrapPhyAddr = (u32)tsk->stTask.bufWrapPhyAddr;
	u8 sb_mode, ldc_dir;
	struct ldc_sb_cfg ldc_sb_cfg;

	in_frame = &tsk->stTask.stImgIn.stVFrame;
	out_frame = &tsk->stTask.stImgOut.stVFrame;
	enPixFormat = in_frame->enPixelFormat;
	enRotation = tsk->enRotation;

	CVI_TRACE_DWA(CVI_DBG_DEBUG, "        enPixFormat=%d, enRotation=%d, mesh_addr=0x%llx\n",
	       enPixFormat, enRotation, mesh_addr);

	mesh_addr = (mesh_addr != DEFAULT_MESH_PADDR) ? mesh_addr : 0;

	memset(&cfg, 0, sizeof(cfg));
	switch (enPixFormat) {
	default:
	case PIXEL_FORMAT_NV12:
	case PIXEL_FORMAT_NV21:
		cfg.pixel_fmt = 1;
		num_of_plane = 2;
		break;
	case PIXEL_FORMAT_YUV_400:
		cfg.pixel_fmt = 0;
		num_of_plane = 1;
		break;
	};

	/* User space(clock wise) -> DWA hw(counter-clock wise) */
	switch (enRotation) {
	case ROTATION_90:
		cfg.dst_mode = LDC_DST_ROT_270;
		break;
	case ROTATION_270:
		cfg.dst_mode = LDC_DST_ROT_90;
		break;
	case ROTATION_XY_FLIP:
		cfg.dst_mode = LDC_DST_XY_FLIP;
		break;
	default:
		cfg.dst_mode = LDC_DST_FLAT;
		break;
	}

	cfg.map_base = mesh_addr;
	cfg.bgcolor = (u16)DWA_YUV_BLACK /*ctx->bgcolor*/;
	cfg.src_width = ALIGN(in_frame->u32Width, 64);
	cfg.src_height = ALIGN(in_frame->u32Height, 64);
	cfg.ras_width = cfg.src_width;
	cfg.ras_height = cfg.src_height;

	if (cfg.map_base == 0)
		cfg.map_bypass = true;
	else
		cfg.map_bypass = false;

	/* slice buffer mode */
	// if((reg_sb_mode>0 && reg_dst_mode!=Flat) ldc_dir = 1, else ldc_dir = 0
	// reg_src_str = 0
	// reg_src_end = (reg_ldc_dir) ? (reg_src_ysize -1) : (reg_src_xsize - 1)
	sb_mode = (pstBufWrap->bEnable && bufWrapPhyAddr) ? LDC_SB_FRAME_BASE : LDC_SB_DISABLE;
	ldc_dir = (sb_mode && cfg.dst_mode != LDC_DST_FLAT) ? 1 : 0;
	cfg.src_xstart = 0;
	cfg.src_xend = ldc_dir ? in_frame->u32Height-1 : in_frame->u32Width-1;

	// TODO: define start/end by crop

	cfg.src_y_base = in_frame->u64PhyAddr[0];
	cfg.dst_y_base = out_frame->u64PhyAddr[0];
	if (num_of_plane == 2) {
		cfg.src_c_base = in_frame->u64PhyAddr[1];
		cfg.dst_c_base = out_frame->u64PhyAddr[1];
	}

	if (sb_mode) {
		/* rotation only, src height -> dst width */
		cfg.dst_y_base = bufWrapPhyAddr;
		//cfg.dst_c_base = cfg.dst_y_base + cfg.src_width * cfg.src_height;
		cfg.dst_c_base = cfg.dst_y_base + cfg.src_height * pstBufWrap->u32BufLine * bufWrapDepth;
	}

	CVI_TRACE_DWA(CVI_DBG_DEBUG, "        update size src(%d %d)\n",
			cfg.src_width, cfg.src_height);
	CVI_TRACE_DWA(CVI_DBG_DEBUG, "        update src-buf: 0x%llx-0x%llx\n",
			in_frame->u64PhyAddr[0], in_frame->u64PhyAddr[1]);
	CVI_TRACE_DWA(CVI_DBG_DEBUG, "        update dst-buf: 0x%x-0x%x\n",
			cfg.dst_y_base, cfg.dst_c_base);
	CVI_TRACE_DWA(CVI_DBG_DEBUG, "        update mesh_addr(%#llx)\n", mesh_addr);

	tsk->state = GDC_TASK_STATE_RUNNING;

	// TODO: hrtimer_start to detect h/w timeout

	// ldc_top
	if (wdev->clk_sys[1])
		clk_prepare_enable(wdev->clk_sys[1]);
	// clk_ldc
	if (wdev->clk_sys[4])
		clk_prepare_enable(wdev->clk_sys[4]);
	if (wdev->clk)
		clk_prepare_enable(wdev->clk);

	gdc_proc_record_hw_start(job);

	// ldc_reset();
	ldc_get_sb_default(&ldc_sb_cfg);
	if (sb_mode) {
		ldc_sb_cfg.sb_mode = sb_mode;
		ldc_sb_cfg.sb_size = (pstBufWrap->u32BufLine / 64) - 1;
		ldc_sb_cfg.sb_nb = tsk->stTask.bufWrapDepth - 1;
		ldc_sb_cfg.sb_set_str = 1;
		ldc_sb_cfg.ldc_dir = ldc_dir;
	}
	ldc_set_sb(&ldc_sb_cfg);

	ldc_engine(&cfg);
}

static enum ENUM_MODULES_ID convert_cb_id(MOD_ID_E enModId)
{
	if (enModId == CVI_ID_VI)
		return E_MODULE_VI;
	else if (enModId == CVI_ID_VO)
		return E_MODULE_VO;
	else if (enModId == CVI_ID_VPSS)
		return E_MODULE_VPSS;

	return E_MODULE_BUTT;
}

static void _dwa_op_done_cb(MOD_ID_E enModId, CVI_VOID *pParam, VB_BLK blk)
{
	struct dwa_op_done_cfg cfg;
	struct base_exe_m_cb exe_cb;
	enum ENUM_MODULES_ID callee = convert_cb_id(enModId);

	cfg.pParam = pParam;
	cfg.blk = blk;

	exe_cb.callee = callee;
	exe_cb.caller = E_MODULE_DWA;
	exe_cb.cmd_id = DWA_CB_GDC_OP_DONE;
	exe_cb.data   = &cfg;
	base_exe_module_cb(&exe_cb);
}

static void cvi_dwa_handle_hw_cb(struct cvi_dwa_vdev *wdev, struct cvi_dwa_job *job,
				 struct gdc_task *tsk, CVI_BOOL is_timeout)
{
	bool is_internal = (tsk->stTask.reserved == CVI_GDC_MAGIC);
	VB_BLK blk_in = VB_INVALID_HANDLE, blk_out = VB_INVALID_HANDLE;
	MOD_ID_E enModId = job->enModId;
	CVI_BOOL isLastTask;

	CVI_TRACE_DWA(CVI_DBG_DEBUG, "is_internal=%d\n", is_internal);

	if (wdev->clk)
		clk_disable_unprepare(wdev->clk);
	if (wdev->clk_sys[4])
		clk_disable_unprepare(wdev->clk_sys[4]);
	if (wdev->clk_sys[1])
		clk_disable_unprepare(wdev->clk_sys[1]);

	/* []internal module]:
	 *  sync_io or async_io
	 *  release imgin vb_blk at task done.
	 *  mesh prepared by module.
	 *
	 * []user case]:
	 *  always sync_io.
	 *  Don't care imgin/imgout vb_blk. User release by themselves.
	 *  mesh prepared by gdc at AddXXXTask.
	 *
	 */
	if (is_internal) {
		// for internal module handshaking. such as vi/vpss rotation.
		isLastTask = (CVI_BOOL)tsk->stTask.au64privateData[1];

		CVI_TRACE_DWA(CVI_DBG_DEBUG, "isLastTask=%d, blk_in(pa=0x%llx), blk_out(pa=0x%llx)\n",
				isLastTask,
				(unsigned long long)tsk->stTask.stImgIn.stVFrame.u64PhyAddr[0],
				(unsigned long long)tsk->stTask.stImgOut.stVFrame.u64PhyAddr[0]);

		blk_in = vb_physAddr2Handle(tsk->stTask.stImgIn.stVFrame.u64PhyAddr[0]);
		blk_out = vb_physAddr2Handle(tsk->stTask.stImgOut.stVFrame.u64PhyAddr[0]);
		if (blk_out == VB_INVALID_HANDLE)
			CVI_TRACE_DWA(CVI_DBG_ERR, "Can't get valid vb_blk.\n");
		else {
			// TODO: not handle timeout yet
			//VB_BLK blk_out_ret = (ret == CVI_SUCCESS) ? blk_out : VB_INVALID_HANDLE;
			VB_BLK blk_out_ret = blk_out;

			((struct vb_s *)blk_out)->mod_ids &= ~BIT(CVI_ID_GDC);
			if (isLastTask && (!is_timeout))
				_dwa_op_done_cb(enModId, (void *)(uintptr_t)tsk->stTask.au64privateData[2],
						blk_out_ret);

			//if (ret != CVI_SUCCESS)
			//	vb_release_block(blk_out);
		}

		// User space:
		//   Caller always assign callback.
		//   Null callback used for internal gdc sub job.
		// Kernel space:
		//  !isLastTask used for internal gdc sub job.
		if (isLastTask && blk_in != VB_INVALID_HANDLE) {
			((struct vb_s *)blk_in)->mod_ids &= ~BIT(CVI_ID_GDC);
			vb_release_block(blk_in);
		} else if (!isLastTask) {
			// free memory allocated in init_ldc_param(), size cbParamSize
			vfree((void *)(uintptr_t)tsk->stTask.au64privateData[2]);
		}
	} else {
		// release mesh
		if (tsk->stTask.au64privateData[0] &&
		    tsk->stTask.au64privateData[0] != DEFAULT_MESH_PADDR)
			sys_ion_free(tsk->stTask.au64privateData[0]);

		// release slice buffer
		if (tsk->stTask.bufWrapPhyAddr)
			sys_ion_free(tsk->stTask.bufWrapPhyAddr);
	}
}

static int _notify_vpp_sb_mode(struct gdc_task *tsk)
{
	struct base_exe_m_cb exe_cb;
	struct vpss_grp_sbm_cfg cfg;
	MMF_CHN_S stSrcChn = {.enModId = CVI_ID_GDC, .s32DevId = 0, .s32ChnId = 0};
	MMF_BIND_DEST_S stBindDest = {.u32Num = 0};
	struct _DWA_BUF_WRAP_S *pstBufWrap = &tsk->stTask.stBufWrap;
	int32_t ret;

	ret = sys_get_bindbysrc(&stSrcChn, &stBindDest);
	if (ret) {
		CVI_TRACE_DWA(CVI_DBG_WARN, "fail to find bind dst\n");
		return ret;
	}

	CVI_TRACE_DWA(CVI_DBG_DEBUG, "stBindDest u32Num=%d\n", stBindDest.u32Num);
	if (!stBindDest.u32Num)
		return -1;

	CVI_TRACE_DWA(CVI_DBG_DEBUG, "stBindDest [0]enModId=%d\n", stBindDest.astMmfChn[0].enModId);
	if (stBindDest.astMmfChn[0].enModId != CVI_ID_VPSS)
		return -1;

	memset(&cfg, 0, sizeof(cfg));
	cfg.grp = stBindDest.astMmfChn[0].s32DevId;
	cfg.sb_mode = pstBufWrap->bEnable ? LDC_SB_FRAME_BASE : LDC_SB_DISABLE;
	cfg.sb_size = (pstBufWrap->u32BufLine / 64) - 1;
	cfg.sb_nb = tsk->stTask.bufWrapDepth - 1;
	cfg.ion_size = pstBufWrap->u32WrapBufferSize;

	CVI_TRACE_DWA(CVI_DBG_DEBUG, "u32BufLine=%d, u32WrapBufferSize=%d, bufWrapDepth=%d\n",
			pstBufWrap->u32BufLine, pstBufWrap->u32WrapBufferSize, tsk->stTask.bufWrapDepth);

	exe_cb.callee = E_MODULE_VPSS;
	exe_cb.caller = E_MODULE_DWA;
	exe_cb.cmd_id = VPSS_CB_SET_DWA_2_VPSS_SBM;
	exe_cb.data = &cfg;
	tsk->state = GDC_TASK_STATE_WAIT_SBM_CFG_DONE;
	ret = base_exe_module_cb(&exe_cb);

	if (!ret) {
		tsk->stTask.bufWrapPhyAddr = cfg.ion_paddr;
		//tsk->state = GDC_TASK_STATE_WAIT_SBM_CFG_DONE;
	}

	return ret;
}

int dwa_vpss_sdm_cb_done(struct cvi_dwa_vdev *wdev)
{
	int ret = -1;
	struct cvi_dwa_job *job, *tmp;
	struct gdc_task *tsk, *tmp2;

	if (!wdev)
		return -1;

	list_for_each_entry_safe(job, tmp, &wdev->jobq, node) {
		list_for_each_entry_safe(tsk, tmp2, &job->task_list, node) {
			if (tsk->state == GDC_TASK_STATE_WAIT_SBM_CFG_DONE) {
				/* Update dwa state in vpss context */
				tsk->state = GDC_TASK_STATE_SBM_CFG_DONE;
				complete(&wdev->sem_sbm);
				ret = 0;
			}
		}
	}

	if (ret)
		CVI_TRACE_DWA(CVI_DBG_WARN, "dwa not wait vpss sbm cb\n");

	return ret;
}

/* == gdc_event_handler */
void gdc_job_worker(struct kthread_work *work)
{
	struct cvi_dwa_vdev *wdev =
		container_of(work, struct cvi_dwa_vdev, work);
	unsigned long flags;
	struct cvi_dwa_job *job, *tmp_job;
	struct gdc_task *tsk, *tmp_tsk;
	int ret;
	unsigned long timeout;
	CVI_BOOL is_timeout;
	u8 intr_status;

	CVI_TRACE_DWA(CVI_DBG_DEBUG, "+\n");

	list_for_each_entry_safe(job, tmp_job, &wdev->jobq, node) {
		gdc_proc_record_job_start(job);

		list_for_each_entry_safe(tsk, tmp_tsk, &job->task_list, node) {
			if (tsk->state == GDC_TASK_STATE_IDLE) {
				if (tsk->stTask.stBufWrap.bEnable) {
					if (_notify_vpp_sb_mode(tsk)) {
						CVI_TRACE_DWA(CVI_DBG_WARN, "_notify_vpp_sb_mode fail\n");
						continue;
					}

					timeout = msecs_to_jiffies(DWA_HW_TIMEOUT_MS);
					ret = wait_for_completion_timeout(&wdev->sem_sbm, timeout);
					if (ret == -ETIME) {
						CVI_TRACE_DWA(CVI_DBG_WARN, "wait vpp_sb_done timeout\n");
						continue;
					}
				}
				cvi_dwa_submit_hw(wdev, job, tsk);
			} else {
				CVI_TRACE_DWA(CVI_DBG_WARN, "dwa hw busy, not done tsk yet\n");
				continue;
			}

			timeout = msecs_to_jiffies(DWA_HW_TIMEOUT_MS);
			ret = wait_for_completion_timeout(&wdev->sem, timeout);
			if (ret == -ETIME) {
				CVI_TRACE_DWA(CVI_DBG_WARN, "dwa hw irq not respond\n");
				intr_status = ldc_intr_status();
				ldc_intr_clr(intr_status);
				is_timeout = CVI_TRUE;
				//continue;
			} else {
				is_timeout = CVI_FALSE;
			}
			cvi_dwa_handle_hw_cb(wdev, job, tsk, is_timeout);
			spin_lock_irqsave(&wdev->lock, flags);
			list_del(&tsk->node);
			spin_unlock_irqrestore(&wdev->lock, flags);
			kfree(tsk);
			CVI_TRACE_DWA(CVI_DBG_DEBUG, "tsk done, del tsk\n");
		}
		spin_lock_irqsave(&wdev->lock, flags);
		list_del(&job->node);
		wdev->job_done = true;
		spin_unlock_irqrestore(&wdev->lock, flags);

		if (job->sync_io)
			wake_up_interruptible(&wdev->cond_queue);

		gdc_proc_record_job_end(job);
		kfree(job);
		CVI_TRACE_DWA(CVI_DBG_DEBUG, "jobq del a job\n");
	}

	CVI_TRACE_DWA(CVI_DBG_DEBUG, "-\n");
}

int cvi_gdc_init(struct cvi_dwa_vdev *wdev)
{
	struct sched_param tsk;
	int ret;

	INIT_LIST_HEAD(&wdev->event_list);
	INIT_LIST_HEAD(&wdev->jobq);
	init_waitqueue_head(&wdev->cond_queue);
	kthread_init_work(&wdev->work, gdc_job_worker);
	spin_lock_init(&wdev->lock);
	init_completion(&wdev->sem);
	init_completion(&wdev->sem_sbm);
	kthread_init_worker(&wdev->worker);
	wdev->thread = kthread_run(kthread_worker_fn, &wdev->worker,
				   "gdc_work");

	// Same as sched_set_fifo in linux 5.x
	tsk.sched_priority = MAX_USER_RT_PRIO / 2;
	ret = sched_setscheduler(wdev->thread, SCHED_FIFO, &tsk);
	if (ret)
		CVI_TRACE_DWA(CVI_DBG_WARN, "gdc thread priority update failed: %d\n", ret);

	if (IS_ERR(wdev->thread)) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "failed to create gdc kthread\n");
		return -1;
	}

	gdc_proc_ctx = wdev->shared_mem;

	gdcq_init();

	return 0;
}

static s32 gdc_update_proc(struct cvi_dwa_job *job)
{
	u16 u16Idx;
	//struct cvi_dwa_job *job;
	struct gdc_task *curelm, *tmp;
	struct timespec64 curTime;

	if (!job) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "job NULL pointer\n");
		return CVI_ERR_GDC_NULL_PTR;
	}
	if (!gdc_proc_ctx) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "gdc_proc_ctx NULL pointer\n");
		return CVI_ERR_GDC_NULL_PTR;
	}

	gdc_proc_ctx->u16JobInfoIdx =
		(++gdc_proc_ctx->u16JobInfoIdx >= GDC_PROC_JOB_INFO_NUM) ?
			      0 :
			      gdc_proc_ctx->u16JobInfoIdx;
	u16Idx = gdc_proc_ctx->u16JobInfoIdx;

	memset(&gdc_proc_ctx->stJobInfo[u16Idx], 0,
	       sizeof(gdc_proc_ctx->stJobInfo[u16Idx]));
	gdc_proc_ctx->stJobInfo[u16Idx].hHandle = (u64)(uintptr_t)job;
	gdc_proc_ctx->stJobInfo[u16Idx].enModId = CVI_ID_BUTT;

	list_for_each_entry_safe(curelm, tmp, &job->task_list, node) {
		gdc_proc_ctx->stJobInfo[u16Idx].u32TaskNum++;
		gdc_proc_ctx->stJobInfo[u16Idx].u32InSize +=
			curelm->stTask.stImgIn.stVFrame.u32Width *
			curelm->stTask.stImgIn.stVFrame.u32Height;
		gdc_proc_ctx->stJobInfo[u16Idx].u32OutSize +=
			curelm->stTask.stImgOut.stVFrame.u32Width *
			curelm->stTask.stImgOut.stVFrame.u32Height;
	}

	gdc_proc_ctx->stJobInfo[u16Idx].eState = GDC_JOB_WORKING;
	ktime_get_ts64(&curTime);
	gdc_proc_ctx->stJobInfo[u16Idx].u64SubmitTime =
		(u64)curTime.tv_sec * USEC_PER_SEC +
		curTime.tv_nsec / NSEC_PER_USEC;

	return CVI_SUCCESS;
}

/**************************************************************************
 *   Public APIs.
 **************************************************************************/
s32 gdc_begin_job(struct cvi_dwa_vdev *wdev, struct gdc_handle_data *data)
{
	struct cvi_dwa_job *job;
	CVI_S32 ret;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_GDC, data);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!data) {
		return -1;
	}

	job = kzalloc(sizeof(struct cvi_dwa_job), GFP_ATOMIC);

	if (job == NULL) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "malloc failed.\n");
		return CVI_ERR_GDC_NOBUF;
	}

	INIT_LIST_HEAD(&job->task_list);

	job->sync_io = true;
	data->handle = (u64)(uintptr_t)job;

	if (gdc_proc_ctx)
		gdc_proc_ctx->stJobStatus.u32BeginNum++;

	return CVI_SUCCESS;
}

s32 gdc_end_job(struct cvi_dwa_vdev *wdev, u64 hHandle)
{
	s32 ret = CVI_SUCCESS;
	struct cvi_dwa_job *job = (struct cvi_dwa_job *)(uintptr_t)hHandle;
	unsigned long flags;

	if (gdc_proc_ctx)
		gdc_proc_ctx->stJobStatus.u32BeginNum--;

	if (list_empty(&job->task_list)) {
		CVI_TRACE_DWA(CVI_DBG_DEBUG, "    no task in job.\n");
		return CVI_ERR_GDC_NOT_PERMITTED;
	}

	gdc_update_proc(job);

	spin_lock_irqsave(&wdev->lock, flags);
	list_add_tail(&job->node, &wdev->jobq);
	wdev->job_done = false;
	spin_unlock_irqrestore(&wdev->lock, flags);

	kthread_queue_work(&wdev->worker, &wdev->work);

	if (job->sync_io)
		/* request from user space, block until finished */
		ret = wait_event_interruptible(wdev->cond_queue, wdev->job_done);

	CVI_TRACE_DWA(CVI_DBG_DEBUG, "job sync_io=%d, ret=%d\n", job->sync_io, ret);

	return ret;
}

s32 gdc_add_rotation_task(struct cvi_dwa_vdev *wdev,
			  struct gdc_task_attr *attr)
{
	struct cvi_dwa_job *job;
	struct gdc_task *tsk;
	u64 handle = attr->handle;
	unsigned long flags;
	CVI_S32 ret;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_GDC, attr);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!attr)
		return -1;

	ret = CHECK_GDC_FORMAT(attr->stImgIn, attr->stImgOut);
	if (ret != CVI_SUCCESS)
		return ret;

	if (attr->enRotation == ROTATION_180) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "do not support rotation 180\n");
		return CVI_ERR_GDC_NOT_SUPPORT;
	}

	if (gdc_rotation_check_size(attr->enRotation, attr) != CVI_SUCCESS) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "gdc_rotation_check_size fail\n");
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}

	job = (struct cvi_dwa_job *)(uintptr_t)handle;
	tsk = kzalloc(sizeof(*tsk), GFP_ATOMIC);

	memcpy(&tsk->stTask, attr, sizeof(tsk->stTask));
	tsk->type = GDC_TASK_TYPE_ROT;
	tsk->enRotation = attr->enRotation;
	tsk->state = GDC_TASK_STATE_IDLE;

	spin_lock_irqsave(&wdev->lock, flags);
	list_add_tail(&tsk->node, &job->task_list);
	spin_unlock_irqrestore(&wdev->lock, flags);

	return CVI_SUCCESS;
}

s32 gdc_add_ldc_task(struct cvi_dwa_vdev *wdev, struct gdc_task_attr *attr)
{
	struct cvi_dwa_job *job;
	struct gdc_task *tsk;
	u64 handle = attr->handle;
	unsigned long flags;
	CVI_S32 ret;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_GDC, attr);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!attr)
		return -1;

	ret = CHECK_GDC_FORMAT(attr->stImgIn, attr->stImgOut);
	if (ret != CVI_SUCCESS)
		return ret;

	if (attr->enRotation == ROTATION_180) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "do not support rotation 180\n");
		return CVI_ERR_GDC_NOT_SUPPORT;
	}

	if (gdc_rotation_check_size(attr->enRotation, attr) != CVI_SUCCESS) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "gdc_rotation_check_size fail\n");
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}

	job = (struct cvi_dwa_job *)(uintptr_t)handle;
	tsk = kzalloc(sizeof(*tsk), GFP_ATOMIC);

	memcpy(&tsk->stTask, attr, sizeof(tsk->stTask));
	tsk->type = GDC_TASK_TYPE_LDC;
	tsk->enRotation = attr->enRotation;
	tsk->state = GDC_TASK_STATE_IDLE;

	spin_lock_irqsave(&wdev->lock, flags);
	list_add_tail(&tsk->node, &job->task_list);
	spin_unlock_irqrestore(&wdev->lock, flags);

	return CVI_SUCCESS;
}

s32 gdc_is_same_task_attr(const struct gdc_task_attr *tsk1, const struct gdc_task_attr *tsk2)
{
	CVI_TRACE_DWA(CVI_DBG_DEBUG, "tsk1: IW=%d, IH=%d, OW=%d, OH=%d\n",
			tsk1->stImgIn.stVFrame.u32Width, tsk1->stImgIn.stVFrame.u32Height,
			tsk1->stImgOut.stVFrame.u32Width, tsk1->stImgOut.stVFrame.u32Height);
	CVI_TRACE_DWA(CVI_DBG_DEBUG, "tsk2: IW=%d, IH=%d, OW=%d, OH=%d\n",
			tsk2->stImgIn.stVFrame.u32Width, tsk2->stImgIn.stVFrame.u32Height,
			tsk2->stImgOut.stVFrame.u32Width, tsk2->stImgOut.stVFrame.u32Height);

	if (tsk1->stImgIn.stVFrame.u32Width != tsk2->stImgIn.stVFrame.u32Width ||
	    tsk1->stImgIn.stVFrame.u32Height != tsk2->stImgIn.stVFrame.u32Height ||
	    tsk1->stImgOut.stVFrame.u32Width != tsk2->stImgOut.stVFrame.u32Width ||
	    tsk1->stImgOut.stVFrame.u32Height != tsk2->stImgOut.stVFrame.u32Height)
		return 0;

	return 1;
}

static CVI_S32 _calc_buf_wrp_depth(struct gdc_task *tsk, const struct dwa_buf_wrap_cfg *cfg, CVI_U32 *buf_wrp_depth)
{
	CVI_U32 min_buf_wrp_depth = 2;
	CVI_U32 min_buf_size, depth;
	CVI_U32 u32Width = tsk->stTask.stImgOut.stVFrame.u32Width;
	CVI_U32 u32BufLine = cfg->stBufWrap.u32BufLine;
	CVI_U32 u32WrapBufferSize = cfg->stBufWrap.u32WrapBufferSize;

	CVI_TRACE_DWA(CVI_DBG_DEBUG, "u32BufLine=%d, u32WrapBufferSize=%d\n",
			u32BufLine, u32WrapBufferSize);

	min_buf_size = u32BufLine * u32Width * min_buf_wrp_depth;
	if (u32WrapBufferSize < min_buf_size) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "invalid bufwrap size %d, min %d\n",
				u32WrapBufferSize, min_buf_size);
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}

	/* Size always calculated from nv21/n12 x1.5 */
	depth = (u32WrapBufferSize * 2) / (u32Width * u32BufLine * 3);
	if (depth < min_buf_wrp_depth) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "invalid bufwrap depth %d, min %d\n",
			      (int)depth, (int)min_buf_wrp_depth);
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}

	*buf_wrp_depth = depth;

	return CVI_SUCCESS;
}

CVI_S32 gdc_set_buf_wrap(struct cvi_dwa_vdev *wdev, const struct dwa_buf_wrap_cfg *cfg)
{
	CVI_S32 s32Ret = CVI_FAILURE;
	struct cvi_dwa_job *job;
	struct gdc_task *tsk, *tmp;
	unsigned long flags;
	const struct gdc_task_attr *wrp_tsk_attr;
	CVI_U32 buf_wrp_depth;

	job = (struct cvi_dwa_job *)(uintptr_t)cfg->handle;

	if (cfg->stBufWrap.bEnable) {
		if (cfg->stBufWrap.u32BufLine != 64 && cfg->stBufWrap.u32BufLine != 128 &&
		    cfg->stBufWrap.u32BufLine != 192 && cfg->stBufWrap.u32BufLine != 256) {
			CVI_TRACE_DWA(CVI_DBG_ERR, "invalid bufwrap line %d\n", cfg->stBufWrap.u32BufLine);
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
	}

	wrp_tsk_attr = &cfg->stTask;
	list_for_each_entry_safe(tsk, tmp, &job->task_list, node) {
		if (gdc_is_same_task_attr(wrp_tsk_attr, &tsk->stTask)) {
			s32Ret = _calc_buf_wrp_depth(tsk, cfg, &buf_wrp_depth);
			if (s32Ret != CVI_SUCCESS)
				break;

			spin_lock_irqsave(&wdev->lock, flags);
			if (tsk->state == GDC_TASK_STATE_IDLE) {
				memcpy(&tsk->stTask.stBufWrap, &cfg->stBufWrap,
				sizeof(tsk->stTask.stBufWrap));
				tsk->stTask.bufWrapDepth = buf_wrp_depth;
				s32Ret = CVI_SUCCESS;
			}
			spin_unlock_irqrestore(&wdev->lock, flags);

			break;
		}
	}

	return s32Ret;
}

s32 gdc_get_buf_wrap(struct cvi_dwa_vdev *wdev, struct dwa_buf_wrap_cfg *cfg)
{
	CVI_S32 s32Ret = CVI_FAILURE;
	struct cvi_dwa_job *job, *tmp, *wrp_job;
	struct gdc_task *tsk, *tmp2;
	const struct gdc_task_attr *wrp_tsk_attr;

	list_for_each_entry_safe(job, tmp, &wdev->jobq, node) {
		wrp_job = (struct cvi_dwa_job *)(uintptr_t)cfg->handle;
		if (job == wrp_job) {
			list_for_each_entry_safe(tsk, tmp2, &job->task_list, node) {
				wrp_tsk_attr = &cfg->stTask;
				if (gdc_is_same_task_attr(wrp_tsk_attr, &tsk->stTask)) {
					memcpy(&cfg->stBufWrap, &tsk->stTask.stBufWrap,
						sizeof(cfg->stBufWrap));
					s32Ret = CVI_SUCCESS;
					break;
				}
			}
		}
	}

	return s32Ret;
}
