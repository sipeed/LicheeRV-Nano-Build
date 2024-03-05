#include <linux/types.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/sched/types.h>

#include <linux/cvi_comm_video.h>
#include <linux/cvi_comm_gdc.h>
#include <linux/dwa_uapi.h>

#include "dwa_debug.h"
#include "dwa_platform.h"
#include "cvi_vip_dwa.h"
#include "gdc.h"
#include "ldc.h"
#include "mesh.h"


#define LDC_DBG (1 << 0)
#define LDC_INFO (1 << 1)
#define LDC_ERR (1 << 2)


#define TILESIZE 64 // HW: data Tile Size
#define HW_MESH_SIZE 8

#define MESH_NUM_ATILE (TILESIZE / HW_MESH_SIZE) // how many mesh in A TILE

typedef struct COORD2D_INT_HW {
	u8 xcor[3]; // s13.10, 24bit
} __attribute__((packed)) COORD2D_INT_HW;

extern struct cvi_gdc_proc_ctx *gdc_proc_ctx;

void mesh_gen_get_1st_size(SIZE_S in_size, u32 *mesh_1st_size)
{
	u32 ori_src_width, ori_src_height, src_width_s1, src_height_s1;
	u32 dst_height_s1, dst_width_s1, num_tilex_s1, num_tiley_s1;

	if (!mesh_1st_size)
		return;

	ori_src_width = in_size.u32Width;
	ori_src_height = in_size.u32Height;

	// In LDC Processing, width & height  aligned to TILESIZE **
	src_width_s1 = ((ori_src_width + TILESIZE - 1) / TILESIZE) * TILESIZE;
	src_height_s1 = ((ori_src_height + TILESIZE - 1) / TILESIZE) * TILESIZE;

	// modify frame size
	dst_height_s1 = src_height_s1;
	dst_width_s1 = src_width_s1;
	num_tilex_s1 = dst_width_s1 / TILESIZE;
	num_tiley_s1 = dst_height_s1 / TILESIZE;

	*mesh_1st_size = sizeof(struct COORD2D_INT_HW) * MESH_NUM_ATILE *
			 MESH_NUM_ATILE * num_tilex_s1 * num_tiley_s1 *
			 4; // 4 = 4 knots in a mesh
}

