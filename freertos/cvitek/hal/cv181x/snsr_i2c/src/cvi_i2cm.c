/*
 * Copyright (c) 2019, Cvitek. All rights reserved.
 *
 */
#ifdef RUN_IN_SRAM
#include "config.h"
#include "marsrv_common.h"
#include "fw_config.h"
#elif (RUN_TYPE == CVIRTOS)
#include <stdio.h>
#include <stdint.h>

#include "cvi_type.h"
#include "top_reg.h"

#endif

// #include "dw_uart.h"
#include "mmio.h"
#include "delay.h"
#include "cvi_i2c.h"
#if CVI_I2C_DMA_ENABLE
#include "sysdma.h"
#endif

#define I2C_WRITEL(reg, val) mmio_write_32(ctrl_base + (reg), val)
#define I2C_READL(reg) mmio_read_32(ctrl_base + (reg))

struct i2c_info g_i2c[CVI_I2C_MAX_NUM] = {0};

#ifndef INFO
#define INFO(...)	{}//printf(__VA_ARGS__)
#endif

//
//  cvi_i2c_master_write(uint16_t reg, uint8_t data)
//		reg: 16-bits register address
//		data: 8-bits data
//
int cvi_i2c_master_write(uint8_t bus_id, uint16_t reg, uint16_t value)
{
	unsigned long ctrl_base;
	unsigned int try_cnt = 100; // timeout = 1ms

	if (bus_id >= CVI_I2C_MAX_NUM || !g_i2c[bus_id].enable) {
		printf("ERROR: I2CW wrong i2c bus id=%d !!\n", bus_id);
		return CVI_FAILURE;
	}
	ctrl_base = g_i2c[bus_id].ctrl_base;

	INFO("I2C write 0x%x = 0x%x\n", reg, value);

	// 1. send reg addr - send MSB first customized for sensor
	if (g_i2c[bus_id].alen == 2) {
		I2C_WRITEL(REG_I2C_DATA_CMD, ((reg >> 8) & 0xFF) | BIT_I2C_CMD_DATA_RESTART_BIT);
		I2C_WRITEL(REG_I2C_DATA_CMD, reg & 0xFF);
	} else {
		I2C_WRITEL(REG_I2C_DATA_CMD, (reg & 0xFF) | BIT_I2C_CMD_DATA_RESTART_BIT);
	}

	// 2. send data
	if (g_i2c[bus_id].dlen == 2)
		I2C_WRITEL(REG_I2C_DATA_CMD, value);
	I2C_WRITEL(REG_I2C_DATA_CMD, value | BIT_I2C_CMD_DATA_STOP_BIT);

	while (try_cnt-- > 0) {
		uint32_t irq = I2C_READL(REG_I2C_RAW_INT_STAT);

		if (irq & BIT_I2C_INT_TX_EMPTY)
			break;

		udelay(10);
	}

	if (!try_cnt) {		// timeout
		printf("I2C write 0x%x = 0x%x fail!\n", reg, value);
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

//
//  cvi_i2c_master_read(uint16_t reg)
//		reg: 16-bits register address
//		return 8-bits data
//
uint16_t cvi_i2c_master_read(uint8_t bus_id, uint16_t reg)
{
	unsigned long ctrl_base;
	uint16_t value = 0xff;
	uint8_t try_cnt = 100; // timeout = 1ms

	if (bus_id >= CVI_I2C_MAX_NUM || !g_i2c[bus_id].enable) {
		printf("ERROR: I2CR wrong i2c bus id=%d !!\n", bus_id);
		return CVI_FAILURE;
	}
	ctrl_base = g_i2c[bus_id].ctrl_base;

	// 1. send reg addr - send MSB first customized for sensor
	if (g_i2c[bus_id].alen == 2) {
		I2C_WRITEL(REG_I2C_DATA_CMD, ((reg >> 8) & 0xFF) | BIT_I2C_CMD_DATA_RESTART_BIT);
		I2C_WRITEL(REG_I2C_DATA_CMD, reg & 0xFF);
	} else {
		I2C_WRITEL(REG_I2C_DATA_CMD, (reg & 0xFF) | BIT_I2C_CMD_DATA_RESTART_BIT);
	}

	// 2. send read cmd
	I2C_WRITEL(REG_I2C_DATA_CMD, 0 |
		BIT_I2C_CMD_DATA_READ_BIT |
		BIT_I2C_CMD_DATA_STOP_BIT);

	while (try_cnt-- > 0) {
		uint32_t irq = I2C_READL(REG_I2C_RAW_INT_STAT);

		if (irq & BIT_I2C_INT_RX_FULL) {
			// 3. get read data
			value = I2C_READL(REG_I2C_DATA_CMD);
			if (g_i2c[bus_id].dlen == 2)
				value = (value << 8) + I2C_READL(REG_I2C_DATA_CMD);

			INFO("I2C read out 0x%x\n", value);
			break;
		}

		udelay(10);
	}

	return value;
}

//
//  cvi_i2c_master_init(unsigned long base, uint16_t addr)
//		base: I2C base address
//		addr: slave address
//		return 0: success, 1: fail
//
int cvi_i2c_master_init(uint8_t bus_id, uint16_t slave_id, uint16_t speed, uint8_t alen, uint8_t dlen)
{
	unsigned long ctrl_base;
	uint32_t value = 0;

	if (bus_id >= CVI_I2C_MAX_NUM) {
		printf("ERROR: init wrong i2c bus id=%d !!\n", bus_id);
		return CVI_FAILURE;
	}

	g_i2c[bus_id].ctrl_base = (I2C0_BASE + 0x10000UL * bus_id);
	ctrl_base = g_i2c[bus_id].ctrl_base;
	g_i2c[bus_id].slave_id = slave_id;
	g_i2c[bus_id].alen = alen;
	g_i2c[bus_id].dlen = dlen;

	I2C_WRITEL(REG_I2C_ENABLE, 0);

	if (speed == I2C_400KHZ) {
		value = BIT_I2C_CON_MASTER_MODE |
			BIT_I2C_CON_SLAVE_DIS |
			BIT_I2C_CON_RESTART_EN |
			BIT_I2C_CON_FULL_SPEED;
	} else {	// default is 100KHz
		value = BIT_I2C_CON_MASTER_MODE |
			BIT_I2C_CON_SLAVE_DIS |
			BIT_I2C_CON_RESTART_EN |
			BIT_I2C_CON_STANDARD_SPEED;
	}

	I2C_WRITEL(REG_I2C_CON, value);

	I2C_WRITEL(REG_I2C_TAR,  slave_id);

	// to get a 100KHz SCL when I2C source clock is 100MHz
	I2C_WRITEL(REG_I2C_SS_SCL_HCNT, 0x1A4);
	I2C_WRITEL(REG_I2C_SS_SCL_LCNT, 0x1F0);
	I2C_WRITEL(REG_I2C_FS_SCL_HCNT, 90);
	I2C_WRITEL(REG_I2C_FS_SCL_LCNT, 160);

	I2C_WRITEL(REG_I2C_INT_MASK, 0x0);

	I2C_WRITEL(REG_I2C_RX_TL,  0x00);
	I2C_WRITEL(REG_I2C_TX_TL,  0x01);

	// DMA mode
	I2C_WRITEL(REG_I2C_DMA_CR, BIT_I2C_DMA_CR_RDMAE | BIT_I2C_DMA_CR_TDMAE);
	I2C_WRITEL(REG_I2C_DMA_TDLR, 0x1);
	I2C_WRITEL(REG_I2C_DMA_RDLR, 0x1);

	I2C_WRITEL(REG_I2C_ENABLE, 1);

	I2C_READL(REG_I2C_CLR_INTR);

	g_i2c[bus_id].enable = 1;
	return CVI_SUCCESS;
}

#if CVI_I2C_DMA_ENABLE
//
//  cvi_i2c_master_dma_init(unsigned long base, uint32_t num_of_blk, uint32_t ch,
//				uint32_t* llp, uint32_t* tx_buf, uint32_t len)
//		base: I2C base address
//		num_of_blk: number dma LLP block, default is 1
//		ch: DMA channel number
//		llp: pointer to LLP descriptor
//		tx_buf: pointer to I2C tx buffer
//		len: length of tx buffer
//		return none
//
void cvi_i2c_master_dma_init(unsigned long base, uint32_t ch, uint16_t *tx_buf, uint32_t len)
{
	uint64_t ctl, ctl_last, cfg;
	uint32_t llp = mmio_read_32(KEEP_SYSDMA_LLP_ADDR);	// read SYSDMA_LLP_ADDR from ATF

	// Force ro flush tx_buf to DRAM
	//flush_dcache_range((uint64_t)tx_buf, len);

	ctl = ((uint64_t)0x0 << 14)	//SRC_MSIZE (0x0: 1 Data Item read from Source in the burst transaction)
		| ((uint64_t)0x0 << 18)	//DST_MSIZE (0x0: 1 Data Item read from Destination in the burst transaction)
		| ((uint64_t)0x1 << 0)	//SMS (0x1: Destination device on Master-2interface layer)
		| ((uint64_t)0x1 << 2)	//DMS (0x1: Destination device on Master-2interface layer)
		| ((uint64_t)0x0 << 4)	//SINC (0: Increment)
		| ((uint64_t)0x1 << 6)	//DINC (1: No change)
		| ((uint64_t)0x1 << 8)	//SRC_TR_WIDTH (0x1: 16bits data)
		| ((uint64_t)0x1 << 11)	//DST_TR_WIDTH (0x1: 16bits data)
		| ((uint64_t)0x1 << 63);	//SHADOWREG_OR_LLI_VALID

	mmio_write_64((uint64_t)llp + LLI_OFFSET_SAR, (uint64_t)tx_buf); //SAR
	mmio_write_64((uint64_t)llp + LLI_OFFSET_DAR, base + REG_I2C_DATA_CMD); //DAR
	mmio_write_64((uint64_t)llp + LLI_OFFSET_BTS, len / 2 - 1); //BLOCK_TS[20:0]: block transfer size
	//Starting Address In Memory of next LLI
	mmio_write_64((uint64_t)llp + LLI_OFFSET_LLP, LLI_BLK_SIZE + (uint64_t)llp);

	ctl_last = ctl | ((uint64_t)1 << 62);
	mmio_write_64((uint64_t)llp + LLI_OFFSET_CTL, ctl_last); //Make itself the last LLP block

	// Flush D-cache
	//flush_dcache_range((uint64_t)llp, LLI_BLK_SIZE);

	cfg = ((uint64_t)15 << 55)  //source outstanding request limit = 16
		| ((uint64_t)15 << 59)  //destination outstanding request limit = 16
		| ((uint64_t)7 << 49)   //7: highest priority, 0 is lowest priority
		| ((uint64_t)3 << 2)    //3: linked list type of destination
		| ((uint64_t)3 << 0)    //3: linked list type of source
		| ((uint64_t)1 << 32)   //1: MEM_TO_PER_DMA
		| ((uint64_t)0 << 36)   //0: hw Handshaking select of destination
		| ((uint64_t)0 << 35)   //0: hw Handshaking select of source
		| ((uint64_t)ch << 39)  //hw handshake interface of source
		| ((uint64_t)ch << 44); //hw handshake interface of destination

	SYSDMA_W64(CH_CFG + ch * 0x100, cfg);	//set CFG reg
	SYSDMA_W64(CH_LLP + ch * 0x100, (uint64_t)llp); //set the first llp mem addr to CHANNEL LLP reg
}

void cvi_i2c_dma_bind_ch(uint32_t i2c_ch)
{
	i2c_ch = DMA_TX_REQ_I2C0 + (i2c_ch << 1);

	mmio_clrsetbits_32(TOP_DMA_CH_REMAP1, 0x3F << DMA_REMAP_CH5_OFFSET, i2c_ch << DMA_REMAP_CH5_OFFSET);
	mmio_clrsetbits_32(TOP_DMA_CH_REMAP1, 0x1UL << DMA_REMAP_UPDATE_OFFSET, 1UL << DMA_REMAP_UPDATE_OFFSET);
}

//
//  cvi_i2c_master_dma_write()
//		ch: DMA channel number
//		return none
//
void cvi_i2c_master_dma_write(uint32_t ch)
{
	cvi_i2c_enable_dma();

	while (cvi_i2c_is_dma_ch_enable(ch))
		;

	cvi_i2c_enable_dma_ch(ch);
}

//
//  cvi_i2c_enable_dma()
//		return none
//
void cvi_i2c_enable_dma(void)
{
//	SYSDMA_W64(DMAC_CFGREG, 0x1);
	uint32_t val = SYSDMA_R32(DMAC_CFGREG);

	SYSDMA_W32(DMAC_CFGREG, val | 0x1);
}

//
//  cvi_i2c_enable_dma()
//		return none
//
void cvi_i2c_disable_dma(void)
{
//	SYSDMA_W64(DMAC_CFGREG, 0x0);
	SYSDMA_W32(DMAC_CFGREG, 0x0);
}

//
//  cvi_i2c_enable_dma_ch(uint32_t ch)
//		return none
//
void cvi_i2c_enable_dma_ch(uint32_t ch)
{
//	SYSDMA_W64(DMAC_CHENREG, 1<<ch|1<<(ch+8));
	SYSDMA_W32(DMAC_CHENREG, 1 << ch | 1 << (ch + 8));
}

//
//  cvi_i2c_is_dma_ch_enable(uint32_t ch)
//		return 0: dma transfer is done
//		return 1: dma transfer is on-going
//
uint8_t cvi_i2c_is_dma_ch_enable(uint32_t ch)
{
//	if (SYSDMA_R64(DMAC_CHENREG) & (1<<ch))
	if (SYSDMA_R32(DMAC_CHENREG) & (1 << ch))
		return 1;
	else
		return 0;
}
#endif
