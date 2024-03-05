#include "xil_types.h"
#include "linux/types.h"
#include "FreeRTOS_POSIX.h"
#include "FreeRTOS_POSIX/pthread.h"
#include "linux/file.h"
#include <assert.h>
#include "linux/videodev2.h"
#include "linux/v4l2-common.h"

#include <getopt.h>		/* getopt_long() */

#include <linux/fcntl.h>		/* low-level i/o */
#include <errno.h>
#include <linux/ioctl.h>
#include <linux/file.h>
#include <linux/mman.h>
#include "malloc.h"
#include <linux/videodev2.h>
#include <cvi_vip.h>

#include "cvi_math.h"
#include "cvi_base.h"
#include "cvi_sys.h"
#include "cvi_vb.h"
#include "devmem.h"
#include "cvi_buffer.h"

#define NUM_OF_SC_BUFFER  1
#define NUM_OF_DISP_BUFFER 3

#define IDX_OF_ISP   0		// 0~1
#define IDX_OF_DWA   2
#define IDX_OF_IMG   3		// 3,4
#define IDX_OF_SC    5		// 5~8
#define IDX_OF_DISP  9

#define BASE_DEV_NAME "/dev/cvi-base"

struct cvi_vi_proc_ctx *vi_prc_ctx;
struct cvi_vi_ctx vi_ctx;
struct cvi_vpss_ctx vpssCtx[VPSS_MAX_GRP_NUM];
struct cvi_vpss_proc_ctx *vpssPrcCtx;
struct cvi_vo_ctx vo_ctx;
struct cvi_vo_proc_ctx *vo_prc_ctx;
struct cvi_venc_vb_ctx venc_vb_ctx[VENC_MAX_CHN_NUM];
struct cvi_vdec_vb_ctx vdec_vb_ctx[VENC_MAX_CHN_NUM];
struct vdev dev_isp, dev_img[VPSS_IP_NUM], dev_sc[VI_MAX_PHY_PIPE_NUM], dev_disp, dev_dwa[2];

static int base_fd = -1;
static void *shared_mem;
static int shared_mem_usr_cnt;

vb_jobs_t *get_jobs_by_chn(MMF_CHN_S chn, enum CHN_TYPE_E chn_type);

/*
 * @param type: 0(isp) 1(img) 2(sc) 3(disp)
 * @param dev_id: dev id
 */
struct vdev *get_dev_info(CVI_U8 type, CVI_U8 dev_id)
{
	if (type == VDEV_TYPE_ISP)
		return &dev_isp;
	else if ((type == VDEV_TYPE_IMG) && (dev_id < VI_MAX_DEV_NUM))
		return &dev_img[dev_id];
	else if (type == VDEV_TYPE_SC)
		return &dev_sc[dev_id];
	else if (type == VDEV_TYPE_DISP)
		return &dev_disp;
	else if (type == VDEV_TYPE_DWA)
		return &dev_dwa[dev_id];

	return NULL;
}

/* get_dev_info_by_chn: get vdev by chn info.
 *
 * @param chn: channel info.
 * @param chn_type: the chn is input(read) or output(write)
 */
struct vdev *get_dev_info_by_chn(MMF_CHN_S chn, enum CHN_TYPE_E chn_type)
{
	if (chn.enModId == CVI_ID_VI) {
		return &dev_isp;
	} else if (chn.enModId == CVI_ID_VPSS) {
		if (CVI_SYS_GetVPSSMode() == VPSS_MODE_SINGLE)
			return (chn_type == CHN_TYPE_OUT) ? &dev_sc[chn.s32ChnId] : &dev_img[1];

		// for VPSS_DUAL
		if (chn_type == CHN_TYPE_OUT) {
			if (vpssCtx[chn.s32DevId].stGrpAttr.u8VpssDev == 0)
				return &dev_sc[0];
			if ((chn.s32ChnId + 1) < VPSS_MAX_CHN_NUM)
				return &dev_sc[chn.s32ChnId + 1];
			return NULL;
		}
		return &dev_img[vpssCtx[chn.s32DevId].stGrpAttr.u8VpssDev];
	} else if (chn.enModId == CVI_ID_VO) {
		return &dev_disp;
	} else if (chn.enModId == CVI_ID_GDC) {
		return (chn_type == CHN_TYPE_OUT) ? &dev_dwa[1] : &dev_dwa[0];
	}

	return NULL;
}

CVI_S32 v4l2_get_frame_info(PIXEL_FORMAT_E fmt, SIZE_S size, struct buffer *buf, CVI_U64 mem_base)
{
	VB_CAL_CONFIG_S stVbCalConfig;

	COMMON_GetPicBufferConfig(size.u32Width, size.u32Height, fmt, DATA_BITWIDTH_8
		, COMPRESS_MODE_NONE, DEFAULT_ALIGN, &stVbCalConfig);

	memset(buf, 0, sizeof(*buf));
	buf->size = size;
	buf->enPixelFormat = fmt;
	for (CVI_U8 i = 0; i < stVbCalConfig.plane_num; ++i) {
		buf->phy_addr[i] = mem_base;
		buf->length[i] = ALIGN((i == 0) ? stVbCalConfig.u32MainYSize : stVbCalConfig.u32MainCSize,
					stVbCalConfig.u16AddrAlign);
		buf->stride[i] = (i == 0) ? stVbCalConfig.u32MainStride : stVbCalConfig.u32CStride;
		mem_base += buf->length[i];

		CVI_TRACE_SYS(CVI_DBG_INFO, "(%#"PRIx64"-%zu-%d)\n",
				buf->phy_addr[i], buf->length[i], buf->stride[i]);
	}

	return CVI_SUCCESS;
}

static void _get_dev_name(char *s, size_t n, CVI_S32 idx)
{
	snprintf(s, n, "/dev/video%d", idx);
}

static struct vbq *_get_doneq(MMF_CHN_S chn)
{
	vb_jobs_t *jobs = get_jobs_by_chn(chn, CVI_TRUE);

	if (chn.enModId == CVI_ID_VI) {
		if (chn.s32ChnId < VI_EXT_CHN_START)
			return &jobs->doneq;
		else
			return &vi_ctx.extchn_doneq[chn.s32ChnId - VI_EXT_CHN_START];
	}

	return &jobs->doneq;
}

CVI_S32 get_chn_buffer(MMF_CHN_S chn, VB_BLK *blk, CVI_S32 timeout_ms)
{
	CVI_S32 ret = CVI_FAILURE;
	vb_jobs_t *jobs = get_jobs_by_chn(chn, CHN_TYPE_OUT);
	VB_S *vb;
	struct vbq *doneq = _get_doneq(chn);

	pthread_mutex_lock(&jobs->dlock);
	if (!FIFO_EMPTY(doneq)) {
		FIFO_POP(doneq, &vb);
		pthread_mutex_unlock(&jobs->dlock);
		*blk = (VB_BLK)vb;
		return CVI_SUCCESS;
	}

	struct timespec abstime;
	pthread_condattr_t condAttr;
	struct snap_s *s;

	s = malloc(sizeof(*s));
	pthread_condattr_init(&condAttr);
	pthread_condattr_setclock(&condAttr, CLOCK_MONOTONIC);
	if (pthread_cond_init(&s->cond, &condAttr) != 0) {
		pthread_mutex_unlock(&jobs->dlock);
		perror("get_chn_buffer cond init failed.");
		goto err_cond;
	}
	s->chn = chn;

	if (timeout_ms < 0) {
		TAILQ_INSERT_TAIL(&jobs->snap_jobs, s, tailq);
		ret = pthread_cond_wait(&s->cond, &jobs->dlock);
	} else {
		if (clock_gettime(CLOCK_MONOTONIC, &abstime) != 0) {
			pthread_mutex_unlock(&jobs->dlock);
			perror("clock_gettime:");
			goto err_clk;
		}

		CVI_U32 timeout_s = timeout_ms / 1000;

		timeout_ms %= 1000;
		abstime.tv_nsec += timeout_ms * 1000000;
		abstime.tv_sec += (timeout_s + abstime.tv_nsec / 1000000000L);
		abstime.tv_nsec = abstime.tv_nsec % 1000000000L;
		TAILQ_INSERT_TAIL(&jobs->snap_jobs, s, tailq);
		ret = pthread_cond_timedwait(&s->cond, &jobs->dlock, &abstime);
	}
	pthread_mutex_unlock(&jobs->dlock);

	if (ret == 0) {
		*blk = s->blk;
	} else {
		pthread_mutex_lock(&jobs->dlock);
		TAILQ_REMOVE(&jobs->snap_jobs, s, tailq);
		pthread_mutex_unlock(&jobs->dlock);
		errno = ret;
		fprintf(stderr, "%s: Mod(%s) Grp(%d) Chn(%d), jobs wait(%d) work(%d) done(%d)\n"
			, strerror(errno), CVI_SYS_GetModName(chn.enModId), chn.s32DevId, chn.s32ChnId
			, FIFO_SIZE(&jobs->waitq), FIFO_SIZE(&jobs->workq), FIFO_SIZE(&jobs->doneq));
	}

err_clk:
	pthread_cond_destroy(&s->cond);
err_cond:
	pthread_condattr_destroy(&condAttr);
	free(s);
	return ret;
}

