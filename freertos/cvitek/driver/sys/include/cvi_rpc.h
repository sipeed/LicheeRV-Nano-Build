#ifndef __CVI_RPC_H__
#define __CVI_RPC_H__

#include <stdbool.h>
#include "cvi_type.h"
#include "cvi_common.h"
#include "cvi_comm_video.h"
#include "cvi_errno.h"
#include "cvi_debug.h"
#include "cvi_comm_vpss.h"
#include "cvi_comm_venc.h"
#include "cvi_comm_vdec.h"
#include "cvi_comm_vb.h"

#define RPC_URL "ipc:///tmp/cvitek.ipc"
#define RPC_MAGIC 0x11223344

#define SET_MSG(mode, dev, chn, cmd)                                           \
	((mode << 24) | (dev << 16) | (chn << 8) | cmd)
#define GET_MODE(msg) (msg >> 24)
#define GET_DEV(msg) ((msg >> 16) & 0xff)
#define GET_CHN(msg) ((msg >> 8) & 0xff)
#define GET_CMD(msg) (msg & 0xff)

#define x_info(format, args...)                                                \
	printf("\033[1m\033[32m[INF|%s|%d] " format "\033[0m\n", __func__,     \
	       __LINE__, ##args)

enum RPC_CMD {
	RPC_CMD_GET_VPSS_CHN_FRAME = 0,
	RPC_CMD_RELEASE_VPSS_CHN_FRAME = 1,
	RPC_CMD_VO_SEND_FRAME = 2,
	RPC_CMD_VO_SHOW_CHN = 3,
	RPC_CMD_VO_HIDE_CHN = 4,
	RPC_CMD_SYS_BIND = 5,
	RPC_CMD_SYS_UNBIND = 6,
	RPC_CMD_SYS_MUNMAP = 7,
	RPC_CMD_SYS_MMAP_CACHE = 8,
	RPC_CMD_SYS_ION_ALLOC = 9,
	RPC_CMD_SYS_ION_FREE = 10,
	RPC_CMD_SYS_GET_CUR_PTS = 11,
	RPC_CMD_VPSS_GET_CHN_ATTR = 12,
	RPC_CMD_VPSS_SET_CHN_ATTR = 13,
	RPC_CMD_VPSS_ENABLE_CHN = 14,
	RPC_CMD_VPSS_DISABLE_CHN = 15,
	RPC_CMD_VPSS_SET_CHN_CROP = 16,
	RPC_CMD_VENC_CREATE_CHN = 17,
	RPC_CMD_VENC_DESTROY_CHN = 18,
	RPC_CMD_VENC_GET_CHN_ATTR = 19,
	RPC_CMD_VENC_SET_CHN_ATTR = 20,
	RPC_CMD_VENC_GET_CHN_PARAM = 21,
	RPC_CMD_VENC_SET_CHN_PARAM = 22,
	RPC_CMD_VENC_QUERY_STATUS = 23,
	RPC_CMD_VENC_GET_STREAM = 24,
	RPC_CMD_VENC_SEND_FRAME = 25,
	RPC_CMD_VENC_RELEASE_STREAM = 26,
	RPC_CMD_VENC_START_RECV_FRAME = 27,
	RPC_CMD_VENC_STOP_RECV_FRAME = 28,
	RPC_CMD_VENC_GET_JPEG_PARAM = 29,
	RPC_CMD_VENC_SET_JPEG_PARAM = 30,
	RPC_CMD_VENC_GET_FD = 31,
	RPC_CMD_VB_CREATE_POOL = 32,
	RPC_CMD_VB_DESTROY_POOL = 33,
	RPC_CMD_VB_GET_BLOCK_WITH_ID = 34,
	RPC_CMD_VB_RELEASE_BLOCK = 35,
	RPC_CMD_MAX
};

struct rpc_msg {
	unsigned int msg;
	unsigned int magic;
	int result;
	char body[0];
};

CVI_S32 rpc_server_init(void);
CVI_S32 rpc_server_deinit(void);
CVI_BOOL isMaster(void);
CVI_S32 rpc_client_vpss_getchnframe(VPSS_GRP VpssGrp, VPSS_CHN VpssChn,
				    VIDEO_FRAME_INFO_S *pstFrameInfo,
				    CVI_S32 s32MilliSec);
CVI_S32 rpc_client_vpss_releasechnframe(VPSS_GRP VpssGrp, VPSS_CHN VpssChn,
					const VIDEO_FRAME_INFO_S *pstFrameInfo);
CVI_S32 rpc_client_vo_sendframe(VO_LAYER VoLayer, VO_CHN VoChn,
				VIDEO_FRAME_INFO_S *pstVideoFrame,
				CVI_S32 s32MilliSec);
CVI_S32 rpc_client_vo_showchn(VO_LAYER VoLayer, VO_CHN VoChn);
CVI_S32 rpc_client_vo_hidechn(VO_LAYER VoLayer, VO_CHN VoChn);
CVI_S32 rpc_client_sys_bind(const MMF_CHN_S *pstSrcChn,
			    const MMF_CHN_S *pstDestChn);
CVI_S32 rpc_client_sys_unbind(const MMF_CHN_S *pstSrcChn,
			      const MMF_CHN_S *pstDestChn);

CVI_S32 rpc_client_sys_munmap(void *pVirAddr, CVI_U32 u32Size);
void *rpc_client_sys_mmapcache(CVI_U64 u64PhyAddr, CVI_U32 u32Size);
CVI_S32 rpc_client_sys_ionalloc(CVI_U64 *pu64PhyAddr, CVI_VOID **ppVirAddr,
				CVI_U32 u32Len);
CVI_S32 rpc_client_sys_ionfree(CVI_U64 u64PhyAddr, CVI_VOID *pVirAddr);
CVI_S32 rpc_client_sys_getcurpts(CVI_U64 *pu64CurPTS);
CVI_S32 rpc_client_vpss_getchnattr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn,
				   VPSS_CHN_ATTR_S *pstChnAttr);
