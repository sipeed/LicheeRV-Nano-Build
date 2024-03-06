#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/queue.h>
#include <pthread.h>
#include <stdatomic.h>
#include <inttypes.h>

#include <fcntl.h>		/* low-level i/o */
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "cvi_base.h"
#include <linux/cvi_math.h>
#include "cvi_sys.h"
#include "cvi_vpss.h"
#include "cvi_vo.h"
#include "cvi_region.h"
#include "hashmap.h"
#include "rgn_ioctl.h"

struct rgn_canvas {
	STAILQ_ENTRY(rgn_canvas) stailq;
	RGN_HANDLE Handle;
	CVI_U64 u64PhyAddr;
	CVI_U8 *pu8VirtAddr;
	CVI_U32 u32Size;
};

static pthread_once_t once = PTHREAD_ONCE_INIT;
static pthread_mutex_t canvas_q_lock = PTHREAD_MUTEX_INITIALIZER;
STAILQ_HEAD(rgn_canvas_q, rgn_canvas) canvas_q;

static void rgn_init(void)
{
	STAILQ_INIT(&canvas_q);
}

/**************************************************************************
 *   Public APIs.
 **************************************************************************/
CVI_S32 CVI_RGN_Create(RGN_HANDLE Handle, const RGN_ATTR_S *pstRegion)
{
	CVI_S32 fd = -1;

	MOD_CHECK_NULL_PTR(CVI_ID_RGN, pstRegion);

	// Driver control
	fd = get_rgn_fd();
	if (rgn_create(fd, Handle, pstRegion) != CVI_SUCCESS) {
		CVI_TRACE_RGN(CVI_DBG_INFO, "Create RGN fail.\n");
		return CVI_FAILURE;
	}

	pthread_mutex_lock(&canvas_q_lock);
	pthread_once(&once, rgn_init);
	pthread_mutex_unlock(&canvas_q_lock);

	return CVI_SUCCESS;
}

