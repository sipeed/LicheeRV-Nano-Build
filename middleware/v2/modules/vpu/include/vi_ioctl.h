#ifndef MODULES_VPU_INCLUDE_VI_IOCTL_H_
#define MODULES_VPU_INCLUDE_VI_IOCTL_H_

#include <linux/vi_isp.h>
#include <linux/vi_tun_cfg.h>
#include <linux/vi_uapi.h>
#include <linux/cvi_comm_vi.h>

int vi_enable_usr_pic(int fd, bool enable);
int vi_set_usr_pic(int fd, struct cvi_isp_usr_pic_cfg *cfg);
int vi_put_usr_pic(int fd, CVI_U64 phyAddr);
int vi_set_usr_pic_timing(int fd, CVI_U32 fps);
int vi_set_be_online(int fd, CVI_BOOL online);
int vi_set_online(int fd, CVI_BOOL online);
int vi_set_hdr(int fd, CVI_BOOL is_hdr_on);
int vi_set_3dnr(int fd, CVI_BOOL is_3dnr_on);
int vi_get_pipe_dump(int fd, struct cvi_vip_isp_raw_blk *memAddr);
int vi_put_pipe_dump(int fd, CVI_U32 dev_num);
int vi_set_yuv_bypass_path(int fd, struct cvi_vip_isp_yuv_param *param);
int vi_set_compress_mode(int fd, CVI_BOOL is_compress_on);
int vi_set_lvds_flow(int fd, CVI_BOOL is_lvds_flow_on);
int vi_get_ip_dump_list(int fd, struct ip_info *ip_info_list);
int vi_set_trig_preraw(int fd, CVI_U32 dev_num);
int vi_set_online2sc(int fd, struct cvi_isp_sc_online *online);
int vi_get_tun_addr(int fd, struct isp_tuning_cfg *tun_buf_info);
int vi_set_clk(int fd, CVI_BOOL clk_on);
int vi_get_dma_size(int fd, CVI_U32 *size);
int vi_set_dma_buf_info(int fd, struct cvi_vi_dma_buf_info *param);
int vi_set_enq_buf(int fd, struct vi_buffer *buf);
int vi_set_start_streaming(int fd);
int vi_set_stop_streaming(int fd);
int vi_get_rgbmap_le_buf(int fd, struct cvi_vip_memblock *buf);
int vi_get_rgbmap_se_buf(int fd, struct cvi_vip_memblock *buf);
#ifdef ARCH_CV182X
int vi_set_rgbir(int fd, CVI_BOOL is_rgbir);
#endif

int vi_sdk_set_dev_attr(int fd, int dev, VI_DEV_ATTR_S *pstDevAttr);
int vi_sdk_get_dev_attr(int fd, int dev, VI_DEV_ATTR_S *pstDevAttr);
int vi_sdk_enable_dev(int fd, int dev);
int vi_sdk_create_pipe(int fd, int pipe, VI_PIPE_ATTR_S *pstPipeAttr);
int vi_sdk_start_pipe(int fd, int pipe);
int vi_sdk_set_chn_attr(int fd, int pipe, int chn, VI_CHN_ATTR_S *pstChnAttr);
int vi_sdk_get_chn_attr(int fd, int pipe, int chn, VI_CHN_ATTR_S *pstChnAttr);
int vi_sdk_set_pipe_attr(int fd, int pipe, VI_PIPE_ATTR_S *pstPipeAttr);
int vi_sdk_get_pipe_attr(int fd, int pipe, VI_PIPE_ATTR_S *pstPipeAttr);
int vi_sdk_get_pipe_dump_attr(int fd, int pipe, VI_DUMP_ATTR_S *pstDumpAttr);
int vi_sdk_set_pipe_dump_attr(int fd, int pipe, VI_DUMP_ATTR_S *pstDumpAttr);
int vi_sdk_enable_chn(int fd, int pipe, int chn);
int vi_sdk_disable_chn(int fd, int pipe, int chn);
int vi_sdk_set_motion_lv(int fd, struct mlv_info_s *mlv_i);
int vi_sdk_enable_dis(int fd, int pipe);
int vi_sdk_disable_dis(int fd, int pipe);
int vi_sdk_set_dis_info(int fd, struct dis_info_s *pdis_i);
int vi_sdk_set_pipe_frm_src(int fd, int pipe, VI_PIPE_FRAME_SOURCE_E *source);
int vi_sdk_send_pipe_raw(int fd, int pipe, VIDEO_FRAME_INFO_S *sVideoFrm);
int vi_sdk_set_dev_timing_attr(int fd, int dev, VI_DEV_TIMING_ATTR_S *pstDevTimingAttr);
int vi_sdk_get_chn_frame(int fd, int pipe, int chn, VIDEO_FRAME_INFO_S *pstFrameInfo, CVI_S32 s32MilliSec);
int vi_sdk_release_chn_frame(int fd, int pipe, int chn, VIDEO_FRAME_INFO_S *pstFrameInfo);
int vi_sdk_set_chn_crop(int fd, int pipe, int chn, VI_CROP_INFO_S *pstCropInfo);
int vi_sdk_get_chn_crop(int fd, int pipe, int chn, VI_CROP_INFO_S *pstCropInfo);
int vi_sdk_get_pipe_frame(int fd, int pipe, VIDEO_FRAME_INFO_S *pstFrameInfo, CVI_S32 s32MilliSec);
int vi_sdk_release_pipe_frame(int fd, int pipe, VIDEO_FRAME_INFO_S *pstFrameInfo);
int vi_sdk_start_smooth_rawdump(int fd, int pipe, struct cvi_vip_isp_smooth_raw_param *smooth_raw_param);
int vi_sdk_stop_smooth_rawdump(int fd, int pipe, struct cvi_vip_isp_smooth_raw_param *smooth_raw_param);
int vi_sdk_get_smooth_rawdump(int fd, int pipe, VIDEO_FRAME_INFO_S *pstFrameInfo, CVI_S32 s32MilliSec);
int vi_sdk_put_smooth_rawdump(int fd, int pipe, VIDEO_FRAME_INFO_S *pstFrameInfo);
int vi_sdk_set_chn_rotation(int fd, const struct vi_chn_rot_cfg *cfg);
int vi_sdk_set_chn_ldc(int fd, const struct vi_chn_ldc_cfg *cfg);
int vi_sdk_attach_vbpool(int fd, const struct vi_vb_pool_cfg *cfg);
int vi_sdk_detach_vbpool(int fd, const struct vi_vb_pool_cfg *cfg);
#endif // MODULES_VPU_INCLUDE_VI_IOCTL_H_
