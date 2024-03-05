/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: tpu_pmu.c
 * Description: tpu pmu control driver code
 */

#include <linux/kernel.h>
#include "tpu_pmu.h"
#include "reg_tdma.h"

struct TPU_PMUCONFIG {
	u8 enable;
	u8 enable_tpu;
	u8 enable_tdma;
	enum TPU_PMUEVENT event;
	u16	tpu_syncID_start;
	u16	tpu_syncID_end;
	u16	tdma_syncID_start;
	u16	tdma_syncID_end;
	u32	bufBaseAddr;		//register setting must right shirt 4bits, value >> 4
	u32	bufSize;		//register setting must right shirt 4bits, value >> 4
};


#define TPUPMU_CTRL			(TDMA_ENGINE_BASE_ADDR + 0x200)
#define TPUPMU_BUFBASE		(TDMA_ENGINE_BASE_ADDR + 0x20C)
#define TPUPMU_BUFSIZE		(TDMA_ENGINE_BASE_ADDR + 0x210)

#define TPUPMU_BUFGUARD		0x12345678


static void TPUPMU_Config(struct TPU_PLATFORM_CFG *pCfg, struct TPU_PMUCONFIG *pconfig)
{
	u32 regValue = 0;

	if (pconfig->enable) {
		//set buffer starting and size
		RAW_WRITE32(pCfg->iomem_tdmaBase + TPUPMU_BUFBASE, pconfig->bufBaseAddr);
		RAW_WRITE32(pCfg->iomem_tdmaBase + TPUPMU_BUFSIZE, pconfig->bufSize);

		//set enable related
		regValue |= 0x1;
		if (pconfig->enable_tpu)
			regValue |= 0x8;

		if (pconfig->enable_tdma)
			regValue |= 0x10;

		//set event type
		regValue |= (pconfig->event << 5);

		//set burst length = 16
		regValue |= (0x3 << 8);

		//enable pmu ring buffer mode
		regValue |= (0x1 << 10);

		//enable pmu dcm
		regValue &= ~0xFFFF0000;

		//set control register
		RAW_WRITE32(pCfg->iomem_tdmaBase + TPUPMU_CTRL, regValue);
	} else {
		//disable register
		regValue = RAW_READ32(pCfg->iomem_tdmaBase + TPUPMU_CTRL);
		RAW_WRITE32(pCfg->iomem_tdmaBase + TPUPMU_CTRL, regValue & ~(0x1));

#if 0
		//waiting writing done
		while (1) {
			//regValue = RAW_READ32(TDMA_INT_MASK);

			//if (regValue & 0x80000000)
			if (u8TDMA_INTACK) {
				//RAW_WRITE32(TDMA_INT_MASK, 0x80000000);
				u8TDMA_INTACK = 0;
				break;
			}

			udelay(1000);
		}
#endif

	}

}

#if 0
void TPUPMU_ResetBuf(u8 *pBuf, u64 length)
{
	u32	i;

	for (i = 0; i < length; i += sizeof(TPU_PMU_DOUBLEEVENT)) {
		*((u32 *)pBuf) = TPUPMU_BUFGUARD;
		pBuf += sizeof(TPU_PMU_DOUBLEEVENT);
	}
}
#endif

int TPUPMU_Enable(struct TPU_PLATFORM_CFG *pCfg, u8 enable, enum TPU_PMUEVENT event)
{
	struct TPU_PMUCONFIG	config;

	if (enable) {
		u64 bufAddr = pCfg->pmubuf_addr_p;
		u64 bufSize = pCfg->pmubuf_size;
		//u64 bufSize = 0x20;

		//TPUPMU_ResetBuf((u8*)bufAddr, bufSize);

		//right shift 4 bits
		bufAddr = bufAddr >> 4;
		bufSize = bufSize >> 4;

		config.enable = 1;
		config.event = event;
		config.enable_tdma = 1;
		config.enable_tpu = 1;
		config.bufBaseAddr = bufAddr;
		config.bufSize = bufSize;
	} else {
		config.enable = 0;
	}

	TPUPMU_Config(pCfg, &config);
	return 0;
}

