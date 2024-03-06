#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <sys/queue.h>
#include <pthread.h>
#include <inttypes.h>
#include <math.h>

#include <getopt.h>		/* getopt_long() */

#include <fcntl.h>		/* low-level i/o */
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include <linux/cvi_base.h>
#include <sys/prctl.h>

#include "cvi_base.h"
#include "cvi_buffer.h"
#include <linux/cvi_errno.h>


struct vdev dev_isp, dev_vpss, dev_disp, dev_dwa, dev_rgn;

static CVI_S32 base_fd = -1, sys_fd = -1;
static void *shared_mem;
static int shared_mem_usr_cnt;

/*
 * @param type: 0(isp) 1(img) 2(sc) 3(disp)
 * @param dev_id: dev id
 */
struct vdev *get_dev_info(CVI_U8 type, CVI_U8 dev_id)
{
	UNUSED(dev_id);

	if (type == VDEV_TYPE_ISP)
		return &dev_isp;
	else if (type == VDEV_TYPE_VPSS)
		return &dev_vpss;
	else if (type == VDEV_TYPE_DISP)
		return &dev_disp;
	else if (type == VDEV_TYPE_DWA)
		return &dev_dwa;
	else if (type == VDEV_TYPE_RGN)
		return &dev_rgn;

	return NULL;
}

int open_device(const char *dev_name, CVI_S32 *fd)
{
	struct stat st;

	*fd = open(dev_name, O_RDWR /* required */  | O_NONBLOCK | O_CLOEXEC, 0);
	if (-1 == *fd) {
		fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_name, errno,
			strerror(errno));
		return -1;
	}

	if (-1 == fstat(*fd, &st)) {
		close(*fd);
		fprintf(stderr, "Cannot identify '%s': %d, %s\n", dev_name,
			errno, strerror(errno));
		return -1;
	}

	if (!S_ISCHR(st.st_mode)) {
		close(*fd);
		fprintf(stderr, "%s is no device\n", dev_name);
		return -ENODEV;
	}
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

CVI_S32 get_rgn_fd(CVI_VOID)
{
	if (dev_rgn.fd <= 0)
		rgn_dev_open();

	return dev_rgn.fd;
}

CVI_S32 rgn_dev_open(CVI_VOID)
{
	if (dev_rgn.state != VDEV_STATE_OPEN) {
		dev_rgn.state = VDEV_STATE_CLOSED;
		if (open_device(RGN_DEV_NAME, &dev_rgn.fd) == -1) {
			perror("RGN open failed\n");
			dev_rgn.fd = -1;
			return CVI_FAILURE;
		}

		dev_rgn.state = VDEV_STATE_OPEN;
	}

	return CVI_SUCCESS;
}

CVI_S32 rgn_dev_close(CVI_VOID)
{
	if (dev_rgn.state != VDEV_STATE_CLOSED) {
		if (close_device(&dev_rgn.fd) == -1) {
			perror("RGN close failed\n");
			dev_rgn.fd = -1;
			return CVI_FAILURE;
		}
		dev_rgn.state = VDEV_STATE_CLOSED;
		dev_rgn.fd = -1;
	}

	return CVI_SUCCESS;
}

CVI_S32 get_vo_fd(CVI_VOID)
{
	if (dev_disp.fd <= 0)
		vo_dev_open();

	return dev_disp.fd;
}

CVI_S32 vo_dev_open(CVI_VOID)
{
	if (dev_disp.state != VDEV_STATE_OPEN) {
		// disp
		strncpy(dev_disp.name, "VO", sizeof(dev_disp.name));

		if (open_device(VO_DEV_NAME, &dev_disp.fd) == -1) {
			CVI_TRACE_SYS(CVI_DBG_WARN, "Warning: Maybe no VO dev?\n");
			dev_disp.fd = -1;
			return CVI_FAILURE;
		}

		dev_disp.state = VDEV_STATE_OPEN;
	}

	return CVI_SUCCESS;
}

CVI_S32 vo_dev_close(CVI_VOID)
{
	if (dev_disp.state != VDEV_STATE_CLOSED) {
		if (close_device(&dev_disp.fd) == -1) {
			perror("VO close failed\n");
			dev_disp.fd = -1;
			return CVI_FAILURE;
		}
		dev_disp.state = VDEV_STATE_CLOSED;
		dev_disp.fd = -1;
	}

	return CVI_SUCCESS;
}

CVI_S32 get_vpss_fd(void)
{
	if (dev_vpss.fd <= 0)
		vpss_dev_open();

	return dev_vpss.fd;
}

CVI_S32 vpss_dev_open(CVI_VOID)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	if (dev_vpss.state != VDEV_STATE_OPEN) {
		snprintf(dev_vpss.name, sizeof(dev_vpss.name) - 1,
			 "/dev/cvi-vpss");
		if (open_device(dev_vpss.name, &dev_vpss.fd) == -1) {
			perror("VPSS open fail\n");
			s32Ret = CVI_FAILURE;
			return s32Ret;
		}

		dev_vpss.state = VDEV_STATE_OPEN;
	}

	return s32Ret;
}

