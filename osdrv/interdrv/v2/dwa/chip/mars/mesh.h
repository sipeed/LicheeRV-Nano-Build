#ifndef _LDC_COMMON_MESH_H
#define _LDC_COMMON_MESH_H

#include <dwa_cb.h>

#define CVI_GDC_MAGIC 0xbabeface

#define CVI_GDC_MESH_SIZE_ROT 0x60000
#define CVI_GDC_MESH_SIZE_AFFINE 0x20000
#define CVI_GDC_MESH_SIZE_FISHEYE 0xB0000

s32 mesh_gdc_do_op(struct cvi_dwa_vdev *wdev, enum GDC_USAGE usage,
		   const void *pUsageParam, struct vb_s *vb_in,
		   PIXEL_FORMAT_E enPixFormat, u64 mesh_addr, CVI_BOOL sync_io,
		   void *pcbParam, u32 cbParamSize, MOD_ID_E enModId,
		   ROTATION_E enRotation);

#endif /* _LDC_COMMON_MESH_H */
