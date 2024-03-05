/*******************************************************************************
 *	Perf chk interfaces
 ******************************************************************************/
void vi_record_sof_perf(struct cvi_vi_dev *vdev, u8 raw_num, u8 chn_num);
void vi_record_fe_perf(struct cvi_vi_dev *vdev, u8 raw_num, u8 chn_num);
void vi_record_be_perf(struct cvi_vi_dev *vdev, u8 raw_num, u8 chn_num);
void vi_record_post_end(struct cvi_vi_dev *vdev, u8 raw_num);
void vi_record_post_trigger(struct cvi_vi_dev *vdev, u8 raw_num);
void vi_perf_record_dump(void);

