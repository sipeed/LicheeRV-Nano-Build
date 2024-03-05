#ifndef __VI_SDK_LAYER_H__
#define __VI_SDK_LAYER_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <vi_defines.h>
#include <linux/vi_uapi.h>
#include <linux/types.h>
#include <vb.h>

/*****************************************************************************
 *  vi structure and enum for vi sdk layer
 ****************************************************************************/
struct cvi_isp_buf {
	struct vi_buffer buf;
	struct list_head list;
};

/*****************************************************************************
 *  vi function prototype for vi sdk layer
 ****************************************************************************/
int vi_create_thread(struct cvi_vi_dev *vdev, enum E_VI_TH th_id);
void vi_destory_thread(struct cvi_vi_dev *vdev, enum E_VI_TH th_id);
int vi_start_streaming(struct cvi_vi_dev *vdev);
int vi_stop_streaming(struct cvi_vi_dev *vdev);
void cvi_isp_buf_queue_wrap(struct cvi_vi_dev *vdev, struct cvi_isp_buf *b);
int vi_mac_clk_ctrl(struct cvi_vi_dev *vdev, u8 mac_num, u8 enable);
int usr_pic_timer_init(struct cvi_vi_dev *vdev);
void usr_pic_time_remove(void);
void vi_destory_dbg_thread(struct cvi_vi_dev *vdev);

/*****************************************************************************
 *  vi sdk ioctl function prototype for vi layer
 ****************************************************************************/
CVI_S32 vi_disable_chn(VI_CHN ViChn);
long vi_sdk_ctrl(struct cvi_vi_dev *vdev, struct vi_ext_control *p);
int vi_sdk_qbuf(MMF_CHN_S chn);
void vi_fill_mlv_info(struct vb_s *blk, u8 dev, struct mlv_i_s *m_lv_i, u8 is_vpss_offline);
void vi_fill_dis_info(struct vb_s *blk);

#ifdef __cplusplus
}
#endif

#endif //__VI_SDK_LAYER_H__
