#ifndef _AICWF_STEERING_H_
#define _AICWF_STEERING_H_

#include <linux/spinlock.h>
#include "lmac_types.h"

#ifdef CONFIG_BAND_STEERING

#define B_STEER_ENTRY_NUM                   64//32

#define B_STEER_BLOCK_ENTRY_EXPIRE          60
#define B_STEER_ROAM_BLOCK_ENTRY_EXPIRE     5
#define STEER_UPFATE_TIME                   2000

struct rwnx_vif;

struct b_steer_block_entry {
	u8_l used;
	u8_l mac[6];
	u32_l entry_expire;
};

struct b_steer_priv {
	struct b_steer_block_entry block_list[B_STEER_ENTRY_NUM];
	spinlock_t lock;
	bool inited;
};


void aicwf_band_steering_expire(struct rwnx_vif *rwnx_vif);
s32_l  aicwf_band_steering_block_chk(struct rwnx_vif *rwnx_vif, u8_l *mac);
void aicwf_band_steering_block_entry_add(struct rwnx_vif *rwnx_vif, u8_l *mac);
void aicwf_band_steering_block_entry_del(struct rwnx_vif *rwnx_vif, u8_l *mac);
void aicwf_band_steering_roam_block_entry_add(struct rwnx_vif *rwnx_vif, u8_l *mac);
void aicwf_band_steering_init(struct rwnx_vif *rwnx_vif);

#endif

#endif

