#ifndef MODULES_SYS_INCLUDE_BASE_H_
#define MODULES_SYS_INCLUDE_BASE_H_


#include <linux/videodev2.h>
#include <sys/queue.h>
//#include <semaphore.h>
#include "linux/types.h"
#include "FreeRTOS_POSIX.h"
#include "FreeRTOS_POSIX/semaphore.h"
//#include <errno.h>
#include <FreeRTOS_POSIX/errno.h>
//#include <linux/ion.h>
//#include <linux/ion_cvitek.h>

#include "cvi_common.h"
#include "cvi_comm_sys.h"
#include "cvi_comm_vb.h"
#include "cvi_vi.h"
#include "cvi_vpss.h"
#include "cvi_vo.h"
#include "cvi_region.h"

#include <cvi_vip.h>
#include <cvi_vip_sc.h>
#include <cvi_vip_dwa.h>
#include <cvi_vip_disp.h>

#include <stdatomic.h>

#ifdef CVI_DEBUG
#define VPU_PROFILE
#endif

#define FPGA_EARLY_PORTING

#define CVI_VB_MAGIC 0xbabeface

#define BIT(nr)      (UINT64_C(1) << (nr))
#define VIP_ALIGN(x) (((x) + 0x1F) & ~0x1F)	// for 32byte alignment
#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define NUM_OF_PLANES      3

#define IS_VDEV_CLOSED(x) ((x) == VDEV_STATE_CLOSED)
#define IS_VDEV_OPEN(x) ((x) == VDEV_STATE_OPEN)
#define IS_VDEV_RUN(x) ((x) == VDEV_STATE_RUN)
#define IS_VDEV_STOP(x) ((x) == VDEV_STATE_STOP)

#define BASE_SHARE_MEM_SIZE         (0xA0000)
#define VI_SHARE_MEM_SIZE           (0x3000)
#define VPSS_SHARE_MEM_SIZE         (0x4000)
#define VO_SHARE_MEM_SIZE           (0x1000)

#define BASE_VB_COMM_POOL_OFFSET    (0x10)
#define BASE_VB_BLK_MOD_ID_OFFSET   (BASE_VB_COMM_POOL_OFFSET + VB_COMM_POOL_RSV_SIZE)
#define BASE_LOG_LEVEL_OFFSET       (BASE_VB_BLK_MOD_ID_OFFSET + VB_BLK_MOD_ID_RSV_SIZE * VB_MAX_POOLS)
#define BASE_BIND_INFO_OFFSET       (BASE_LOG_LEVEL_OFFSET + LOG_LEVEL_RSV_SIZE)
#define BASE_VERSION_INFO_OFFSET    (BASE_BIND_INFO_OFFSET + BIND_INFO_RSV_SIZE)

#define VB_COMM_POOL_RSV_SIZE       (sizeof(VB_POOL_S) * VB_MAX_POOLS)
#define VB_BLK_MOD_ID_RSV_SIZE      (sizeof(CVI_U64) * VB_POOL_MAX_BLK) //per pool
#define LOG_LEVEL_RSV_SIZE          (sizeof(CVI_S32) * CVI_ID_BUTT)
#define BIND_INFO_RSV_SIZE          (sizeof(BIND_NODE_S) * BIND_NODE_MAXNUM)
#define VERSION_INFO_RSV_SIZE       (sizeof(MMF_VERSION_S))


#define ISP_CHECK_PIPE(pipe)                                                                                           \
	do {                                                                                                           \
		if (((pipe) < 0) || ((pipe) >= VI_MAX_PIPE_NUM)) {                                                     \
			return -ENODEV;                                                                                \
		}                                                                                                      \
	} while (0)