int open_device(const char *dev_name, CVI_S32 *fd)
{
	//poshiun
	//struct stat st;

	*fd = open(dev_name, O_RDWR /* required */  | O_NONBLOCK, 0);
	if (-1 == *fd) {
		//poshiun
		//fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_name, errno,
		//	strerror(errno));
		fprintf(stderr, "Cannot open '%s': %d, %d\n", dev_name, errno,
			(errno));
		return -1;
	}
//poshiun
//	if (-1 == fstat(*fd, &st)) {
//		close(*fd);
//		fprintf(stderr, "Cannot identify '%s': %d, %s\n", dev_name,
//			errno, strerror(errno));
//		return -1;
//	}
//
//	if (!S_ISCHR(st.st_mode)) {
//		close(*fd);
//		fprintf(stderr, "%s is no device\n", dev_name);
//		return -ENODEV;
//	}
	return 0;
}

CVI_S32 close_device(CVI_S32 *fd)
{
	if (*fd == -1)
		return -1;

	if (-1 == close(*fd)) {
		fprintf(stderr, "%s: fd(%d) failure\n", __func__, *fd);
		return -1;
	}

	*fd = -1;

	return CVI_SUCCESS;
}

CVI_S32 v4l2_reqbufs(CVI_S32 fd, CVI_S32 count, enum v4l2_buf_type type)
{
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count = count;
	req.type = type;
	req.memory = V4L2_MEMORY_USERPTR;

	if (-1 == ioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (errno == EINVAL)
			fprintf(stderr, "%s does not support user pointer i/o\n", __func__);
		else
			perror("VIDIOC_REQBUFS");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 v4l2_getfmt(CVI_S32 fd, struct v4l2_format *fmt)
{
	if (-1 == ioctl(fd, VIDIOC_G_FMT, fmt)) {
		perror("VIDIOC_G_FMT");
		return -1;
	}

	return 0;
}

CVI_S32 v4l2_setfmt(struct vdev *d, CVI_S32 width, CVI_S32 height, enum v4l2_buf_type type,
		CVI_U32 pxlfmt)
{
	struct v4l2_format fmt;

	CLEAR(fmt);

	fmt.type = type;
	fmt.fmt.pix_mp.width = width;
	fmt.fmt.pix_mp.height = height;
	fmt.fmt.pix_mp.pixelformat = pxlfmt;
	fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
	switch (pxlfmt) {
	case V4L2_PIX_FMT_YUV420M:
	case V4L2_PIX_FMT_YUV422M:
	case V4L2_PIX_FMT_GREY:
	case V4L2_PIX_FMT_NV12M:
	case V4L2_PIX_FMT_NV21M:
	case V4L2_PIX_FMT_NV61M:
	case V4L2_PIX_FMT_NV16M:
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_VYUY:
		fmt.fmt.pix_mp.colorspace = V4L2_COLORSPACE_SMPTE170M;
		break;
	default:
	case V4L2_PIX_FMT_HSVM:
	case V4L2_PIX_FMT_HSV24:
	case V4L2_PIX_FMT_RGBM:
	case V4L2_PIX_FMT_RGB24:
	case V4L2_PIX_FMT_BGR24:
		fmt.fmt.pix_mp.colorspace = V4L2_COLORSPACE_SRGB;
		break;
	}
	switch (pxlfmt) {
	default:
	case V4L2_PIX_FMT_YUV420M:
	case V4L2_PIX_FMT_YUV422M:
	case V4L2_PIX_FMT_HSVM:
	case V4L2_PIX_FMT_RGBM:
		fmt.fmt.pix_mp.num_planes = 3;
		break;
	case V4L2_PIX_FMT_NV12M:
	case V4L2_PIX_FMT_NV21M:
	case V4L2_PIX_FMT_NV61M:
	case V4L2_PIX_FMT_NV16M:
		fmt.fmt.pix_mp.num_planes = 2;
		break;
	case V4L2_PIX_FMT_GREY:
	case V4L2_PIX_FMT_HSV24:
	case V4L2_PIX_FMT_RGB24:
	case V4L2_PIX_FMT_BGR24:
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_VYUY:
		fmt.fmt.pix_mp.num_planes = 1;
		break;
	}
	d->numOfPlanes = fmt.fmt.pix_mp.num_planes;

//	if (-1 == ioctl(d->fd, VIDIOC_TRY_FMT, &fmt))
//		perror("VIDIOC_TRY_FMT");

	CVI_TRACE_SYS(CVI_DBG_INFO, "bytesperline 0(%d) 1(%d)\n",
		      fmt.fmt.pix_mp.plane_fmt[0].bytesperline,
		      fmt.fmt.pix_mp.plane_fmt[1].bytesperline);

	if (-1 == ioctl(d->fd, VIDIOC_S_FMT, &fmt))
		perror("VIDIOC_S_FMT");

	CVI_TRACE_SYS(CVI_DBG_INFO, "fmt-pix (%d-%d) sizeimage(%d) num_planes(%d)\n", fmt.fmt.pix_mp.width,
		      fmt.fmt.pix_mp.height, fmt.fmt.pix.sizeimage, fmt.fmt.pix_mp.num_planes);

	return fmt.fmt.pix.sizeimage;
}

CVI_S32 v4l2_qbuf(struct vdev *d, enum v4l2_buf_type type, struct buffer *buf)
{
	struct v4l2_buffer v4l2_buf;
	struct v4l2_plane plane[VIDEO_MAX_PLANES];
	CVI_S32 fd = d->fd;
	CVI_U8 idx = 0;

	//CVI_TRACE_SYS(CVI_DBG_DEBUG, "#buf(%d) availIdx(%x)\n", d->numOfBuffers, d->availIndex);
	// find available index
	do {
		if (d->availIndex & (1 << idx))
			break;
	} while (++idx < d->numOfBuffers);

	//CVI_TRACE_SYS(CVI_DBG_DEBUG, "index(%d)\n", idx);
	if (idx >= d->numOfBuffers) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "idx(%#x) not available(%#x)\n", idx, d->availIndex);
		return CVI_ERR_VB_NOBUF;
	}
	d->availIndex ^= (1 << idx);

	CLEAR(v4l2_buf);
	v4l2_buf.type = type;
	v4l2_buf.memory = V4L2_MEMORY_USERPTR;
	v4l2_buf.index = idx;
	v4l2_buf.length = d->numOfPlanes;
	v4l2_buf.m.planes = plane;

	for (CVI_U8 i = 0; i < v4l2_buf.length; ++i) {
		v4l2_buf.m.planes[i].m.userptr = buf->phy_addr[i];
		v4l2_buf.m.planes[i].length = buf->length[i];
		v4l2_buf.m.planes[i].bytesused = buf->length[i];
		v4l2_buf.m.planes[i].data_offset = 0;
		//printf("v4l2_qbuf %d: len(%d), num_plane(%d)\n", i, v4l2_buf.m.planes[i].length, v4l2_buf.length);
	}

	if (buf->enPixelFormat == PIXEL_FORMAT_BGR_888_PLANAR) {
		v4l2_buf.m.planes[0].m.userptr = buf->phy_addr[2];
		v4l2_buf.m.planes[2].m.userptr = buf->phy_addr[0];
	}

	if (strncmp(d->name, "ISP", sizeof("ISP")) == 0) {
		if (buf->dev_num == 0)
			v4l2_buf.flags = V4L2_BUF_FLAG_FRAME_ISP_0;
		else
			v4l2_buf.flags = V4L2_BUF_FLAG_FRAME_ISP_1;
	}

	if (-1 == ioctl(fd, VIDIOC_QBUF, &v4l2_buf)) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "VIDIOC_QBUF %s num_plane(%d) pxlfmt(%d)\n",
			      strerror(errno), v4l2_buf.length, buf->enPixelFormat);
		for (CVI_U8 i = 0; i < v4l2_buf.length; ++i)
			CVI_TRACE_SYS(CVI_DBG_ERR, "%d: addr(%#lx) len(%d)\n ", i
				, v4l2_buf.m.planes[i].m.userptr, v4l2_buf.m.planes[i].length);
		return CVI_ERR_SYS_ILLEGAL_PARAM;
	}

	return CVI_SUCCESS;
}

