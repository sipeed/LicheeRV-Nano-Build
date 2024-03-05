#ifndef _CIF_H_
#define _CIF_H_

#include <linux/miscdevice.h>
#include "drv/cif_drv.h"
#include "linux/cif_uapi.h"

#define CIF_MAX_CSI_NUM		2

struct cvi_csi_status {
	unsigned int			errcnt_ecc;
	unsigned int			errcnt_crc;
	unsigned int			errcnt_hdr;
	unsigned int			errcnt_wc;
	unsigned int			fifo_full;
};

struct cvi_lvds_status {
	unsigned int			fifo_full;
};

struct cvi_link {
	struct cif_ctx			cif_ctx;
	int				irq_num;
	struct reset_control		*phy_reset;
	struct reset_control		*phy_apb_reset;
	unsigned int			is_on;
	struct cif_param		param;
	struct combo_dev_attr_s		attr;
	enum clk_edge_e			clk_edge;
	enum output_msb_e		msb;
	unsigned int			crop_top;
	unsigned int			distance_fp;
	int				snsr_rst_pin;
	enum of_gpio_flags		snsr_rst_pol;
	union {
		struct cvi_csi_status	sts_csi;
		struct cvi_lvds_status	sts_lvds;
	};
	struct device			*dev;
	enum rx_mac_clk_e		mac_clk;
	enum ttl_bt_fmt_out		bt_fmt_out;
};

struct cvi_cam_clk {
	int				is_on;
	struct clk			*clk_o;
};

struct cvi_cif_dev {
	struct miscdevice	miscdev;
	spinlock_t		lock;
	struct mutex		mutex;
	struct cvi_link		link[MAX_LINK_NUM];
	struct cvi_cam_clk	clk_cam0;
	struct cvi_cam_clk	clk_cam1;
	struct cvi_cam_clk	vip_sys2;
	struct cvi_cam_clk	clk_mipimpll; /* mipipll */
	struct cvi_cam_clk	clk_disppll; /* disppll */
	struct cvi_cam_clk	clk_fpll; /* fpll */
	unsigned int		max_mac_clk;
	void			*pad_ctrl;
};

#endif