#define CVI_TRACE_ID(level, id, fmt, ...)                                           \
	CVI_TRACE(level, id, "%s:%d:%s(): " fmt, __FILENAME__, __LINE__, __func__, ##__VA_ARGS__)

#define MOD_CHECK_NULL_PTR(id, ptr)                                                                                   \
	do {                                                                                                           \
		if (!(ptr)) {                                                                                          \
			CVI_TRACE_ID(CVI_DBG_ERR, id, #ptr " NULL pointer\n");                                         \
			return CVI_DEF_ERR(id, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR);                                   \
		}                                                                                                      \
	} while (0)

/* [FIFO] is implemented by the idea of circular-array.
 *
 * FIFO_HEAD: Declare new struct.
 * FIFO_INIT: Initialize FIFO.
 * FIFO_EXIT: Finalize FIFO.
 * FIFO_EMPTY: If FIFO is empty.
 * FIFO_CAPACITY: How many elements can be placed in FIFO.
 * FIFO_SIZE: Number of elements in FIFO.
 * FIFO_FULL: If FIFO is full.
 * FIFO_PUSH: PUSH elm into FIFO's tail. overwritren old data if full.
 * FIFO_POP: POP elm from FIFO's front.
 * FIFO_FOREACH: For-loop go through FIFO.
 */
#ifdef __arm__
#define FIFO_HEAD(name, type)						\
	struct name {							\
		struct type *fifo;					\
		__u32 padding;						\
		int front, tail, capacity;				\
	} __attribute__((aligned(8)))
#else
#define FIFO_HEAD(name, type)						\
	struct name {							\
		struct type *fifo;					\
		int front, tail, capacity;				\
	}
#endif

#define FIFO_INIT(head, _capacity) do {					\
	(head)->fifo = malloc(sizeof(*(head)->fifo) * _capacity);	\
	(head)->front = (head)->tail = -1;				\
	(head)->capacity = _capacity;					\
} while (0)

#define FIFO_EXIT(head) do {						\
	(head)->front = (head)->tail = -1;				\
	(head)->capacity = 0;						\
	if ((head)->fifo)						\
		free((head)->fifo);					\
} while (0)

#define FIFO_EMPTY(head)    ((head)->front == -1)

#define FIFO_FULL(head)     (((head)->front == ((head)->tail + 1))	\
			    || (((head)->front == 0) && ((head)->tail == ((head)->capacity - 1))))

#define FIFO_CAPACITY(head) ((head)->capacity)

#define FIFO_SIZE(head)     (FIFO_EMPTY(head) ?\
	0 : ((((head)->tail + (head)->capacity - (head)->front) % (head)->capacity) + 1))

#define FIFO_PUSH(head, elm) do {					\
	if (FIFO_EMPTY(head))						\
		(head)->front = (head)->tail = 0;			\
	else								\
		(head)->tail = ((head)->tail == (head)->capacity - 1)	\
				? 0 : (head)->tail + 1;			\
	(head)->fifo[(head)->tail] = elm;				\
} while (0)

#define FIFO_POP(head, pelm) do {					\
	*(pelm) = (head)->fifo[(head)->front];				\
	if ((head)->front == (head)->tail)				\
		(head)->front = (head)->tail = -1;			\
	else								\
		(head)->front = ((head)->front == (head)->capacity - 1)	\
				? 0 : (head)->front + 1;		\
} while (0)

#define FIFO_FOREACH(var, head, idx)					\
	for (idx = (head)->front, var = (head)->fifo[idx];		\
		idx != (head)->tail;					\
		idx = (idx + 1) % (head)->capacity, var = (head)->fifo[idx])

#define FIFO_GET_FRONT(head, pelm) (*(pelm) = (head)->fifo[(head)->front])

#define FIFO_GET_TAIL(head, pelm) (*(pelm) = (head)->fifo[(head)->tail])

/* This macro permits both remove and free var within the loop safely.*/
#ifndef TAILQ_FOREACH_SAFE
#define TAILQ_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = TAILQ_FIRST((head));				\
		(var) && ((tvar) = TAILQ_NEXT((var), field), 1);	\
		(var) = (tvar))
#endif

#define CHN_MATCH(x, y) (((x)->enModId == (y)->enModId) && ((x)->s32DevId == (y)->s32DevId)             \
	&& ((x)->s32ChnId == (y)->s32ChnId))

#define CVI_VI_VPSS_EXTRA_BUF 0

#define NUM_OF_ISP_0_BUFFER (2 + CVI_VI_VPSS_EXTRA_BUF)
#define NUM_OF_ISP_1_BUFFER (2 + CVI_VI_VPSS_EXTRA_BUF)
#define NUM_OF_ISP_BUFFER   (NUM_OF_ISP_0_BUFFER + NUM_OF_ISP_1_BUFFER)

