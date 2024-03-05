#ifndef _GDC_H_
#define _GDC_H_

#include <linux/list.h>
#include <linux/cvi_errno.h>
#include "vb.h"

enum gdc_task_type {
	GDC_TASK_TYPE_ROT = 0,
	GDC_TASK_TYPE_FISHEYE,
	GDC_TASK_TYPE_AFFINE,
	GDC_TASK_TYPE_LDC,
	GDC_TASK_TYPE_MAX,
};

enum gdc_task_state {
	GDC_TASK_STATE_IDLE,
	GDC_TASK_STATE_WAIT_SBM_CFG_DONE,
	GDC_TASK_STATE_SBM_CFG_DONE,
	GDC_TASK_STATE_RUNNING,
	GDC_TASK_STATE_DONE,
	GDC_TASK_STATE_ABORT,
	GDC_TASK_STATE_MAX,
};

enum gdc_op_id { GDC_OP_MESH_JOB = 0, GDC_OP_MAX };

/* gdc_task: the gdc task.
 *
 * stTask: define the in/out image info.
 * type: the type of gdc task.
 * param: the parameters for gdc task.
 */
struct gdc_task {
	//STAILQ_ENTRY(gdc_task_param) stailq;
	struct list_head node;

	struct gdc_task_attr stTask;
	enum gdc_task_type type;
	ROTATION_E enRotation;
	enum gdc_task_state state;
};

/* Begin a gdc job,then add task into the job,gdc will finish all the task in the job.
 *
 * @param phHandle: u64 *phHandle
 * @return Error code (0 if successful)
 */
s32 gdc_begin_job(struct cvi_dwa_vdev *wdev,
		  struct gdc_handle_data *phHandle);

/* End a job,all tasks in the job will be submmitted to gdc
 *
 * @param phHandle: u64 *phHandle
 * @return Error code (0 if successful)
 */
s32 gdc_end_job(struct cvi_dwa_vdev *wdev, u64 hHandle);

/* Cancel a job ,then all tasks in the job will not be submmitted to gdc
 *
 * @param phHandle: u64 *phHandle
 * @return Error code (0 if successful)
 */
s32 gdc_cancel_job(struct cvi_dwa_vdev *wdev, u64 hHandle);

/* Add a rotation task to a gdc job
 *
 * @param phHandle: u64 *phHandle
 * @param pstTask: to describe what to do
 * @param enRotation: for further settings
 * @return Error code (0 if successful)
 */
s32 gdc_add_rotation_task(struct cvi_dwa_vdev *wdev,
			  struct gdc_task_attr *attr);

s32 gdc_add_ldc_task(struct cvi_dwa_vdev *wdev, struct gdc_task_attr *attr);

// color night vision
//s32 cvi_gdc_addcnvwarptask(const float *pfmesh_data, u64 hHandle, const struct gdc_task_attr *pstTask,
//			       const FISHEYE_ATTR_S *pstAffineAttr, bool *bReNew);

//s32 cvi_gdc_addcorrectiontaskcnv(u64 hHandle, const struct gdc_task_attr *pstTask,
//		const FISHEYE_ATTR_S *pstFishEyeAttr, uint8_t *p_tbl, uint8_t *p_idl, uint32_t *tbl_param);

/* set meshsize for rotation only
 *
 * @param nMeshHor: mesh counts horizontal
 * @param nMeshVer: mesh counts vertical
 * @return Error code (0 if successful)
 */
s32 cvi_gdc_set_mesh_size(int nMeshHor, int nMeshVer);

s32 gdc_set_buf_wrap(struct cvi_dwa_vdev *wdev, const struct dwa_buf_wrap_cfg *cfg);
s32 gdc_get_buf_wrap(struct cvi_dwa_vdev *wdev, struct dwa_buf_wrap_cfg *cfg);

int cvi_gdc_init(struct cvi_dwa_vdev *wdev);

void gdc_proc_record_hw_end(struct cvi_dwa_job *job);

int dwa_vpss_sdm_cb_done(struct cvi_dwa_vdev *wdev);

#endif /* _GDC_H_ */