CVI_S32 CVI_RGN_Destroy(RGN_HANDLE Handle)
{
	CVI_S32 fd = -1;

	// Driver control
	fd = get_rgn_fd();
	if (rgn_destroy(fd, Handle) != CVI_SUCCESS) {
		CVI_TRACE_RGN(CVI_DBG_INFO, "Destroy RGN fail.\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_RGN_GetAttr(RGN_HANDLE Handle, RGN_ATTR_S *pstRegion)
{
	CVI_S32 fd = -1;

	MOD_CHECK_NULL_PTR(CVI_ID_RGN, pstRegion);

	// Driver control
	fd = get_rgn_fd();
	if (rgn_get_attr(fd, Handle, pstRegion) != CVI_SUCCESS) {
		CVI_TRACE_RGN(CVI_DBG_INFO, "Get RGN attributes fail.\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_RGN_SetAttr(RGN_HANDLE Handle, const RGN_ATTR_S *pstRegion)
{
	CVI_S32 fd = -1;

	MOD_CHECK_NULL_PTR(CVI_ID_RGN, pstRegion);
	// Driver control
	fd = get_rgn_fd();
	if (rgn_set_attr(fd, Handle, pstRegion) != CVI_SUCCESS) {
		CVI_TRACE_RGN(CVI_DBG_INFO, "Set RGN attributes fail.\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_RGN_SetBitMap(RGN_HANDLE Handle, const BITMAP_S *pstBitmap)
{
	CVI_S32 fd = -1;
	CVI_S32 s32Ret;

	MOD_CHECK_NULL_PTR(CVI_ID_RGN, pstBitmap);
	// Driver control
	fd = get_rgn_fd();

	s32Ret = rgn_set_bit_map(fd, Handle, pstBitmap);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_RGN(CVI_DBG_INFO, "Set RGN Bitmap fail, s32Ret=%x.\n", s32Ret);
		return s32Ret;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_RGN_AttachToChn(RGN_HANDLE Handle, const MMF_CHN_S *pstChn, const RGN_CHN_ATTR_S *pstChnAttr)
{
	CVI_S32 fd = -1;

	MOD_CHECK_NULL_PTR(CVI_ID_RGN, pstChn);
	MOD_CHECK_NULL_PTR(CVI_ID_RGN, pstChnAttr);

#ifdef __SOC_PHOBOS__
	if (pstChn->enModId == CVI_ID_VO) {
		CVI_TRACE_RGN(CVI_DBG_ERR, "No vo device, cannot attach to vo!\n");
		return CVI_ERR_RGN_ILLEGAL_PARAM;
	}
#endif

	// Driver control
	fd = get_rgn_fd();
	if (rgn_attach_to_chn(fd, Handle, pstChn, pstChnAttr) != CVI_SUCCESS) {
		CVI_TRACE_RGN(CVI_DBG_INFO, "Attach RGN to channel fail.\n");
		return CVI_FAILURE;
	}
	return CVI_SUCCESS;
}

CVI_S32 CVI_RGN_DetachFromChn(RGN_HANDLE Handle, const MMF_CHN_S *pstChn)
{
	CVI_S32 fd = -1;

	MOD_CHECK_NULL_PTR(CVI_ID_RGN, pstChn);

#ifdef __SOC_PHOBOS__
	if (pstChn->enModId == CVI_ID_VO) {
		CVI_TRACE_RGN(CVI_DBG_ERR, "No vo device, cannot detach from vo!\n");
		return CVI_ERR_RGN_ILLEGAL_PARAM;
	}
#endif

	// Driver control
	fd = get_rgn_fd();
	if (rgn_detach_from_chn(fd, Handle, pstChn) != CVI_SUCCESS) {
		CVI_TRACE_RGN(CVI_DBG_INFO, "Detach RGN from channel fail.\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_RGN_SetDisplayAttr(RGN_HANDLE Handle, const MMF_CHN_S *pstChn, const RGN_CHN_ATTR_S *pstChnAttr)
{
	CVI_S32 fd = -1;

	MOD_CHECK_NULL_PTR(CVI_ID_RGN, pstChn);
	MOD_CHECK_NULL_PTR(CVI_ID_RGN, pstChnAttr);

	// Driver control
	fd = get_rgn_fd();
	if (rgn_set_display_attr(fd, Handle, pstChn, pstChnAttr) != CVI_SUCCESS) {
		CVI_TRACE_RGN(CVI_DBG_INFO, "Set display RGN attributes fail.\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_RGN_GetDisplayAttr(RGN_HANDLE Handle, const MMF_CHN_S *pstChn, RGN_CHN_ATTR_S *pstChnAttr)
{
	CVI_S32 fd = -1;

	MOD_CHECK_NULL_PTR(CVI_ID_RGN, pstChn);
	MOD_CHECK_NULL_PTR(CVI_ID_RGN, pstChnAttr);

	// Driver control
	fd = get_rgn_fd();
	if (rgn_get_display_attr(fd, Handle, pstChn, pstChnAttr) != CVI_SUCCESS) {
		CVI_TRACE_RGN(CVI_DBG_INFO, "Get display RGN attributes fail.\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_RGN_GetCanvasInfo(RGN_HANDLE Handle, RGN_CANVAS_INFO_S *pstCanvasInfo)
{
	CVI_S32 fd = -1;
	CVI_S32 s32Ret;
	struct rgn_canvas *canvas;

	MOD_CHECK_NULL_PTR(CVI_ID_RGN, pstCanvasInfo);

	// Driver control
	fd = get_rgn_fd();

	s32Ret = rgn_get_canvas_info(fd, Handle, pstCanvasInfo);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_RGN(CVI_DBG_INFO, "Get RGN canvas information fail, s32Ret=%x.\n", s32Ret);
		return s32Ret;
	}

	canvas = calloc(1, sizeof(struct rgn_canvas));
	if (!canvas) {
		CVI_TRACE_RGN(CVI_DBG_ERR, "malloc failed.\n");
		return CVI_ERR_RGN_NOMEM;
	}
	pthread_mutex_lock(&canvas_q_lock);
	canvas->u64PhyAddr = pstCanvasInfo->u64PhyAddr;
	canvas->u32Size = pstCanvasInfo->u32Stride * pstCanvasInfo->stSize.u32Height;
	pstCanvasInfo->pu8VirtAddr = canvas->pu8VirtAddr =
			CVI_SYS_Mmap(pstCanvasInfo->u64PhyAddr, canvas->u32Size);
	if (pstCanvasInfo->pu8VirtAddr == NULL) {
		free(canvas);
		CVI_TRACE_RGN(CVI_DBG_INFO, "CVI_SYS_Mmap NG.\n");
		return CVI_FAILURE;
	}
	canvas->Handle = Handle;
	pstCanvasInfo->pstCanvasCmprAttr = (RGN_CANVAS_CMPR_ATTR_S *)pstCanvasInfo->pu8VirtAddr;
	pstCanvasInfo->pstObjAttr = (RGN_CMPR_OBJ_ATTR_S *)(pstCanvasInfo->pu8VirtAddr +
			sizeof(RGN_CANVAS_CMPR_ATTR_S));
	STAILQ_INSERT_TAIL(&canvas_q, canvas, stailq);
	pthread_mutex_unlock(&canvas_q_lock);

	return CVI_SUCCESS;
}

CVI_S32 CVI_RGN_UpdateCanvas(RGN_HANDLE Handle)
{
	CVI_S32 fd = -1;
	struct rgn_canvas *canvas;

	// Driver control
	fd = get_rgn_fd();
	if (rgn_update_canvas(fd, Handle) != CVI_SUCCESS) {
		CVI_TRACE_RGN(CVI_DBG_INFO, "Update RGN canvas fail.\n");
		return CVI_FAILURE;
	}

	pthread_mutex_lock(&canvas_q_lock);
	if (!STAILQ_EMPTY(&canvas_q)) {
		STAILQ_FOREACH(canvas, &canvas_q, stailq) {
			if (canvas->Handle == Handle) {
				STAILQ_REMOVE(&canvas_q, canvas, rgn_canvas, stailq);
				CVI_SYS_Munmap(canvas->pu8VirtAddr, canvas->u32Size);
				free(canvas);
				break;
			}
		}
	}
	pthread_mutex_unlock(&canvas_q_lock);

	return CVI_SUCCESS;
}

/* CVI_RGN_Invert_Color - invert color per luma statistics of video content
 *   Chns' pixel-format should be YUV.
 *   RGN's pixel-format should be ARGB1555.
 *
 * @param Handle: RGN Handle
 * @param pstChn: the chn which rgn attached
 * @param pu32Color: rgn's content
 */
CVI_S32 CVI_RGN_Invert_Color(RGN_HANDLE Handle, MMF_CHN_S *pstChn, CVI_U32 *pu32Color)
{
	CVI_S32 fd = -1;

	MOD_CHECK_NULL_PTR(CVI_ID_RGN, pstChn);
	MOD_CHECK_NULL_PTR(CVI_ID_RGN, pu32Color);

	// Driver control
	fd = get_rgn_fd();
	if (rgn_invert_color(fd, Handle, pstChn, (void *)pu32Color) != CVI_SUCCESS) {
		CVI_TRACE_RGN(CVI_DBG_INFO, "Invert RGN color fail.\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_RGN_SetChnPalette(RGN_HANDLE Handle, const MMF_CHN_S *pstChn, RGN_PALETTE_S *pstPalette)
{
	struct vdev *d;

	// Driver control
	d = get_dev_info(VDEV_TYPE_RGN, 0);
	if (!IS_VDEV_OPEN(d->state)) {
		CVI_TRACE_RGN(CVI_DBG_ERR, "rgn state(%d) incorrect.", d->state);
		return CVI_ERR_RGN_SYS_NOTREADY;
	}

	if (rgn_set_chn_palette(d->fd, Handle, pstChn, pstPalette) != CVI_SUCCESS) {
		CVI_TRACE_RGN(CVI_DBG_INFO, "Invert RGN color fail.\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}
