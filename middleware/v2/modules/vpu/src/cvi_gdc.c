#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/mman.h>

#include "cvi_buffer.h"
#include "cvi_base.h"
#include "cvi_sys.h"
#include "cvi_vb.h"

#include "cvi_gdc.h"
#include "ioctl_vio.h"

#include "gdc_mesh.h"

#define DWA_YUV_BLACK 0x808000
#define DWA_RGB_BLACK 0x0

#define CHECK_GDC_FORMAT(imgIn, imgOut)                                                                                \
	do {                                                                                                           \
		if (imgIn.stVFrame.enPixelFormat != imgOut.stVFrame.enPixelFormat) {                                   \
			CVI_TRACE_GDC(CVI_DBG_ERR, "in/out pixelformat(%d-%d) mismatch\n",                             \
				      imgIn.stVFrame.enPixelFormat, imgOut.stVFrame.enPixelFormat);                    \
			return CVI_ERR_GDC_ILLEGAL_PARAM;                                                              \
		}                                                                                                      \
		if (!GDC_SUPPORT_FMT(imgIn.stVFrame.enPixelFormat)) {                                                  \
			CVI_TRACE_GDC(CVI_DBG_ERR, "pixelformat(%d) unsupported\n", imgIn.stVFrame.enPixelFormat);     \
			return CVI_ERR_GDC_ILLEGAL_PARAM;                                                              \
		}                                                                                                      \
	} while (0)

static CVI_S32 gdc_rotation_check_size(ROTATION_E enRotation, const GDC_TASK_ATTR_S *pstTask)
{
	if (enRotation > ROTATION_270) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "invalid rotation(%d).\n", enRotation);
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}

	if (enRotation == ROTATION_90 || enRotation == ROTATION_270) {
		if (pstTask->stImgOut.stVFrame.u32Width < pstTask->stImgIn.stVFrame.u32Height) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "rotation(%d) invalid: 'output width(%d) < input height(%d)'\n",
				enRotation, pstTask->stImgOut.stVFrame.u32Width,
				pstTask->stImgIn.stVFrame.u32Height);
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
		if (pstTask->stImgOut.stVFrame.u32Height < pstTask->stImgIn.stVFrame.u32Width) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "rotation(%d) invalid: 'output height(%d) < input width(%d)'\n",
				enRotation, pstTask->stImgOut.stVFrame.u32Height,
				pstTask->stImgIn.stVFrame.u32Width);
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
	} else {
		if (pstTask->stImgOut.stVFrame.u32Width < pstTask->stImgIn.stVFrame.u32Width) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "rotation(%d) invalid: 'output width(%d) < input width(%d)'\n",
				enRotation, pstTask->stImgOut.stVFrame.u32Width,
				pstTask->stImgIn.stVFrame.u32Width);
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
		if (pstTask->stImgOut.stVFrame.u32Height < pstTask->stImgIn.stVFrame.u32Height) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "rotation(%d) invalid: 'output height(%d) < input height(%d)'\n",
				enRotation, pstTask->stImgOut.stVFrame.u32Height,
				pstTask->stImgIn.stVFrame.u32Height);
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_GDC_Suspend(void)
{
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "+\n");

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "-\n");
	return CVI_SUCCESS;
}

CVI_S32 CVI_GDC_Resume(void)
{
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "+\n");

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "-\n");
	return CVI_SUCCESS;
}

/**************************************************************************
 *   Public APIs.
 **************************************************************************/
CVI_S32 CVI_GDC_BeginJob(GDC_HANDLE *phHandle)
{
	MOD_CHECK_NULL_PTR(CVI_ID_GDC, phHandle);

	return CVI_SUCCESS;
}

CVI_S32 CVI_GDC_EndJob(GDC_HANDLE hHandle)
{
	UNUSED(hHandle);

	return CVI_SUCCESS;
}

