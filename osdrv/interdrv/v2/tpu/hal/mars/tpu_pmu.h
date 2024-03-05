/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: tpu_pmu.h
 * Description: tpu pmu control header file
 */

#ifndef __TPU_PMU_H__
#define __TPU_PMU_H__

#include "tpu_platform.h"

enum TPU_PMUEVENT {
	TPU_PMUEVENT_BANKCONFLICT	= 0x0,
	TPU_PMUEVENT_STALLCNT		= 0x1,
	TPU_PMUEVENT_TDMABW			= 0x2,
	TPU_PMUEVENT_TDMAWSTRB		= 0x3,
};

enum TPU_PMUTYPE {
	TPU_PMUTYPE_TDMALOAD		= 1,
	TPU_PMUTYPE_TDMASTORE		= 2,
	TPU_PMUTYPE_TDMAMOVE		= 3,
	TPU_PMUTYPE_TIU				= 4,
};

struct TPU_PMU_DOUBLEEVENT {
	u64 type:4;
	u64 desID:16;
	u64 eventCnt0:22;
	u64 eventCnt1:22;
	u32 endTime;
	u32 startTime;
};

int TPUPMU_Enable(struct TPU_PLATFORM_CFG *pCfg, u8 enable, enum TPU_PMUEVENT event);
int TPUPMU_ParsingResult(u8 *pbuf_start);


#endif