CVI_S32 v4l2_dqbuf(struct vdev *d, enum v4l2_buf_type type, struct v4l2_buffer *buf)
{
	struct v4l2_plane plane[VIDEO_MAX_PLANES];

	CLEAR(*buf);

	buf->type = type;
	buf->memory = V4L2_MEMORY_USERPTR;
	buf->length = d->numOfPlanes;
	buf->m.planes = plane;

	if (-1 == ioctl(d->fd, VIDIOC_DQBUF, buf)) {
		switch (errno) {
		case EAGAIN:
			fprintf(stderr, "Err: %s no data available, try again later\n", __func__);
			return CVI_FAILURE;

		case EIO:
		/* Could ignore EIO, see spec. */

		/* fall through */

		default:
			CVI_TRACE_SYS(CVI_DBG_ERR, "VIDIOC_DQBUF %s num_plane(%d)\n",
				      strerror(errno), buf->length);
			return CVI_FAILURE;
		}
	}

	d->availIndex |= (1 << buf->index);
	return CVI_SUCCESS;
}

static int _cvi_qbuf(CVI_S32 fd, struct cvi_vip_buffer2 *b)
{
	struct v4l2_ext_controls ecs;
	struct v4l2_ext_control ec1;

	memset(&ecs, 0, sizeof(ecs));
	memset(&ec1, 0, sizeof(ec1));
	ec1.id = V4L2_CID_DV_VIP_QBUF;
	ec1.ptr = (void *)b;
	ec1.size = sizeof(*b);
	ecs.controls = &ec1;
	ecs.count = 1;
	ecs.ctrl_class = V4L2_CTRL_ID2CLASS(ec1.id);
	if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &ecs) < 0) {
		fprintf(stderr, "S_EXT_CTRLS - %s NG\n", __func__);
		return -1;
	}
	return 0;
}

CVI_S32 cvi_qbuf(struct vdev *d, struct buffer *buf, CVI_U8 grp_id)
{
	struct cvi_vip_buffer2 b;
	CVI_S32 fd = d->fd;

	// index only work for vi-2-vpss online
	b.index = grp_id;
	switch (buf->enPixelFormat) {
	default:
	case PIXEL_FORMAT_YUV_PLANAR_420:
	case PIXEL_FORMAT_YUV_PLANAR_422:
	case PIXEL_FORMAT_YUV_PLANAR_444:
	case PIXEL_FORMAT_HSV_888_PLANAR:
	case PIXEL_FORMAT_RGB_888_PLANAR:
	case PIXEL_FORMAT_BGR_888_PLANAR:
		b.length = 3;
		break;
	case PIXEL_FORMAT_NV21:
	case PIXEL_FORMAT_NV12:
	case PIXEL_FORMAT_NV61:
	case PIXEL_FORMAT_NV16:
		b.length = 2;
		break;
	case PIXEL_FORMAT_YUV_400:
	case PIXEL_FORMAT_HSV_888:
	case PIXEL_FORMAT_RGB_888:
	case PIXEL_FORMAT_BGR_888:
	case PIXEL_FORMAT_YUYV:
	case PIXEL_FORMAT_YVYU:
	case PIXEL_FORMAT_UYVY:
	case PIXEL_FORMAT_VYUY:
		b.length = 1;
		break;
	}

	for (CVI_U8 i = 0; i < b.length; ++i) {
		b.planes[i].addr = buf->phy_addr[i];
		b.planes[i].length = buf->length[i];
	}

	if (buf->enPixelFormat == PIXEL_FORMAT_BGR_888_PLANAR) {
		CVI_U64 tmp = b.planes[0].addr;

		b.planes[0].addr = b.planes[2].addr;
		b.planes[2].addr = tmp;
	}

	if (-1 == _cvi_qbuf(fd, &b)) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "VIDIOC_QBUF %s num_plane(%d) pxlfmt(%d)\n",
			      strerror(errno), b.length, buf->enPixelFormat);
		for (CVI_U8 i = 0; i < b.length; ++i)
			CVI_TRACE_SYS(CVI_DBG_ERR, "%d: addr(%#llx) len(%d)\n ", i
				, b.planes[i].addr, b.planes[i].length);
		return CVI_ERR_SYS_ILLEGAL_PARAM;
	}

	return CVI_SUCCESS;
}

vb_jobs_t *get_jobs_by_chn(MMF_CHN_S chn, enum CHN_TYPE_E chn_type)
{
	if (chn.enModId == CVI_ID_VI) {
		if (chn.s32ChnId < VI_EXT_CHN_START)
			return &vi_ctx.vb_jobs[chn.s32ChnId];
		return &vi_ctx.vb_jobs[vi_ctx.stExtChnAttr[chn.s32ChnId - VI_EXT_CHN_START].s32BindChn];
	} else if (chn.enModId == CVI_ID_VO)
		return &vo_ctx.vb_jobs;
	else if (chn.enModId == CVI_ID_VPSS) {
		return (chn_type == CHN_TYPE_OUT) ? &vpssCtx[chn.s32DevId].stChnCfgs[chn.s32ChnId].vb_jobs
		       : &vpssCtx[chn.s32DevId].vb_jobs;
	} else if (chn.enModId == CVI_ID_VENC) {
		return &venc_vb_ctx[chn.s32ChnId].vb_jobs;
	} else if (chn.enModId == CVI_ID_VDEC) {
		return &vdec_vb_ctx[chn.s32ChnId].vb_jobs;
	}

	return NULL;
}

/* mod_jobs_init: initialize the jobs.
 *
 * @param jobs: jobs which we work on.
 * @param waitq_depth: the depth for waitq.
 * @param workq_depth: the depth for workq.
 * @param doneq_depth: the depth for doneq.
 * @param doneq_vir_depth: the depth for doneq for virtual channel.
 */
