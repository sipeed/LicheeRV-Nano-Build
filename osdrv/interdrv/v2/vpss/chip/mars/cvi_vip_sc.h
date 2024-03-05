#ifndef _CVI_VIP_SC_H_
#define _CVI_VIP_SC_H_

#include <linux/cvi_vip_sc.h>
#include <base_ctx.h>

int sc_create_instance(struct platform_device *pdev);
int sc_destroy_instance(struct platform_device *pdev);
int cvi_sc_streamon(struct cvi_sc_vdev *sdev);
int cvi_sc_streamoff(struct cvi_sc_vdev *sdev, bool isForce);
void sc_irq_handler(union sclr_intr intr_status, struct cvi_vip_dev *bdev);
void cvi_sc_device_run(void *priv, bool is_tile, bool is_work_on_r_tile, u8 grp_id);
void cvi_sc_update(struct cvi_sc_vdev *sdev, const struct cvi_vpss_chn_cfg *chn_cfg);

int sc_set_src_to_imgv(struct cvi_sc_vdev *sdev, u8 enable);
int sc_set_vpss_chn_cfg(struct cvi_sc_vdev *sdev, struct cvi_vpss_chn_cfg *cfg);
CVI_S32 vpss_sc_qbuf(struct cvi_vip_dev *dev, struct cvi_buffer *buf, MMF_CHN_S chn);
CVI_VOID vpss_sc_sb_qbuf(struct cvi_vip_dev *dev, struct cvi_buffer *buf, MMF_CHN_S chn);
CVI_VOID vpss_sc_set_vc_sbm(struct cvi_vip_dev *dev, MMF_CHN_S chn, bool sb_vc_ready);
void cvi_sc_buf_remove_all(struct cvi_sc_vdev *vdev, const u8 grp_id);
CVI_VOID sc_set_vpss_chn_bind_fb(struct cvi_sc_vdev *sdev, bool bind_fb);

#endif
