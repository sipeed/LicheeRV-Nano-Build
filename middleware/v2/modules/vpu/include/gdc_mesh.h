/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: module/vpu/include/gdc_mesh.h
 * Description:
 *   GDC's mesh generator for hw.
 */

#ifndef MODULES_VPU_INCLUDE_GDC_MESH_H_
#define MODULES_VPU_INCLUDE_GDC_MESH_H_

#define CVI_GDC_MAGIC 0xbabeface

#define CVI_GDC_MESH_SIZE_ROT 0x60000
#define CVI_GDC_MESH_SIZE_AFFINE 0x20000
#define CVI_GDC_MESH_SIZE_FISHEYE 0xB0000

enum gdc_task_type {
	GDC_TASK_TYPE_ROT = 0,
	GDC_TASK_TYPE_FISHEYE,
	GDC_TASK_TYPE_AFFINE,
	GDC_TASK_TYPE_LDC,
	GDC_TASK_TYPE_MAX,
};

/* gdc_task_param: the gdc task.
 *
 * stTask: define the in/out image info.
 * type: the type of gdc task.
 * param: the parameters for gdc task.
 */
struct gdc_task_param {
	STAILQ_ENTRY(gdc_task_param) stailq;

	GDC_TASK_ATTR_S stTask;
	enum gdc_task_type type;
	union {
		ROTATION_E enRotation;
		FISHEYE_ATTR_S stFishEyeAttr;
		AFFINE_ATTR_S stAffineAttr;
		LDC_ATTR_S stLDCAttr;
	};
};

/* gdc_job: the handle of gdc.
 *
 * ctx: the list of gdc task in the gdc job.
 * mutex: used if this job is sync-io.
 * cond: used if this job is sync-io.
 * sync_io: CVI_GDC_EndJob() will blocked until done is this is true.
 *          only meaningful if internal module use gdc.
 *          Default true;
 */
struct gdc_job {
	STAILQ_ENTRY(gdc_job) stailq;

	STAILQ_HEAD(gdc_job_ctx, gdc_task_param) ctx;
	pthread_cond_t cond;
	CVI_BOOL sync_io;
};

enum gdc_job_state {
	GDC_JOB_SUCCESS = 0,
	GDC_JOB_FAIL,
	GDC_JOB_WORKING,
};

struct gdc_job_info {
	CVI_S64 hHandle;
	MOD_ID_E enModId; // the module submitted gdc job
	CVI_U32 u32TaskNum; // number of tasks
	enum gdc_job_state eState; // job state
	CVI_U32 u32InSize;
	CVI_U32 u32OutSize;
	CVI_U32 u32CostTime; // From job submitted to job done
	CVI_U32 u32HwTime; // HW cost time
	CVI_U32 u32BusyTime; // From job submitted to job commit to driver
	CVI_U64 u64SubmitTime; // us
};

struct gdc_job_status {
	CVI_U32 u32Success;
	CVI_U32 u32Fail;
	CVI_U32 u32Cancel;
	CVI_U32 u32BeginNum;
	CVI_U32 u32BusyNum;
	CVI_U32 u32ProcingNum;
};

struct gdc_task_status {
	CVI_U32 u32Success;
	CVI_U32 u32Fail;
	CVI_U32 u32Cancel;
	CVI_U32 u32BusyNum;
};

struct gdc_operation_status {
	CVI_U32 u32AddTaskSuc;
	CVI_U32 u32AddTaskFail;
	CVI_U32 u32EndSuc;
	CVI_U32 u32EndFail;
	CVI_U32 u32CbCnt;
};

int get_mesh_size(int *p_mesh_hor, int *p_mesh_ver);
int set_mesh_size(int mesh_hor, int mesh_ver);
void mesh_gen_get_size(SIZE_S in_size, SIZE_S out_size, CVI_U32 *mesh_id_size, CVI_U32 *mesh_tbl_size);
void mesh_gen_rotation(SIZE_S in_size, SIZE_S out_size, ROTATION_E rot, uint64_t mesh_phy_addr, void *mesh_vir_addr);
void mesh_gen_affine(SIZE_S in_size, SIZE_S out_size, const AFFINE_ATTR_S *pstAffineAttr, uint64_t mesh_phy_addr,
		     void *mesh_vir_addr);
void mesh_gen_fisheye(SIZE_S in_size, SIZE_S out_size, const FISHEYE_ATTR_S *pstFisheyeAttr, uint64_t mesh_phy_addr,
		      void *mesh_vir_addr, ROTATION_E rot);
CVI_S32 mesh_gen_ldc(SIZE_S in_size, SIZE_S out_size, const LDC_ATTR_S *pstLDCAttr,
		     uint64_t mesh_phy_addr, void *mesh_vir_addr, ROTATION_E rot);

// cnv
void mesh_gen_cnv(const float *pfmesh_data, SIZE_S in_size, SIZE_S out_size, const FISHEYE_ATTR_S *pstFisheyeAttr,
		  uint64_t mesh_phy_addr, void *mesh_vir_addr);

void get_cnv_warp_mesh_tbl(SIZE_S in_size, SIZE_S out_size, const AFFINE_ATTR_S *pstAffineAttr, uint64_t mesh_phy_addr,
			   void *mesh_vir_addr);

#endif // MODULES_VPU_INCLUDE_GDC_MESH_H_