void mod_jobs_init(vb_jobs_t *jobs, CVI_U8 waitq_depth, CVI_U8 workq_depth
	, CVI_U8 doneq_depth)
{
	pthread_mutexattr_t ma;

	pthread_mutexattr_init(&ma);
	pthread_mutexattr_setpshared(&ma, PTHREAD_PROCESS_SHARED);
	pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST);
	if (jobs == NULL) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "Null parameter\n");
		return;
	}

	pthread_mutex_init(&jobs->lock, &ma);
	pthread_mutex_init(&jobs->dlock, &ma);
	sem_init(&jobs->sem, 0, 0);
	FIFO_INIT(&jobs->waitq, waitq_depth);
	FIFO_INIT(&jobs->workq, workq_depth);
	FIFO_INIT(&jobs->doneq, doneq_depth);
	TAILQ_INIT(&jobs->snap_jobs);
}

/* mod_jobs_exit: end the jobs and release all resources.
 *
 * @param jobs: jobs which we work on.
 */
void mod_jobs_exit(vb_jobs_t *jobs)
{
	VB_S *vb;
	struct snap_s *s, *s_tmp;

	if (jobs == NULL) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "Null parameter\n");
		return;
	}

	pthread_mutex_lock(&jobs->lock);
	while (!FIFO_EMPTY(&jobs->waitq)) {
		FIFO_POP(&jobs->waitq, &vb);
		CVI_VB_ReleaseBlock((VB_BLK)vb);
	}
	FIFO_EXIT(&jobs->waitq);
	while (!FIFO_EMPTY(&jobs->workq)) {
		FIFO_POP(&jobs->workq, &vb);
		CVI_VB_ReleaseBlock((VB_BLK)vb);
	}
	FIFO_EXIT(&jobs->workq);
	pthread_mutex_unlock(&jobs->lock);
	pthread_mutex_destroy(&jobs->lock);

	pthread_mutex_lock(&jobs->dlock);
	while (!FIFO_EMPTY(&jobs->doneq)) {
		FIFO_POP(&jobs->doneq, &vb);
		CVI_VB_ReleaseBlock((VB_BLK)vb);
	}
	FIFO_EXIT(&jobs->doneq);

	TAILQ_FOREACH_SAFE(s, &jobs->snap_jobs, tailq, s_tmp)
		TAILQ_REMOVE(&jobs->snap_jobs, s, tailq);
	pthread_mutex_unlock(&jobs->dlock);
	pthread_mutex_destroy(&jobs->dlock);
}

/* mod_jobs_enque_work: Put job into work.
 *     Move vb from waitq into workq and put into driver.
 *
 * @param d: vdev which provide info of driver.
 * @param jobs: jobs which we work on.
 * @return: CVI_SUCCESS if OK.
 */
CVI_S32 mod_jobs_enque_work(struct vdev *d, vb_jobs_t *jobs)
{
	VB_S *vb;
	CVI_S32 ret;

	if ((d == NULL) || (jobs == NULL)) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "Null parameter\n");
		return CVI_ERR_SYS_NULL_PTR;
	}

	pthread_mutex_lock(&jobs->lock);
	if (FIFO_EMPTY(&jobs->waitq)) {
		pthread_mutex_unlock(&jobs->lock);
		CVI_TRACE_SYS(CVI_DBG_NOTICE, "waitq is empty.\n");
		return CVI_ERR_VB_NOTREADY;
	}
	if (FIFO_FULL(&jobs->workq)) {
		pthread_mutex_unlock(&jobs->lock);
		CVI_TRACE_SYS(CVI_DBG_NOTICE, "workq is full.\n");
		return CVI_ERR_VB_SIZE_NOT_ENOUGH;
	}

	FIFO_POP(&jobs->waitq, &vb);
	FIFO_PUSH(&jobs->workq, vb);
	pthread_mutex_unlock(&jobs->lock);

	CVI_TRACE_VB(CVI_DBG_DEBUG, "phy-addr(%#"PRIx64").\n", vb->phy_addr);
	ret = v4l2_qbuf(d, d->type, &vb->buf);
	if (ret != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "v4l2_qbuf error\n");
		return ret;
	}
	return CVI_SUCCESS;
}

/* mod_jobs_enque_work2: Put job into work.
 *     Move vb from waitq into workq and put into driver.
 *
 *  {NOTE} This is work with new qbuf ioctl
 *
 * @param d: vdev which provide info of driver.
 * @param jobs: jobs which we work on.
 * @return: CVI_SUCCESS if OK.
 */
CVI_S32 mod_jobs_enque_work2(struct vdev *d, vb_jobs_t *jobs)
{
	VB_S *vb;
	CVI_S32 ret;

	if ((d == NULL) || (jobs == NULL)) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "Null parameter\n");
		return CVI_ERR_SYS_NULL_PTR;
	}

	pthread_mutex_lock(&jobs->lock);
	if (FIFO_EMPTY(&jobs->waitq)) {
		pthread_mutex_unlock(&jobs->lock);
		CVI_TRACE_SYS(CVI_DBG_NOTICE, "waitq is empty.\n");
		return CVI_ERR_VB_NOTREADY;
	}
	if (FIFO_FULL(&jobs->workq)) {
		pthread_mutex_unlock(&jobs->lock);
		CVI_TRACE_SYS(CVI_DBG_NOTICE, "workq is full.\n");
		return CVI_ERR_VB_SIZE_NOT_ENOUGH;
	}

	FIFO_POP(&jobs->waitq, &vb);
	FIFO_PUSH(&jobs->workq, vb);
	pthread_mutex_unlock(&jobs->lock);

	CVI_TRACE_VB(CVI_DBG_DEBUG, "phy-addr(%#"PRIx64").\n", vb->phy_addr);
	ret = cvi_qbuf(d, &vb->buf, 0);
	if (ret != CVI_SUCCESS) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "v4l2_qbuf error\n");
		return ret;
	}
	return CVI_SUCCESS;
}

/* mod_jobs_waitq_empty: if waitq is empty
 *
 * @param jobs: jobs which we work on.
 * @return: TRUE if empty.
 */
CVI_BOOL mod_jobs_waitq_empty(vb_jobs_t *jobs)
{
	CVI_BOOL is_empty;

	if (jobs == NULL) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "Null parameter\n");
		return CVI_ERR_SYS_NULL_PTR;
	}

	pthread_mutex_lock(&jobs->lock);
	is_empty = FIFO_EMPTY(&jobs->waitq);
	pthread_mutex_unlock(&jobs->lock);

	return is_empty;
}

/* mod_jobs_workq_empty: if workq is empty
 *
 * @param jobs: jobs which we work on.
 * @return: TRUE if empty.
 */
CVI_BOOL mod_jobs_workq_empty(vb_jobs_t *jobs)
{
	CVI_BOOL is_empty;

	if (jobs == NULL) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "Null parameter\n");
		return CVI_ERR_SYS_NULL_PTR;
	}

	pthread_mutex_lock(&jobs->lock);
	is_empty = FIFO_EMPTY(&jobs->workq);
	pthread_mutex_unlock(&jobs->lock);

	return is_empty;
}

/* mod_jobs_waitq_pop: pop out from waitq.
 *
 * @param jobs: jobs which we work on.
 * @return: VB_INVALID_HANDLE is not available; o/w, the VB_BLK.
 */
VB_BLK mod_jobs_waitq_pop(vb_jobs_t *jobs)
{
	VB_S *p;

	pthread_mutex_lock(&jobs->lock);
	if (FIFO_EMPTY(&jobs->waitq)) {
		pthread_mutex_unlock(&jobs->lock);
		CVI_TRACE_SYS(CVI_DBG_ERR, "No more vb in waitq for dequeue.\n");
		return VB_INVALID_HANDLE;
	}
	FIFO_POP(&jobs->waitq, &p);
	pthread_mutex_unlock(&jobs->lock);
	return (VB_BLK)p;
}

/* mod_jobs_workq_pop: pop out from workq.
 *
 * @param jobs: jobs which we work on.
 * @return: VB_INVALID_HANDLE is not available; o/w, the VB_BLK.
 */