CVI_S32 CVI_GDC_CancelJob(GDC_HANDLE hHandle)
{
	if (hHandle == CVI_NULL) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "Null pointer.\n");
		return CVI_ERR_GDC_NULL_PTR;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_GDC_AddCorrectionTask(GDC_HANDLE hHandle, const GDC_TASK_ATTR_S *pstTask,
				  const FISHEYE_ATTR_S *pstFishEyeAttr)
{
	MOD_CHECK_NULL_PTR(CVI_ID_GDC, pstTask);
	MOD_CHECK_NULL_PTR(CVI_ID_GDC, pstFishEyeAttr);
	CHECK_GDC_FORMAT(pstTask->stImgIn, pstTask->stImgOut);

	struct gdc_task_param *param = calloc(1, sizeof(*param));

	if (pstFishEyeAttr->bEnable) {
		if (pstFishEyeAttr->u32RegionNum == 0) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "RegionNum(%d) can't be 0 if enable fisheye.\n",
				      pstFishEyeAttr->u32RegionNum);
			//gdc_proc_ctx->stFishEyeStatus.u32AddTaskFail++;
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
		if (((CVI_U32)pstFishEyeAttr->s32HorOffset > pstTask->stImgIn.stVFrame.u32Width) ||
		    ((CVI_U32)pstFishEyeAttr->s32VerOffset > pstTask->stImgIn.stVFrame.u32Height)) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "center pos(%d %d) out of frame size(%d %d).\n",
				      pstFishEyeAttr->s32HorOffset, pstFishEyeAttr->s32VerOffset,
				      pstTask->stImgIn.stVFrame.u32Width, pstTask->stImgIn.stVFrame.u32Height);
			//gdc_proc_ctx->stFishEyeStatus.u32AddTaskFail++;
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
		for (CVI_U32 i = 0; i < pstFishEyeAttr->u32RegionNum; ++i) {
			if ((pstFishEyeAttr->enMountMode == FISHEYE_WALL_MOUNT) &&
			    (pstFishEyeAttr->astFishEyeRegionAttr[i].enViewMode == FISHEYE_VIEW_360_PANORAMA)) {
				CVI_TRACE_GDC(CVI_DBG_ERR, "Rgn(%d): WALL_MOUNT not support Panorama_360.\n", i);
				//gdc_proc_ctx->stFishEyeStatus.u32AddTaskFail++;
				return CVI_ERR_GDC_ILLEGAL_PARAM;
			}
			if ((pstFishEyeAttr->enMountMode == FISHEYE_CEILING_MOUNT) &&
			    (pstFishEyeAttr->astFishEyeRegionAttr[i].enViewMode == FISHEYE_VIEW_180_PANORAMA)) {
				CVI_TRACE_GDC(CVI_DBG_ERR, "Rgn(%d): CEILING_MOUNT not support Panorama_180.\n", i);
				//gdc_proc_ctx->stFishEyeStatus.u32AddTaskFail++;
				return CVI_ERR_GDC_ILLEGAL_PARAM;
			}
			if ((pstFishEyeAttr->enMountMode == FISHEYE_DESKTOP_MOUNT) &&
			    (pstFishEyeAttr->astFishEyeRegionAttr[i].enViewMode == FISHEYE_VIEW_180_PANORAMA)) {
				CVI_TRACE_GDC(CVI_DBG_ERR, "Rgn(%d): DESKTOP_MOUNT not support Panorama_180.\n", i);
				//gdc_proc_ctx->stFishEyeStatus.u32AddTaskFail++;
				return CVI_ERR_GDC_ILLEGAL_PARAM;
			}
		}
	}

	param->stTask = *pstTask;
	param->type = GDC_TASK_TYPE_FISHEYE;
	param->stFishEyeAttr = *pstFishEyeAttr;

	if (param->stTask.reserved != CVI_GDC_MAGIC) {
		CVI_U64 paddr;
		CVI_VOID *vaddr;

		if (CVI_SYS_IonAlloc(&paddr, &vaddr, "gdc_mesh", CVI_GDC_MESH_SIZE_FISHEYE) != CVI_SUCCESS) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "Can't acquire memory for mesh.\n");
			//gdc_proc_ctx->stFishEyeStatus.u32AddTaskFail++;
			return CVI_ERR_GDC_NOBUF;
		}
		param->stTask.au64privateData[0] = paddr;
		param->stTask.au64privateData[1] = (uintptr_t)vaddr;