static int init_ldc_param(const struct vb_s *vb_in, struct vb_s *vb_out,
			  struct gdc_task_attr *stTask,
			  PIXEL_FORMAT_E enPixFormat, u64 mesh_addr,
			  CVI_BOOL isLastTask,
			  void *pcbParam, u32 cbParamSize,
			  ROTATION_E enRotation)
{
	int i;
	void *_pcbParam;

	memset(stTask, 0, sizeof(*stTask));
	// prepare the in/out image info of the gdc task.
	stTask->stImgIn.stVFrame.enPixelFormat = enPixFormat;
	stTask->stImgIn.stVFrame.u32Width = vb_in->buf.size.u32Width;
	stTask->stImgIn.stVFrame.u32Height = vb_in->buf.size.u32Height;
	stTask->stImgIn.stVFrame.u64PTS = vb_in->buf.u64PTS;
	for (i = 0; i < 3; ++i) {
		stTask->stImgIn.stVFrame.u64PhyAddr[i] = vb_in->buf.phy_addr[i];
		stTask->stImgIn.stVFrame.u32Length[i] = vb_in->buf.length[i];
		stTask->stImgIn.stVFrame.u32Stride[i] = vb_in->buf.stride[i];
	}

	stTask->stImgOut.stVFrame.enPixelFormat = enPixFormat;
	stTask->stImgOut.stVFrame.u32Width = vb_out->buf.size.u32Width;
	stTask->stImgOut.stVFrame.u32Height = vb_out->buf.size.u32Height;
	for (i = 0; i < 3; ++i) {
		stTask->stImgOut.stVFrame.u64PhyAddr[i] =
			vb_out->buf.phy_addr[i];
		stTask->stImgOut.stVFrame.u32Length[i] = vb_out->buf.length[i];
		stTask->stImgOut.stVFrame.u32Stride[i] = vb_out->buf.stride[i];
	}

	stTask->stImgOut.stVFrame.u64PTS = stTask->stImgIn.stVFrame.u64PTS;
	stTask->stImgOut.stVFrame.pPrivateData = vb_out;

	switch (enRotation) {
	default:
	case ROTATION_0:
		stTask->stImgOut.stVFrame.s16OffsetTop =
			vb_in->buf.s16OffsetTop;
		stTask->stImgOut.stVFrame.s16OffsetLeft =
			vb_in->buf.s16OffsetLeft;
		stTask->stImgOut.stVFrame.s16OffsetBottom =
			vb_in->buf.s16OffsetBottom;
		stTask->stImgOut.stVFrame.s16OffsetRight =
			vb_in->buf.s16OffsetRight;
		break;
	case ROTATION_90:
		stTask->stImgOut.stVFrame.s16OffsetTop =
			vb_in->buf.s16OffsetLeft;
		stTask->stImgOut.stVFrame.s16OffsetLeft =
			vb_in->buf.s16OffsetBottom;
		stTask->stImgOut.stVFrame.s16OffsetBottom =
			vb_in->buf.s16OffsetRight;
		stTask->stImgOut.stVFrame.s16OffsetRight =
			vb_in->buf.s16OffsetTop;
		break;
	case ROTATION_180:
		stTask->stImgOut.stVFrame.s16OffsetTop =
			vb_in->buf.s16OffsetBottom;
		stTask->stImgOut.stVFrame.s16OffsetLeft =
			vb_in->buf.s16OffsetRight;
		stTask->stImgOut.stVFrame.s16OffsetBottom =
			vb_in->buf.s16OffsetTop;
		stTask->stImgOut.stVFrame.s16OffsetRight =
			vb_in->buf.s16OffsetLeft;
		break;
	case ROTATION_270:
		stTask->stImgOut.stVFrame.s16OffsetTop =
			vb_in->buf.s16OffsetRight;
		stTask->stImgOut.stVFrame.s16OffsetLeft =
			vb_in->buf.s16OffsetTop;
		stTask->stImgOut.stVFrame.s16OffsetBottom =
			vb_in->buf.s16OffsetLeft;
		stTask->stImgOut.stVFrame.s16OffsetRight =
			vb_in->buf.s16OffsetBottom;
		break;
	}

	vb_out->buf.s16OffsetTop = stTask->stImgOut.stVFrame.s16OffsetTop;
	vb_out->buf.s16OffsetBottom = stTask->stImgOut.stVFrame.s16OffsetBottom;
	vb_out->buf.s16OffsetLeft = stTask->stImgOut.stVFrame.s16OffsetLeft;
	vb_out->buf.s16OffsetRight = stTask->stImgOut.stVFrame.s16OffsetRight;
	vb_out->buf.u64PTS = stTask->stImgOut.stVFrame.u64PTS;
	vb_out->buf.frm_num = vb_in->buf.frm_num;
	vb_out->buf.motion_lv = vb_in->buf.motion_lv;
	memcpy(vb_out->buf.motion_table, vb_in->buf.motion_table,
	       MO_TBL_SIZE);

	_pcbParam = vmalloc(cbParamSize);
	if (!_pcbParam) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "vmalloc failed, cbParamSize=%d\n", cbParamSize);
		return -1;
	}

	memcpy(_pcbParam, pcbParam, cbParamSize);
	stTask->au64privateData[0] = mesh_addr;
	stTask->au64privateData[1] = isLastTask;
	stTask->au64privateData[2] = (uintptr_t)_pcbParam;
	stTask->reserved = CVI_GDC_MAGIC;

	return 0;
}