enum vdev_type {
	VDEV_TYPE_ISP = 0,
	VDEV_TYPE_IMG,
	VDEV_TYPE_SC,
	VDEV_TYPE_DISP,
	VDEV_TYPE_DWA,
	VDEV_TYPE_MAX,
};

enum vdev_state {
	VDEV_STATE_CLOSED = 0,
	VDEV_STATE_OPEN,
	VDEV_STATE_RUN,
	VDEV_STATE_STOP,
	VDEV_STATE_MAX,
};

enum CHN_TYPE_E {
	CHN_TYPE_IN = 0,
	CHN_TYPE_OUT,
	CHN_TYPE_MAX
};

// start point is included.
// end point is excluded.
struct crop_size {
	CVI_U16  start_x;
	CVI_U16  start_y;
	CVI_U16  end_x;
	CVI_U16  end_y;
};

struct buffer {
	CVI_U64 phy_addr[NUM_OF_PLANES];
	size_t length[NUM_OF_PLANES];
	CVI_U32 stride[NUM_OF_PLANES];
	SIZE_S size;
	CVI_U64 u64PTS;
	uint8_t dev_num;
	PIXEL_FORMAT_E enPixelFormat;
	CVI_U32 frm_num;
	struct crop_size frame_crop;
};

/*
 * vb_pool: the pool this blk belongs to.
 * phy_addr: physical address of the blk.
 * vir_addr: virtual address of the blk.
 * usr_cnt: ref-count of the blk.
 * buf: the usage which define planes of buffer.
 * magic: magic number to avoid wrong reference.
 * mod_ids: the users of this blk. BIT(MOD_ID) will be set is MOD using.
 */
typedef struct vb_s {
	VB_POOL vb_pool;
	CVI_U64 phy_addr;
	void *vir_addr;
	atomic_uint usr_cnt;
	struct buffer buf;
	CVI_U32 magic;
	CVI_U64 *mod_ids;
	CVI_BOOL external;
} VB_S;

FIFO_HEAD(vbq, vb_s*);

typedef struct _pool {
	VB_POOL poolID;
	CVI_S16 ownerID;
	CVI_U64 memBase;
	void *vmemBase;
#ifdef __arm__
	__u32 padding; /* padding for keeping same size of this structure */
#endif
	struct vbq freeList;
	VB_POOL_CONFIG_S config;
	CVI_BOOL bIsCommPool;
	CVI_U32 u32FreeBlkCnt;
	CVI_U32 u32MinFreeBlkCnt;
	CVI_CHAR acPoolName[MAX_VB_POOL_NAME_LEN];
} __attribute__((aligned(8))) VB_POOL_S;

struct snap_s {
	TAILQ_ENTRY(snap_s) tailq;

	pthread_cond_t cond;
	MMF_CHN_S chn;
	VB_BLK blk;
};

/*
 * lock: lock for waitq/workq.
 * waitq: the queue of VB_BLK waiting to be done. For TDM modules, such as VPSS.
 * workq: the queue of VB_BLK module is working on.
 * dlock: lock for doneq.
 * doneq: the queue of VB_BLK to be taken. Size decided by u32Depth.
 * sem: sem to notify waitq is updated.
 * snap_jobs: the req to get frame for this chn.
 */
typedef struct _vb_jobs_t {
	pthread_mutex_t lock;
	struct vbq waitq;
	struct vbq workq;
	pthread_mutex_t dlock;
	struct vbq doneq;
	sem_t sem;
	TAILQ_HEAD(snap_q, snap_s) snap_jobs;
} vb_jobs_t;

typedef CVI_S32(*vb_acquire_fp)(MMF_CHN_S);

struct vb_req {
	STAILQ_ENTRY(vb_req) stailq;

	vb_acquire_fp fp;
	MMF_CHN_S chn;
};

struct cvi_venc_vb_ctx {
	CVI_BOOL enable_bind_mode;
	CVI_BOOL currBindMode;
	pthread_t thread;
	vb_jobs_t vb_jobs;
	CVI_BOOL pause;
};