#if 0
		FILE *fp;
		int f_size;
		CVI_U64 mesh_tbl_addr;
		CVI_U16 *mesh_id;

		fp = fopen("mesh_id.bin", "r");
		if (fp == NULL) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "mesh id file not found.\n");
			return CVI_ERR_VO_NOT_PERMIT;
		}
		fseek(fp, 0, SEEK_END);
		f_size = ftell(fp);
		if (f_size == -1) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "mesh id file can't tell.\n");
			return CVI_ERR_VO_NOT_PERMIT;
		}
		fseek(fp, 0, SEEK_SET);
		fread(vaddr, 1, f_size, fp);
		fclose(fp);

		mesh_id = vaddr;
		paddr += ALIGN(f_size + 0x1000, 32);
		vaddr += ALIGN(f_size + 0x1000, 32);

		fp = fopen("mesh_tbl.bin", "r");
		if (fp == NULL) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "mesh tbl file not found.\n");
			return CVI_ERR_VO_NOT_PERMIT;
		}
		fseek(fp, 0, SEEK_END);
		f_size = ftell(fp);
		if (f_size == -1) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "mesh tbl file can't tell.\n");
			return CVI_ERR_VO_NOT_PERMIT;
		}
		fseek(fp, 0, SEEK_SET);
		fread(vaddr, 1, f_size, fp);
		fclose(fp);

		mesh_tbl_addr = paddr;

		/**!! offset changes by mesh file !!**/
		CVI_U32 offset[3] = {0x300, 0x300, 0x2800};
		CVI_U8 index = 0;

		while (*mesh_id != 0xffff) {
			if (*mesh_id == 0xfffb) {
				if (index > 3) {
					CVI_TRACE_GDC(CVI_DBG_ERR, "check mesh.\n");
					continue;
				}
				*(++mesh_id) = mesh_tbl_addr & 0x0fff;
				*(++mesh_id) = (mesh_tbl_addr >> 12) & 0x0fff;
				*(++mesh_id) = (mesh_tbl_addr >> 24) & 0x0fff;
				*(++mesh_id) = (mesh_tbl_addr >> 36) & 0x000f;
				mesh_tbl_addr += offset[index++];
			}
			++mesh_id;
		}
#else
		SIZE_S in_size, out_size;

		in_size.u32Width = pstTask->stImgIn.stVFrame.u32Width;
		in_size.u32Height = pstTask->stImgIn.stVFrame.u32Height;
		out_size.u32Width = pstTask->stImgOut.stVFrame.u32Width;
		out_size.u32Height = pstTask->stImgOut.stVFrame.u32Height;
		mesh_gen_fisheye(in_size, out_size, pstFishEyeAttr, paddr, vaddr, ROTATION_0);
		CVI_SYS_IonFlushCache(paddr, vaddr, CVI_GDC_MESH_SIZE_FISHEYE);
#endif
	}
	//gdc_proc_ctx->stFishEyeStatus.u32AddTaskSuc++;
	return CVI_SUCCESS;
}

CVI_S32 CVI_GDC_AddRotationTask(GDC_HANDLE hHandle, const GDC_TASK_ATTR_S *pstTask, ROTATION_E enRotation)
{
	MOD_CHECK_NULL_PTR(CVI_ID_GDC, pstTask);
	CHECK_GDC_FORMAT(pstTask->stImgIn, pstTask->stImgOut);

	if (gdc_rotation_check_size(enRotation, pstTask) != CVI_SUCCESS) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "gdc_rotation_check_size fail\n");
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}

	struct gdc_task_param *param = calloc(1, sizeof(*param));

	param->stTask = *pstTask;
	param->type = GDC_TASK_TYPE_ROT;
	param->enRotation = enRotation;

	if (param->stTask.reserved != CVI_GDC_MAGIC) {
		SIZE_S in_size, out_size;
		CVI_U64 paddr;
		CVI_VOID *vaddr;

		if (CVI_SYS_IonAlloc_Cached(&paddr, &vaddr, "gdc_mesh", CVI_GDC_MESH_SIZE_ROT) != CVI_SUCCESS) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "Can't acquire memory for mesh.\n");
			return CVI_ERR_GDC_NOBUF;
		}

		in_size.u32Width = pstTask->stImgIn.stVFrame.u32Width;
		in_size.u32Height = pstTask->stImgIn.stVFrame.u32Height;
		out_size.u32Width = pstTask->stImgOut.stVFrame.u32Width;
		out_size.u32Height = pstTask->stImgOut.stVFrame.u32Height;
		mesh_gen_rotation(in_size, out_size, enRotation, paddr, vaddr);
		CVI_SYS_IonFlushCache(paddr, vaddr, CVI_GDC_MESH_SIZE_ROT);
		param->stTask.au64privateData[0] = paddr;
		param->stTask.au64privateData[1] = (uintptr_t)vaddr;
	}
	return CVI_SUCCESS;
}

