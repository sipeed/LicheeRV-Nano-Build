// SPDX-License-Identifier: GPL-2.0-or-later
/* Driver for CVITEK PHYs */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/phy.h>
#include <linux/netdevice.h>
#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/cv180x_efuse.h>

// #define NEW_ETH_DETECT

#define REG_EPHY_TOP_WRAP 0x03009800
#define REG_EPHY_BASE 0x03009000
#define EPHY_EFUSE_TXECHORC_FLAG 0x00000100 // bit 8
#define EPHY_EFUSE_TXITUNE_FLAG 0x00000200 // bit 9
#define EPHY_EFUSE_TXRXTERM_FLAG 0x00000800 // bit 11

#define CVI_INT_EVENTS \
	(CVI_LNK_STS_CHG_INT_MSK | CVI_MGC_PKT_DET_INT_MSK)

#ifdef NEW_ETH_DETECT
static u32 link_status;
static u32 retry_time;
#endif
static int cv182xa_phy_config_intr(struct phy_device *phydev)
{
	return 0;
}

static int cv182xa_phy_ack_interrupt(struct phy_device *phydev)
{
	return 0;
}

static int cv182xa_read_status(struct phy_device *phydev)
{
#ifdef NEW_ETH_DETECT
	u32 lp_val, lp_val_cap, cap_val, cap_val_temp, i, ramdom_cap;
	u32 get_random;
	static void __iomem *ADC3_register;
	static void __iomem *ADC2_register;

	if (!ADC3_register) {
		ADC3_register = ioremap(0x030010F0, 0x8);
		if (!ADC3_register)
			pr_err("ioremap failed!!!");
		ADC2_register = ADC3_register + 4;
	}
#endif
	int err = genphy_read_status(phydev);
#ifdef NEW_ETH_DETECT
	// pr_notice("link status=%x, retry_time=%x\n", phydev->link, retry_time);
	if (retry_time > 0)
		retry_time--;

	if (phydev->link == 0) {
		if (retry_time == 0)
			link_status = 0;

	} else if (phydev->speed == SPEED_100 && link_status == 0) {
		link_status = 1;
		lp_val = phy_read(phydev, 0x5);
		pr_err("lp1=%x\n", lp_val);
		if (phydev->autoneg == AUTONEG_ENABLE && lp_val == 0x4d61) {
			cap_val = phy_read(phydev, 0x4);
			pr_notice("cap1=%x\n", cap_val);
			get_random_bytes(&get_random, sizeof(int32_t));
			//ramdom_cap = (get_random % 13) << 5;
			//if(ramdom_cap == 0x160)
			//{
			//	ramdom_cap = 0x1e0;
			//}
			//cap_val_temp = cap_val & ~0x1e0 | ramdom_cap;
			ramdom_cap =  get_random & 0xde0 | 0x20;
			if (ramdom_cap == 0xd60) {
				pr_notice("ramdom_cap=%x\n\n", ramdom_cap);
				ramdom_cap = 0xde0;
			}
			cap_val_temp = cap_val & ~0xde0 | ramdom_cap;
			phy_write(phydev, 0x4, cap_val_temp);
			pr_notice("get_random=%x, ramdom_cap=%x, cap_val_temp=%x\n",
				  get_random, ramdom_cap, cap_val_temp);
			for (i = 0; i < 150; i++) {
				if ((phy_read(phydev, 0x1) & 0x20) == 0)
					break;

				mdelay(10);
				}
			pr_notice("i=%d\n", i);
			phy_modify(phydev, MII_BMCR, BMCR_ISOLATE, BMCR_ANENABLE | BMCR_ANRESTART);
			for (i = 0; i < 1000; i++) {
				if (phy_read(phydev, 0x1) & 0x20)
					break;
				mdelay(10);
			}
			lp_val = phy_read(phydev, 0x5);
			lp_val_cap = lp_val & 0xde0;
			pr_notice(" %d, link status=%x, lp2=%x\n", i, phy_read(phydev, 0x1), lp_val);

			if (((phy_read(phydev, 0x1) & 0x4) != 0) && lp_val != 0 && lp_val_cap != ramdom_cap) {
				retry_time = 10;
				phy_write(phydev, 0x4, cap_val);
				phy_modify(phydev, MII_BMCR, BMCR_ISOLATE, BMCR_ANENABLE | BMCR_ANRESTART);
				for (i = 0; i < 1000; i++) {
					if (phy_read(phydev, 0x1) & 0x20)
						break;

					mdelay(10);
				}
				//mdelay(8000);
				//lp_val = phy_read(phydev, 0x5);
				pr_notice(" %d, true link status=%x, lp2=%x\n", i,
					  phy_read(phydev, 0x1), phy_read(phydev, 0x5));
			} else {
				phydev->link = 0;
				link_status = 0;
				phy_write(phydev, 0x4, cap_val);
				phy_modify(phydev, MII_BMCR, BMCR_ISOLATE, BMCR_ANENABLE | BMCR_ANRESTART);
				//err = phy_start_aneg(phydev);
				//lp_val = phy_read(phydev, 0x5);
				//pr_notice("lp3=%x\n", lp_val);
				pr_notice(" %d, false link status=%x, lp2=%x\n", i,
					  phy_read(phydev, 0x1), phy_read(phydev, 0x5));
			}

			//err = genphy_read_status(phydev);
			//lp_val = phy_read(phydev, 0x5);
			//pr_notice("lp3=%x\n", lp_val);
		}
	}

	if (phydev->speed == SPEED_100) {
		phy_write(phydev, 0x1f, 0x100);
		if (phydev->link == 0) {
			// select LED_LNK/SPD/DPX out to LED_PAD
			phy_write(phydev, 0x1a, phy_read(phydev, 0x1a) | 0xf00);
			if (ADC3_register) {
				writel(0x3, ADC2_register);
				writel(0x3, ADC3_register);
			}

		} else {
			if (ADC3_register) {
				writel(0x5, ADC2_register);
				writel(0x5, ADC3_register);
			}
			phy_write(phydev, 0x1a, phy_read(phydev, 0x1a) & ~0xf00);
		}
		phy_write(phydev, 0x1f, 0x0);
	}
#endif
	pr_debug("%s, speed=%d, duplex=%d, ", __func__, phydev->speed, phydev->duplex);
	pr_debug("pasue=%d, asym_pause=%d, autoneg=%d ", phydev->pause, phydev->asym_pause, phydev->autoneg);

	return err;
}

