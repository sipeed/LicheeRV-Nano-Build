/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_ive.h
 * Description:
 */

#ifndef __CVI_GYRO_H__
#define __CVI_GYRO_H__

#include "linux/cvi_type.h"
#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif
#ifdef __cplusplus
#if __cplusplus
extern "C" {
extern __attribute__((weak)) void *__dso_handle;
#endif
#endif /* End of #ifdef __cplusplus */

enum registers {
	//main chip
	USER_CTRL         = 0x6A,
	PWR_MGMT_1        = 0x6B,
	PWR_MGMT_2        = 0x6C,
	SIGNAL_PATH_RESET = 0x68,
	INT_PIN_CFG       = 0x37,
	ACCEL_CONFIG      = 0x1C,
	ACCEL_CONFIG_2    = 0x1D,
	MOT_DETECT_CTRL   = 0x69,
	WOM_THR           = 0x1F,
	GYRO_CONFIG       = 0x1B,
	CONFIG            = 0x1A,
	SMPLRT_DIV        = 0x19,
	INT_ENABLE        = 0x38,
	INT_STATUS        = 0x3A,
	WHO_AM_I          = 0x75,

	/// gyro
	GYRO_XOUT_H       = 0x43,
	GYRO_XOUT_L       = 0x44,
	GYRO_YOUT_H       = 0x45,
	GYRO_YOUT_L       = 0x46,
	GYRO_ZOUT_H       = 0x47,
	GYRO_ZOUT_L       = 0x48,

	/// accelerometer
	ACCEL_XOUT_H      = 0x3B,
	ACCEL_XOUT_L      = 0x3C,
	ACCEL_YOUT_H      = 0x3D,
	ACCEL_YOUT_L      = 0x3E,
	ACCEL_ZOUT_H      = 0x3F,
	ACCEL_ZOUT_L      = 0x40,

	// thermometer
	TEMP_OUT_H        = 0x41,
	TEMP_OUT_L        = 0x42,
};

/**
 * @brief Create gyro instance.
 *
 * @return CVI_SUCCESS if create succeed.
 */
CVI_S32 CVI_GYRO_Create(void);

/**
 * @brief Destroy gyro instance.
 */
CVI_VOID CVI_GYRO_Destroy(void);

/**
 * @brief Reset gyro.
 *
 * @return CVI_SUCCESS if create succeed.
 */
CVI_S32 CVI_GYRO_Reset(void);

/**
 * @brief Check gyro sensor id.
 *
 * @param val Receive sensor id result, expect 0x70.
 * @return CVI_SUCCESS if create succeed.
 */
CVI_S32 CVI_GYRO_WHO_AM_I(CVI_U8 *val);

/**
 * @brief Gyro (x y z axis) calibration.
 *
 * @return CVI_SUCCESS if calibration succeed.
 */
CVI_S32 CVI_GYRO_CALIBRATION(void);

/**
 * @brief Accelerometer (x y z axis) calibration.
 *
 * @return CVI_SUCCESS if calibration succeed.
 */
CVI_S32 CVI_GYRO_ACC_CALIBRATION(void);

/**
 * @brief Get gyro x, y, z axis value.
 *
 * @param val Receive axis result.
 * @return CVI_SUCCESS if create succeed.
 */
CVI_S32 CVI_GYRO_GET_GX(CVI_S16 *val);
CVI_S32 CVI_GYRO_GET_GY(CVI_S16 *val);
CVI_S32 CVI_GYRO_GET_GZ(CVI_S16 *val);

/**
 * @brief Get accelerometer x, y, z axis value.
 *
 * @param val Receive axis result.
 * @return CVI_SUCCESS if create succeed.
 */
CVI_S32 CVI_GYRO_GET_AX(CVI_S16 *val);
CVI_S32 CVI_GYRO_GET_AY(CVI_S16 *val);
CVI_S32 CVI_GYRO_GET_AZ(CVI_S16 *val);

/**
 * @brief Get acc/gyro x, y, z axis value at same time.
 *
 * @param x_val Receive x axis result.
 * @param y_val Receive y axis result.
 * @param z_val Receive z axis result.
 * @return CVI_SUCCESS if create succeed.
 */
CVI_S32 CVI_GYRO_GET_G_XYZ(CVI_S16 *x_val, CVI_S16 *y_val, CVI_S16 *z_val);
CVI_S32 CVI_GYRO_GET_A_XYZ(CVI_S16 *x_val, CVI_S16 *y_val, CVI_S16 *z_val);

/**
 * @brief Get thermometer value.
 *
 * @param val Receive thermometer result.
 * @return CVI_SUCCESS if create succeed.
 */
CVI_S32 CVI_GYRO_GET_TEMP(CVI_S16 *val);

/**
 * @brief Read mpuxxxx register address value.
 *
 * @param addr Set register address, you want.
 * @param val Receive address value.
 * @return CVI_SUCCESS if create succeed.
 */
CVI_S32 CVI_GYRO_READ(CVI_U8 addr, CVI_U8 *val);

/**
 * @brief Write value to mpuxxxx register address.
 *
 * @param addr Set register address, you want.
 * @param val Set value to addr.
 * @return CVI_SUCCESS if create succeed.
 */
CVI_S32 CVI_GYRO_WRITE(CVI_U8 addr, CVI_U8 val);

/**
 * @brief The value with addr OR first, then write.
 *
 * @param addr Set register address, you want.
 * @param val Set (value | addr value) to addr.
 * @return CVI_SUCCESS if create succeed.
 */
CVI_S32 CVI_GYRO_WRITE_OR(CVI_U8 addr, CVI_U8 val);

/**
 * @brief The value with addr AND first, then write.
 *
 * @param addr Set register address, you want.
 * @param val Set (value & addr value) to addr.
 * @return CVI_SUCCESS if create succeed.
 */
CVI_S32 CVI_GYRO_WRITE_AND(CVI_U8 addr, CVI_U8 val);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif // __CVI_GYRO_H__
