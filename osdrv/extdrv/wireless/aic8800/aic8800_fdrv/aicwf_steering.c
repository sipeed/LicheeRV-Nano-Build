/**
 ******************************************************************************
 *
 * Copyright (C) 2020 AIC semiconductor.
 *
 * @file aicwf_steering.c
 *
 * @brief band steering definitions
 *
 ******************************************************************************
 */

#include "aicwf_steering.h"
#include "rwnx_defs.h"

#define MAC_FMT_B "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_ARG_B(x) ((u8_l *)(x))[0], ((u8_l *)(x))[1], ((u8_l *)(x))[2], ((u8_l *)(x))[3], ((u8_l *)(x))[4], ((u8_l *)(x))[5]
#define STEEER_STR "[STEERING] SDIO "

#ifdef CONFIG_BAND_STEERING

static struct b_steer_block_entry *block_entry_lookup(struct rwnx_vif *rwnx_vif, u8_l *mac)
{
	u8_l i;
	struct b_steer_block_entry *ent = NULL;

	for (i = 0; i < B_STEER_ENTRY_NUM; i++) {
		ent = &(rwnx_vif->bsteerpriv.block_list[i]);
		if (ent->used && (memcmp(ent->mac, mac, 6) == 0)) {
			AICWFDBG(LOGSTEER, STEEER_STR"%s "MAC_FMT_B"\n", __func__, MAC_ARG_B(mac));
			return ent;
		}
	}

	return NULL;
}

void aicwf_band_steering_expire(struct rwnx_vif *rwnx_vif)
{
	u8_l i;
	struct b_steer_block_entry *ent = NULL;

	if (rwnx_vif->bsteerpriv.inited == false) {
		AICWFDBG(LOGSTEER, STEEER_STR"%s bsteerpriv not inited\n", __func__);
		return;
	}

	spin_lock_bh(&(rwnx_vif->bsteerpriv.lock));

	/* block entry */
	for (i = 0; i < B_STEER_ENTRY_NUM; i++) {
		ent = &(rwnx_vif->bsteerpriv.block_list[i]);
		if (!ent->used)
			continue;

		if (ent->entry_expire) {
			ent->entry_expire--;
			if (ent->entry_expire == 0)
				ent->used = 0;
		}
	}

	spin_unlock_bh(&(rwnx_vif->bsteerpriv.lock));

	return;
}

s32_l aicwf_band_steering_block_chk(struct rwnx_vif *rwnx_vif, u8_l *mac)
{
	s32 ret = 0;
	struct b_steer_block_entry *ent = NULL;

	if (rwnx_vif->bsteerpriv.inited == false) {
		AICWFDBG(LOGSTEER, STEEER_STR"%s bsteerpriv not inited\n", __func__);
		return 0;
	}

	spin_lock_bh(&(rwnx_vif->bsteerpriv.lock));

	ent = block_entry_lookup(rwnx_vif, mac);
	if (ent)
		ret = 1;

	spin_unlock_bh(&(rwnx_vif->bsteerpriv.lock));

	return ret;
}

void aicwf_band_steering_block_entry_add(struct rwnx_vif *rwnx_vif, u8_l *mac)
{
	u8_l i;
	struct b_steer_block_entry *ent = NULL;

	if (rwnx_vif->bsteerpriv.inited == false) {
		AICWFDBG(LOGSTEER, STEEER_STR"%s bsteerpriv not inited\n", __func__);
		return;
	}

	AICWFDBG(LOGSTEER, STEEER_STR"%s "MAC_FMT_B"\n", __func__, MAC_ARG_B(mac));

	spin_lock_bh(&(rwnx_vif->bsteerpriv.lock));

	ent = block_entry_lookup(rwnx_vif, mac);

	/* already exist */
	if (ent) {
		AICWFDBG(LOGSTEER, STEEER_STR"%s already exist\n", __func__);
		ent->entry_expire = B_STEER_BLOCK_ENTRY_EXPIRE;
		goto func_return;
	}

	/* find an empty entry */
	for (i = 0; i < B_STEER_ENTRY_NUM; i++) {
		if (!rwnx_vif->bsteerpriv.block_list[i].used) {
			ent = &(rwnx_vif->bsteerpriv.block_list[i]);
			break;
		}
	}

	/* add the entry */
	if (ent) {
		AICWFDBG(LOGSTEER, STEEER_STR"%s "MAC_FMT_B"\n", __func__, MAC_ARG_B(mac));
		ent->used = 1;
		memcpy(ent->mac, mac, 6);
		ent->entry_expire = B_STEER_BLOCK_ENTRY_EXPIRE;
	}

func_return:
	spin_unlock_bh(&(rwnx_vif->bsteerpriv.lock));

	return;
}

void aicwf_band_steering_block_entry_del(struct rwnx_vif *rwnx_vif, u8_l *mac)
{
	struct b_steer_block_entry *ent = NULL;

	if (rwnx_vif->bsteerpriv.inited == false) {
		AICWFDBG(LOGSTEER, STEEER_STR"%s bsteerpriv not inited\n", __func__);
		return;
	}

	AICWFDBG(LOGSTEER, STEEER_STR"%s "MAC_FMT_B"\n", __func__, MAC_ARG_B(mac));

	spin_lock_bh(&(rwnx_vif->bsteerpriv.lock));

	ent = block_entry_lookup(rwnx_vif, mac);
	if (ent)
		ent->used = 0;

	spin_unlock_bh(&(rwnx_vif->bsteerpriv.lock));

	return;
}

void aicwf_band_steering_roam_block_entry_add(struct rwnx_vif *rwnx_vif, u8_l *mac)
{
	u8_l i;
	struct b_steer_block_entry *ent = NULL;

	if (rwnx_vif->bsteerpriv.inited == false) {
		AICWFDBG(LOGSTEER, STEEER_STR"%s bsteerpriv not inited\n", __func__);
		return;
	}

	AICWFDBG(LOGSTEER, STEEER_STR"%s "MAC_FMT_B"\n", __func__, MAC_ARG_B(mac));

	spin_lock_bh(&rwnx_vif->bsteerpriv.lock);

	ent = block_entry_lookup(rwnx_vif, mac);

	/* already exist */
	if (ent)
		goto func_return;

	/* find an empty entry */
	for (i = 0; i < B_STEER_ENTRY_NUM; i++) {
		if (!rwnx_vif->bsteerpriv.block_list[i].used) {
			ent = &(rwnx_vif->bsteerpriv.block_list[i]);
			break;
		}
	}

	/* add the entry */
	if (ent) {
		ent->used = 1;
		memcpy(ent->mac, mac, 6);
		ent->entry_expire = B_STEER_ROAM_BLOCK_ENTRY_EXPIRE;
	}

func_return:
	spin_unlock_bh(&rwnx_vif->bsteerpriv.lock);

	return;
}

void aicwf_band_steering_init(struct rwnx_vif *rwnx_vif)
{
	u8_l i;

	AICWFDBG(LOGSTEER, STEEER_STR"%s\n", __func__);
	spin_lock_init(&rwnx_vif->bsteerpriv.lock);
	rwnx_vif->bsteerpriv.inited = true;

	spin_lock_bh(&rwnx_vif->bsteerpriv.lock);

	/* block entry */
	for (i = 0; i < B_STEER_ENTRY_NUM; i++)
		memset(&(rwnx_vif->bsteerpriv.block_list[i]), 0, sizeof(struct b_steer_block_entry));

	spin_unlock_bh(&rwnx_vif->bsteerpriv.lock);

	return;
}

#endif

