/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: reg_tiu.h
 * Description: tiu register define header file
 */

#ifndef __REG_BDCAST_H__
#define __REG_BDCAST_H__

#define BDC_ENGINE_CMD_ALIGNED_BIT	8

//base related virtual address
#define TIU_ENGINE_BASE_ADDR		0
#define BD_CMD_BASE_ADDR			(TIU_ENGINE_BASE_ADDR + 0)
#define BD_CTRL_BASE_ADDR			(TIU_ENGINE_BASE_ADDR + 0x100)

// BD control bits base on BD_CTRL_BASE_ADDR
#define BD_TPU_EN					0    // TPU Enable bit
#define BD_LANE_NUM					22   // Lane number bit[29:22]
#define BD_DES_ADDR_VLD				30   // enable descriptor mode
#define BD_INTR_ENABLE				31   // TIU interrupt global enable

enum TIU_LANNUM {
	TIU_LANNUM_2		= 0x1,
	TIU_LANNUM_4		= 0x2,
	TIU_LANNUM_8		= 0x3,
	TIU_LANNUM_16		= 0x4,
	TIU_LANNUM_32		= 0x5,
	TIU_LANNUM_64		= 0x6,
};

#endif /* __REG_TIU_H__ */
