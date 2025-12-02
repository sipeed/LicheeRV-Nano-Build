/**
 ******************************************************************************
 *
 * @file rwnx_main.h
 *
 * Copyright (C) RivieraWaves 2012-2019
 *
 ******************************************************************************
 */

#ifndef _RWNX_MAIN_H_
#define _RWNX_MAIN_H_

#include "rwnx_defs.h"

struct rwnx_sta *rwnx_retrieve_sta(struct rwnx_hw *rwnx_hw,
                                          struct rwnx_vif *rwnx_vif, u8 *addr,
                                          __le16 fc, bool ap);


#ifdef CONFIG_BAND_STEERING
void aicwf_steering_work(struct work_struct *work);
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
void aicwf_steering_timeout(ulong data);
#else
void aicwf_steering_timeout(struct timer_list *t);
#endif
#endif

int rwnx_cfg80211_init(struct rwnx_plat *rwnx_plat, void **platform_data);
void rwnx_cfg80211_deinit(struct rwnx_hw *rwnx_hw);
extern int testmode;
extern u8 chip_sub_id;
extern u8 chip_mcu_id;
extern u8 chip_id;

#define CHIP_ID_H_MASK  0xC0
#define IS_CHIP_ID_H()  ((chip_id & CHIP_ID_H_MASK) == CHIP_ID_H_MASK)

#endif /* _RWNX_MAIN_H_ */