struct cvi_vdec_vb_ctx {
	CVI_BOOL enable_bind_mode;
	CVI_BOOL currBindMode;
	pthread_t thread;
	vb_jobs_t vb_jobs;
	CVI_BOOL pause;
};

extern struct cvi_venc_vb_ctx venc_vb_ctx[VENC_MAX_CHN_NUM];
extern struct cvi_vdec_vb_ctx vdec_vb_ctx[VENC_MAX_CHN_NUM];

enum GDC_USAGE {
	GDC_USAGE_ROTATION,
	GDC_USAGE_FISHEYE,
	GDC_USAGE_LDC,
	GDC_USAGE_MAX
};

struct cvi_gdc_mesh {
	CVI_U64 paddr;
	CVI_VOID *vaddr;
	pthread_mutex_t lock;
};


struct cvi_vi_ctx {
	CVI_U8   total_chn_num;
	CVI_BOOL is_enable[VI_MAX_CHN_NUM + VI_MAX_EXT_CHN_NUM];
	vb_jobs_t vb_jobs[VI_MAX_CHN_NUM];
	pthread_t thread;
	CVI_BOOL isDevEnable[VI_MAX_DEV_NUM];

	// mod param
	VI_MOD_PARAM_S modParam;

	// dev
	VI_DEV_ATTR_S devAttr[VI_MAX_DEV_NUM];
	VI_DEV_ATTR_EX_S devAttrEx[VI_MAX_DEV_NUM];
	VI_DEV_TIMING_ATTR_S stTimingAttr[VI_MAX_DEV_NUM];
	VI_DEV_BIND_PIPE_S devBindPipeAttr[VI_MAX_DEV_NUM];

	// pipe
	CVI_BOOL isPipeCreated[VI_MAX_PIPE_NUM];
	VI_PIPE_FRAME_SOURCE_E enSource[VI_MAX_PIPE_NUM];
	VI_PIPE_ATTR_S pipeAttr[VI_MAX_PIPE_NUM];
	CROP_INFO_S    pipeCrop[VI_MAX_PIPE_NUM];
	VI_DUMP_ATTR_S dumpAttr[VI_MAX_PIPE_NUM];

	// chn
	VI_CHN_ATTR_S chnAttr[VI_MAX_CHN_NUM];
	VI_CHN_STATUS_S chnStatus[VI_MAX_CHN_NUM];
	VI_CROP_INFO_S chnCrop[VI_MAX_CHN_NUM];
	ROTATION_E enRotation[VI_MAX_CHN_NUM];
	VI_LDC_ATTR_S stLDCAttr[VI_MAX_CHN_NUM];
	VI_EARLY_INTERRUPT_S enEalyInt[VI_MAX_CHN_NUM];

	CVI_U32 blk_size[VI_MAX_CHN_NUM];
	CVI_U32 timeout_cnt;
	struct cvi_gdc_mesh mesh[VI_MAX_CHN_NUM][GDC_USAGE_MAX];

	VI_CHN chn_bind[VI_MAX_CHN_NUM][VI_MAX_EXTCHN_BIND_PER_CHN];
	struct vbq extchn_doneq[VI_MAX_EXT_CHN_NUM];
	VI_EXT_CHN_ATTR_S stExtChnAttr[VI_MAX_EXT_CHN_NUM];
	FISHEYE_ATTR_S stFishEyeAttr[VI_MAX_EXT_CHN_NUM];

	CVI_U8 bypass_frm[VI_MAX_CHN_NUM];
};

struct cvi_vi_proc_ctx {
	CVI_U8   total_chn_num;
	CVI_BOOL is_enable[VI_MAX_CHN_NUM + VI_MAX_EXT_CHN_NUM];
	CVI_BOOL isDevEnable[VI_MAX_DEV_NUM];

	// mod param
	VI_MOD_PARAM_S modParam;

	// dev
	VI_DEV_ATTR_S devAttr[VI_MAX_DEV_NUM];
	VI_DEV_BIND_PIPE_S devBindPipeAttr[VI_MAX_DEV_NUM];
	VI_DEV_TIMING_ATTR_S stTimingAttr[VI_MAX_DEV_NUM];