CVI_S32 CVI_GDC_AddAffineTask(GDC_HANDLE hHandle, const GDC_TASK_ATTR_S *pstTask, const AFFINE_ATTR_S *pstAffineAttr)
{
	MOD_CHECK_NULL_PTR(CVI_ID_GDC, pstTask);
	MOD_CHECK_NULL_PTR(CVI_ID_GDC, pstAffineAttr);
	CHECK_GDC_FORMAT(pstTask->stImgIn, pstTask->stImgOut);

	if (pstAffineAttr->u32RegionNum == 0) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "u32RegionNum(%d) can't be zero.\n", pstAffineAttr->u32RegionNum);
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}

	if (pstAffineAttr->stDestSize.u32Width > pstTask->stImgOut.stVFrame.u32Width) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "dest's width(%d) can't be larger than frame's width(%d)\n",
			      pstAffineAttr->stDestSize.u32Width, pstTask->stImgOut.stVFrame.u32Width);
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}
	for (CVI_U32 i = 0; i < pstAffineAttr->u32RegionNum; ++i) {
		CVI_TRACE_GDC(CVI_DBG_INFO, "u32RegionNum(%d) (%f, %f) (%f, %f) (%f, %f) (%f, %f)\n", i,
			      pstAffineAttr->astRegionAttr[i][0].x, pstAffineAttr->astRegionAttr[i][0].y,
			      pstAffineAttr->astRegionAttr[i][1].x, pstAffineAttr->astRegionAttr[i][1].y,
			      pstAffineAttr->astRegionAttr[i][2].x, pstAffineAttr->astRegionAttr[i][2].y,
			      pstAffineAttr->astRegionAttr[i][3].x, pstAffineAttr->astRegionAttr[i][3].y);
		if ((pstAffineAttr->astRegionAttr[i][0].x < 0) || (pstAffineAttr->astRegionAttr[i][0].y < 0) ||
		    (pstAffineAttr->astRegionAttr[i][1].x < 0) || (pstAffineAttr->astRegionAttr[i][1].y < 0) ||
		    (pstAffineAttr->astRegionAttr[i][2].x < 0) || (pstAffineAttr->astRegionAttr[i][2].y < 0) ||
		    (pstAffineAttr->astRegionAttr[i][3].x < 0) || (pstAffineAttr->astRegionAttr[i][3].y < 0)) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "u32RegionNum(%d) affine point can't be negative\n", i);
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
		if ((pstAffineAttr->astRegionAttr[i][1].x < pstAffineAttr->astRegionAttr[i][0].x) ||
		    (pstAffineAttr->astRegionAttr[i][3].x < pstAffineAttr->astRegionAttr[i][2].x)) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "u32RegionNum(%d) point1/3's x should be bigger thant 0/2's\n", i);
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
		if ((pstAffineAttr->astRegionAttr[i][2].y < pstAffineAttr->astRegionAttr[i][0].y) ||
		    (pstAffineAttr->astRegionAttr[i][3].y < pstAffineAttr->astRegionAttr[i][1].y)) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "u32RegionNum(%d) point2/3's y should be bigger thant 0/1's\n", i);
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
	}

	struct gdc_task_param *param = calloc(1, sizeof(*param));

	param->stTask = *pstTask;
	param->type = GDC_TASK_TYPE_AFFINE;
	param->stAffineAttr = *pstAffineAttr;

	SIZE_S in_size, out_size;
	CVI_U64 paddr;
	CVI_VOID *vaddr;

	if (CVI_SYS_IonAlloc_Cached(&paddr, &vaddr, "gdc_mesh", CVI_GDC_MESH_SIZE_AFFINE) != CVI_SUCCESS) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "Can't acquire memory for mesh.\n");
		return CVI_ERR_GDC_NOBUF;
	}

	in_size.u32Width = pstTask->stImgIn.stVFrame.u32Width;
	in_size.u32Height = pstTask->stImgIn.stVFrame.u32Height;
	out_size.u32Width = pstTask->stImgOut.stVFrame.u32Width;
	out_size.u32Height = pstTask->stImgOut.stVFrame.u32Height;
	mesh_gen_affine(in_size, out_size, pstAffineAttr, paddr, vaddr);
	CVI_SYS_IonFlushCache(paddr, vaddr, CVI_GDC_MESH_SIZE_AFFINE);
	param->stTask.au64privateData[0] = paddr;
	param->stTask.au64privateData[1] = (uintptr_t)vaddr;
	return CVI_SUCCESS;
}