VB_BLK mod_jobs_workq_pop(vb_jobs_t *jobs)
{
	VB_S *p;

	pthread_mutex_lock(&jobs->lock);
	if (FIFO_EMPTY(&jobs->workq)) {
		pthread_mutex_unlock(&jobs->lock);
		CVI_TRACE_SYS(CVI_DBG_ERR, "No more vb in workq for dequeue.\n");
		return VB_INVALID_HANDLE;
	}
	FIFO_POP(&jobs->workq, &p);
	pthread_mutex_unlock(&jobs->lock);
	return (VB_BLK)p;
}

/* vb_qbuf: queue vb into the specified channel.
 *     (src) Put into workq and driver.
 *     (dst) Put into waitq and sem_post
 *
 * @param chn: the channel to be queued.
 * @param chn_type: the chn is input(read) or output(write)
 * @param blk: VB_BLK to be queued.
 */
CVI_S32 vb_qbuf(MMF_CHN_S chn, enum CHN_TYPE_E chn_type, VB_BLK blk)
{
	vb_jobs_t *jobs = get_jobs_by_chn(chn, chn_type);
	VB_S *vb = (VB_S *)blk;
	CVI_S32 ret = CVI_SUCCESS;

	CVI_TRACE_ID(CVI_DBG_INFO, chn.enModId, "%s dev(%d) chn(%d) chnType(%d): phy-addr(%#"PRIx64") cnt(%d)\n",
		     CVI_SYS_GetModName(chn.enModId), chn.s32DevId, chn.s32ChnId, chn_type,
		     vb->phy_addr, vb->usr_cnt);
	atomic_fetch_add(&vb->usr_cnt, 1);
	if (chn_type == CHN_TYPE_OUT) {
		struct vdev *d = get_dev_info_by_chn(chn, chn_type);

		pthread_mutex_lock(&jobs->lock);
		if (FIFO_FULL(&jobs->workq)) {
			pthread_mutex_unlock(&jobs->lock);
			CVI_VB_ReleaseBlock(blk);
			CVI_TRACE_ID(CVI_DBG_NOTICE, chn.enModId, "%s workq is full. drop new one.\n"
				     , CVI_SYS_GetModName(chn.enModId));
			return CVI_ERR_VB_SIZE_NOT_ENOUGH;
		}
		FIFO_PUSH(&jobs->workq, vb);
		vb->buf.dev_num = chn.s32ChnId;
		pthread_mutex_unlock(&jobs->lock);

		if (chn.enModId == CVI_ID_VPSS)
			ret = cvi_qbuf(d, &vb->buf, chn.s32DevId);
		else
			ret = v4l2_qbuf(d, d->type, &vb->buf);
		if (ret != CVI_SUCCESS) {
			mod_jobs_workq_pop(jobs);
			CVI_TRACE_ID(CVI_DBG_ERR, chn.enModId, "%s v4l2_qbuf error\n",
				     CVI_SYS_GetModName(chn.enModId));
			return ret;
		}
	} else {
		pthread_mutex_lock(&jobs->lock);
		if (FIFO_FULL(&jobs->waitq)) {
			pthread_mutex_unlock(&jobs->lock);
			CVI_VB_ReleaseBlock(blk);
			CVI_TRACE_ID(CVI_DBG_NOTICE, chn.enModId, "%s waitq is full. drop new one.\n"
				     , CVI_SYS_GetModName(chn.enModId));
			if (chn.enModId == CVI_ID_VI)
				vi_ctx.chnStatus[chn.s32ChnId].u32LostFrame++;
			if (chn.enModId == CVI_ID_VPSS)
				vpssPrcCtx[chn.s32DevId].stGrpWorkStatus.u32LostCnt++;
			return CVI_ERR_VB_SIZE_NOT_ENOUGH;
		}
		FIFO_PUSH(&jobs->waitq, vb);
		pthread_mutex_unlock(&jobs->lock);
		sem_post(&jobs->sem);
		if (chn.enModId == CVI_ID_VPSS)
			CVI_VPSS_PostJob(vpssCtx[chn.s32DevId].stGrpAttr.u8VpssDev);
	}

	*vb->mod_ids |= BIT(chn.enModId);
	return ret;
}

/* _handle_snap: if there is get-frame request, hanlde it.
 *
 * @param chn: the channel where the blk is dequeued.
 * @param chn_type: the chn is input(read) or output(write)
 * @param blk: the VB_BLK to handle.
 */
static void _handle_snap(MMF_CHN_S chn, enum CHN_TYPE_E chn_type, VB_BLK blk)
{
	vb_jobs_t *jobs = get_jobs_by_chn(chn, chn_type);
	VB_S *p = (VB_S *)blk;
	struct vbq *doneq;
	struct snap_s *s, *s_tmp;

	if (chn_type != CHN_TYPE_OUT)
		return;

	if (chn.enModId == CVI_ID_VDEC)
		return;

	pthread_mutex_lock(&jobs->dlock);
	TAILQ_FOREACH_SAFE(s, &jobs->snap_jobs, tailq, s_tmp) {
		if (CHN_MATCH(&s->chn, &chn)) {
			TAILQ_REMOVE(&jobs->snap_jobs, s, tailq);
			s->blk = blk;
			atomic_fetch_add(&p->usr_cnt, 1);
			*p->mod_ids |= BIT(CVI_ID_USER);
			pthread_cond_signal(&s->cond);
			pthread_mutex_unlock(&jobs->dlock);
			return;
		}
	}

	doneq = _get_doneq(chn);
	// check if there is a snap-queue
	if (FIFO_CAPACITY(doneq)) {
		if (FIFO_FULL(doneq)) {
			VB_S *vb = NULL;

			FIFO_POP(doneq, &vb);
			*vb->mod_ids &= ~BIT(CVI_ID_USER);
			CVI_VB_ReleaseBlock((VB_BLK)vb);
		}
		atomic_fetch_add(&p->usr_cnt, 1);
		*p->mod_ids |= BIT(CVI_ID_USER);
		FIFO_PUSH(doneq, p);
	}
	pthread_mutex_unlock(&jobs->dlock);
}

/* vb_dqbuf: dequeue vb from the specified channel(driver).
 *
 * @param chn: the channel to be dequeued.
 * @param chn_type: the chn is input(read) or output(write)
 * @param blk: the VB_BLK dequeued.
 * @return: status of operation. CVI_SUCCESS if OK.
 */
CVI_S32 vb_dqbuf(MMF_CHN_S chn, enum CHN_TYPE_E chn_type, VB_BLK *blk)
{
	struct vdev *d = get_dev_info_by_chn(chn, chn_type);
	vb_jobs_t *jobs;
	struct v4l2_buffer buf;
	VB_S *p;

	*blk = VB_INVALID_HANDLE;

	if (d == NULL) {
		CVI_TRACE_ID(CVI_DBG_ERR, chn.enModId, "can't get file-handler\n");
		return CVI_ERR_VB_ILLEGAL_PARAM;
	}

	if (v4l2_dqbuf(d, d->type, &buf) != CVI_SUCCESS) {
		CVI_TRACE_ID(CVI_DBG_ERR, chn.enModId, "Mod(%s) v4l2 dqbuf fail\n", CVI_SYS_GetModName(chn.enModId));
		return CVI_FAILURE;
	}

	if (chn.enModId == CVI_ID_VI)
		chn.s32ChnId = (buf.flags & V4L2_BUF_FLAG_FRAME_ISP_0) ? 0 : 1;

	jobs = get_jobs_by_chn(chn, chn_type);

	pthread_mutex_lock(&jobs->lock);
	// get vb from workq which is done.
	if (FIFO_EMPTY(&jobs->workq)) {
		pthread_mutex_unlock(&jobs->lock);
		CVI_TRACE_ID(CVI_DBG_ERR, chn.enModId, "%s ChnId(%d) No more vb for dequeue.\n",
			     CVI_SYS_GetModName(chn.enModId), chn.s32ChnId);
		return CVI_ERR_VB_NOBUF;
	}
	FIFO_POP(&jobs->workq, &p);
	pthread_mutex_unlock(&jobs->lock);
	*blk = (VB_BLK)p;
	*p->mod_ids &= ~BIT(chn.enModId);

	// only vi's timestamp from driver, others(vpss) use vi's.
	if (chn.enModId == CVI_ID_VI)
		p->buf.u64PTS = buf.timestamp.tv_sec * 1000000 + buf.timestamp.tv_usec;
	if (chn.enModId == CVI_ID_VI) {
		p->buf.dev_num = (buf.flags & V4L2_BUF_FLAG_FRAME_ISP_0) ? 0 : 1;
		p->buf.frm_num = buf.sequence;
	}

	return CVI_SUCCESS;
}

