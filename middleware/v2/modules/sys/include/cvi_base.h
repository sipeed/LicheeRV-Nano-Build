#ifndef MODULES_SYS_INCLUDE_BASE_H_
#define MODULES_SYS_INCLUDE_BASE_H_

#include <stdbool.h>
#include <stdint.h>
#include <sys/queue.h>
#include <semaphore.h>
#include <errno.h>
#include <pthread.h>

#include <linux/cvi_common.h>
#include <linux/cvi_comm_sys.h>
#include "cvi_vi.h"
#include "cvi_vpss.h"

#include <linux/cvi_vip_sc.h>
#include <linux/cvi_vip_dwa.h>
#include <linux/vo_disp.h>

#include <stdatomic.h>


#ifdef CVI_DEBUG
#define VPU_PROFILE
#endif

#define BIT(nr)      (UINT64_C(1) << (nr))

#define IS_VDEV_CLOSED(x) ((x) == VDEV_STATE_CLOSED)
#define IS_VDEV_OPEN(x) ((x) == VDEV_STATE_OPEN)
#define IS_VDEV_RUN(x) ((x) == VDEV_STATE_RUN)
#define IS_VDEV_STOP(x) ((x) == VDEV_STATE_STOP)

#if defined(ARCH_CV183X)
#define VPSS_GRP_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_RGB_888_PLANAR) || (fmt == PIXEL_FORMAT_BGR_888_PLANAR) ||	\
	 (fmt == PIXEL_FORMAT_RGB_888) || (fmt == PIXEL_FORMAT_BGR_888) ||			\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_420) || (fmt == PIXEL_FORMAT_YUV_PLANAR_422) ||	\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_444) || (fmt == PIXEL_FORMAT_YUV_400))
#define VPSS_CHN_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_RGB_888_PLANAR) || (fmt == PIXEL_FORMAT_BGR_888_PLANAR) ||	\
	 (fmt == PIXEL_FORMAT_RGB_888) || (fmt == PIXEL_FORMAT_BGR_888) ||			\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_420) || (fmt == PIXEL_FORMAT_YUV_PLANAR_422) ||	\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_444) || (fmt == PIXEL_FORMAT_YUV_400) ||		\
	 (fmt == PIXEL_FORMAT_HSV_888) || (fmt == PIXEL_FORMAT_HSV_888_PLANAR))
#define VO_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_RGB_888_PLANAR) || (fmt == PIXEL_FORMAT_BGR_888_PLANAR) ||	\
	 (fmt == PIXEL_FORMAT_RGB_888) || (fmt == PIXEL_FORMAT_BGR_888) ||			\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_420) || (fmt == PIXEL_FORMAT_YUV_PLANAR_422) ||	\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_444) || (fmt == PIXEL_FORMAT_YUV_400))
#define GDC_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_RGB_888_PLANAR) || (fmt == PIXEL_FORMAT_BGR_888_PLANAR) ||	\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_420) || (fmt == PIXEL_FORMAT_YUV_400))
#elif defined(ARCH_CV182X) || defined(__SOC_MARS__) || defined(__SOC_PHOBOS__)
#define VPSS_GRP_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_RGB_888_PLANAR) || (fmt == PIXEL_FORMAT_BGR_888_PLANAR) ||	\
	 (fmt == PIXEL_FORMAT_RGB_888) || (fmt == PIXEL_FORMAT_BGR_888) ||			\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_420) || (fmt == PIXEL_FORMAT_YUV_PLANAR_422) ||	\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_444) || (fmt == PIXEL_FORMAT_YUV_400) ||		\
	 (fmt == PIXEL_FORMAT_NV12) || (fmt == PIXEL_FORMAT_NV21) ||				\
	 (fmt == PIXEL_FORMAT_NV16) || (fmt == PIXEL_FORMAT_NV61) ||				\
	 (fmt == PIXEL_FORMAT_YUYV) || (fmt == PIXEL_FORMAT_UYVY) ||				\
	 (fmt == PIXEL_FORMAT_YVYU) || (fmt == PIXEL_FORMAT_VYUY))
