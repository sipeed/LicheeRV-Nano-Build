#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "cvi_gyro.h"
#include "linux/cvi_gyro_ioctl.h"

#define DEV_NODE "/dev/cvi-gyro"

int g_devfd = CVI_FAILURE;

CVI_S32 _dev_check(CVI_VOID)
{
	if (g_devfd <= 0) {
		printf("Device gyro is not open, please check it\n");
		return -1;
	}
	return CVI_SUCCESS;
}

CVI_S32 _cvi_gyro_get_2byte(int reg, CVI_S16 *val)
{
	CVI_S32 ret = CVI_FAILURE;

	if (_dev_check() == 0) {
		struct cvi_gy_regval regval;

		regval.addr = reg;
		ret = ioctl(g_devfd, CVI_GYRO_IOC_READ_2BYTE, &regval);
		*val = regval.val;
	}
	return ret;
}

CVI_S32 _cvi_gyro_get_xyz(int reg, CVI_S16 *x_val, CVI_S16 *y_val, CVI_S16 *z_val)
{
	CVI_S32 ret = CVI_FAILURE;

	if (_dev_check() == 0) {
		struct cvi_gy_regval_6byte regval;

		regval.addr = reg;
		ret = ioctl(g_devfd, CVI_GYRO_IOC_READ_6BYTE, &regval);
		if (x_val != NULL)
			*x_val = regval.x_val;
		if (y_val != NULL)
			*y_val = regval.y_val;
		if (z_val != NULL)
			*z_val = regval.z_val;
		return ret;
	}
	return ret;
}

CVI_S32 CVI_GYRO_Create(CVI_VOID)
{
	if (g_devfd <= 0) {
		g_devfd = open(DEV_NODE, O_RDWR);
		if (g_devfd < 0) {
			printf("Can't open %s\n", DEV_NODE);
			return CVI_FAILURE;
		}
	}
	return CVI_SUCCESS;
}

CVI_VOID CVI_GYRO_Destroy(CVI_VOID)
{
	if (g_devfd > 0) {
		close(g_devfd);
	}
	g_devfd = 0;
}

CVI_S32 CVI_GYRO_Reset(CVI_VOID)
{
	CVI_S32 ret = CVI_FAILURE;

	ret = CVI_GYRO_WRITE(PWR_MGMT_1, 0x80);
	usleep(100 * 1000);
	return ret;
}

CVI_S32 CVI_GYRO_GET_G_XYZ(CVI_S16 *x_val, CVI_S16 *y_val, CVI_S16 *z_val)
{
	return _cvi_gyro_get_xyz(GYRO_XOUT_H, x_val, y_val, z_val);
}

CVI_S32 CVI_GYRO_GET_A_XYZ(CVI_S16 *x_val, CVI_S16 *y_val, CVI_S16 *z_val)
{
	return _cvi_gyro_get_xyz(ACCEL_XOUT_H, x_val, y_val, z_val);
}

CVI_S32 CVI_GYRO_GET_GX(CVI_S16 *val)
{
	return _cvi_gyro_get_2byte(GYRO_XOUT_H, val);
}

CVI_S32 CVI_GYRO_GET_GY(CVI_S16 *val)
{
	return _cvi_gyro_get_2byte(GYRO_YOUT_H, val);
}

CVI_S32 CVI_GYRO_GET_GZ(CVI_S16 *val)
{
	return _cvi_gyro_get_2byte(GYRO_ZOUT_H, val);
}

CVI_S32 CVI_GYRO_GET_AX(CVI_S16 *val)
{
	return _cvi_gyro_get_2byte(ACCEL_XOUT_H, val);
}

CVI_S32 CVI_GYRO_GET_AY(CVI_S16 *val)
{
	return _cvi_gyro_get_2byte(ACCEL_YOUT_H, val);
}

CVI_S32 CVI_GYRO_GET_AZ(CVI_S16 *val)
{
	return _cvi_gyro_get_2byte(ACCEL_ZOUT_H, val);
}

CVI_S32 CVI_GYRO_GET_TEMP(CVI_S16 *val)
{
	return _cvi_gyro_get_2byte(TEMP_OUT_H, val);
}

CVI_S32 CVI_GYRO_ACC_CALIBRATION(CVI_VOID)
{
	if (_dev_check() == 0) {
		return ioctl(g_devfd, CVI_GYRO_IOC_ACC_ADJUST);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_GYRO_CALIBRATION(CVI_VOID)
{
	if (_dev_check() == 0) {
		return ioctl(g_devfd, CVI_GYRO_IOC_ADJUST);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_GYRO_WHO_AM_I(CVI_U8 *val)
{
	CVI_S32 ret = CVI_FAILURE;
	struct cvi_gy_regval reg;

	if (_dev_check() == 0) {
		ret = ioctl(g_devfd, CVI_GYRO_IOC_CHECK, &reg);
		if (val != NULL)
			*val = (CVI_U8) (reg.val & 0xFF);
	}
	return ret;
}

CVI_S32 CVI_GYRO_READ(CVI_U8 addr, CVI_U8 *val)
{
	CVI_S32 ret = CVI_FAILURE;
	struct cvi_gy_regval reg;

	if (_dev_check() == 0) {
		reg.addr = addr;
		ret = ioctl(g_devfd, CVI_GYRO_IOC_READ, &reg);
		if (val != NULL)
			*val = (CVI_U8) (reg.val & 0xFF);
	}
	return ret;
}

CVI_S32 CVI_GYRO_WRITE(CVI_U8 addr, CVI_U8 val)
{
	CVI_S32 ret = CVI_FAILURE;
	struct cvi_gy_regval reg;

	if (_dev_check() == 0) {
		reg.addr = addr;
		reg.val  = val;
		ret = ioctl(g_devfd, CVI_GYRO_IOC_WRITE, &reg);
	}
	return ret;
}

CVI_S32 CVI_GYRO_WRITE_OR(CVI_U8 addr, CVI_U8 val)
{
	CVI_S32 ret = CVI_FAILURE;
	struct cvi_gy_regval reg;

	if (_dev_check() == 0) {
		reg.addr = addr;
		reg.val  = val;
		ret = ioctl(g_devfd, CVI_GYRO_IOC_WRITE_OR, &reg);
	}
	return ret;
}

CVI_S32 CVI_GYRO_WRITE_AND(CVI_U8 addr, CVI_U8 val)
{
	CVI_S32 ret = CVI_FAILURE;
	struct cvi_gy_regval reg;

	if (_dev_check() == 0) {
		reg.addr = addr;
		reg.val  = val;
		ret = ioctl(g_devfd, CVI_GYRO_IOC_WRITE_AND, &reg);
	}
	return ret;
}