/* vb_done_handler: called when vb on specified chn is ready for delivery.
 *    Get vb from chn and deliver to its binding dsts if available;
 *    O/W, release back to vb_pool.
 *
 * @param chn: the chn which has vb to be released
 * @param chn_type: for modules which has both in/out.
 *                True: module generates(output) vb.
 *                False: module take(input) vb.
 * @param blk: VB_BLK.
 * @return: status of operation. CVI_SUCCESS if OK.
 */
CVI_S32 vb_done_handler(MMF_CHN_S chn, enum CHN_TYPE_E chn_type, VB_BLK blk)
{
	MMF_BIND_DEST_S stBindDest;
	CVI_S32 ret;

	_handle_snap(chn, chn_type, blk);

	if (chn_type == CHN_TYPE_OUT) {
		if (CVI_SYS_GetBindbySrc(&chn, &stBindDest) == CVI_SUCCESS) {
			for (CVI_U8 i = 0; i < stBindDest.u32Num; ++i) {
				vb_qbuf(stBindDest.astMmfChn[i], false, blk);
				CVI_TRACE_SYS(CVI_DBG_INFO, " Mod(%s) chn(%d) dev(%d) -> Mod(%s) chn(%d) dev(%d)\n"
					     , CVI_SYS_GetModName(chn.enModId), chn.s32ChnId, chn.s32DevId
					     , CVI_SYS_GetModName(stBindDest.astMmfChn[i].enModId)
					     , stBindDest.astMmfChn[i].s32ChnId
					     , stBindDest.astMmfChn[i].s32DevId);
			}
		} else {
			// release if not found
			CVI_TRACE_SYS(CVI_DBG_INFO, "Mod(%s) chn(%d) dev(%d) src no dst release\n"
				     , CVI_SYS_GetModName(chn.enModId), chn.s32ChnId, chn.s32DevId);

		}
	} else {
		CVI_TRACE_SYS(CVI_DBG_INFO, "Mod(%s) chn(%d) dev(%d) dst out release\n"
			     , CVI_SYS_GetModName(chn.enModId), chn.s32ChnId, chn.s32DevId);
	}
	ret = CVI_VB_ReleaseBlock(blk);

	return ret;
}

CVI_S32 v4l2_isp_open(CVI_VOID)
{
	char dev_name[16];
	CVI_S32 s32Ret = CVI_SUCCESS;

	if (dev_isp.state != VDEV_STATE_OPEN) {
		// isp
		strncpy(dev_isp.name, "ISP", sizeof(dev_isp.name));
		dev_isp.is_online = false;
		dev_isp.state = VDEV_STATE_CLOSED;
		dev_isp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

		_get_dev_name(dev_name, 16, IDX_OF_ISP);
		if (open_device(dev_name, &dev_isp.fd) == -1) {
			perror("ISP open failed\n");
			s32Ret = CVI_FAILURE;
			return s32Ret;
		}

		dev_isp.state = VDEV_STATE_OPEN;
		dev_isp.numOfBuffers = NUM_OF_ISP_BUFFER;
		dev_isp.availIndex = 0xff;

		s32Ret = v4l2_reqbufs(dev_isp.fd, dev_isp.numOfBuffers, dev_isp.type);
		if (s32Ret != CVI_SUCCESS) {
			perror("isp device v4l2_reqbufs\n");
			s32Ret = CVI_FAILURE;
			return s32Ret;
		}
	}

	return s32Ret;
}

CVI_S32 v4l2_isp_close(CVI_VOID)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	if (dev_isp.state != VDEV_STATE_CLOSED) {
		s32Ret = close_device(&dev_isp.fd);
		if (s32Ret != CVI_SUCCESS) {
			perror("ISP close failed\n");
			return s32Ret;
		}
		dev_isp.state = VDEV_STATE_CLOSED;
	}

	return s32Ret;
}

CVI_S32 v4l2_dev_open(CVI_VOID)
{
	char dev_name[16];
	CVI_S32 i = 0;

	if (dev_isp.state != VDEV_STATE_OPEN) {
		// isp
		strncpy(dev_isp.name, "ISP", sizeof(dev_isp.name));
		dev_isp.is_online = false;
		dev_isp.state = VDEV_STATE_CLOSED;
		dev_isp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

		_get_dev_name(dev_name, 16, IDX_OF_ISP);
		if (open_device(dev_name, &dev_isp.fd) == -1) {
			perror("ISP open failed");
			return -EIO;
		}
		dev_isp.state = VDEV_STATE_OPEN;
	}

	for (i = 0; i < 2; ++i) {
		// img_in
		dev_img[i].is_online = false;
		dev_img[i].state = VDEV_STATE_CLOSED;
		dev_img[i].type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

		_get_dev_name(dev_name, 16, IDX_OF_IMG + i);
		if (open_device(dev_name, &dev_img[i].fd) == -1) {
			perror("IMG open failed");
			return -EIO;
		}
		dev_img[i].state = VDEV_STATE_OPEN;
	}

	for (i = 0; i < VPSS_MAX_PHY_CHN_NUM; ++i) {
		// sc
		dev_sc[i].is_online = false;
		dev_sc[i].state = VDEV_STATE_CLOSED;
		dev_sc[i].type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;

		_get_dev_name(dev_name, 16, IDX_OF_SC + i);
		if (open_device(dev_name, &dev_sc[i].fd) == -1) {
			perror("SC open failed");
			return -EIO;
		}
		dev_sc[i].state = VDEV_STATE_OPEN;
	}

	// disp
	dev_disp.is_online = false;
	dev_disp.state = VDEV_STATE_CLOSED;
	dev_disp.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;

	_get_dev_name(dev_name, 16, IDX_OF_DISP);
	if (open_device(dev_name, &dev_disp.fd) == -1) {
		perror("disp open failed");
		return -EIO;
	}
	dev_disp.state = VDEV_STATE_OPEN;

	// dwa - 0: input queue, 1: output queue.
	dev_dwa[0].is_online = dev_dwa[1].is_online = false;
	dev_dwa[0].state = dev_dwa[1].state = VDEV_STATE_CLOSED;
	_get_dev_name(dev_name, 16, IDX_OF_DWA);
	if (open_device(dev_name, &dev_dwa[0].fd) == -1) {
		perror("dwa open failed");
		return -EIO;
	}
	dev_dwa[1].fd = dev_dwa[0].fd;
	dev_dwa[0].type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	dev_dwa[1].type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	dev_dwa[0].state = dev_dwa[1].state = VDEV_STATE_OPEN;

	//printf("FD: isp(%d) img(%d %d) sc(%d %d %d %d) disp(%d)\n", dev_isp.fd,
	//	dev_img[0].fd, dev_img[1].fd, dev_sc[0].fd, dev_sc[1].fd, dev_sc[2].fd, dev_sc[3].fd, dev_disp.fd);

	return CVI_SUCCESS;
}

