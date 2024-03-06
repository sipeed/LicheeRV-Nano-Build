#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include <fcntl.h>		/* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "mipi_tx.h"

static void _cal_htt_extra(struct combo_dev_cfg_s *dev_cfg)
{
	unsigned char lane_num = 0, bits = 0;
	unsigned short htt_old, htt_new, htt_new_extra;
	unsigned short vtt;
	float fps;
	float bit_rate_MHz;
	float clk_hs_MHz;
	float clk_hs_ns;
	float line_rate_KHz, line_time_us;
	float over_head;
	float t_period_max, t_period_real;
	struct sync_info_s *sync_info = &dev_cfg->sync_info;

	for (int i = 0; i < LANE_MAX_NUM; i++) {
		if ((dev_cfg->lane_id[i] < 0) || (dev_cfg->lane_id[i] >= MIPI_TX_LANE_MAX))
			continue;
		if (dev_cfg->lane_id[i] != MIPI_TX_LANE_CLK)
			++lane_num;
	}

	switch (dev_cfg->output_format) {
	case OUT_FORMAT_RGB_16_BIT:
		bits = 16;
		break;

	case OUT_FORMAT_RGB_18_BIT:
		bits = 18;
		break;

	case OUT_FORMAT_RGB_24_BIT:
		bits = 24;
		break;

	case OUT_FORMAT_RGB_30_BIT:
		bits = 30;
		break;

	default:
		break;
	}

	htt_old = sync_info->vid_hsa_pixels + sync_info->vid_hbp_pixels
			+ sync_info->vid_hline_pixels + sync_info->vid_hfp_pixels;
	vtt = sync_info->vid_vsa_lines + sync_info->vid_vbp_lines
			+ sync_info->vid_active_lines + sync_info->vid_vfp_lines;
	fps = dev_cfg->pixel_clk * 1000.0 / (htt_old * vtt);
	bit_rate_MHz = dev_cfg->pixel_clk / 1000.0 * bits / lane_num;
	clk_hs_MHz = bit_rate_MHz / 2;
	clk_hs_ns = 1000 / clk_hs_MHz;
	line_rate_KHz = vtt * fps / 1000;
	line_time_us = 1000 / line_rate_KHz;
	over_head = (3 * 50 * 2 * 3) + clk_hs_ns * 360;
	t_period_max = line_time_us * 1000 - over_head;
	t_period_real = clk_hs_ns * sync_info->vid_hline_pixels * bits / 4 / 2;
	htt_new = (unsigned short)(htt_old * t_period_real / t_period_max);
	if (htt_new > htt_old) {
		if (htt_new & 0x0003)
			htt_new += (4 - (htt_new & 0x0003));
		htt_new_extra = htt_new - htt_old;
		sync_info->vid_hfp_pixels += htt_new_extra;
		dev_cfg->pixel_clk = htt_new * vtt * fps / 1000;
	}
}

int mipi_tx_cfg(int fd, struct combo_dev_cfg_s *dev_cfg)
{
	struct combo_dev_cfg_s dev_cfg_t = *dev_cfg;

	_cal_htt_extra(&dev_cfg_t);
	if (-1 == ioctl(fd, CVI_VIP_MIPI_TX_SET_DEV_CFG, &dev_cfg_t)) {
		perror("CVI_VIP_MIPI_TX_SET_DEV_CFG");
		return -1;
	}
	return 0;
}

int mipi_tx_send_cmd(int fd, struct cmd_info_s *cmd_info)
{
	if (cmd_info->cmd_size == 0)
		return -1;

	if (-1 == ioctl(fd, CVI_VIP_MIPI_TX_SET_CMD, cmd_info)) {
		perror("CVI_VIP_MIPI_TX_SET_CMD");
		return -1;
	}
	return 0;
}


int mipi_tx_recv_cmd(int fd, struct get_cmd_info_s *cmd_info)
{
	if (cmd_info->get_data_size == 0)
		return -1;

	if (-1 == ioctl(fd, CVI_VIP_MIPI_TX_GET_CMD, cmd_info)) {
		perror("CVI_VIP_MIPI_TX_GET_CMD");
		return -1;
	}
	return 0;
}

int mipi_tx_enable(int fd)
{
	if (-1 == ioctl(fd, CVI_VIP_MIPI_TX_ENABLE, NULL)) {
		perror("CVI_VIP_MIPI_TX_ENABLE");
		return -1;
	}
	return 0;
}

int mipi_tx_disable(int fd)
{
	if (-1 == ioctl(fd, CVI_VIP_MIPI_TX_DISABLE, NULL)) {
		perror("CVI_VIP_MIPI_TX_DISABLE");
		return -1;
	}
	return 0;
}

int mipi_tx_set_hs_settle(int fd, const struct hs_settle_s *hs_cfg)
{
	if (-1 == ioctl(fd, CVI_VIP_MIPI_TX_SET_HS_SETTLE, hs_cfg)) {
		perror("CVI_VIP_MIPI_TX_SET_HS_SETTLE");
		return -1;
	}
	return 0;
}

int mipi_tx_get_hs_settle(int fd, struct hs_settle_s *hs_cfg)
{
	if (-1 == ioctl(fd, CVI_VIP_MIPI_TX_GET_HS_SETTLE, hs_cfg)) {
		perror("CVI_VIP_MIPI_TX_GET_HS_SETTLE");
		return -1;
	}
	return 0;
}