CVI_S32 rpc_client_vpss_setchnattr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn,
				   const VPSS_CHN_ATTR_S *pstChnAttr);
CVI_S32 rpc_client_vpss_enablechn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn);
CVI_S32 rpc_client_vpss_disablechn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn);
CVI_S32 rpc_client_vpss_setchncrop(VPSS_GRP VpssGrp, VPSS_CHN VpssChn,
				   const VPSS_CROP_INFO_S *pstCropInfo);
CVI_S32 rpc_client_venc_createchn(VENC_CHN VeChn,
				  const VENC_CHN_ATTR_S *pstAttr);
CVI_S32 rpc_client_venc_destroychn(VENC_CHN VeChn);
CVI_S32 rpc_client_venc_getchnattr(VENC_CHN VeChn, VENC_CHN_ATTR_S *pstChnAttr);
CVI_S32 rpc_client_venc_setchnattr(VENC_CHN VeChn,
				   const VENC_CHN_ATTR_S *pstChnAttr);
CVI_S32 rpc_client_venc_getchnparam(VENC_CHN VeChn,
				    VENC_CHN_PARAM_S *pstChnParam);
CVI_S32 rpc_client_venc_setchnparam(VENC_CHN VeChn,
				    const VENC_CHN_PARAM_S *pstChnParam);
CVI_S32 rpc_client_venc_querystatus(VENC_CHN VeChn,
				    VENC_CHN_STATUS_S *pstStatus);
CVI_S32 rpc_client_venc_getstream(VENC_CHN VeChn, VENC_STREAM_S *pstStream,
				  CVI_S32 S32MilliSec);
CVI_S32 rpc_client_venc_sendframe(VENC_CHN VeChn,
				  const VIDEO_FRAME_INFO_S *pstFrame,
				  CVI_S32 S32MilliSec);
CVI_S32 rpc_client_venc_releasestream(VENC_CHN VeChn, VENC_STREAM_S *pstStream);
CVI_S32
rpc_client_venc_startrecvframe(VENC_CHN VeChn,
			       const VENC_RECV_PIC_PARAM_S *pstRecvParam);
CVI_S32 rpc_client_venc_stoprecvframe(VENC_CHN VeChn);
CVI_S32 rpc_client_venc_getjpegparam(VENC_CHN VeChn,
				     VENC_JPEG_PARAM_S *pstJpegParam);
CVI_S32 rpc_client_venc_setjpegparam(VENC_CHN VeChn,
				     const VENC_JPEG_PARAM_S *pstJpegParam);
CVI_S32 rpc_client_venc_getfd(VENC_CHN VeChn);
VB_POOL rpc_client_vb_createpool(VB_POOL_CONFIG_S *pstVbPoolCfg);
CVI_S32 rpc_client_vb_destroypool(VB_POOL Pool);
VB_BLK rpc_client_vb_getblockwithid(VB_POOL Pool, CVI_U32 u32BlkSize,
				    MOD_ID_E modId);
CVI_S32 rpc_client_vb_releaseblock(VB_BLK Block);

#endif