CVI_S32 v4l2_dev_close(CVI_VOID)
{
	if (dev_isp.state != VDEV_STATE_CLOSED) {
		v4l2_streamoff(&dev_isp, dev_isp.type);
		close_device(&dev_isp.fd);
		dev_isp.state = VDEV_STATE_CLOSED;
	}

	vpss_close();
	vo_close();

	if (dev_dwa[0].state != VDEV_STATE_CLOSED) {
		v4l2_streamoff(&dev_dwa[0], dev_dwa[0].type);
		close_device(&dev_dwa[0].fd);
		dev_dwa[0].state = dev_dwa[1].state = VDEV_STATE_CLOSED;
	}

	return CVI_SUCCESS;
}

CVI_S32 vpss_close(void)
{
	for (CVI_U8 i = 0; i < VPSS_IP_NUM; ++i) {
		if (dev_img[i].state != VDEV_STATE_CLOSED) {
			v4l2_streamoff(&dev_img[i], dev_img[i].type);
			close_device(&dev_img[i].fd);
			dev_img[i].state = VDEV_STATE_CLOSED;
		}
	}
	for (CVI_U8 i = 0; i < VI_MAX_PHY_PIPE_NUM; ++i) {
		if (dev_sc[i].state != VDEV_STATE_CLOSED) {
			v4l2_streamoff(&dev_sc[i], dev_sc[i].type);
			close_device(&dev_sc[i].fd);
			dev_sc[i].state = VDEV_STATE_CLOSED;
		}
	}
	return CVI_SUCCESS;
}

CVI_S32 vo_close(void)
{
	if (dev_disp.state != VDEV_STATE_CLOSED) {
		v4l2_streamoff(&dev_disp, dev_disp.type);
		close_device(&dev_disp.fd);
		dev_disp.state = VDEV_STATE_CLOSED;
	}
	return CVI_SUCCESS;
}

CVI_S32 init_isp_device(CVI_VOID)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	CVI_S32 fd = dev_isp.fd;

	dev_isp.numOfBuffers = NUM_OF_ISP_BUFFER;
	dev_isp.availIndex = 0xff;
	s32Ret = v4l2_reqbufs(fd, dev_isp.numOfBuffers, dev_isp.type);
	if (s32Ret != CVI_SUCCESS) {
		perror("init_isp_device");
		s32Ret = CVI_FAILURE;
	}

	return s32Ret;
}

CVI_S32 init_vpss_device(void)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	for (CVI_U8 i = 0; i < VI_MAX_DEV_NUM; ++i) {
		dev_img[i].numOfBuffers = NUM_OF_SC_BUFFER;
		dev_img[i].availIndex = 0xff;
		s32Ret = v4l2_reqbufs(dev_img[i].fd, NUM_OF_SC_BUFFER, dev_img[i].type);
		if (s32Ret != CVI_SUCCESS) {
			perror("init_vpss_device");
			return CVI_FAILURE;
		}
	}

	return s32Ret;
}

CVI_S32 init_disp_device(void)
{
	CVI_S32 *fd = &dev_disp.fd;
	CVI_S32 s32Ret = CVI_SUCCESS;

	dev_disp.numOfBuffers = NUM_OF_DISP_BUFFER;
	dev_disp.availIndex = 0xff;
	s32Ret = v4l2_reqbufs(*fd, NUM_OF_DISP_BUFFER, dev_disp.type);
	if (s32Ret != CVI_SUCCESS) {
		perror("init_disp_device");
		s32Ret = CVI_FAILURE;
	}

	return s32Ret;
}

CVI_S32 init_dwa_device(void)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	dev_dwa[0].numOfBuffers = dev_dwa[1].numOfBuffers = 1;
	dev_dwa[0].availIndex = dev_dwa[1].availIndex = 0xff;
	s32Ret = v4l2_reqbufs(dev_dwa[1].fd, 1, dev_dwa[1].type);
	if (s32Ret != CVI_SUCCESS)
		return s32Ret;
	s32Ret = v4l2_reqbufs(dev_dwa[0].fd, 1, dev_dwa[0].type);
	if (s32Ret != CVI_SUCCESS)
		return s32Ret;
	return s32Ret;
}

CVI_S32 v4l2_streamoff(struct vdev *d, enum v4l2_buf_type type)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	if (!d) {
		perror("v4l2_streamoff, Null pointer");
		return CVI_FAILURE;
	}

	if (-1 == ioctl(d->fd, VIDIOC_STREAMOFF, &type)) {
		perror("VIDIOC_STREAMOFF");
		s32Ret = CVI_FAILURE;
	} else {
		d->state = VDEV_STATE_OPEN;
		d->availIndex = 0xff;
	}

	return s32Ret;
}

CVI_S32 v4l2_streamon(struct vdev *d, enum v4l2_buf_type type)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	if (!d) {
		perror("v4l2_streamon, Null pointer");
		return CVI_FAILURE;
	}

	if (-1 == ioctl(d->fd, VIDIOC_STREAMON, &type)) {
		perror("VIDIOC_STREAMON");
		s32Ret = CVI_FAILURE;
	} else
		d->state = VDEV_STATE_RUN;

	return s32Ret;
}

CVI_U32 v4l2_remap_pxlfmt(PIXEL_FORMAT_E pxlfmt)
{
	switch (pxlfmt) {
	case PIXEL_FORMAT_YUV_PLANAR_420:
		return V4L2_PIX_FMT_YUV420M;
	case PIXEL_FORMAT_YUV_PLANAR_422:
		return V4L2_PIX_FMT_YUV422M;
	case PIXEL_FORMAT_YUV_400:
		return V4L2_PIX_FMT_GREY;
	case PIXEL_FORMAT_RGB_888_PLANAR:
	case PIXEL_FORMAT_BGR_888_PLANAR:
		return V4L2_PIX_FMT_RGBM;
	case PIXEL_FORMAT_HSV_888:
		return V4L2_PIX_FMT_HSV24;
	case PIXEL_FORMAT_HSV_888_PLANAR:
		return V4L2_PIX_FMT_HSVM;
	case PIXEL_FORMAT_RGB_888:
		return V4L2_PIX_FMT_RGB24;
	case PIXEL_FORMAT_BGR_888:
		return V4L2_PIX_FMT_BGR24;
	case PIXEL_FORMAT_NV12:
		return V4L2_PIX_FMT_NV12M;
	case PIXEL_FORMAT_NV21:
		return V4L2_PIX_FMT_NV21M;
	case PIXEL_FORMAT_NV16:
		return V4L2_PIX_FMT_NV16M;
	case PIXEL_FORMAT_NV61:
		return V4L2_PIX_FMT_NV61M;
	case PIXEL_FORMAT_YUYV:
		return V4L2_PIX_FMT_YUYV;
	case PIXEL_FORMAT_YVYU:
		return V4L2_PIX_FMT_YVYU;
	case PIXEL_FORMAT_UYVY:
		return V4L2_PIX_FMT_UYVY;
	case PIXEL_FORMAT_VYUY:
		return V4L2_PIX_FMT_VYUY;
	default:
		return 0;
	}

	return 0;
}

CVI_S32 v4l2_set_sel(CVI_S32 fd, enum v4l2_buf_type type, CVI_U32 target, struct v4l2_rect *rect)
{
	CVI_S32 ret = 0;
	struct v4l2_selection sel = {
		.type = type,
		.target = target,
		.flags = V4L2_SEL_FLAG_LE,
		.r = *rect,
	};

	ret = ioctl(fd, VIDIOC_S_SELECTION, &sel);
	if (ret == -1)
		fprintf(stderr, "fd(%d) S_SELECTION NG\n", fd);

	return ret;
}

