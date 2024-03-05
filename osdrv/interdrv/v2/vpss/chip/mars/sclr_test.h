#ifndef _CVI_SCL_TEST_H_
#define _CVI_SCL_TEST_H_

extern uint8_t sclr_test_enabled;

int32_t sclr_test_proc_init(struct cvi_vip_dev *dev);
int32_t sclr_test_proc_deinit(void);
void sclr_test_irq_handler(uint32_t intr_raw_status);

int sclr_to_vc_sb_ktest(void *data);
int sclr_force_img_in_trigger(void);

#endif /* _CVI_SCL_TEST_H_ */