#define VPSS_CHN_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_RGB_888_PLANAR) || (fmt == PIXEL_FORMAT_BGR_888_PLANAR) ||	\
	 (fmt == PIXEL_FORMAT_RGB_888) || (fmt == PIXEL_FORMAT_BGR_888) ||			\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_420) || (fmt == PIXEL_FORMAT_YUV_PLANAR_422) ||	\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_444) || (fmt == PIXEL_FORMAT_YUV_400) ||		\
	 (fmt == PIXEL_FORMAT_HSV_888) || (fmt == PIXEL_FORMAT_HSV_888_PLANAR) ||		\
	 (fmt == PIXEL_FORMAT_NV12) || (fmt == PIXEL_FORMAT_NV21) ||				\
	 (fmt == PIXEL_FORMAT_NV16) || (fmt == PIXEL_FORMAT_NV61) ||				\
	 (fmt == PIXEL_FORMAT_YUYV) || (fmt == PIXEL_FORMAT_UYVY) ||				\
	 (fmt == PIXEL_FORMAT_YVYU) || (fmt == PIXEL_FORMAT_VYUY))
#define VO_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_RGB_888_PLANAR) || (fmt == PIXEL_FORMAT_BGR_888_PLANAR) ||	\
	 (fmt == PIXEL_FORMAT_RGB_888) || (fmt == PIXEL_FORMAT_BGR_888) ||			\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_420) || (fmt == PIXEL_FORMAT_YUV_PLANAR_422) ||	\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_444) || (fmt == PIXEL_FORMAT_YUV_400) ||		\
	 (fmt == PIXEL_FORMAT_NV12) || (fmt == PIXEL_FORMAT_NV21) ||				\
	 (fmt == PIXEL_FORMAT_NV16) || (fmt == PIXEL_FORMAT_NV61) ||				\
	 (fmt == PIXEL_FORMAT_YUYV) || (fmt == PIXEL_FORMAT_UYVY) ||				\
	 (fmt == PIXEL_FORMAT_YVYU) || (fmt == PIXEL_FORMAT_VYUY))
#define GDC_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_NV12) || (fmt == PIXEL_FORMAT_NV21) ||				       \
	 (fmt == PIXEL_FORMAT_YUV_400))
#else
#error "ARCH not defined"
#endif

#define BASE_LOG_LEVEL_OFFSET       (0x10)
#define BASE_BIND_INFO_OFFSET       (BASE_LOG_LEVEL_OFFSET + LOG_LEVEL_RSV_SIZE)
#define BASE_VERSION_INFO_OFFSET    (BASE_BIND_INFO_OFFSET + BIND_INFO_RSV_SIZE)

#define LOG_LEVEL_RSV_SIZE          (sizeof(CVI_S32) * CVI_ID_BUTT)
#define BIND_INFO_RSV_SIZE          (sizeof(BIND_NODE_S) * BIND_NODE_MAXNUM)
#define VERSION_INFO_RSV_SIZE       (sizeof(MMF_VERSION_S))

#define BASE_SHARE_MEM_SIZE         ALIGN(BASE_VERSION_INFO_OFFSET + VERSION_INFO_RSV_SIZE, 0x1000)

#define BASE_DEV_NAME "/dev/cvi-base"
#define SYS_DEV_NAME  "/dev/cvi-sys"
#define VI_DEV_NAME   "/dev/cvi-vi"
#define VO_DEV_NAME   "/dev/cvi-vo"
#define RGN_DEV_NAME  "/dev/cvi-rgn"

#define ISP_CHECK_PIPE(pipe)                                                                                           \
	do {                                                                                                           \
		if (((pipe) < 0) || ((pipe) >= VI_MAX_PIPE_NUM)) {                                                     \
			return -ENODEV;                                                                                \
		}                                                                                                      \
	} while (0)

#define MOD_CHECK_NULL_PTR(id, ptr)                                                                                   \
	do {                                                                                                           \
		if (!(ptr)) {                                                                                          \
			CVI_TRACE_ID(CVI_DBG_ERR, id, #ptr " NULL pointer\n");                                         \
			return CVI_DEF_ERR(id, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR);                                   \
		}                                                                                                      \
	} while (0)

#define STAILQ_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = STAILQ_FIRST((head));				\
		(var) && ((tvar) = STAILQ_NEXT((var), field), 1);	\
		(var) = (tvar))

#define Mo_Table_Size 256
#define DEFAULT_MESH_PADDR	0x80000000

enum vdev_type {
	VDEV_TYPE_ISP = 0,
	VDEV_TYPE_VPSS,
	VDEV_TYPE_DISP,
	VDEV_TYPE_DWA,
	VDEV_TYPE_RGN,
	VDEV_TYPE_MAX,
};