CVI_S32 v4l2_sel_input(CVI_S32 fd, CVI_S32 input_idx)
{
	CVI_S32 ret = 0;

	ret = ioctl(fd, VIDIOC_S_INPUT, &input_idx);
	if (ret == -1)
		fprintf(stderr, "fd(%d) VIDIOC_S_INPUT NG, %s\n", fd, strerror(errno));

	return ret;
}

long get_diff_in_us(struct timespec t1, struct timespec t2)
{
	struct timespec diff;

	if (t2.tv_nsec-t1.tv_nsec < 0) {
		diff.tv_sec  = t2.tv_sec - t1.tv_sec - 1;
		diff.tv_nsec = t2.tv_nsec - t1.tv_nsec + 1000000000;
	} else {
		diff.tv_sec  = t2.tv_sec - t1.tv_sec;
		diff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
	}
	return (diff.tv_sec * 1000000.0 + diff.tv_nsec / 1000.0);
}

/* aspect_ratio_resize: calculate the new rect to keep aspect ratio
 *   according to given in/out size.
 *
 * @param in: input video size.
 * @param out: output display size.
 *
 * @return: the rect which describe the video on output display.
 */
RECT_S aspect_ratio_resize(SIZE_S in, SIZE_S out)
{
	RECT_S rect;
	float ratio = MIN2((float)out.u32Width / in.u32Width, (float)out.u32Height / in.u32Height);

	rect.u32Height = (float)in.u32Height * ratio + 0.5;
	rect.u32Width = (float)in.u32Width * ratio + 0.5;
	rect.s32X = (out.u32Width - rect.u32Width) >> 1;
	rect.s32Y = (out.u32Height - rect.u32Height) >> 1;
	return rect;
}

/* base_fill_videoframe2buffer: fill buffer for driver per stVideoFrame.
 *
 * @param chn: image's pixel format.
 * @param pstVideoFrame: the videoframe information.
 * @param buf: the buffer which will be filled per given stVideoFrame.
 * @return: CVI_SUCCESS if OK.
 */
CVI_S32 base_fill_videoframe2buffer(MMF_CHN_S chn, const VIDEO_FRAME_INFO_S *pstVideoFrame,
	struct buffer *buf)
{
	CVI_U32 plane_size;
	VB_CAL_CONFIG_S stVbCalConfig;

	COMMON_GetPicBufferConfig(pstVideoFrame->stVFrame.u32Width, pstVideoFrame->stVFrame.u32Height,
		pstVideoFrame->stVFrame.enPixelFormat, DATA_BITWIDTH_8, COMPRESS_MODE_NONE,
		DEFAULT_ALIGN, &stVbCalConfig);

	buf->size.u32Width = pstVideoFrame->stVFrame.u32Width;
	buf->size.u32Height = pstVideoFrame->stVFrame.u32Height;
	buf->enPixelFormat = pstVideoFrame->stVFrame.enPixelFormat;
	buf->frame_crop.start_x = pstVideoFrame->stVFrame.s16OffsetLeft;
	buf->frame_crop.start_y = pstVideoFrame->stVFrame.s16OffsetTop;
	buf->frame_crop.end_x = pstVideoFrame->stVFrame.u32Width - pstVideoFrame->stVFrame.s16OffsetRight;
	buf->frame_crop.end_y = pstVideoFrame->stVFrame.u32Height - pstVideoFrame->stVFrame.s16OffsetBottom;
	buf->u64PTS = pstVideoFrame->stVFrame.u64PTS;

	for (CVI_U8 i = 0; i < NUM_OF_PLANES; ++i) {
		if (i >= stVbCalConfig.plane_num) {
			buf->phy_addr[i] = 0;
			buf->length[i] = 0;
			buf->stride[i] = 0;
			continue;
		}

		plane_size = (i == 0) ? stVbCalConfig.u32MainYSize : stVbCalConfig.u32MainCSize;
		buf->phy_addr[i] = pstVideoFrame->stVFrame.u64PhyAddr[i];
		buf->length[i] = pstVideoFrame->stVFrame.u32Length[i];
		buf->stride[i] = pstVideoFrame->stVFrame.u32Stride[i];
		if (buf->length[i] < plane_size) {
			CVI_TRACE_ID(CVI_DBG_ERR, chn.enModId, "Mod(%s) Dev(%d) Chn(%d) Plane[%d]\n"
				, CVI_SYS_GetModName(chn.enModId), chn.s32DevId, chn.s32ChnId, i);
			CVI_TRACE_ID(CVI_DBG_ERR, chn.enModId, " length(%zu) less than expected(%d).\n"
				, buf->length[i], plane_size);
			return CVI_FAILURE;
		}
		if (buf->stride[i] % DEFAULT_ALIGN) {
			CVI_TRACE_ID(CVI_DBG_ERR, chn.enModId, "Mod(%s) Dev(%d) Chn(%d) Plane[%d]\n"
				, CVI_SYS_GetModName(chn.enModId), chn.s32DevId, chn.s32ChnId, i);
			CVI_TRACE_ID(CVI_DBG_ERR, chn.enModId, " stride(%d) not aligned(%d).\n"
				, buf->stride[i], DEFAULT_ALIGN);
			return CVI_FAILURE;
		}
		if (buf->phy_addr[i] % DEFAULT_ALIGN) {
			CVI_TRACE_ID(CVI_DBG_ERR, chn.enModId, "Mod(%s) Dev(%d) Chn(%d) Plane[%d]\n"
				, CVI_SYS_GetModName(chn.enModId), chn.s32DevId, chn.s32ChnId, i);
			CVI_TRACE_ID(CVI_DBG_ERR, chn.enModId, " address(%#"PRIx64") not aligned(%d).\n"
				, buf->phy_addr[i], DEFAULT_ALIGN);
			return CVI_FAILURE;
		}
	}
	// [WA-01]
	if (stVbCalConfig.plane_num > 1) {
		if (((buf->phy_addr[0] & (stVbCalConfig.u16AddrAlign - 1))
		    != (buf->phy_addr[1] & (stVbCalConfig.u16AddrAlign - 1)))
		 || ((buf->phy_addr[0] & (stVbCalConfig.u16AddrAlign - 1))
		    != (buf->phy_addr[2] & (stVbCalConfig.u16AddrAlign - 1)))) {
			CVI_TRACE_ID(CVI_DBG_ERR, chn.enModId, "Mod(%s) Dev(%d) Chn(%d)\n"
				, CVI_SYS_GetModName(chn.enModId), chn.s32DevId, chn.s32ChnId);
			CVI_TRACE_ID(CVI_DBG_ERR, chn.enModId, "plane address offset (%#"PRIx64"-%#"PRIx64"-%#"PRIx64")"
				, buf->phy_addr[0], buf->phy_addr[1], buf->phy_addr[2]);
			CVI_TRACE_ID(CVI_DBG_ERR, chn.enModId, "not aligned to %#x.\n", stVbCalConfig.u16AddrAlign);
			return CVI_FAILURE;
		}
	}
	return CVI_SUCCESS;
}

void *base_get_shm(void)
{
	if (shared_mem == NULL) {
		if (base_fd == -1 && open_device(BASE_DEV_NAME, &base_fd) == -1) {
			CVI_TRACE_SYS(CVI_DBG_ERR, "base dev open fail!\n");
			return NULL;
		}

		shared_mem = mmap(NULL, BASE_SHARE_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, base_fd, 0);
		if (shared_mem == MAP_FAILED) {
			CVI_TRACE_SYS(CVI_DBG_ERR, "base dev mmap fail!\n");
			return NULL;
		}
	}

	shared_mem_usr_cnt++;
	return shared_mem;
}

void base_release_shm(void)
{
	shared_mem_usr_cnt--;

	if (shared_mem_usr_cnt == 0 && shared_mem != NULL) {
		munmap((void *)shared_mem, BASE_SHARE_MEM_SIZE);
		shared_mem = NULL;
		close_device(&base_fd);
		base_fd = -1;
	}
}

