/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef _CVI_EFUSE_H
#define _CVI_EFUSE_H 1

#ifndef CVI_U8
#define CVI_U8 uint8_t
#endif
#ifndef CVI_U32
#define CVI_U32 uint32_t
#endif
#ifndef CVI_S8
#define CVI_S8 int8_t
#endif
#ifndef CVI_S32
#define CVI_S32 int32_t
#endif

#ifndef CVI_SUCCESS
#define CVI_SUCCESS 0
#endif
#ifndef CVI_FAILURE
#define CVI_FAILURE (-1)
#endif

#define CVI_ERR_EFUSE_INVALID_AREA -0x0001
#define CVI_ERR_EFUSE_INVALID_PARA -0x0002

enum CVI_EFUSE_AREA_E {
	CVI_EFUSE_AREA_USER,
	CVI_EFUSE_AREA_DEVICE_ID,
	CVI_EFUSE_AREA_HASH0_PUBLIC,
	CVI_EFUSE_AREA_LOADER_EK,
	CVI_EFUSE_AREA_DEVICE_EK,
	CVI_EFUSE_AREA_LAST
};

enum CVI_EFUSE_LOCK_E {
	CVI_EFUSE_LOCK_HASH0_PUBLIC = CVI_EFUSE_AREA_LAST + 1,
	CVI_EFUSE_LOCK_LOADER_EK,
	CVI_EFUSE_LOCK_DEVICE_EK,
	CVI_EFUSE_LOCK_LAST
};

enum CVI_EFUSE_OTHERS_E {
	CVI_EFUSE_SECUREBOOT = CVI_EFUSE_LOCK_LAST + 1,
	CVI_EFUSE_OTHERS
};

CVI_S32 CVI_EFUSE_get_size(enum CVI_EFUSE_AREA_E area, CVI_U32 *size);
CVI_S32 CVI_EFUSE_read(enum CVI_EFUSE_AREA_E area, CVI_U8 *buf, CVI_U32 buf_size);
CVI_S32 CVI_EFUSE_write(enum CVI_EFUSE_AREA_E area, const CVI_U8 *buf, CVI_U32 buf_size);
CVI_S32 CVI_EFUSE_enable_secure_boot(uint32_t sel);
CVI_S32 CVI_EFUSE_is_secure_boot_enabled(void);
CVI_S32 CVI_EFUSE_lock(enum CVI_EFUSE_LOCK_E lock);
CVI_S32 CVI_EFUSE_is_locked(enum CVI_EFUSE_LOCK_E lock);
CVI_S32 CVI_EFUSE_lock_write(enum CVI_EFUSE_LOCK_E lock);
CVI_S32 CVI_EFUSE_is_write_locked(enum CVI_EFUSE_LOCK_E lock);

#endif /* _CVI_EFUSE_H */