static s32 mesh_gdc_do_rot(struct cvi_dwa_vdev *wdev, struct vb_s *vb_in,
			   PIXEL_FORMAT_E enPixFormat, u64 mesh_addr,
			   bool sync_io, void *pcbParam,
			   u32 cbParamSize, MOD_ID_E enModId,
			   ROTATION_E enRotation)
{
	struct gdc_handle_data data;
	struct cvi_dwa_job *job;
	struct gdc_task_attr *pstTask = NULL;
	VB_BLK blk;
	struct vb_s *vb_out;
	u32 buf_size;
	s32 ret;
	SIZE_S size_out;

	if (enRotation == ROTATION_0 || enRotation == ROTATION_180) {
		size_out = vb_in->buf.size;
	} else {
		size_out.u32Width = ALIGN(vb_in->buf.size.u32Height, DEFAULT_ALIGN);
		size_out.u32Height = ALIGN(vb_in->buf.size.u32Width, DEFAULT_ALIGN);
	}

	pstTask = vmalloc(sizeof(*pstTask));
	if (!pstTask)
		return CVI_ERR_GDC_NOBUF;

	// get buf for gdc output.
	buf_size = vb_in->buf.length[0] + vb_in->buf.length[1] +
		   vb_in->buf.length[2];
	blk = vb_get_block_with_id(VB_INVALID_POOLID, buf_size, CVI_ID_GDC);
	if (blk == VB_INVALID_HANDLE) {
		vb_release_block((VB_BLK)vb_in);
		ret = CVI_ERR_GDC_NOBUF;
		goto ROT_FAIL_EXIT;
	}
	vb_in->mod_ids &= ~BIT(enModId);
	vb_in->mod_ids |= BIT(CVI_ID_GDC);
	vb_out = (struct vb_s *)blk;
	base_get_frame_info(enPixFormat, size_out, &vb_out->buf, vb_out->phy_addr, DEFAULT_ALIGN);
	CVI_TRACE_DWA(CVI_DBG_DEBUG, "GDC usage(%d) rot(%d) src phy-addr(0x%llx) dst phy-addr(0x%llx)\n",
		      GDC_USAGE_ROTATION, enRotation, (unsigned long long)vb_in->phy_addr,
		      (unsigned long long)vb_out->phy_addr);

	ret = init_ldc_param(vb_in, vb_out, pstTask, enPixFormat, mesh_addr,
			     CVI_TRUE, pcbParam, cbParamSize, enRotation);
	if (ret != CVI_SUCCESS)
		goto ROT_FAIL_EXIT;

	// start gdc job.
	gdc_begin_job(wdev, &data);
	pstTask->handle = data.handle;
	pstTask->enRotation = enRotation;
	gdc_add_rotation_task(wdev, pstTask);

	job = (struct cvi_dwa_job *)(uintptr_t)data.handle;
	job->sync_io = sync_io;
	job->enModId = enModId;
	ret = gdc_end_job(wdev, data.handle);
	if (ret == CVI_SUCCESS && gdc_proc_ctx)
		gdc_proc_ctx->stJobInfo[gdc_proc_ctx->u16JobInfoIdx].enModId =
			enModId;

ROT_FAIL_EXIT:
	vfree(pstTask);

	return ret;
}

static s32 mesh_gdc_do_ldc(struct cvi_dwa_vdev *wdev, const void *pUsageParam,
			   struct vb_s *vb_in, PIXEL_FORMAT_E enPixFormat,
			   u64 mesh_addr, bool sync_io,
			   void *pcbParam, u32 cbParamSize, MOD_ID_E enModId,
			   ROTATION_E enRotation)
{
	struct gdc_handle_data data;
	struct cvi_dwa_job *job;
	struct gdc_task_attr *pstTask[2] = {NULL, NULL};
	VB_BLK blk;
	struct vb_s *vb_out[2];
	u32 buf_size;
	s32 ret;
	// void *_pcbParam;
	SIZE_S size_out[2];
	ROTATION_E enRotationOut[2];
	u32 mesh_1st_size;

	pstTask[0] = vmalloc(sizeof(struct gdc_task_attr));
	pstTask[1] = vmalloc(sizeof(struct gdc_task_attr));
	if (!pstTask[0] || !pstTask[1])
		goto LDC_FAIL_EXIT;

	// get buf for gdc output.
	buf_size = vb_in->buf.length[0] + vb_in->buf.length[1] +
		   vb_in->buf.length[2];
	blk = vb_get_block_with_id(VB_INVALID_POOLID, buf_size, CVI_ID_GDC);
	if (blk == VB_INVALID_HANDLE) {
		vb_release_block((VB_BLK)vb_in);
		ret = CVI_ERR_GDC_NOBUF;
		goto LDC_FAIL_EXIT;
	}

	// Rotate 90/270 for 1st job
	size_out[0].u32Width = vb_in->buf.size.u32Height;
	size_out[0].u32Height = vb_in->buf.size.u32Width;

	if (enRotation == ROTATION_0 || enRotation == ROTATION_180) {
		size_out[1].u32Width = ALIGN(vb_in->buf.size.u32Width, DEFAULT_ALIGN);
		size_out[1].u32Height = ALIGN(vb_in->buf.size.u32Height, DEFAULT_ALIGN);
	} else {
		size_out[1].u32Width = ALIGN(vb_in->buf.size.u32Height, DEFAULT_ALIGN);
		size_out[1].u32Height = ALIGN(vb_in->buf.size.u32Width, DEFAULT_ALIGN);
	}

	vb_in->mod_ids &= ~BIT(enModId);
	vb_in->mod_ids |= BIT(CVI_ID_GDC);

	vb_out[0] = (struct vb_s *)blk;
	vb_out[1] = (struct vb_s *)vb_in; // Reuse input buffer
	base_get_frame_info(enPixFormat, size_out[0], &vb_out[0]->buf, vb_out[0]->phy_addr, DEFAULT_ALIGN);
	CVI_TRACE_DWA(CVI_DBG_DEBUG,
		      "GDC usage(%d) src phy-addr(0x%llx) dst phy-addr(0x%llx, 0x%llx)\n",
		      GDC_USAGE_LDC, (unsigned long long)vb_in->phy_addr,
		      (unsigned long long)vb_out[0]->phy_addr,
		      (unsigned long long)vb_out[1]->phy_addr);

	switch (enRotation) {
	default:
	case ROTATION_0:
		enRotationOut[0] = ROTATION_90;
		enRotationOut[1] = ROTATION_270;
		break;
	case ROTATION_90:
		enRotationOut[0] = ROTATION_90;
		enRotationOut[1] = ROTATION_0;
		break;
	case ROTATION_180:
		enRotationOut[0] = ROTATION_90;
		enRotationOut[1] = ROTATION_90;
		break;
	case ROTATION_270:
		enRotationOut[0] = ROTATION_270;
		enRotationOut[1] = ROTATION_0;
		break;
	}

	ret = init_ldc_param(vb_in, vb_out[0], pstTask[0], enPixFormat,
			     mesh_addr, CVI_FALSE, pcbParam, cbParamSize,
			     enRotationOut[0]);
	if (ret) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "init ldc param failed\n");
		goto LDC_FAIL_EXIT;
	}

	// start gdc job.
	gdc_begin_job(wdev, &data);
	pstTask[0]->handle = data.handle;
	pstTask[0]->enRotation = enRotationOut[0];
	gdc_add_ldc_task(wdev, pstTask[0]);

	// Reuse vb_in after 1st job assigned
	base_get_frame_info(enPixFormat, size_out[1], &vb_out[1]->buf, vb_out[1]->phy_addr, DEFAULT_ALIGN);

	mesh_gen_get_1st_size(size_out[0], &mesh_1st_size);
	ret = init_ldc_param(vb_out[0], vb_out[1], pstTask[1], enPixFormat,
			     mesh_addr + mesh_1st_size, CVI_TRUE, pcbParam,
			     cbParamSize, enRotationOut[1]);
	if (ret) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "init 2nd ldc param failed\n");
		vfree((void *)(uintptr_t)pstTask[0]->au64privateData[2]);
		goto LDC_FAIL_EXIT;
	}

	pstTask[1]->handle = data.handle;
	pstTask[1]->enRotation = enRotationOut[1];
	gdc_add_ldc_task(wdev, pstTask[1]);

	job = (struct cvi_dwa_job *)(uintptr_t)data.handle;
	job->sync_io = sync_io;
	job->enModId = enModId;
	ret = gdc_end_job(wdev, data.handle);
	if (ret == CVI_SUCCESS && gdc_proc_ctx)
		gdc_proc_ctx->stJobInfo[gdc_proc_ctx->u16JobInfoIdx].enModId =
			enModId;

