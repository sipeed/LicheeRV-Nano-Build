#ifndef _CVI_VIP_IMG_H_
#define _CVI_VIP_IMG_H_

int img_create_instance(struct platform_device *pdev);
void img_irq_handler(union sclr_intr intr_status, u8 cmdq_intr_status, struct cvi_vip_dev *bdev);
int img_destroy_instance(struct platform_device *pdev);
void cvi_img_stop_streaming(struct cvi_img_vdev *idev);
int cvi_img_get_input(enum sclr_img_in img_type,
	enum cvi_input_type input_type, enum sclr_input *input);
void cvi_img_update(struct cvi_img_vdev *idev, const struct cvi_vpss_grp_cfg *grp_cfg);
void cvi_img_get_sc_bound(struct cvi_img_vdev *idev, bool sc_bound[]);

int img_s_input(struct cvi_img_vdev *idev, CVI_U32 input_type);
int img_g_input(struct cvi_img_vdev *idev, CVI_U32 *i);
int img_set_vpss_grp_cfg(struct cvi_img_vdev *idev, const struct cvi_vpss_grp_cfg *grp_cfg);
int cvi_img_streamon(struct cvi_img_vdev *idev);
int cvi_img_streamoff(struct cvi_img_vdev *idev, bool isForce);

#endif