CVI_S32 vpss_dev_close(CVI_VOID)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	if (dev_vpss.state != VDEV_STATE_CLOSED) {
		s32Ret = close_device(&dev_vpss.fd);
		if (s32Ret != CVI_SUCCESS) {
			perror("VPSS close failed\n");
			s32Ret = CVI_FAILURE;
			return s32Ret;
		}
		dev_vpss.fd = -1;
		dev_vpss.state = VDEV_STATE_CLOSED;
	}

	return s32Ret;
}

CVI_S32 get_vi_fd(CVI_VOID)
{
	if (dev_isp.fd <= 0)
		vi_dev_open();

	return dev_isp.fd;
}

CVI_S32 vi_dev_open(CVI_VOID)
{
	if (dev_isp.state != VDEV_STATE_OPEN) {
		// isp
		strncpy(dev_isp.name, "VI", sizeof(dev_isp.name));
		dev_isp.state = VDEV_STATE_CLOSED;

		if (open_device(VI_DEV_NAME, &dev_isp.fd) == -1) {
			perror("VI open failed\n");
			dev_isp.fd = -1;
			return CVI_FAILURE;
		}

		dev_isp.state = VDEV_STATE_OPEN;
	}

	return CVI_SUCCESS;
}

CVI_S32 vi_dev_close(CVI_VOID)
{
	if (dev_isp.state != VDEV_STATE_CLOSED) {
		if (close_device(&dev_isp.fd) == -1) {
			perror("VI close failed\n");
			dev_isp.fd = -1;
			return CVI_FAILURE;
		}
		dev_isp.state = VDEV_STATE_CLOSED;
		dev_isp.fd = -1;
	}

	return CVI_SUCCESS;
}

CVI_S32 sys_dev_open(CVI_VOID)
{
	if (sys_fd != -1) {
		CVI_TRACE_SYS(CVI_DBG_INFO, "sys dev has already opened\n");
		return CVI_SUCCESS;
	}

	if (open_device(SYS_DEV_NAME, &sys_fd) != 0) {
		perror("sys open failed");
		sys_fd = -1;
		return CVI_ERR_SYS_NOTREADY;
	}
	return CVI_SUCCESS;
}

CVI_S32 sys_dev_close(CVI_VOID)
{
	if (sys_fd == -1) {
		CVI_TRACE_SYS(CVI_DBG_INFO, "sys dev is not opened\n");
		return CVI_SUCCESS;
	}

	close_device(&sys_fd);
	sys_fd = -1;
	return CVI_SUCCESS;
}

CVI_S32 get_sys_fd(CVI_VOID)
{
	if (sys_fd == -1)
		sys_dev_open();
	return sys_fd;
}

CVI_S32 dwa_dev_open(CVI_VOID)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	dwa_dev_close();

	snprintf(dev_dwa.name, sizeof(dev_dwa.name) - 1, "/dev/cvi-dwa");
	if (open_device(dev_dwa.name, &dev_dwa.fd) == -1) {
		perror("dwa open failed");
		return CVI_FAILURE;
	}
	dev_dwa.state = VDEV_STATE_OPEN;

	return s32Ret;
}

CVI_S32 dwa_dev_close(CVI_VOID)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	if (dev_dwa.state != VDEV_STATE_CLOSED) {
		close_device(&dev_dwa.fd);
		dev_dwa.fd = -1;
		dev_dwa.state = VDEV_STATE_CLOSED;
	}

	return s32Ret;
}

CVI_S32 base_dev_open(CVI_VOID)
{
	if (base_fd != -1) {
		CVI_TRACE_SYS(CVI_DBG_DEBUG, "base dev has already opened\n");
		return CVI_SUCCESS;
	}

	if (open_device(BASE_DEV_NAME, &base_fd) != 0) {
		perror("base open failed");
		base_fd = -1;
		return CVI_ERR_SYS_NOTREADY;
	}
	return CVI_SUCCESS;
}

CVI_S32 base_dev_close(CVI_VOID)
{
	if (base_fd == -1) {
		CVI_TRACE_SYS(CVI_DBG_INFO, "base dev is not opened\n");
		return CVI_SUCCESS;
	}

	close_device(&base_fd);
	base_fd = -1;
	return CVI_SUCCESS;
}

CVI_S32 get_base_fd(CVI_VOID)
{
	if (base_fd == -1)
		base_dev_open();
	return base_fd;
}

CVI_S32 vpss_close(void)
{
	if (dev_vpss.state != VDEV_STATE_CLOSED) {
		close_device(&dev_vpss.fd);
		dev_vpss.state = VDEV_STATE_CLOSED;
	}
	return CVI_SUCCESS;
}

CVI_S32 vo_close(void)
{
	if (dev_disp.state != VDEV_STATE_CLOSED) {
		close_device(&dev_disp.fd);
		dev_disp.state = VDEV_STATE_CLOSED;
	}
	return CVI_SUCCESS;
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

void *base_get_shm(void)
{
	base_dev_open();
	if (shared_mem == NULL) {
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
	}
}