enum vdev_state {
	VDEV_STATE_CLOSED = 0,
	VDEV_STATE_OPEN,
	VDEV_STATE_RUN,
	VDEV_STATE_STOP,
	VDEV_STATE_MAX,
};

// start point is included.
// end point is excluded.
struct crop_size {
	CVI_U16  start_x;
	CVI_U16  start_y;
	CVI_U16  end_x;
	CVI_U16  end_y;
};

enum GDC_USAGE {
	GDC_USAGE_ROTATION,
	GDC_USAGE_FISHEYE,
	GDC_USAGE_LDC,
	GDC_USAGE_MAX
};

struct cvi_gdc_mesh {
	CVI_U64 paddr;
	CVI_VOID *vaddr;
	CVI_U32 meshSize;
	pthread_mutex_t lock;
};

/* dis_info: pass dis crop infor to VI.
 *
 * sensor_num: define which sensor.
 * frm_num: define the crop info is for which frame.
 * crop_size: dis crop size info.
 */
// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
struct dis_info {
	CVI_U8  sensor_num;
	CVI_U32 frm_num;
	struct crop_size dis_i;
};
// -------- If you want to change these interfaces, please contact the isp team. --------

// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
struct mlv_info {
	CVI_U8  sensor_num;
	CVI_U32 frm_num;
	CVI_U8  mlv;
	CVI_U8  mtable[Mo_Table_Size];
};
// -------- If you want to change these interfaces, please contact the isp team. --------

// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
struct vdev {
	char name[16];
	CVI_S32 fd;
	CVI_U8 numOfBuffers;
	CVI_U8 availIndex;
	CVI_U8 numOfPlanes;
	enum vdev_state state;
	bool is_online;
};
// -------- If you want to change these interfaces, please contact the isp team. --------

typedef struct {
	CVI_S32 proc_amp[PROC_AMP_MAX];
} VPSS_BIN_DATA;
typedef struct {
	VPSS_BIN_DATA vpss_bin_data[VPSS_MAX_GRP_NUM];
} VPSS_PARAMETER_BUFFER;

CVI_S32 vpss_close(void);
CVI_S32 vo_close(void);

// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
struct vdev *get_dev_info(CVI_U8 type, CVI_U8 dev_id);
// -------- If you want to change these interfaces, please contact the isp team. --------
int open_device(const char *dev_name, CVI_S32 *fd);
CVI_S32 close_device(CVI_S32 *fd);

CVI_S32 base_dev_open(CVI_VOID);
CVI_S32 base_dev_close(CVI_VOID);
CVI_S32 get_base_fd(CVI_VOID);

long get_diff_in_us(struct timespec t1, struct timespec t2);

// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
CVI_VOID CVI_VI_SET_DIS_INFO(struct dis_info dis_i);
CVI_S32 CVI_VI_SetBypassFrm(CVI_U32 snr_num, CVI_U8 bypass_num);
CVI_VOID CVI_VI_SetMotionLV(struct mlv_info mlevel_i);
// -------- If you want to change these interfaces, please contact the isp team. --------

void *base_get_shm(void);
void base_release_shm(void);

CVI_S32 CVI_VPSS_Suspend(void);
CVI_S32 CVI_VPSS_Resume(void);
CVI_S32 CVI_GDC_Suspend(void);
CVI_S32 CVI_GDC_Resume(void);
CVI_S32 CVI_VO_Suspend(void);
CVI_S32 CVI_VO_Resume(void);

CVI_S32 get_sys_fd(CVI_VOID);
CVI_S32 sys_dev_open(CVI_VOID);
CVI_S32 sys_dev_close(CVI_VOID);
CVI_S32 get_vi_fd(void);
CVI_S32 vi_dev_open(CVI_VOID);
CVI_S32 vi_dev_close(CVI_VOID);
CVI_S32 get_vpss_fd(void);
CVI_S32 vpss_dev_open(CVI_VOID);
CVI_S32 vpss_dev_close(CVI_VOID);
CVI_S32 get_vo_fd(void);
CVI_S32 vo_dev_open(CVI_VOID);
CVI_S32 vo_dev_close(CVI_VOID);
CVI_S32 dwa_dev_open(CVI_VOID);
CVI_S32 dwa_dev_close(CVI_VOID);
CVI_S32 get_rgn_fd(void);
CVI_S32 rgn_dev_open(void);
CVI_S32 rgn_dev_close(void);


#endif // MODULES_SYS_INCLUDE_BASE_H_