	// chn
	VI_CHN_ATTR_S chnAttr[VI_MAX_CHN_NUM];
	VI_CHN_STATUS_S chnStatus[VI_MAX_CHN_NUM];
	VI_CROP_INFO_S chnCrop[VI_MAX_CHN_NUM];
	VI_EARLY_INTERRUPT_S enEalyInt[VI_MAX_CHN_NUM];
};

extern struct cvi_vi_proc_ctx *vi_prc_ctx;
extern struct cvi_vi_ctx vi_ctx;

struct VPSS_CHN_HW_CFG {
	struct v4l2_rect rect_crop;
	struct v4l2_rect rect_output;
	struct cvi_sc_border_param border_param;
	struct cvi_sc_quant_param quantCfg;
	struct cvi_rgn_cfg rgn_cfg;
	enum cvi_sc_scaling_coef coef;
	struct cvi_csc_cfg csc_cfg;
	CVI_U32 bytesperline[2];
};

struct VPSS_GRP_HW_CFG {
	struct VPSS_CHN_HW_CFG stChnCfgs[VPSS_MAX_CHN_NUM];

	struct cvi_csc_cfg csc_cfg;
	struct v4l2_rect rect_crop;
	CVI_U32 bytesperline[2];
};

struct VPSS_CHN_CFG {
	CVI_BOOL isEnabled;
	VPSS_CHN_ATTR_S stChnAttr;
	VPSS_CROP_INFO_S stCropInfo;
	ROTATION_E enRotation;
	vb_jobs_t vb_jobs;
	CVI_U32 blk_size;
	CVI_U32 align;
	RGN_HANDLE rgn_handle[RGN_MAX_NUM_VPSS];
	VPSS_SCALE_COEF_E enCoef;
	CVI_FLOAT YRatio;
	VPSS_LDC_ATTR_S stLDCAttr;

	// hw cfgs;
	CVI_BOOL is_cfg_changed;
};

struct cvi_vpss_ctx {
	CVI_BOOL isCreated;
	CVI_BOOL isStarted;
	VPSS_GRP_ATTR_S stGrpAttr;
	VPSS_CROP_INFO_S stGrpCropInfo;
	CVI_U8 chnNum;
	struct VPSS_CHN_CFG stChnCfgs[VPSS_MAX_CHN_NUM];
	vb_jobs_t vb_jobs;
	CVI_S32 proc_amp[PROC_AMP_MAX];
	struct crop_size frame_crop;

	// hw cfgs;
	CVI_BOOL is_cfg_changed;
};

struct VPSS_PROC_CHN_CFG {
	CVI_BOOL isEnabled;
	VPSS_CHN_ATTR_S stChnAttr;
	VPSS_CROP_INFO_S stCropInfo;
	ROTATION_E enRotation;
	VPSS_LDC_ATTR_S stLDCAttr;
	CVI_U32 u32SendOk; // send OK cnt after latest chn enable
	CVI_U64 u64EnableTime; // latest chn enable time (us)
	CVI_U32 u32RealFrameRate; // chn real time frame rate
};

struct cvi_vpss_proc_ctx {
	CVI_BOOL isCreated;
	CVI_BOOL isStarted;
	VPSS_GRP_ATTR_S stGrpAttr;
	VPSS_CROP_INFO_S stGrpCropInfo;
	CVI_U8 chnNum;
	struct VPSS_PROC_CHN_CFG stChnCfgs[VPSS_MAX_CHN_NUM];
	VPSS_GRP_WORK_STATUS_S stGrpWorkStatus;
};

extern struct cvi_vpss_ctx vpssCtx[VPSS_MAX_GRP_NUM];
extern struct VPSS_GRP_HW_CFG vpss_hw_cfgs[VPSS_MAX_GRP_NUM];
extern struct cvi_vpss_proc_ctx *vpssPrcCtx;

struct cvi_vo_ctx {
	CVI_BOOL is_dev_enable[VO_MAX_DEV_NUM];
	CVI_BOOL is_layer_enable[VO_MAX_LAYER_NUM];
	CVI_BOOL is_chn_enable[VO_MAX_LAYER_NUM][VO_MAX_CHN_NUM];