LDC_FAIL_EXIT:
	vfree(pstTask[0]);
	vfree(pstTask[1]);

	return ret;
}

s32 mesh_gdc_do_op(struct cvi_dwa_vdev *wdev, enum GDC_USAGE usage,
		   const void *pUsageParam, struct vb_s *vb_in,
		   PIXEL_FORMAT_E enPixFormat, u64 mesh_addr, CVI_BOOL sync_io,
		   void *pcbParam, u32 cbParamSize, MOD_ID_E enModId,
		   ROTATION_E enRotation)
{
	s32 ret = CVI_ERR_GDC_ILLEGAL_PARAM;

	CVI_TRACE_DWA(CVI_DBG_DEBUG, "GDC usage(%d) enRotation(%d), mesh-addr(0x%llx), cbParamSize(%d)\n",
			usage, enRotation, (unsigned long long)mesh_addr, cbParamSize);

	if (usage == GDC_USAGE_FISHEYE) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "GDC usage(%d) mesh-addr(0x%llx) not support\n",
			      usage, (unsigned long long)mesh_addr);
		return CVI_FAILURE;
	}
	if (mesh_addr < 0x80000000) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "GDC mod(0x%x) usage(%d) mesh-addr(0x%llx) invalid\n",
			      enModId, usage, (unsigned long long)mesh_addr);
		return CVI_FAILURE;
	}

	switch (usage) {
	case GDC_USAGE_LDC:
		ret = mesh_gdc_do_ldc(wdev, pUsageParam, vb_in, enPixFormat,
				      mesh_addr, sync_io, pcbParam,
				      cbParamSize, enModId, enRotation);
		break;
	case GDC_USAGE_ROTATION:
		ret = mesh_gdc_do_rot(wdev, vb_in, enPixFormat, mesh_addr,
				      sync_io, pcbParam, cbParamSize,
				      enModId, enRotation);
		break;
	default:
		break;
	}

	return ret;
}