#if defined(CONFIG_CVITEK_PHY_UAPS)
/* Ultra Auto Power Saving mode */
static int cv182xa_phy_aps_enable(struct phy_device *phydev)
{
	return 0;
}
#endif

static int cv182xa_phy_config_aneg(struct phy_device *phydev)
{
	int ret;

#if defined(CONFIG_CVITEK_PHY_UAPS)
	cv182xa_phy_aps_enable(phydev); /* if phy not work, disable this function for try */
#endif

	ret = genphy_config_aneg(phydev);

	if (ret < 0)
		return ret;

	return 0;
}

static int cv182xa_phy_config_init(struct phy_device *phydev)
{
	int ret = 0;
	u32 val = 0;
	void __iomem *reg_ephy_top_wrap = NULL;
	void __iomem *reg_ephy_base = NULL;

	reg_ephy_top_wrap = ioremap(REG_EPHY_TOP_WRAP, 0x8);
	if (!reg_ephy_top_wrap) {
		ret = -EBUSY;
		goto err_ephy_mem_1;
	}
	reg_ephy_base = ioremap(REG_EPHY_BASE, 0x80);
	if (!reg_ephy_base) {
		ret = -EBUSY;
		goto err_ephy_mem_2;
	}

	// set rg_ephy_apb_rw_sel 0x0804@[0]=1/APB by using APB interface
	writel(0x0001, reg_ephy_top_wrap + 4);

	writel(0x0, reg_ephy_base + 0x7c);

	/* do this in board.c */
	// Release 0x0800[0]=0/shutdown
	// writel(0x0900, reg_ephy_top_wrap);

	// // Release 0x0800[2]=1/dig_rst_n, Let mii_reg can be accessabile
	// writel(0x0904, reg_ephy_top_wrap);

	// //mdelay(10);

	// // ANA INIT (PD/EN), switch to MII-page5
	// writel(0x0500, reg_ephy_base + 0x7c);
	// // Release ANA_PD p5.0x10@[13:8] = 6'b001100
	// writel(0x0c00, reg_ephy_base + 0x40);
	// // Release ANA_EN p5.0x10@[7:0] = 8'b01111110
	// writel(0x0c7e, reg_ephy_base + 0x40);

	// // Wait PLL_Lock, Lock_Status p5.0x12@[15] = 1
	// //mdelay(1);

	// // Release 0x0800[1] = 1/ana_rst_n
	// writel(0x0906, reg_ephy_top_wrap);

	// // ANA INIT
	// // @Switch to MII-page5
	// writel(0x0500, reg_ephy_base + 0x7c);

// Efuse register
	// Set Double Bias Current
	//Set rg_eth_txitune1  reg_ephy_base + 0x64 [15:8]
	//Set rg_eth_txitune0  reg_ephy_base + 0x64 [7:0]
	if ((cvi_efuse_read_from_shadow(0x20) & EPHY_EFUSE_TXITUNE_FLAG) ==
		EPHY_EFUSE_TXITUNE_FLAG) {
		val = ((cvi_efuse_read_from_shadow(0x24) >> 24) & 0xFF) |
				(((cvi_efuse_read_from_shadow(0x24) >> 16) & 0xFF) << 8);
		writel((readl(reg_ephy_base + 0x64) & ~0xFFFF) | val, reg_ephy_base + 0x64);
	} else
		writel(0x5a5a, reg_ephy_base + 0x64);

	// Set Echo_I
	// Set rg_eth_txechoiadj reg_ephy_base + 0x54  [15:8]
	if ((cvi_efuse_read_from_shadow(0x20) & EPHY_EFUSE_TXECHORC_FLAG) ==
		EPHY_EFUSE_TXECHORC_FLAG) {
		writel((readl(reg_ephy_base + 0x54) & ~0xFF00) |
			   (((cvi_efuse_read_from_shadow(0x24) >> 8) & 0xFF) << 8), reg_ephy_base + 0x54);
	} else
		writel(0x0000, reg_ephy_base + 0x54);

	//Set TX_Rterm & Echo_RC_Delay
	// Set rg_eth_txrterm_p1  reg_ephy_base + 0x58 [11:8]
	// Set rg_eth_txrterm     reg_ephy_base + 0x58  [7:4]
	// Set rg_eth_txechorcadj reg_ephy_base + 0x58  [3:0]
	if ((cvi_efuse_read_from_shadow(0x20) & EPHY_EFUSE_TXRXTERM_FLAG) ==
		EPHY_EFUSE_TXRXTERM_FLAG) {
		val = (((cvi_efuse_read_from_shadow(0x20) >> 28) & 0xF) << 4) |
				(((cvi_efuse_read_from_shadow(0x20) >> 24) & 0xF) << 8);
		writel((readl(reg_ephy_base + 0x58) & ~0xFF0) | val, reg_ephy_base + 0x58);
	} else
		writel(0x0bb0, reg_ephy_base + 0x58);

// ETH_100BaseT
	// Set Rise update
	writel(0x0c10, reg_ephy_base + 0x5c);

	// Set Falling phase
	writel(0x0003, reg_ephy_base + 0x68);

	// Set Double TX Bias Current
	writel(0x0000, reg_ephy_base + 0x54);

	// Switch to MII-page16
	writel(0x1000, reg_ephy_base + 0x7c);

	// Set MLT3 Positive phase code, Set MLT3 +0
	writel(0x1000, reg_ephy_base + 0x68);
	writel(0x3020, reg_ephy_base + 0x6c);
	writel(0x5040, reg_ephy_base + 0x70);
	writel(0x7060, reg_ephy_base + 0x74);

	// Set MLT3 +I
	writel(0x1708, reg_ephy_base + 0x58);
	writel(0x3827, reg_ephy_base + 0x5c);
	writel(0x5748, reg_ephy_base + 0x60);
	writel(0x7867, reg_ephy_base + 0x64);

	// Switch to MII-page17
	writel(0x1100, reg_ephy_base + 0x7c);

	// Set MLT3 Negative phase code, Set MLT3 -0
	writel(0x9080, reg_ephy_base + 0x40);
	writel(0xb0a0, reg_ephy_base + 0x44);
	writel(0xd0c0, reg_ephy_base + 0x48);
	writel(0xf0e0, reg_ephy_base + 0x4c);

	// Set MLT3 -I
	writel(0x9788, reg_ephy_base + 0x50);
	writel(0xb8a7, reg_ephy_base + 0x54);
	writel(0xd7c8, reg_ephy_base + 0x58);
	writel(0xf8e7, reg_ephy_base + 0x5c);

	// @Switch to MII-page5
	writel(0x0500, reg_ephy_base + 0x7c);

	// En TX_Rterm
	writel((0x0001 | readl(reg_ephy_base + 0x40)), reg_ephy_base + 0x40);
	//Change rx vcm
	writel((0x820 |readl(reg_ephy_base + 0x4c)), reg_ephy_base + 0x4c);
//	Link Pulse
	// Switch to MII-page10
	writel(0x0a00, reg_ephy_base + 0x7c);
#if 1
	// Set Link Pulse
	writel(0x3e00, reg_ephy_base + 0x40);
	writel(0x7864, reg_ephy_base + 0x44);
	writel(0x6470, reg_ephy_base + 0x48);
	writel(0x5f62, reg_ephy_base + 0x4c);
	writel(0x5a5a, reg_ephy_base + 0x50);
	writel(0x5458, reg_ephy_base + 0x54);
	writel(0xb23a, reg_ephy_base + 0x58);
	writel(0x94a0, reg_ephy_base + 0x5c);
	writel(0x9092, reg_ephy_base + 0x60);
	writel(0x8a8e, reg_ephy_base + 0x64);
	writel(0x8688, reg_ephy_base + 0x68);
	writel(0x8484, reg_ephy_base + 0x6c);
	writel(0x0082, reg_ephy_base + 0x70);
#else 
	// from sean
	// Fix err: the status is still linkup when removed the network cable.
	writel(0x2000, reg_ephy_base + 0x40);
	writel(0x3832, reg_ephy_base + 0x44);
	writel(0x3132, reg_ephy_base + 0x48);
	writel(0x2d2f, reg_ephy_base + 0x4c);
	writel(0x2c2d, reg_ephy_base + 0x50);
	writel(0x1b2b, reg_ephy_base + 0x54);
	writel(0x94a0, reg_ephy_base + 0x58);
	writel(0x8990, reg_ephy_base + 0x5c);
	writel(0x8788, reg_ephy_base + 0x60);
	writel(0x8485, reg_ephy_base + 0x64);
	writel(0x8283, reg_ephy_base + 0x68);
	writel(0x8182, reg_ephy_base + 0x6c);
	writel(0x0081, reg_ephy_base + 0x70);
#endif
// TP_IDLE
	// Switch to MII-page11
	writel(0x0b00, reg_ephy_base + 0x7c);

// Set TP_IDLE
	writel(0x5252, reg_ephy_base + 0x40);
	writel(0x5252, reg_ephy_base + 0x44);
	writel(0x4B52, reg_ephy_base + 0x48);
	writel(0x3D47, reg_ephy_base + 0x4c);
	writel(0xAA99, reg_ephy_base + 0x50);
	writel(0x989E, reg_ephy_base + 0x54);
	writel(0x9395, reg_ephy_base + 0x58);
	writel(0x9091, reg_ephy_base + 0x5c);
	writel(0x8E8F, reg_ephy_base + 0x60);
	writel(0x8D8E, reg_ephy_base + 0x64);
	writel(0x8C8C, reg_ephy_base + 0x68);
	writel(0x8B8B, reg_ephy_base + 0x6c);
	writel(0x008A, reg_ephy_base + 0x70);

// ETH 10BaseT Data
	// Switch to MII-page13
	writel(0x0d00, reg_ephy_base + 0x7c);

	writel(0x1E0A, reg_ephy_base + 0x40);
	writel(0x3862, reg_ephy_base + 0x44);
	writel(0x1E62, reg_ephy_base + 0x48);
	writel(0x2A08, reg_ephy_base + 0x4c);
	writel(0x244C, reg_ephy_base + 0x50);
	writel(0x1A44, reg_ephy_base + 0x54);
	writel(0x061C, reg_ephy_base + 0x58);

	// Switch to MII-page14
	writel(0x0e00, reg_ephy_base + 0x7c);

	writel(0x2D30, reg_ephy_base + 0x40);
	writel(0x3470, reg_ephy_base + 0x44);
	writel(0x0648, reg_ephy_base + 0x48);
	writel(0x261C, reg_ephy_base + 0x4c);
	writel(0x3160, reg_ephy_base + 0x50);
	writel(0x2D5E, reg_ephy_base + 0x54);

	// Switch to MII-page15
	writel(0x0f00, reg_ephy_base + 0x7c);

	writel(0x2922, reg_ephy_base + 0x40);
	writel(0x366E, reg_ephy_base + 0x44);
	writel(0x0752, reg_ephy_base + 0x48);
	writel(0x2556, reg_ephy_base + 0x4c);
	writel(0x2348, reg_ephy_base + 0x50);
	writel(0x0C30, reg_ephy_base + 0x54);

	// Switch to MII-page16
	writel(0x1000, reg_ephy_base + 0x7c);

	writel(0x1E08, reg_ephy_base + 0x40);
	writel(0x3868, reg_ephy_base + 0x44);
	writel(0x1462, reg_ephy_base + 0x48);
	writel(0x1A0E, reg_ephy_base + 0x4c);
	writel(0x305E, reg_ephy_base + 0x50);
	writel(0x2F62, reg_ephy_base + 0x54);

// LED
	// Switch to MII-page1
	writel(0x0100, reg_ephy_base + 0x7c);

	// select LED_LNK/SPD/DPX out to LED_PAD
	writel((readl(reg_ephy_base + 0x68) & ~0x0f00), reg_ephy_base + 0x68);

#ifdef NEW_ETH_DETECT
// led pol
	// Switch to MII-page0
	writel(0x0, reg_ephy_base + 0x7c);
	// printk(KERN_EMERG "ethernet: reg_ephy_base + 0x4c   %lx\n",readl(reg_ephy_base + 0x4c));
	// printk(KERN_EMERG "---------------\n");

	// h13 10~8 LED polarity invert, 0(high-active),1(low-active)
	writel((readl(reg_ephy_base + 0x4c) | 0x0700), reg_ephy_base + 0x4c);
	//printk("ethernet: reg_ephy_base + 0x4c %lx\n\n\n",readl(reg_ephy_base + 0x4c));
	// printk(KERN_EMERG "---------------\n");
#endif
	// Switch to MII-page19
	writel(0x1300, reg_ephy_base + 0x7c);
	writel(0x0012, reg_ephy_base + 0x58);
	// set agc max/min swing
	writel(0x6848, reg_ephy_base + 0x5c);

	// Switch to MII-page18
	writel(0x1200, reg_ephy_base + 0x7c);
#if IS_ENABLED(CONFIG_ARCH_CV181X)
	/* mars LPF(8, 8, 8, 8) HPF(-8, 50(+32), -36, -8) */
	// lpf
	writel(0x0808, reg_ephy_base + 0x48);
	writel(0x0808, reg_ephy_base + 0x4c);
	// hpf
	writel(0x32f8, reg_ephy_base + 0x50);
	writel(0xf8dc, reg_ephy_base + 0x54);
#elif IS_ENABLED(CONFIG_ARCH_CV180X)
	/* phobos LPF:(1 8 23 23 8 1) HPF:(-4,58,-45,8,-5, 0) from sean PPT */
	// lpf
	writel(0x0801, reg_ephy_base + 0x48);
	writel(0x1717, reg_ephy_base + 0x4C);
	writel(0x0108, reg_ephy_base + 0x5C);
	// hpf
	writel(0x3afc, reg_ephy_base + 0x50);
	writel(0x08d3, reg_ephy_base + 0x54);
	writel(0x00fb, reg_ephy_base + 0x60);
#endif

	// Switch to MII-page0
	writel(0x0000, reg_ephy_base + 0x7c);
	// EPHY start auto-neg procedure
	writel(0x090e, reg_ephy_top_wrap);

	// from jinyu.zhao
	/* EPHY is configured as half-duplex after reset, but we need force full-duplex */
	writel((readl(reg_ephy_base) | 0x100), reg_ephy_base);

	// switch to MDIO control by ETH_MAC
	writel(0x0000, reg_ephy_top_wrap + 4);

#ifdef NEW_ETH_DETECT
	link_status = 0;
	retry_time = 0;
#endif
	iounmap(reg_ephy_base);
err_ephy_mem_2:
	iounmap(reg_ephy_top_wrap);
err_ephy_mem_1:
	return ret;
}

static struct phy_driver cv182xa_phy_driver[] = {
{
	.phy_id		= 0x00435649,
	.phy_id_mask	= 0xffffffff,
	.name		= "CVITEK CV182XA",
	.config_init	= cv182xa_phy_config_init,
	.config_aneg	= cv182xa_phy_config_aneg,
	.read_status	= cv182xa_read_status,
	/* IRQ related */
	.ack_interrupt	= cv182xa_phy_ack_interrupt,
	.config_intr	= cv182xa_phy_config_intr,
	.aneg_done	= genphy_aneg_done,
	.suspend	= genphy_suspend,
	.resume		= genphy_resume,
	.set_loopback   = genphy_loopback,
} };

module_phy_driver(cv182xa_phy_driver);

MODULE_DESCRIPTION("CV182XA EPHY driver");
MODULE_AUTHOR("Ethan Chen");
MODULE_LICENSE("GPL");

static struct mdio_device_id __maybe_unused cv182xa_tbl[] = {
	{ 0x00435649, 0xffffffff },
	{ }
};

MODULE_DEVICE_TABLE(mdio, cv182xa_tbl);