	// dev
	VO_PUB_ATTR_S stPubAttr;

	// layer
	VO_VIDEO_LAYER_ATTR_S stLayerAttr;
	CVI_U32 u32DisBufLen;
	CVI_S32 proc_amp[PROC_AMP_MAX];

	// chn
	VO_CHN_ATTR_S stChnAttr;
	ROTATION_E enRotation;
	RGN_HANDLE rgn_handle[RGN_MAX_NUM_VO];
	struct cvi_rgn_cfg rgn_cfg;
	struct {
		CVI_U64 paddr;
		CVI_VOID *vaddr;
	} mesh;
	pthread_t thread[VO_MAX_LAYER_NUM][VO_MAX_CHN_NUM];
	vb_jobs_t vb_jobs;
	CVI_BOOL pause;
};

struct cvi_vo_proc_ctx {
	CVI_BOOL is_dev_enable[VO_MAX_DEV_NUM];
	CVI_BOOL is_layer_enable[VO_MAX_LAYER_NUM];
	CVI_BOOL is_chn_enable[VO_MAX_LAYER_NUM][VO_MAX_CHN_NUM];

	// dev
	VO_PUB_ATTR_S stPubAttr[VO_MAX_DEV_NUM];

	// layer
	VO_VIDEO_LAYER_ATTR_S stLayerAttr[VO_MAX_LAYER_NUM];
	CVI_U32 u32DisBufLen[VO_MAX_LAYER_NUM];
	CVI_S32 proc_amp[VO_MAX_LAYER_NUM][PROC_AMP_MAX];

	// chn
	VO_CHN_ATTR_S stChnAttr[VO_MAX_LAYER_NUM][VO_MAX_CHN_NUM];
	ROTATION_E enRotation[VO_MAX_LAYER_NUM][VO_MAX_CHN_NUM];
	CVI_BOOL pause[VO_MAX_LAYER_NUM][VO_MAX_CHN_NUM];
	CVI_BOOL show[VO_MAX_LAYER_NUM][VO_MAX_CHN_NUM];
	CVI_U64 u64DisplayPts[VO_MAX_LAYER_NUM][VO_MAX_CHN_NUM];
	CVI_U64 u64PreDonePts[VO_MAX_LAYER_NUM][VO_MAX_CHN_NUM];

	// for calculating chn frame rate
	CVI_U32 u32frameCnt[VO_MAX_LAYER_NUM][VO_MAX_CHN_NUM]; // frame cnt after latest chn enable
	CVI_U64 u64EnableTime[VO_MAX_LAYER_NUM][VO_MAX_CHN_NUM]; // latest chn enable time
	CVI_U32 u32RealFrameRate[VO_MAX_LAYER_NUM][VO_MAX_CHN_NUM];
};

extern struct cvi_vo_ctx vo_ctx;
extern struct cvi_vo_proc_ctx *vo_prc_ctx;

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

/* dis_info: pass dis crop infor to VI.
 *
 * sensor_num: define which sensor.
 * frm_num: define the crop info is for which frame.
 * crop_size: dis crop size info.
 */
struct dis_info {
	CVI_U8  sensor_num;
	CVI_U32 frm_num;
	struct crop_size dis_i;
};

struct vdev {
	char name[16];
	CVI_S32 fd;
	enum v4l2_buf_type type;
	CVI_U8 numOfBuffers;
	CVI_U8 availIndex;
	CVI_U8 numOfPlanes;
	enum vdev_state state;
	bool is_online;
};

CVI_S32 v4l2_isp_open(void);
CVI_S32 v4l2_isp_close(void);
CVI_S32 v4l2_dev_open(void);
CVI_S32 v4l2_dev_close(void);
CVI_S32 vpss_close(void);
CVI_S32 vo_close(void);