CVI_S32 CVI_GDC_AddLDCTask(GDC_HANDLE hHandle, const GDC_TASK_ATTR_S *pstTask
	, const LDC_ATTR_S *pstLDCAttr, ROTATION_E enRotation)
{
	MOD_CHECK_NULL_PTR(CVI_ID_GDC, pstTask);
	MOD_CHECK_NULL_PTR(CVI_ID_GDC, pstLDCAttr);
	CHECK_GDC_FORMAT(pstTask->stImgIn, pstTask->stImgOut);

	if (enRotation != ROTATION_0) {
		if (gdc_rotation_check_size(enRotation, pstTask) != CVI_SUCCESS) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "gdc_rotation_check_size fail\n");
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
	}

	struct gdc_task_param *param = calloc(1, sizeof(*param));

	param->stTask = *pstTask;
	param->type = GDC_TASK_TYPE_LDC;
	param->stLDCAttr = *pstLDCAttr;
	param->stTask.au64privateData[3] = enRotation;

	return CVI_SUCCESS;
}

// cnv
#define CVI_GDC_MESH_SIZE_CNV (100000)
//offset[0:2]=tbl_param[0:2], len_tbl = tbl_param[3], len_idl = tbl_param[4]
CVI_S32 CVI_GDC_AddCorrectionTaskCNV(GDC_HANDLE hHandle, const GDC_TASK_ATTR_S *pstTask
	, const FISHEYE_ATTR_S *pstFishEyeAttr, uint8_t *p_tbl, uint8_t *p_idl, uint32_t *tbl_param)
{
	CHECK_GDC_FORMAT(pstTask->stImgIn, pstTask->stImgOut);

	struct gdc_task_param *param = calloc(1, sizeof(*param));
	int32_t len_tbl = tbl_param[4];
	int32_t len_idl = tbl_param[3];


	if (pstFishEyeAttr->bEnable) {
		if (pstFishEyeAttr->u32RegionNum == 0) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "RegionNum(%d) can't be 0 if enable fisheye.\n"
				, pstFishEyeAttr->u32RegionNum);
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
		if (((CVI_U32)pstFishEyeAttr->s32HorOffset > pstTask->stImgIn.stVFrame.u32Width)
		 || ((CVI_U32)pstFishEyeAttr->s32VerOffset > pstTask->stImgIn.stVFrame.u32Height)) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "center pos(%d %d) out of frame size(%d %d).\n"
				, pstFishEyeAttr->s32HorOffset, pstFishEyeAttr->s32VerOffset
				, pstTask->stImgIn.stVFrame.u32Width, pstTask->stImgIn.stVFrame.u32Height);
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
		for (CVI_U32 i = 0; i < pstFishEyeAttr->u32RegionNum; ++i) {
			if ((pstFishEyeAttr->enMountMode == FISHEYE_WALL_MOUNT)
			 && (pstFishEyeAttr->astFishEyeRegionAttr[i].enViewMode == FISHEYE_VIEW_360_PANORAMA)) {
				CVI_TRACE_GDC(CVI_DBG_ERR, "Rgn(%d): WALL_MOUNT not support Panorama_360.\n", i);
				return CVI_ERR_GDC_ILLEGAL_PARAM;
			}
			if ((pstFishEyeAttr->enMountMode == FISHEYE_CEILING_MOUNT)
			 && (pstFishEyeAttr->astFishEyeRegionAttr[i].enViewMode == FISHEYE_VIEW_180_PANORAMA)) {
				CVI_TRACE_GDC(CVI_DBG_ERR, "Rgn(%d): CEILING_MOUNT not support Panorama_180.\n", i);
				return CVI_ERR_GDC_ILLEGAL_PARAM;
			}
			if ((pstFishEyeAttr->enMountMode == FISHEYE_DESKTOP_MOUNT)
			 && (pstFishEyeAttr->astFishEyeRegionAttr[i].enViewMode == FISHEYE_VIEW_180_PANORAMA)) {
				CVI_TRACE_GDC(CVI_DBG_ERR, "Rgn(%d): DESKTOP_MOUNT not support Panorama_180.\n", i);
				return CVI_ERR_GDC_ILLEGAL_PARAM;
			}
		}
	}

	param->stTask = *pstTask;
	param->type = GDC_TASK_TYPE_FISHEYE;
	param->stFishEyeAttr = *pstFishEyeAttr;

	if (param->stTask.reserved != CVI_GDC_MAGIC) {
		CVI_U64 paddr;
		CVI_VOID *vaddr;

		if (CVI_SYS_IonAlloc(&paddr, &vaddr, "gdc_mesh", CVI_GDC_MESH_SIZE_CNV) != CVI_SUCCESS) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "Can't acquire memory for mesh.\n");
			return CVI_ERR_GDC_NOBUF;
		}
		param->stTask.au64privateData[0] = paddr;
		param->stTask.au64privateData[1] = (uintptr_t)vaddr;

		#if 1
		int f_size;
		CVI_U64 mesh_tbl_addr;
		CVI_U16 *mesh_id;

		f_size = len_idl;
		memcpy(vaddr, p_idl, f_size);
		mesh_id = vaddr;
		paddr += ALIGN(f_size + 0x1000, 32);
		vaddr += ALIGN(f_size + 0x1000, 32);

		f_size = len_tbl;
		memcpy(vaddr, p_tbl, f_size);
		mesh_tbl_addr = paddr;

		/**!! offset changes by mesh file !!**/
		CVI_U32 offset[3] = {0x1440, 0x1200, 0x1200};
		CVI_U8 index = 0;

		offset[0] = tbl_param[0];
		offset[1] = tbl_param[1];
		offset[2] = tbl_param[2];
		while (*mesh_id != 0xffff) {
			if (*mesh_id == 0xfffb) {
				if (index > 3) {
					CVI_TRACE_GDC(CVI_DBG_ERR, "check mesh.\n");
					continue;
				}
				*(++mesh_id) = mesh_tbl_addr & 0x0fff;
				*(++mesh_id) = (mesh_tbl_addr >> 12) & 0x0fff;
				*(++mesh_id) = (mesh_tbl_addr >> 24) & 0x0fff;
				*(++mesh_id) = (mesh_tbl_addr >> 36) & 0x000f;
				mesh_tbl_addr += offset[index++];
			}
			++mesh_id;
		}
		#else
		SIZE_S in_size, out_size;

		in_size.u32Width = pstTask->stImgIn.stVFrame.u32Width;
		in_size.u32Height = pstTask->stImgIn.stVFrame.u32Height;
		out_size.u32Width = pstTask->stImgOut.stVFrame.u32Width;
		out_size.u32Height = pstTask->stImgOut.stVFrame.u32Height;
		mesh_gen_fisheye(in_size, out_size, pstFishEyeAttr, paddr, vaddr);
		CVI_SYS_IonFlushCache(paddr, vaddr, CVI_GDC_MESH_SIZE_FISHEYE);
		#endif
	}
	return CVI_SUCCESS;
}

//#define CVI_GDC_MESH_SIZE_CNV (100000)
unsigned char ptbl[200000];
unsigned char ptbid[100000];
// static int flag_mem = 1;
int f_size;

#define USE_BIN_TXT 0 // 0: use mesh txt data, 1: use mesh tbl bin data

CVI_S32 CVI_GDC_AddCnvWarpTask(const float *pfmesh_data, GDC_HANDLE hHandle, const GDC_TASK_ATTR_S *pstTask,
			       const FISHEYE_ATTR_S *pstAffineAttr, bool *bReNew)
{
	CHECK_GDC_FORMAT(pstTask->stImgIn, pstTask->stImgOut);

	struct gdc_task_param *param = calloc(1, sizeof(*param));

	param->stTask = *pstTask;
	param->type = GDC_TASK_TYPE_FISHEYE; // GDC_TASK_TYPE_AFFINE;
	param->stFishEyeAttr = *pstAffineAttr;
	*bReNew = true;

	if (*bReNew == false) {
		return CVI_SUCCESS;
	}
	if (param->stTask.reserved != CVI_GDC_MAGIC) {
		CVI_U64 paddr;
		CVI_VOID *vaddr;

		if (CVI_SYS_IonAlloc_Cached(&paddr, &vaddr, "gdc_mesh", CVI_GDC_MESH_SIZE_CNV) != CVI_SUCCESS) {
			*bReNew = true;
			CVI_TRACE_GDC(CVI_DBG_ERR, "Can't acquire memory for mesh.\n");
			return CVI_ERR_GDC_NOBUF;
		}

#if USE_BIN_TXT
		FILE *fp;
		// int f_size;
		// flag_mem = 1;

		CVI_U64 mesh_tbl_addr;
		CVI_U16 *mesh_id;

		if (flag_mem) {
			memset(ptbid, 0, sizeof(ptbid));
			fp = fopen("cnv_mesh_id.bin", "r");
			if (fp == NULL) {
				CVI_TRACE_GDC(CVI_DBG_ERR, "mesh id file not found.\n");
				return CVI_ERR_VO_NOT_PERMIT;
			}
			fseek(fp, 0, SEEK_END);
			f_size = ftell(fp);
			if (f_size == -1) {
				CVI_TRACE_GDC(CVI_DBG_ERR, "mesh id file can't tell.\n");
				return CVI_ERR_VO_NOT_PERMIT;
			}
			fseek(fp, 0, SEEK_SET);
			fread(ptbid, 1, f_size, fp);
			fclose(fp);
			printf("mesh id size: %d\n", f_size);
		}
		mesh_id = vaddr;
		memcpy(mesh_id, ptbid, f_size);
		paddr += ALIGN(f_size + 0x1000, 32);
		vaddr += ALIGN(f_size + 0x1000, 32);

		if (flag_mem) {
			flag_mem = 0;
			memset(ptbl, 0, sizeof(ptbl));

			fp = fopen("cnv_mesh_tbl.bin", "r");
			if (fp == NULL) {
				CVI_TRACE_GDC(CVI_DBG_ERR, "mesh tbl file not found.\n");
				return CVI_ERR_VO_NOT_PERMIT;
			}
			fseek(fp, 0, SEEK_END);
			f_size = ftell(fp);
			if (f_size == -1) {
				CVI_TRACE_GDC(CVI_DBG_ERR, "mesh tbl file can't tell.\n");
				return CVI_ERR_VO_NOT_PERMIT;
			}
			fseek(fp, 0, SEEK_SET);
			fread(ptbl, 1, f_size, fp);
			fclose(fp);
		}

		memcpy(vaddr, ptbl, f_size);

		printf("mesh tbl size: %d\n", f_size);
		mesh_tbl_addr = paddr;

		/**!! offset changes by mesh file !!**/
		CVI_U32 offset[3] = { 0x1440, 0x1200, 0x1200 };
		CVI_U8 index = 0;

		while (*mesh_id != 0xffff) {
			if (*mesh_id == 0xfffb) {
				if (index > 3) {
					CVI_TRACE_GDC(CVI_DBG_ERR, "check mesh.\n");
					continue;
				}
				*(++mesh_id) = mesh_tbl_addr & 0x0fff;
				*(++mesh_id) = (mesh_tbl_addr >> 12) & 0x0fff;
				*(++mesh_id) = (mesh_tbl_addr >> 24) & 0x0fff;
				*(++mesh_id) = (mesh_tbl_addr >> 36) & 0x000f;
				mesh_tbl_addr += offset[index++];
			}
			++mesh_id;
		}
#else
		SIZE_S in_size, out_size;

		in_size.u32Width = pstTask->stImgIn.stVFrame.u32Width;
		in_size.u32Height = pstTask->stImgIn.stVFrame.u32Height;
		out_size.u32Width = pstTask->stImgOut.stVFrame.u32Width;
		out_size.u32Height = pstTask->stImgOut.stVFrame.u32Height;

		struct timeval t0, t1;

		gettimeofday(&t0, NULL);
		// got cnv mesh table from file
		// get_cnv_warp_mesh_tbl(in_size, out_size, pstAffineAttr, paddr, vaddr);
		// FISHEYE_ATTR_S pstFisheyeAttr;
		mesh_gen_cnv(pfmesh_data, in_size, out_size, pstAffineAttr, paddr, vaddr);

		gettimeofday(&t1, NULL);
		unsigned long elapsed_dwa = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;

		printf("mesh_gen_cnv speed: %lu\n", elapsed_dwa);

#endif
		printf("out[0]\n");
		CVI_SYS_IonFlushCache(paddr, vaddr,
				      CVI_GDC_MESH_SIZE_CNV); // 0x100000);
		printf("out[1]\n");
		param->stTask.au64privateData[0] = paddr;
		param->stTask.au64privateData[1] = (uintptr_t)vaddr;
		printf("finished\n");

		*bReNew = false;
	}
	return CVI_SUCCESS;
}

CVI_S32 CVI_GDC_SetMeshSize(int nMeshHor, int nMeshVer)
{
	return set_mesh_size(nMeshHor, nMeshVer);
}