struct vdev *get_dev_info(CVI_U8 type, CVI_U8 dev_id);
struct vdev *get_dev_info_by_chn(MMF_CHN_S chn, enum CHN_TYPE_E chn_type);
CVI_S32 v4l2_get_frame_info(PIXEL_FORMAT_E fmt, SIZE_S size, struct buffer *buf, CVI_U64 mem_base);
CVI_S32 v4l2_getfmt(CVI_S32 fd, struct v4l2_format *fmt);
CVI_S32 v4l2_setfmt(struct vdev *d, CVI_S32 width, CVI_S32 height, enum v4l2_buf_type type, CVI_U32 pxlfmt);
int open_device(const char *dev_name, CVI_S32 *fd);
CVI_S32 close_device(CVI_S32 *fd);
CVI_S32 v4l2_reqbufs(CVI_S32 fd, CVI_S32 count, enum v4l2_buf_type type);
CVI_S32 v4l2_qbuf(struct vdev *d, enum v4l2_buf_type type, struct buffer *buf);
CVI_S32 v4l2_dqbuf(struct vdev *d, enum v4l2_buf_type type, struct v4l2_buffer *buf);
CVI_S32 v4l2_streamon(struct vdev *d, enum v4l2_buf_type type);
CVI_S32 v4l2_streamoff(struct vdev *d, enum v4l2_buf_type type);
void mod_jobs_init(vb_jobs_t *jobs, CVI_U8 waitq_depth, CVI_U8 workq_depth, CVI_U8 doneq_depth);
void mod_jobs_exit(vb_jobs_t *jobs);
CVI_S32 mod_jobs_enque_work(struct vdev *d, vb_jobs_t *jobs);
CVI_S32 mod_jobs_enque_work2(struct vdev *d, vb_jobs_t *jobs);
CVI_BOOL mod_jobs_waitq_empty(vb_jobs_t *jobs);
CVI_BOOL mod_jobs_workq_empty(vb_jobs_t *jobs);
VB_BLK mod_jobs_waitq_pop(vb_jobs_t *jobs);
VB_BLK mod_jobs_workq_pop(vb_jobs_t *jobs);
vb_jobs_t *get_jobs_by_chn(MMF_CHN_S chn, enum CHN_TYPE_E chn_type);
CVI_S32 vb_qbuf(MMF_CHN_S chn, enum CHN_TYPE_E chn_type, VB_BLK blk);
CVI_S32 vb_dqbuf(MMF_CHN_S chn, enum CHN_TYPE_E chn_type, VB_BLK *blk);
CVI_S32 vb_done_handler(MMF_CHN_S chn, enum CHN_TYPE_E chn_type, VB_BLK blk);

CVI_S32 init_isp_device(void);
CVI_S32 init_vpss_device(void);
CVI_S32 init_sc_device(CVI_S32 dev);
CVI_S32 init_disp_device(void);
CVI_S32 init_dwa_device(void);

CVI_S32 get_chn_buffer(MMF_CHN_S chn, VB_BLK *blk, CVI_S32 timeout_ms);
CVI_U32 v4l2_remap_pxlfmt(PIXEL_FORMAT_E pxlfmt);
CVI_S32 v4l2_set_sel(CVI_S32 fd, enum v4l2_buf_type type, CVI_U32 target, struct v4l2_rect *rect);
CVI_S32 v4l2_sel_input(CVI_S32 fd, CVI_S32 input_idx);

long get_diff_in_us(struct timespec t1, struct timespec t2);
RECT_S aspect_ratio_resize(SIZE_S in, SIZE_S out);

CVI_S32 base_fill_videoframe2buffer(MMF_CHN_S chn, const VIDEO_FRAME_INFO_S *pstVideoFrame,
	struct buffer *buf);

CVI_VOID CVI_VB_AcquireBlock(vb_acquire_fp fp, MMF_CHN_S chn);
void CVI_VPSS_PostJob(CVI_U8 vpss_dev);
VB_BLK CVI_VB_GetBlockwithID(VB_POOL Pool, CVI_U32 u32BlkSize, MOD_ID_E modId);

CVI_VOID CVI_VI_SET_DIS_INFO(struct dis_info dis_i);
CVI_S32 CVI_VI_SetBypassFrm(CVI_U32 snr_num, CVI_U8 bypass_num);

void *base_get_shm(void);
void base_release_shm(void);

#endif // MODULES_SYS_INCLUDE_BASE_H_
