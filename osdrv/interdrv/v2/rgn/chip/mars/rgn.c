#include <linux/module.h>
#include <linux/hashtable.h>
#include <linux/uaccess.h>
#include <uapi/linux/sched/types.h>

#include <linux/cvi_base_ctx.h>
#include <linux/cvi_errno.h>
#include <linux/delay.h>
#include <linux/random.h>

#include <rgn.h>
#include "proc/rgn_proc.h"
#include <cif_cb.h>
#include <vpss_cb.h>
#include <vo_cb.h>
#include <rgn_cb.h>
#include <cmdqu_cb.h>
#include "sys.h"

/*******************************************************
 *  type definition
 ******************************************************/
struct rgn_mem_info {
	__u64 phy_addr;
	void *vir_addr;
	__u32 buf_len;
};

/*******************************************************
 *  Global variables
 ******************************************************/
//Keep every ctx include rgn_ctx and rgn_proc_ctx
struct cvi_rgn_ctx rgn_prc_ctx[RGN_MAX_NUM];
static struct rgn_mem_info s_astMosaicBuf[VPSS_MAX_GRP_NUM][VPSS_MAX_CHN_NUM];

static CVI_U32 u32RgnNum;
static struct mutex hdlslock, g_rgnlock, g_rgnhashlock;

DECLARE_HASHTABLE(rgn_hash, 6);

/*******************************************************
 *  Internal APIs
 ******************************************************/
static bool _rgn_hash_find(RGN_HANDLE Handle, struct cvi_rgn_ctx **ctx, bool del)
{
	bool is_found = false;
	struct cvi_rgn_ctx *obj;
	int bkt;
	struct hlist_node *tmp;

#if 0 //Linux Hashmap for print every element
	unsigned int bkt;

	CVI_TRACE_RGN(RGN_INFO, "_rgn_hash_find find handle(%d), del(%d)\n", Handle, del);
	hash_for_each(rgn_hash, bkt, obj, node) {
		CVI_TRACE_RGN(RGN_INFO, "rgn_hash: handle(%d), bCreated:(%d), stRegion.enType:(%d)\n"
			, obj->Handle, obj->bCreated, obj->stRegion.enType);
	}
#endif

	mutex_lock(&g_rgnhashlock);
	// hash_for_each_possible(rgn_hash, obj, node, Handle) {
	hash_for_each_safe(rgn_hash, bkt, tmp, obj, node) {
		if (obj->Handle == Handle) {
			if (del)
				hash_del(&obj->node);
			is_found = true;
			break;
		}
	}

	if (is_found)
		*ctx = obj;

	mutex_unlock(&g_rgnhashlock);
	return is_found;
}

static CVI_S32 CHECK_RGN_HANDLE(struct cvi_rgn_ctx **ctx, RGN_HANDLE Handle)
{
	if (!_rgn_hash_find(Handle, ctx, false)) {
		CVI_TRACE_RGN(RGN_ERR, "RGN_HANDLE(%d) is non-existent.\n", Handle);
		return CVI_ERR_RGN_UNEXIST;
	}

	return CVI_SUCCESS;
}

static int _rgn_call_cb(u32 m_id, u32 cmd_id, void *data)
{
	struct base_exe_m_cb exe_cb;

	exe_cb.callee = m_id;
	exe_cb.caller = E_MODULE_RGN;
	exe_cb.cmd_id = cmd_id;
	exe_cb.data   = (void *)data;

	return base_exe_module_cb(&exe_cb);
}

int32_t _rgn_init(void)
{
	// Only init once until exit.
	// It is guarantueed in rgn_open

	hash_init(rgn_hash);
	memset(rgn_prc_ctx, 0, sizeof(rgn_prc_ctx));
	u32RgnNum = 0;
	mutex_init(&hdlslock);
	mutex_init(&g_rgnlock);
	mutex_init(&g_rgnhashlock);
	CVI_TRACE_RGN(RGN_INFO, "-\n");
	return CVI_SUCCESS;
}

int32_t _rgn_exit(void)
{
	unsigned int bkt;
	struct cvi_rgn_ctx *obj;
	struct hlist_node *tmp;

	// Only exit once.
	// It is guarantueed in rgn_release

	hash_for_each_safe(rgn_hash, bkt, tmp, obj, node) {
		rgn_detach_from_chn(obj->Handle, &obj->stChn);
		rgn_destory(obj->Handle);
	}
	memset(rgn_prc_ctx, 0, sizeof(rgn_prc_ctx));
	u32RgnNum = 0;
	mutex_destroy(&hdlslock);
	mutex_destroy(&g_rgnlock);
	mutex_destroy(&g_rgnhashlock);

	CVI_TRACE_RGN(RGN_INFO, "-\n");
	return CVI_SUCCESS;
}

CVI_U32 _rgn_proc_get_idx(RGN_HANDLE hHandle)
{
	CVI_U32 i, idx = 0;

	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (rgn_prc_ctx[i].Handle == hHandle && rgn_prc_ctx[i].bCreated) {
			idx = i;
			break;
		}
	}

	if (i == RGN_MAX_NUM)
		CVI_TRACE_RGN(RGN_ERR, "cannot find corresponding rgn in rgn_prc_ctx, handle: %d\n", hHandle);

	return idx;
}

static inline CVI_S32 _rgn_get_bytesperline(PIXEL_FORMAT_E enPixelFormat, CVI_U32 width, CVI_U32 *bytesperline)
{
	switch (enPixelFormat) {
	case PIXEL_FORMAT_ARGB_8888:
		*bytesperline = width << 2;
		break;
	case PIXEL_FORMAT_ARGB_4444:
	case PIXEL_FORMAT_ARGB_1555:
		*bytesperline = width << 1;
		break;
	case PIXEL_FORMAT_8BIT_MODE:
		*bytesperline = width;
		break;
	default:
		CVI_TRACE_RGN(RGN_ERR, "not supported pxl-fmt(%d).\n", enPixelFormat);
		return CVI_ERR_RGN_ILLEGAL_PARAM;
	}
	return CVI_SUCCESS;
}

bool is_rect_overlap(RECT_S *r0, RECT_S *r1)
{
	// If one rectangle is on left side of other
	if (((CVI_U32)r0->s32X >= (r1->s32X + r1->u32Width)) || ((CVI_U32)r1->s32X >= (r0->s32X + r0->u32Width)))
		return false;

	// If one rectangle is above other
	if (((CVI_U32)r0->s32Y >= (r1->s32Y + r1->u32Height)) || ((CVI_U32)r1->s32Y >= (r0->s32Y + r0->u32Height)))
		return false;

	return true;
}

static CVI_S32 _rgn_get_rect(struct cvi_rgn_ctx *ctx, RECT_S *rect)
{
	if (ctx->stRegion.enType == OVERLAY_RGN) {
		rect->s32X = ctx->stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X;
		rect->s32Y = ctx->stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y;
		rect->u32Width = ctx->stRegion.unAttr.stOverlay.stSize.u32Width;
		rect->u32Height = ctx->stRegion.unAttr.stOverlay.stSize.u32Height;
		return CVI_SUCCESS;
	}
	if (ctx->stRegion.enType == OVERLAYEX_RGN) {
		rect->s32X = ctx->stChnAttr.unChnAttr.stOverlayExChn.stPoint.s32X;
		rect->s32Y = ctx->stChnAttr.unChnAttr.stOverlayExChn.stPoint.s32Y;
		rect->u32Width = ctx->stRegion.unAttr.stOverlayEx.stSize.u32Width;
		rect->u32Height = ctx->stRegion.unAttr.stOverlayEx.stSize.u32Height;
		return CVI_SUCCESS;
	}
	if (ctx->stRegion.enType == COVER_RGN) {
		*rect = ctx->stChnAttr.unChnAttr.stCoverChn.stRect;
		return CVI_SUCCESS;
	}
	if (ctx->stRegion.enType == COVEREX_RGN) {
		*rect = ctx->stChnAttr.unChnAttr.stCoverExChn.stRect;
		return CVI_SUCCESS;
	}
	if (ctx->stRegion.enType == MOSAIC_RGN) {
		*rect = ctx->stChnAttr.unChnAttr.stMosaicChn.stRect;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

static CVI_S32 _rgn_get_layer(struct cvi_rgn_ctx *ctx, CVI_U32 *layer)
{
	if (ctx->stRegion.enType == OVERLAY_RGN)
		*layer = ctx->stChnAttr.unChnAttr.stOverlayChn.u32Layer;
	else if (ctx->stRegion.enType == OVERLAYEX_RGN)
		*layer = ctx->stChnAttr.unChnAttr.stOverlayExChn.u32Layer;
	else if (ctx->stRegion.enType == COVER_RGN)
		*layer = ctx->stChnAttr.unChnAttr.stCoverChn.u32Layer;
	else if (ctx->stRegion.enType == COVEREX_RGN)
		*layer = ctx->stChnAttr.unChnAttr.stCoverExChn.u32Layer;
	else if (ctx->stRegion.enType == MOSAIC_RGN)
		*layer = ctx->stChnAttr.unChnAttr.stMosaicChn.u32Layer;
	else
		return CVI_FAILURE;

	return CVI_SUCCESS;
}


/* _rgn_check_order: check if r1's order should before r0.
 *
 */
CVI_BOOL _rgn_check_order(RECT_S *r0, RECT_S *r1)
{
	if (r0->s32X > r1->s32X)
		return CVI_TRUE;
	if ((r0->s32X == r1->s32X) && (r0->s32Y > r1->s32Y))
		return CVI_TRUE;
	return CVI_FALSE;
}

int _rgn_insert(RGN_HANDLE hdls[], CVI_U8 size, RGN_HANDLE hdl)
{
	struct cvi_rgn_ctx *ctx = NULL;
	RECT_S rect_old, rect_new;
	CVI_U8 i, j;
	CVI_S32 s32Ret;

	s32Ret = CHECK_RGN_HANDLE(&ctx, hdl);
	if (s32Ret != CVI_SUCCESS)
		return s32Ret;

	if (ctx->stCanvasInfo[ctx->canvas_idx].bCompressed
		&& hdls[0] != RGN_INVALID_HANDLE) {
		CVI_TRACE_RGN(RGN_ERR, "cannot insert hdl(%d), only allow one ow when odec on\n",
			hdl);
		return CVI_FAILURE;
	}
	if (_rgn_get_rect(ctx, &rect_new) != CVI_SUCCESS) {
		CVI_TRACE_RGN(RGN_ERR, "RGN_HANDLE(%d) can't get rect.\n", hdl);
		return CVI_FAILURE;
	}
	if (hdls[size - 1] != RGN_INVALID_HANDLE) {
		CVI_TRACE_RGN(RGN_ERR, "RGN_HANDLE(%d) rgn's count is full.\n", hdl);
		return CVI_FAILURE;
	}

	//Check if previously attached rgns overlap new rgn before inserting new rgn
	for (i = 0; i < size; ++i) {
		if (hdls[i] == RGN_INVALID_HANDLE)
			break;

		s32Ret = CHECK_RGN_HANDLE(&ctx, hdls[i]);
		if (s32Ret != CVI_SUCCESS)
			return s32Ret;
		if (_rgn_get_rect(ctx, &rect_old) != CVI_SUCCESS) {
			CVI_TRACE_RGN(RGN_ERR, "RGN_HANDLE(%d) can't get rect.\n", hdls[i]);
			return CVI_FAILURE;
		}

		if (is_rect_overlap(&rect_old, &rect_new)) {
			CVI_TRACE_RGN(RGN_ERR, "RGN_HANDLE(%d) is overlapped on another RGN_HANDLE(%d).\n"
				, hdl, hdls[i]);
			CVI_TRACE_RGN(RGN_DEBUG, "(%d %d %d %d) <-> (%d %d %d %d)\n"
				, rect_new.s32X, rect_new.s32Y, rect_new.u32Width, rect_new.u32Height
				, rect_old.s32X, rect_old.s32Y, rect_old.u32Width, rect_old.u32Height);
			return CVI_ERR_RGN_NOT_PERM;
		}
	}

	for (i = 0; i < size; ++i) {
		if (hdls[i] == RGN_INVALID_HANDLE) {
			hdls[i] = hdl;
			CVI_TRACE_RGN(RGN_DEBUG, "RGN_HANDLE(%d) at index(%d).\n", hdl, i);
			return CVI_SUCCESS;
		}

		s32Ret = CHECK_RGN_HANDLE(&ctx, hdls[i]);
		if (s32Ret != CVI_SUCCESS)
			return s32Ret;
		if (_rgn_get_rect(ctx, &rect_old) != CVI_SUCCESS) {
			CVI_TRACE_RGN(RGN_ERR, "RGN_HANDLE(%d) can't get rect.\n", hdls[i]);
			return CVI_FAILURE;
		}

		if (_rgn_check_order(&rect_old, &rect_new)) {
			if ((i + 1) < size)
				for (j = size - 1; j > i; j--)
					hdls[j] = hdls[j - 1];
			hdls[i] = hdl;
			CVI_TRACE_RGN(RGN_DEBUG, "RGN_HANDLE(%d) at index(%d).\n", hdl, i);
			return CVI_SUCCESS;
		}
	}
	return CVI_FAILURE;
}

int _rgn_ex_insert(RGN_HANDLE hdls[], CVI_U8 size, RGN_HANDLE hdl)
{
	struct cvi_rgn_ctx *ctx = NULL;
	CVI_U32 layer_new, layer_old;
	CVI_U8 i, j;
	CVI_S32 s32Ret;

	s32Ret = CHECK_RGN_HANDLE(&ctx, hdl);
	if (s32Ret != CVI_SUCCESS)
		return s32Ret;

	if (hdls[size - 1] != RGN_INVALID_HANDLE) {
		CVI_TRACE_RGN(RGN_ERR, "RGN_HANDLE(%d) rgn_ex's count is full.\n", hdl);
		return CVI_FAILURE;
	}

	if (_rgn_get_layer(ctx, &layer_new) != CVI_SUCCESS) {
		CVI_TRACE_RGN(RGN_ERR, "RGN_HANDLE(%d) can't get layer.\n", hdl);
		return CVI_FAILURE;
	}

	for (i = 0; i < size; ++i) {
		if (hdls[i] == RGN_INVALID_HANDLE) {
			hdls[i] = hdl;
			CVI_TRACE_RGN(RGN_DEBUG, "RGN_HANDLE(%d) at index(%d).\n", hdl, i);
			return CVI_SUCCESS;
		}

		s32Ret = CHECK_RGN_HANDLE(&ctx, hdls[i]);
		if (s32Ret != CVI_SUCCESS)
			return s32Ret;
		if (_rgn_get_layer(ctx, &layer_old) != CVI_SUCCESS) {
			CVI_TRACE_RGN(RGN_ERR, "RGN_HANDLE(%d) can't get layer.\n", hdls[i]);
			return CVI_FAILURE;
		}

		if (layer_new < layer_old) {
			if ((i + 1) < size)
				for (j = size - 1; j > i; j--)
					hdls[j] = hdls[j - 1];
			hdls[i] = hdl;
			CVI_TRACE_RGN(RGN_DEBUG, "RGN_HANDLE(%d) at index(%d).\n", hdl, i);
			return CVI_SUCCESS;
		}
	}
	return CVI_FAILURE;
}

int _rgn_remove(RGN_HANDLE hdls[], CVI_U8 size, RGN_HANDLE hdl)
{
	CVI_U8 i;

	for (i = 0; i < size; ++i) {
		if (hdls[i] == RGN_INVALID_HANDLE)
			return CVI_ERR_RGN_UNEXIST;

		if (hdl == hdls[i]) {
			if ((i + 1) < size)
				memmove(&hdls[i], &hdls[i + 1], sizeof(hdls[i]) * (size - 1 - i));
			hdls[size - 1] = RGN_INVALID_HANDLE;
			CVI_TRACE_RGN(RGN_DEBUG, "RGN_HANDLE(%d) at index(%d).\n", hdl, i);
			break;
		}
	}
	return CVI_SUCCESS;
}

/* _rgn_fill_pattern: fill buffer of length with pattern.
 *
 * @buf: buffer address
 * @len: buffer length
 * @color: color pattern. It depends on the pixel-format.
 * @bpp: bytes-per-pixel. Only 2 or 4 works.
 */
void _rgn_fill_pattern(void *buf, CVI_U32 len, CVI_U32 color, CVI_U8 bpp)
{
	CVI_U32 *p = buf;
	CVI_U32 i;
	CVI_U32 pattern = color;

	if (bpp == 2)
		pattern |= (pattern << 16);
	for (i = 0; i < len / sizeof(CVI_U32); ++i)
		p[i] = pattern;

	if (len & 0x02)
		*(((CVI_U16 *)&p[i]) + 1) = color;
}

/* _rgn_update_cover_canvas: fill cover/coverex if needed.
 *
 * @ctx: the region ctx to be updated.
 * @pstChnAttr: could be NULL. If not, will be used to check if update needed.
 */
static CVI_S32 _rgn_update_cover_canvas(struct cvi_rgn_ctx *ctx, const RGN_CHN_ATTR_S *pstChnAttr)
{
	PIXEL_FORMAT_E enPixelFormat = PIXEL_FORMAT_ARGB_1555;
	CVI_U32 bytesperline;
	RECT_S rect;
	CVI_U32 buf_len;
	CVI_U32 stride;
	CVI_BOOL bfill = CVI_FALSE;
	CVI_U32 proc_idx;

	if (!ctx || !pstChnAttr)
		return CVI_ERR_RGN_NULL_PTR;

	rect = (pstChnAttr->enType == COVER_RGN)
	     ? pstChnAttr->unChnAttr.stCoverChn.stRect
	     : pstChnAttr->unChnAttr.stCoverExChn.stRect;

	// check if color changed.
	bfill = (((ctx->stChnAttr.enType == COVER_RGN)
		&& (ctx->stChnAttr.unChnAttr.stCoverChn.u32Color
		 != pstChnAttr->unChnAttr.stCoverChn.u32Color))) ||
		(((ctx->stChnAttr.enType == COVEREX_RGN)
		&& (ctx->stChnAttr.unChnAttr.stCoverExChn.u32Color
		 != pstChnAttr->unChnAttr.stCoverExChn.u32Color)));

	_rgn_get_bytesperline(enPixelFormat, rect.u32Width, &bytesperline);
	stride = ALIGN(bytesperline, 32);
	buf_len = stride * rect.u32Height;

	proc_idx = _rgn_proc_get_idx(ctx->Handle);
	if (ctx->ion_len == 0) {
		ctx->canvas_idx = rgn_prc_ctx[proc_idx].canvas_idx = 0;
		rgn_prc_ctx[proc_idx].stCanvasInfo[0] = *ctx->stCanvasInfo;
	}

	// update canvas
	if (buf_len > ctx->ion_len) {
		if (ctx->ion_len != 0)
			sys_ion_free(ctx->stCanvasInfo[0].u64PhyAddr);

		ctx->stCanvasInfo[0].enPixelFormat = enPixelFormat;
		ctx->stCanvasInfo[0].stSize.u32Width = rect.u32Width;
		ctx->stCanvasInfo[0].stSize.u32Height = rect.u32Height;
		ctx->stCanvasInfo[0].u32Stride = stride;
		ctx->ion_len = ctx->u32MaxNeedIon = buf_len;
		if (sys_ion_alloc(&ctx->stCanvasInfo[0].u64PhyAddr, (void **)&ctx->stCanvasInfo[0].pu8VirtAddr,
						  "rgn_canvas", ctx->ion_len, false) != CVI_SUCCESS) {
			CVI_TRACE_RGN(RGN_ERR, "Region(%d) Can't acquire ion for Canvas.\n", ctx->Handle);
			return CVI_ERR_RGN_NOBUF;
		}
		bfill = CVI_TRUE;
	} else if (buf_len <= ctx->ion_len) {
		ctx->stCanvasInfo[0].stSize.u32Width = rect.u32Width;
		ctx->stCanvasInfo[0].stSize.u32Height = rect.u32Height;
		ctx->stCanvasInfo[0].u32Stride = stride;
	}
	rgn_prc_ctx[proc_idx].stCanvasInfo[0] = ctx->stCanvasInfo[0];

	// fill color pattern to cover/coverex.
	if (bfill) {
		CVI_U16 color;

		color = (ctx->stChnAttr.enType == COVEREX_RGN)
		      ? RGB888_2_ARGB1555(pstChnAttr->unChnAttr.stCoverExChn.u32Color)
		      : RGB888_2_ARGB1555(pstChnAttr->unChnAttr.stCoverChn.u32Color);
		_rgn_fill_pattern(ctx->stCanvasInfo[0].pu8VirtAddr, ctx->ion_len, color, 2);
	}

	return CVI_SUCCESS;
}

static CVI_S32 _rgn_update_hw_cfg_to_module(RGN_HANDLE *hdls, void *cfg, MMF_CHN_S *pstChn,
			RGN_TYPE_E enType, CVI_BOOL bCmpr)
{
	struct _rgn_hdls_cb_param rgn_hdls_arg;

	rgn_hdls_arg.stChn = *pstChn;
	rgn_hdls_arg.hdls = hdls;
	rgn_hdls_arg.enType = enType;
	rgn_hdls_arg.layer = 0;

	//Set VO module hdls and rgn_cfg
	if (pstChn->enModId == CVI_ID_VO) {
		if (_rgn_call_cb(E_MODULE_VO, VO_CB_SET_RGN_HDLS, &rgn_hdls_arg) != 0) {
			CVI_TRACE_RGN(RGN_ERR, "VO_CB_SET_RGN_HDLS is failed\n");
			return CVI_ERR_RGN_ILLEGAL_PARAM;
		}

		if (enType == OVERLAY_RGN || enType == COVER_RGN) {
			struct _rgn_cfg_cb_param rgn_cfg_arg;

			rgn_cfg_arg.stChn = *pstChn;
			rgn_cfg_arg.rgn_cfg = *(struct cvi_rgn_cfg *)cfg;
			if (_rgn_call_cb(E_MODULE_VO, VO_CB_SET_RGN_CFG, &rgn_cfg_arg) != 0) {
				CVI_TRACE_RGN(RGN_ERR, "VO_CB_SET_RGN_CFG is failed\n");
				return CVI_ERR_RGN_ILLEGAL_PARAM;
			}
		} else if (enType == COVEREX_RGN) {
			struct _rgn_coverex_cfg_cb_param rgn_coverex_cfg_arg;

			rgn_coverex_cfg_arg.stChn = *pstChn;
			rgn_coverex_cfg_arg.rgn_coverex_cfg = *(struct cvi_rgn_coverex_cfg *)cfg;
			if (_rgn_call_cb(E_MODULE_VO, VO_CB_SET_RGN_COVEREX_CFG, &rgn_coverex_cfg_arg) != 0) {
				CVI_TRACE_RGN(RGN_ERR, "VO_CB_SET_RGN_COVER_CFG is failed\n");
				return CVI_ERR_RGN_ILLEGAL_PARAM;
			}
		}
	} else if (pstChn->enModId == CVI_ID_VPSS) {
		CVI_U32 osd_layer = bCmpr ? RGN_ODEC_LAYER_VPSS : RGN_NORMAL_LAYER_VPSS;

		if (enType == OVERLAY_RGN || enType == COVER_RGN)
			rgn_hdls_arg.layer = osd_layer;

		//Set VPSS module rgn hdls
		if (_rgn_call_cb(E_MODULE_VPSS, VPSS_CB_SET_RGN_HDLS, &rgn_hdls_arg) != 0) {
			CVI_TRACE_RGN(RGN_ERR, "VPSS_CB_SET_RGN_HDLS is failed\n");
			return CVI_ERR_RGN_ILLEGAL_PARAM;
		}

		//Set VPSS module rgn_cfg
		if (enType == OVERLAY_RGN || enType == COVER_RGN) {
			struct _rgn_cfg_cb_param rgn_cfg_arg;

			rgn_cfg_arg.stChn = *pstChn;
			rgn_cfg_arg.rgn_cfg = *(struct cvi_rgn_cfg *)cfg;
			rgn_cfg_arg.layer = osd_layer;
			if (_rgn_call_cb(E_MODULE_VPSS, VPSS_CB_SET_RGN_CFG, &rgn_cfg_arg) != 0) {
				CVI_TRACE_RGN(RGN_ERR, "VPSS_CB_SET_RGN_CFG is failed\n");
				return CVI_ERR_RGN_ILLEGAL_PARAM;
			}
		} else if (enType == COVEREX_RGN) {
			//Set VPSS module cover_ex_cfg
			struct _rgn_coverex_cfg_cb_param rgn_coverex_cfg_arg;

			rgn_coverex_cfg_arg.stChn = *pstChn;
			rgn_coverex_cfg_arg.rgn_coverex_cfg = *(struct cvi_rgn_coverex_cfg *)cfg;
			if (_rgn_call_cb(E_MODULE_VPSS, VPSS_CB_SET_COVEREX_CFG, &rgn_coverex_cfg_arg) != 0) {
				CVI_TRACE_RGN(RGN_ERR, "VPSS_CB_SET_COVEREX_CFG is failed\n");
				return CVI_ERR_RGN_ILLEGAL_PARAM;
			}
		} else if (enType == OVERLAYEX_RGN) {
			//Set VPSS module rgn_ex_cfg
			struct _rgn_ex_cfg_cb_param rgnex_cfg_arg;

			rgnex_cfg_arg.stChn = *pstChn;
			rgnex_cfg_arg.rgn_ex_cfg = *(struct cvi_rgn_ex_cfg *)cfg;
			if (_rgn_call_cb(E_MODULE_VPSS, VPSS_CB_SET_RGNEX_CFG, &rgnex_cfg_arg) != 0) {
				CVI_TRACE_RGN(RGN_ERR, "VPSS_CB_SET_RGNEX_CFG is failed\n");
				return CVI_ERR_RGN_ILLEGAL_PARAM;
			}
		} else if (enType == MOSAIC_RGN) {
			struct _rgn_mosaic_cfg_cb_param rgn_mosaic_cfg_arg;

			rgn_mosaic_cfg_arg.stChn = *pstChn;
			rgn_mosaic_cfg_arg.rgn_mosaic_cfg = *(struct cvi_rgn_mosaic_cfg *)cfg;
			if (_rgn_call_cb(E_MODULE_VPSS, VPSS_CB_SET_MOSAIC_CFG, &rgn_mosaic_cfg_arg) != 0) {
				CVI_TRACE_RGN(RGN_ERR, "VPSS_CB_SET_MOSAIC_CFG is failed\n");
				return CVI_ERR_RGN_ILLEGAL_PARAM;
			}
		}
	} else {
		CVI_TRACE_RGN(RGN_ERR, "Unsupported mod_id(%d)\n", pstChn->enModId);
		return CVI_ERR_RGN_ILLEGAL_PARAM;
	}

	return CVI_SUCCESS;
}

/* _rgn_set_hw_cfg: update cfg based on the rgn attached.
 *
 * @hdls: rgn handles on chn.
 * @size: size of gop acceptable.
 * @pstChn: attached chn.
 * @enType: rgn type.
 * @bCmpr: rgn compress enable.
 */
static CVI_S32 _rgn_set_hw_cfg(RGN_HANDLE hdls[], CVI_U8 size, MMF_CHN_S *pstChn, RGN_TYPE_E enType, CVI_BOOL bCmpr)
{
	struct cvi_rgn_ctx *ctx = NULL;
	RECT_S rect;
	CVI_U8 rgn_idx = 0;
	CVI_U8 i;
	CVI_S32 s32Ret;
	struct cvi_rgn_cfg cfg = {0};

	for (i = 0; i < size; ++i) {
		if (hdls[i] == RGN_INVALID_HANDLE)
			break;

		s32Ret = CHECK_RGN_HANDLE(&ctx, hdls[i]);
		if (s32Ret != CVI_SUCCESS)
			return s32Ret;

		if (!ctx->stChnAttr.bShow)
			continue;
		if (_rgn_get_rect(ctx, &rect) != CVI_SUCCESS)
			continue;

		switch (ctx->stCanvasInfo[ctx->canvas_idx].enPixelFormat) {
		case PIXEL_FORMAT_ARGB_8888:
			cfg.param[rgn_idx].fmt = CVI_RGN_FMT_ARGB8888;
			break;
		case PIXEL_FORMAT_ARGB_4444:
			cfg.param[rgn_idx].fmt = CVI_RGN_FMT_ARGB4444;
			break;
		case PIXEL_FORMAT_8BIT_MODE:
			cfg.param[rgn_idx].fmt = CVI_RGN_FMT_256LUT;
			break;
		case PIXEL_FORMAT_ARGB_1555:
		default:
			cfg.param[rgn_idx].fmt = CVI_RGN_FMT_ARGB1555;
			break;
		}
		cfg.param[rgn_idx].rect.left	= rect.s32X;
		cfg.param[rgn_idx].rect.top	= rect.s32Y;
		cfg.param[rgn_idx].rect.width	= ctx->stCanvasInfo[ctx->canvas_idx].stSize.u32Width;
		cfg.param[rgn_idx].rect.height	= ctx->stCanvasInfo[ctx->canvas_idx].stSize.u32Height;
		cfg.param[rgn_idx].phy_addr	= ctx->stCanvasInfo[ctx->canvas_idx].u64PhyAddr;
		cfg.param[rgn_idx].stride	= ctx->stCanvasInfo[ctx->canvas_idx].u32Stride;

		//only one OSD window can be attached.
		if (ctx->stCanvasInfo[ctx->canvas_idx].bCompressed) {
			// this function is first called by rgn_attach_to_chn, but cmpr data is not valid.
			// so, do not enable odec at first time to void garbage
			if (ctx->odec_data_valid) {
				cfg.odec.enable	= true;
			} else {
				cfg.odec.enable	= false;
			}
			cfg.odec.attached_ow	= rgn_idx;
			cfg.odec.bso_sz	= ctx->stCanvasInfo[ctx->canvas_idx].u32CompressedSize;
			break;
		} else
			cfg.odec.enable	= false;
		++rgn_idx;
	}
	cfg.num_of_rgn = (cfg.odec.enable) ? 1 : rgn_idx;

	cfg.hscale_x2 = false;
	cfg.vscale_x2 = false;
	cfg.colorkey_en = false;

	s32Ret = _rgn_update_hw_cfg_to_module(hdls, (void *)&cfg, pstChn, enType, bCmpr);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_RGN(RGN_ERR, "_rgn_update_hw_cfg_to_module failed\n");
	}

	return s32Ret;
}

/* _rgn_ex_set_hw_cfg: update cfg based on the rgn_ex attached.
 *
 * @hdls: rgn_ex handles on chn.
 * @size: max num of rgn.
 * @pstChn: attached chn.
 * @enType: rgn type.
 */
static CVI_S32 _rgn_ex_set_hw_cfg(RGN_HANDLE hdls[], CVI_U8 size, MMF_CHN_S *pstChn, RGN_TYPE_E enType)
{
	struct cvi_rgn_ctx *ctx = NULL;
	RECT_S rect;
	CVI_U8 rgn_idx = 0;
	CVI_U8 rgn_tile = 0; // 1: left tile, 2: right tile
	CVI_U32 left_w = 0, bpp = 0;
	CVI_U8 i;
	CVI_S32 s32Ret;
	struct cvi_rgn_ex_cfg ex_cfg = {0};

	for (i = 0; i < size; ++i) {
		if (hdls[i] == RGN_INVALID_HANDLE)
			break;

		s32Ret = CHECK_RGN_HANDLE(&ctx, hdls[i]);
		if (s32Ret != CVI_SUCCESS)
			return s32Ret;
		if (!ctx->stChnAttr.bShow)
			continue;
		if (_rgn_get_rect(ctx, &rect) != CVI_SUCCESS)
			continue;

		if (rect.u32Width > RGN_EX_MAX_WIDTH) {
			CVI_U32 byte_offset;

			left_w = rect.u32Width / 2;
			bpp = (ctx->stCanvasInfo[ctx->canvas_idx].enPixelFormat == PIXEL_FORMAT_ARGB_8888)
				? 4 : 2;

			byte_offset = (bpp * left_w) & (0x20 - 1);
			if (byte_offset) {
				left_w -= (byte_offset / bpp);
			}
			rgn_tile = 1;
		}

		do {
			switch (ctx->stCanvasInfo[ctx->canvas_idx].enPixelFormat) {
			case PIXEL_FORMAT_ARGB_8888:
				ex_cfg.rgn_ex_param[rgn_idx].fmt = CVI_RGN_FMT_ARGB8888;
				break;
			case PIXEL_FORMAT_ARGB_4444:
				ex_cfg.rgn_ex_param[rgn_idx].fmt = CVI_RGN_FMT_ARGB4444;
				break;
			case PIXEL_FORMAT_ARGB_1555:
			default:
				ex_cfg.rgn_ex_param[rgn_idx].fmt = CVI_RGN_FMT_ARGB1555;
				break;
			}
			ex_cfg.rgn_ex_param[rgn_idx].stride = ctx->stCanvasInfo[ctx->canvas_idx].u32Stride;

			if (rgn_tile == 1) { // left tile
				ex_cfg.rgn_ex_param[rgn_idx].phy_addr = ctx->stCanvasInfo[ctx->canvas_idx].u64PhyAddr;
				ex_cfg.rgn_ex_param[rgn_idx].rect.left = rect.s32X;
				ex_cfg.rgn_ex_param[rgn_idx].rect.top = rect.s32Y;
				ex_cfg.rgn_ex_param[rgn_idx].rect.width = left_w;
				ex_cfg.rgn_ex_param[rgn_idx].rect.height
						= ctx->stCanvasInfo[ctx->canvas_idx].stSize.u32Height;
				rgn_tile = 2;
			} else if (rgn_tile == 2) { // right tile
				ex_cfg.rgn_ex_param[rgn_idx].phy_addr
					= ctx->stCanvasInfo[ctx->canvas_idx].u64PhyAddr + (left_w * bpp);
				ex_cfg.rgn_ex_param[rgn_idx].rect.left = rect.s32X + left_w;
				ex_cfg.rgn_ex_param[rgn_idx].rect.top = rect.s32Y;
				ex_cfg.rgn_ex_param[rgn_idx].rect.width
						= ctx->stCanvasInfo[ctx->canvas_idx].stSize.u32Width - left_w;
				ex_cfg.rgn_ex_param[rgn_idx].rect.height
						= ctx->stCanvasInfo[ctx->canvas_idx].stSize.u32Height;
				rgn_tile = 0;
			} else { // no tile
				ex_cfg.rgn_ex_param[rgn_idx].phy_addr = ctx->stCanvasInfo[ctx->canvas_idx].u64PhyAddr;
				ex_cfg.rgn_ex_param[rgn_idx].rect.left = rect.s32X;
				ex_cfg.rgn_ex_param[rgn_idx].rect.top = rect.s32Y;
				ex_cfg.rgn_ex_param[rgn_idx].rect.width
						= ctx->stCanvasInfo[ctx->canvas_idx].stSize.u32Width;
				ex_cfg.rgn_ex_param[rgn_idx].rect.height
						= ctx->stCanvasInfo[ctx->canvas_idx].stSize.u32Height;
				++rgn_idx;
				break;
			}
			++rgn_idx;
		} while (rgn_tile);
	}
	ex_cfg.num_of_rgn_ex = rgn_idx;

	ex_cfg.hscale_x2 = false;
	ex_cfg.vscale_x2 = false;
	ex_cfg.colorkey_en = false;

	s32Ret = _rgn_update_hw_cfg_to_module(hdls, (void *)&ex_cfg, pstChn, enType, false);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_RGN(RGN_ERR, "_rgn_update_hw_cfg_to_module failed\n");
	}

	return s32Ret;
}

static CVI_S32 _rgn_coverex_set_hw_cfg(RGN_HANDLE hdls[], CVI_U8 size, MMF_CHN_S *pstChn, RGN_TYPE_E enType)
{
	struct cvi_rgn_ctx *ctx = NULL;
	RECT_S rect;
	CVI_U8 rgn_idx = 0;
	CVI_U8 i;
	CVI_S32 s32Ret;
	struct cvi_rgn_coverex_cfg coverex_cfg = {0};

	for (i = 0; i < size; ++i) {
		if (hdls[i] == RGN_INVALID_HANDLE)
			break;

		s32Ret = CHECK_RGN_HANDLE(&ctx, hdls[i]);
		if (s32Ret != CVI_SUCCESS)
			return s32Ret;

		if (!ctx->stChnAttr.bShow)
			continue;
		if (_rgn_get_rect(ctx, &rect) != CVI_SUCCESS)
			continue;

		coverex_cfg.rgn_coverex_param[rgn_idx].rect.left = rect.s32X;
		coverex_cfg.rgn_coverex_param[rgn_idx].rect.top = rect.s32Y;
		coverex_cfg.rgn_coverex_param[rgn_idx].rect.width = rect.u32Width;
		coverex_cfg.rgn_coverex_param[rgn_idx].rect.height = rect.u32Height;
		coverex_cfg.rgn_coverex_param[rgn_idx].enable = 1;
		coverex_cfg.rgn_coverex_param[rgn_idx].color = ctx->stChnAttr.unChnAttr.stCoverExChn.u32Color;
		++rgn_idx;
	}

	s32Ret = _rgn_update_hw_cfg_to_module(hdls, (void *)&coverex_cfg, pstChn, enType, false);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_RGN(RGN_ERR, "_rgn_update_hw_cfg_to_module failed\n");
	}

	return s32Ret;
}

static CVI_S32 _rgn_mosaic_set_hw_cfg(RGN_HANDLE hdls[], CVI_U8 size, MMF_CHN_S *pstChn, RGN_TYPE_E enType)
{
	CVI_S32 s32Ret;
	struct cvi_rgn_ctx *ctx = NULL;
	RECT_S rect[RGN_MOSAIC_MAX_NUM];
	CVI_U8 rgn_idx = 0;
	CVI_U8 i, j, k, tmp;
	MOSAIC_BLK_SIZE_E enBlkSize = MOSAIC_BLK_SIZE_16;
	CVI_U8 grid_size = 16, grid_offset;
	CVI_U32 buf_size;
	CVI_S32 start_x = 0xffff, end_x = 0, start_y = 0xffff, end_y = 0;
	CVI_U16 u32Stride, grid_num_w, grid_num_h, x_offset, y_offset, index;
	CVI_U64 u64PhyAddr;
	CVI_VOID *pu8VirtAddr;
	CVI_U8 *pData;
	struct rgn_mem_info *ion_buf = &s_astMosaicBuf[pstChn->s32DevId][pstChn->s32ChnId];
	struct cvi_rgn_mosaic_cfg mosaic_cfg = {0};

	for (i = 0; i < size; ++i) {
		if (hdls[i] == RGN_INVALID_HANDLE)
			break;
		s32Ret = CHECK_RGN_HANDLE(&ctx, hdls[i]);
		if (s32Ret != CVI_SUCCESS)
			return s32Ret;
		if (!ctx->stChnAttr.bShow)
			continue;
		if (_rgn_get_rect(ctx, &rect[rgn_idx]) != CVI_SUCCESS)
			continue;
		if (ctx->stChnAttr.unChnAttr.stMosaicChn.enBlkSize == MOSAIC_BLK_SIZE_8) {
			enBlkSize = MOSAIC_BLK_SIZE_8;
			grid_size = 8;
		}
		start_x = MIN(start_x, rect[rgn_idx].s32X);
		start_y = MIN(start_y, rect[rgn_idx].s32Y);
		rgn_idx++;
	}

	if (rgn_idx == 0) {
		if (ion_buf->buf_len != 0) {
			sys_ion_free(ion_buf->phy_addr);
			ion_buf->phy_addr = 0;
			ion_buf->buf_len = 0;
			ion_buf->vir_addr = NULL;
		}
		return 0;
	}

	//rect align to 8x8 or 16x16
	for (i = 0; i < rgn_idx; ++i) {
		grid_offset = (rect[i].s32X - start_x) % grid_size;
		rect[i].s32X -= grid_offset;
		rect[i].u32Width = ALIGN(rect[i].u32Width + grid_offset, grid_size);
		grid_offset = (rect[i].s32Y - start_y) % grid_size;
		rect[i].s32Y -= grid_offset;
		rect[i].u32Height = ALIGN(rect[i].u32Height + grid_offset, grid_size);

		end_x = MAX(end_x, rect[i].s32X + rect[i].u32Width);
		end_y = MAX(end_y, rect[i].s32Y + rect[i].u32Height);
	}

	grid_num_w = (end_x - start_x) / grid_size;
	grid_num_h = (end_y - start_y) / grid_size;
	u32Stride = ALIGN(grid_num_w, 16); //byte base, must be 16-byte align
	buf_size = u32Stride * grid_num_h;

	if (buf_size > ion_buf->buf_len) {
		if (ion_buf->buf_len != 0) {
			sys_ion_free(ion_buf->phy_addr);
			ion_buf->phy_addr = 0;
			ion_buf->buf_len = 0;
			ion_buf->vir_addr = NULL;
		}
		if (sys_ion_alloc(&u64PhyAddr, &pu8VirtAddr, "mosaic_canvas", buf_size, false) != CVI_SUCCESS) {
			CVI_TRACE_RGN(RGN_ERR, "Region(%d) Can't acquire ion for Canvas.\n", ctx->Handle);
			return CVI_ERR_RGN_NOBUF;
		}
		ion_buf->phy_addr = u64PhyAddr;
		ion_buf->buf_len = buf_size;
		ion_buf->vir_addr = pu8VirtAddr;
	} else {
		pu8VirtAddr = ion_buf->vir_addr;
		u64PhyAddr = ion_buf->phy_addr;
		buf_size = ion_buf->buf_len;
	}

	memset(pu8VirtAddr, 0, buf_size);
	pData = (CVI_U8 *)pu8VirtAddr;
	for (i = 0; i < rgn_idx; ++i) {
		y_offset = (rect[i].s32Y - start_y) / grid_size * u32Stride;
		for (j = 0; j < (rect[i].u32Height / grid_size); ++j) {
			for (k = 0; k < (rect[i].u32Width / grid_size); ++k) {
				x_offset = (rect[i].s32X - start_x) / grid_size;
				index = y_offset + j * u32Stride + x_offset + k;
				tmp = (u8)get_random_u32();
				pData[index] = tmp ? tmp : 1;
			}
		}
	}

	CVI_TRACE_RGN(RGN_DEBUG, "mosaic num(%d),rect(%d %d %d %d), phy_addr:0x%llx buf_len:%d.\n",
		rgn_idx, start_x, start_y, end_x, end_y, u64PhyAddr, buf_size);

	mosaic_cfg.start_x = start_x;
	mosaic_cfg.start_y = start_y;
	mosaic_cfg.end_x = end_x;
	mosaic_cfg.end_y = end_y;
	mosaic_cfg.enable = 1;
	mosaic_cfg.blk_size = enBlkSize;
	mosaic_cfg.phy_addr = u64PhyAddr;

	s32Ret = _rgn_update_hw_cfg_to_module(hdls, (void *)&mosaic_cfg, pstChn, enType, false);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_RGN(RGN_ERR, "_rgn_update_hw_cfg_to_module failed\n");
	}

	return s32Ret;
}


/* _rgn_get_chn_info: get info of chn for rgn.
 *
 * @pstChn: the chn want to check.
 * @numOfLayer: number of gop layer supported on the chn.
 * @size: number of ow supported on the chn.
 */
static CVI_S32 _rgn_get_chn_info(struct cvi_rgn_ctx *ctx,
	RGN_HANDLE *hdls, CVI_U8 *size)
{
	struct _rgn_hdls_cb_param rgn_hdls_arg;

	rgn_hdls_arg.stChn = ctx->stChn;
	rgn_hdls_arg.hdls = hdls;
	rgn_hdls_arg.enType = ctx->stRegion.enType;

	if (ctx->stChn.enModId == CVI_ID_VO) {
		rgn_hdls_arg.layer = 0; // vo only has one layer gop
		if (_rgn_call_cb(E_MODULE_VO, VO_CB_GET_RGN_HDLS, &rgn_hdls_arg) != 0) {
			CVI_TRACE_RGN(RGN_ERR, "VO_CB_GET_RGN_HDLS is failed\n");
			return CVI_ERR_RGN_ILLEGAL_PARAM;
		}
		if (ctx->stRegion.enType == OVERLAY_RGN || ctx->stRegion.enType == COVER_RGN)
			*size = RGN_MAX_NUM_VO;
		else if (ctx->stRegion.enType == COVEREX_RGN)
			*size = RGN_COVEREX_MAX_NUM;
	} else if (ctx->stChn.enModId == CVI_ID_VPSS) {
		if (ctx->stRegion.enType == OVERLAY_RGN || ctx->stRegion.enType == COVER_RGN) {
			rgn_hdls_arg.layer = (ctx->stCanvasInfo[ctx->canvas_idx].bCompressed) ?
				RGN_ODEC_LAYER_VPSS : RGN_NORMAL_LAYER_VPSS;
			*size = RGN_MAX_NUM_VPSS;
		} else if (ctx->stRegion.enType == COVEREX_RGN) {
			rgn_hdls_arg.layer = 0; //invalid
			*size = RGN_COVEREX_MAX_NUM;
		} else if (ctx->stRegion.enType == OVERLAYEX_RGN) {
			rgn_hdls_arg.layer = 0; //invalid
			*size = RGN_EX_MAX_NUM_VPSS;
		} else if (ctx->stRegion.enType == MOSAIC_RGN) {
			rgn_hdls_arg.layer = 0; //invalid
			*size = RGN_MOSAIC_MAX_NUM;
		}

		if (_rgn_call_cb(E_MODULE_VPSS, VPSS_CB_GET_RGN_HDLS, &rgn_hdls_arg) != 0) {
			CVI_TRACE_RGN(RGN_ERR, "VPSS_CB_GET_RGN_HDLS is failed\n");
			return CVI_ERR_RGN_ILLEGAL_PARAM;
		}
	} else {
		return CVI_ERR_RGN_INVALID_DEVID;
	}

	return CVI_SUCCESS;
}

static CVI_S32 _rgn_update_hw(struct cvi_rgn_ctx *ctx, enum RGN_OP op)
{
	CVI_U8 size;
	CVI_S32 ret;
	RGN_HANDLE hdls[RGN_EX_MAX_NUM_VPSS];

	mutex_lock(&hdlslock);
	ret = _rgn_get_chn_info(ctx, hdls, &size);
	if (ret != CVI_SUCCESS) {
		mutex_unlock(&hdlslock);
		return ret;
	}

	if (op == RGN_OP_INSERT) {
		if (ctx->stRegion.enType == OVERLAY_RGN || ctx->stRegion.enType == COVER_RGN) {
			ret = _rgn_insert(hdls, size, ctx->Handle);
			if (ret != CVI_SUCCESS) {
				mutex_unlock(&hdlslock);
				return ret;
			}
		} else {
			ret = _rgn_ex_insert(hdls, size, ctx->Handle);
			if (ret != CVI_SUCCESS) {
				mutex_unlock(&hdlslock);
				return ret;
			}
		}
	} else if (op == RGN_OP_REMOVE) {
		ret = _rgn_remove(hdls, size, ctx->Handle);
		if (ret != CVI_SUCCESS) {
			mutex_unlock(&hdlslock);
			CVI_TRACE_RGN(RGN_ERR, "RGN_HANDLE(%d) not at CHN(%s-%d-%d).\n", ctx->Handle
			, sys_get_modname(ctx->stChn.enModId), ctx->stChn.s32DevId, ctx->stChn.s32ChnId);
			return ret;
		}
	} else if (op == RGN_OP_UPDATE) {
		_rgn_remove(hdls, size, ctx->Handle);
		if (ctx->stRegion.enType == OVERLAY_RGN || ctx->stRegion.enType == COVER_RGN) {
			ret = _rgn_insert(hdls, size, ctx->Handle);
			if (ret != CVI_SUCCESS) {
				mutex_unlock(&hdlslock);
				return ret;
			}
		} else {
			ret = _rgn_ex_insert(hdls, size, ctx->Handle);
			if (ret != CVI_SUCCESS) {
				mutex_unlock(&hdlslock);
				return ret;
			}
		}
	}

	if (ctx->stRegion.enType == OVERLAY_RGN || ctx->stRegion.enType == COVER_RGN) {
		ret = _rgn_set_hw_cfg(hdls, size, &ctx->stChn, ctx->stRegion.enType,
					ctx->stCanvasInfo[ctx->canvas_idx].bCompressed);
		if (ret != CVI_SUCCESS) {
			CVI_TRACE_RGN(RGN_ERR, "_rgn_set_hw_cfg failed\n");
			mutex_unlock(&hdlslock);
			return ret;
		}
	} else if (ctx->stRegion.enType == COVEREX_RGN) {
		ret = _rgn_coverex_set_hw_cfg(hdls, size, &ctx->stChn, ctx->stRegion.enType);
		if (ret != CVI_SUCCESS) {
			CVI_TRACE_RGN(RGN_ERR, "_rgn_coverex_set_hw_cfg failed\n");
			mutex_unlock(&hdlslock);
			return ret;
		}
	} else if (ctx->stRegion.enType == OVERLAYEX_RGN) {
		ret = _rgn_ex_set_hw_cfg(hdls, size, &ctx->stChn, ctx->stRegion.enType);
		if (ret != CVI_SUCCESS) {
			CVI_TRACE_RGN(RGN_ERR, "_rgn_ex_set_hw_cfg failed\n");
			mutex_unlock(&hdlslock);
			return ret;
		}
	} else if (ctx->stRegion.enType == MOSAIC_RGN) {
		ret = _rgn_mosaic_set_hw_cfg(hdls, size, &ctx->stChn, ctx->stRegion.enType);
		if (ret != CVI_SUCCESS) {
			CVI_TRACE_RGN(RGN_ERR, "_rgn_mosaic_set_hw_cfg failed\n");
			mutex_unlock(&hdlslock);
			return ret;
		}
	} else {
		mutex_unlock(&hdlslock);
		return CVI_ERR_RGN_NOT_SUPPORT;
	}

	#if 0
	sys_cache_flush(ctx->stCanvasInfo[ctx->canvas_idx].u64PhyAddr,
		ctx->stCanvasInfo[ctx->canvas_idx].pu8VirtAddr, ctx->ion_len);
	#endif
	mutex_unlock(&hdlslock);

	return CVI_SUCCESS;
}

CVI_S32 _rgn_check_chn_attr(RGN_HANDLE Handle, const MMF_CHN_S *pstChn, const RGN_CHN_ATTR_S *pstChnAttr)
{
	RECT_S rect_chn, rect_rgn;
	struct cvi_rgn_ctx *ctx;

	CHECK_RGN_HANDLE(&ctx, Handle);

	if ((pstChnAttr->enType == OVERLAYEX_RGN || pstChnAttr->enType == MOSAIC_RGN) &&
			(pstChn->enModId != CVI_ID_VPSS)) {
		CVI_TRACE_RGN(RGN_ERR, "overlayex/mosaic only vpss support!\n");
		return CVI_ERR_RGN_NOT_SUPPORT;
	}

	if (pstChn->enModId == CVI_ID_VO) {
		struct _rgn_chn_size_cb_param rgn_chn_arg;

		rgn_chn_arg.stChn = *pstChn;

		if (_rgn_call_cb(E_MODULE_VO, VO_CB_GET_CHN_SIZE, &rgn_chn_arg)) {
			CVI_TRACE_RGN(RGN_ERR, "VO_CB_GET_CHN_SIZE failed!\n");
			return CVI_ERR_RGN_INVALID_CHNID;
		}
		rect_chn = rgn_chn_arg.rect;
	} else if (pstChn->enModId == CVI_ID_VPSS) {
		struct _rgn_chn_size_cb_param rgn_chn_arg;

		rgn_chn_arg.stChn = *pstChn;

		if (_rgn_call_cb(E_MODULE_VPSS, VPSS_CB_GET_CHN_SIZE, &rgn_chn_arg)) {
			CVI_TRACE_RGN(RGN_ERR, "VPSS_CB_GET_CHN_SIZE failed!\n");
			return CVI_ERR_RGN_INVALID_CHNID;
		}
		rect_chn = rgn_chn_arg.rect;
	} else
		return CVI_ERR_RGN_ILLEGAL_PARAM;

	if (pstChnAttr->enType == COVEREX_RGN) {
		if (pstChnAttr->unChnAttr.stCoverExChn.enCoverType != AREA_RECT) {
			CVI_TRACE_RGN(RGN_ERR, "AREA_RECT only now.\n");
			return CVI_ERR_RGN_ILLEGAL_PARAM;
		}
		rect_rgn = pstChnAttr->unChnAttr.stCoverExChn.stRect;
	} else if (pstChnAttr->enType == COVER_RGN) {
		if (pstChnAttr->unChnAttr.stCoverChn.enCoverType != AREA_RECT) {
			CVI_TRACE_RGN(RGN_ERR, "AREA_RECT only now.\n");
			return CVI_ERR_RGN_ILLEGAL_PARAM;
		}
		if (pstChnAttr->unChnAttr.stCoverChn.enCoordinate != RGN_ABS_COOR) {
			CVI_TRACE_RGN(RGN_ERR, "abs-coordinate only now.\n");
			return CVI_ERR_RGN_ILLEGAL_PARAM;
		}
		rect_rgn = pstChnAttr->unChnAttr.stCoverChn.stRect;
	} else if (pstChnAttr->enType == OVERLAY_RGN) {
		rect_rgn.s32X = pstChnAttr->unChnAttr.stOverlayChn.stPoint.s32X;
		rect_rgn.s32Y = pstChnAttr->unChnAttr.stOverlayChn.stPoint.s32Y;
		rect_rgn.u32Width = ctx->stRegion.unAttr.stOverlay.stSize.u32Width;
		rect_rgn.u32Height = ctx->stRegion.unAttr.stOverlay.stSize.u32Height;
	} else if (pstChnAttr->enType == OVERLAYEX_RGN) {
		rect_rgn.s32X = pstChnAttr->unChnAttr.stOverlayExChn.stPoint.s32X;
		rect_rgn.s32Y = pstChnAttr->unChnAttr.stOverlayExChn.stPoint.s32Y;
		rect_rgn.u32Width = ctx->stRegion.unAttr.stOverlayEx.stSize.u32Width;
		rect_rgn.u32Height = ctx->stRegion.unAttr.stOverlayEx.stSize.u32Height;
	} else if (pstChnAttr->enType == MOSAIC_RGN) {
		if (pstChnAttr->unChnAttr.stMosaicChn.enBlkSize != MOSAIC_BLK_SIZE_8
			&& pstChnAttr->unChnAttr.stMosaicChn.enBlkSize != MOSAIC_BLK_SIZE_16) {
			CVI_TRACE_RGN(RGN_ERR, "Mosaic only support block size 8/16.\n");
			return CVI_ERR_RGN_ILLEGAL_PARAM;
		}
		rect_rgn = pstChnAttr->unChnAttr.stMosaicChn.stRect;
	} else {
		return CVI_ERR_RGN_NOT_SUPPORT;
	}

	if (rect_rgn.s32X < 0 || rect_rgn.s32Y < 0) {
		CVI_TRACE_RGN(RGN_ERR, "The start point can't be less than 0, start point(%d %d).\n",
			rect_rgn.s32X, rect_rgn.s32Y);
		return CVI_ERR_RGN_ILLEGAL_PARAM;
	}
	if ((rect_rgn.u32Width + rect_rgn.s32X) > rect_chn.u32Width
		|| (rect_rgn.u32Height + rect_rgn.s32Y) > rect_chn.u32Height) {
		CVI_TRACE_RGN(RGN_ERR, "size(%d %d %d %d) larger than chnsize(%d %d).\n",
			rect_rgn.s32X, rect_rgn.u32Width, rect_rgn.s32Y, rect_rgn.u32Height,
			rect_chn.u32Width, rect_chn.u32Height);
		return CVI_ERR_RGN_ILLEGAL_PARAM;
	}

	return CVI_SUCCESS;
}

/**************************************************************************
 *   Sinking RGN APIs related functions.
 **************************************************************************/
CVI_S32 rgn_create(RGN_HANDLE Handle, const RGN_ATTR_S *pstRegion)
{
	struct cvi_rgn_ctx *ctx = NULL;
	CVI_S32 ret;
	CVI_U32 proc_idx;

	mutex_lock(&g_rgnlock);

	if (_rgn_hash_find(Handle, &ctx, false)) {
		CVI_TRACE_RGN(RGN_ERR, "RGN_HANDLE(%d) is already existing.\n", Handle);
		mutex_unlock(&g_rgnlock);
		return CVI_ERR_RGN_EXIST;
	}

	if (u32RgnNum >= RGN_MAX_NUM) {
		CVI_TRACE_RGN(RGN_ERR, "over max rgn number %d\n", RGN_MAX_NUM);
		mutex_unlock(&g_rgnlock);
		return CVI_ERR_RGN_NOT_PERM;
	}

	if (pstRegion->enType == OVERLAYEX_RGN) {
		if (sys_get_vpssmode() != VPSS_MODE_RGNEX) {
			CVI_TRACE_RGN(RGN_ERR, "rgn extension only support on vpss rgnex mode.\n");
			mutex_unlock(&g_rgnlock);
			return CVI_ERR_RGN_NOT_SUPPORT;
		}
	}

	// check parameters.
	if (pstRegion->enType == OVERLAY_RGN || pstRegion->enType == OVERLAYEX_RGN) {
		PIXEL_FORMAT_E pixelFormat;
		CVI_U32 canvasNum;
		CVI_BOOL check_fail = CVI_FALSE;

		if (pstRegion->enType == OVERLAY_RGN) {
			pixelFormat = pstRegion->unAttr.stOverlay.enPixelFormat;
			canvasNum = pstRegion->unAttr.stOverlay.u32CanvasNum;
		} else {
			pixelFormat = pstRegion->unAttr.stOverlayEx.enPixelFormat;
			canvasNum = pstRegion->unAttr.stOverlayEx.u32CanvasNum;
		}

		if (canvasNum == 0 || canvasNum > RGN_MAX_BUF_NUM) {
			CVI_TRACE_RGN(RGN_ERR, "invalid u32CanvasNum(%d).\n", canvasNum);
			check_fail = CVI_TRUE;
		}

		if ((pixelFormat != PIXEL_FORMAT_ARGB_8888)
		 && (pixelFormat != PIXEL_FORMAT_ARGB_4444)
		 && (pixelFormat != PIXEL_FORMAT_ARGB_1555)
		 && (pixelFormat != PIXEL_FORMAT_8BIT_MODE)) {
			CVI_TRACE_RGN(RGN_ERR, "unsupported pxl-fmt(%d).\n", pixelFormat);
			check_fail = CVI_TRUE;
		}

		if (check_fail) {
			mutex_unlock(&g_rgnlock);
			return CVI_ERR_RGN_ILLEGAL_PARAM;
		}
	} else if (pstRegion->enType >= RGN_BUTT) {
		CVI_TRACE_RGN(RGN_ERR, "enType(%d) not supported yet.\n", pstRegion->enType);
		mutex_unlock(&g_rgnlock);
		return CVI_ERR_RGN_NOT_SUPPORT;
	}

	ctx = kzalloc(sizeof(*ctx), GFP_ATOMIC);
	if (ctx == CVI_NULL) {
		CVI_TRACE_RGN(RGN_ERR, "malloc failed.\n");
		ret = CVI_ERR_RGN_NOMEM;
		goto RGN_CTX_MALLOC_FAIL;
	}
	ctx->Handle = Handle;
	ctx->stRegion = *pstRegion;

	mutex_lock(&g_rgnhashlock);
	hash_add(rgn_hash, &ctx->node, Handle);
	mutex_unlock(&g_rgnhashlock);
	u32RgnNum++;

#if 0 //Linux Hashmap for print every element
	unsigned int bkt;
	struct cvi_rgn_ctx *cur;

	hash_for_each(rgn_hash, bkt, cur, node) {
		CVI_TRACE_RGN(RGN_INFO, "handle:%d, bCreated:%d, stRegion.enType:%d\n"
			, cur->Handle, cur->bCreated, cur->stRegion.enType);
	}
#endif

	// update rgn proc info
	for (proc_idx = 0; proc_idx < RGN_MAX_NUM; ++proc_idx) {
		if (!rgn_prc_ctx[proc_idx].bCreated) {
			rgn_prc_ctx[proc_idx].bCreated = true;
			rgn_prc_ctx[proc_idx].Handle = ctx->Handle;
			rgn_prc_ctx[proc_idx].stRegion = ctx->stRegion;
			break;
		}
	}

	mutex_unlock(&g_rgnlock);
	return CVI_SUCCESS;

RGN_CTX_MALLOC_FAIL:
	mutex_unlock(&g_rgnlock);
	return ret;

}

CVI_S32 rgn_destory(RGN_HANDLE Handle)
{
	struct cvi_rgn_ctx *ctx = NULL;
	CVI_U32 proc_idx;

	mutex_lock(&g_rgnlock);

	if (!_rgn_hash_find(Handle, &ctx, false)) {
		CVI_TRACE_RGN(RGN_ERR, "RGN_HANDLE(%d) is non-existent.\n", Handle);
		mutex_unlock(&g_rgnlock);
		return CVI_ERR_RGN_UNEXIST;
	}

	if (ctx->stChn.enModId != CVI_ID_BASE) {
		_rgn_update_hw(ctx, RGN_OP_REMOVE);
	}

	CVI_TRACE_RGN(RGN_INFO, "RGN_HANDLE(%d) canvas fmt(%d) size(%d * %d) stride(%d).\n"
	, Handle, ctx->stCanvasInfo[0].enPixelFormat, ctx->stCanvasInfo[0].stSize.u32Width
	, ctx->stCanvasInfo[0].stSize.u32Height, ctx->stCanvasInfo[0].u32Stride);

	//remove ctx in rgn_hash
	if (!_rgn_hash_find(Handle, &ctx, true)) {
		CVI_TRACE_RGN(RGN_ERR, "RGN_HANDLE(%d) remove failed.\n", Handle);
		mutex_unlock(&g_rgnlock);
		return CVI_ERR_RGN_UNEXIST;
	}

	// update rgn proc info
	proc_idx = _rgn_proc_get_idx(Handle);
	memset(&rgn_prc_ctx[proc_idx], 0, sizeof(rgn_prc_ctx[proc_idx]));
	u32RgnNum--;

	mutex_unlock(&g_rgnlock);

	kfree(ctx);

	return CVI_SUCCESS;
}

CVI_S32 rgn_get_attr(RGN_HANDLE Handle, RGN_ATTR_S *pstRegion)
{
	struct cvi_rgn_ctx *ctx = NULL;
	CVI_S32 s32Ret;

	s32Ret = CHECK_RGN_HANDLE(&ctx, Handle);
	if (s32Ret != CVI_SUCCESS)
		return s32Ret;

	*pstRegion = ctx->stRegion;

	return CVI_SUCCESS;
}

CVI_S32 rgn_set_attr(RGN_HANDLE Handle, const RGN_ATTR_S *pstRegion)
{
	struct cvi_rgn_ctx *ctx = NULL;
	CVI_U32 proc_idx;
	CVI_S32 s32Ret;

	s32Ret = CHECK_RGN_HANDLE(&ctx, Handle);
	if (s32Ret != CVI_SUCCESS)
		return s32Ret;

	proc_idx = _rgn_proc_get_idx(Handle);

	ctx->stRegion = rgn_prc_ctx[proc_idx].stRegion = *pstRegion;

	return CVI_SUCCESS;
}

CVI_S32 rgn_set_bit_map(RGN_HANDLE Handle, const BITMAP_S *pstBitmap)
{
	struct cvi_rgn_ctx *ctx = NULL;
	CVI_U32 bytesperline;
	RGN_CANVAS_INFO_S *pstCanvasInfo;
	CVI_U32 proc_idx;
	PIXEL_FORMAT_E pixelFormat;
	CVI_U32 canvasNum;
	SIZE_S rgnSize;
	CVI_U16 h;
	CVI_S32 s32Ret;
	struct _rgn_get_ow_addr_cb_param cb_param;
	CVI_S32 cnt = 0;

	s32Ret = CHECK_RGN_HANDLE(&ctx, Handle);
	if (s32Ret != CVI_SUCCESS)
		return s32Ret;

	proc_idx = _rgn_proc_get_idx(Handle);

	if (ctx->stCanvasInfo[0].bCompressed &&
		ctx->stCanvasInfo[0].enOSDCompressMode == OSD_COMPRESS_MODE_HW) {
		CVI_TRACE_RGN(RGN_ERR,
			"This function only supports OSD_COMPRESS_MODE_SW and OSD_COMPRESS_MODE_NONE.\n");
		return CVI_ERR_RGN_NOT_SUPPORT;
	}

	if (ctx->stRegion.enType != OVERLAY_RGN && ctx->stRegion.enType != OVERLAYEX_RGN) {
		CVI_TRACE_RGN(RGN_ERR, "Only Overlay/OverlayEx support. type(%d).\n", ctx->stRegion.enType);
		return CVI_ERR_RGN_NOT_SUPPORT;
	}

	if (ctx->canvas_get) {
		CVI_TRACE_RGN(RGN_ERR, "Need CVI_RGN_UpdateCanvas() first.\n");
		return CVI_ERR_RGN_NOT_PERM;
	}

	if (ctx->stChn.enModId == CVI_ID_BASE) {
		CVI_TRACE_RGN(RGN_ERR, "RGN_HANDLE(%d) not attached to any chn yet.\n", Handle);
		return CVI_ERR_RGN_NOT_CONFIG;
	}

	if (ctx->stRegion.enType == OVERLAY_RGN) {
		pixelFormat = ctx->stRegion.unAttr.stOverlay.enPixelFormat;
		canvasNum = ctx->stRegion.unAttr.stOverlay.u32CanvasNum;
		rgnSize = ctx->stRegion.unAttr.stOverlay.stSize;
	} else {
		pixelFormat = ctx->stRegion.unAttr.stOverlayEx.enPixelFormat;
		canvasNum = ctx->stRegion.unAttr.stOverlayEx.u32CanvasNum;
		rgnSize = ctx->stRegion.unAttr.stOverlayEx.stSize;
	}

	if ((pstBitmap->u32Width > rgnSize.u32Width)
	 || (pstBitmap->u32Height > rgnSize.u32Height)) {
		CVI_TRACE_RGN(RGN_ERR, "size of bitmap (%d * %d) > region (%d * %d).\n"
			, pstBitmap->u32Width, pstBitmap->u32Height
			, rgnSize.u32Width, rgnSize.u32Height);
		return CVI_ERR_RGN_ILLEGAL_PARAM;
	}

	if (pstBitmap->enPixelFormat != pixelFormat) {
		CVI_TRACE_RGN(RGN_ERR, "pxl-fmt of bitmap(%d) != region(%d).\n"
			, pstBitmap->enPixelFormat, pixelFormat);
		return CVI_ERR_RGN_ILLEGAL_PARAM;
	}

	CVI_TRACE_RGN(RGN_INFO, "bitmap size(%d * %d) fmt(%d), region size(%d * %d) fmt(%d).\n"
		, pstBitmap->u32Width, pstBitmap->u32Height, pstBitmap->enPixelFormat
		, rgnSize.u32Width, rgnSize.u32Height, pixelFormat);

	if (_rgn_get_bytesperline(pstBitmap->enPixelFormat, pstBitmap->u32Width, &bytesperline) != CVI_SUCCESS)
		return CVI_ERR_RGN_ILLEGAL_PARAM;

	// single/double buffer update per u32CanvasNum.
	if (canvasNum == 1)
		pstCanvasInfo = &ctx->stCanvasInfo[0];
	else {
		ctx->canvas_idx = 1 - ctx->canvas_idx;
		pstCanvasInfo = &ctx->stCanvasInfo[ctx->canvas_idx];
	}

	pstCanvasInfo->enPixelFormat = pstBitmap->enPixelFormat;
	pstCanvasInfo->stSize.u32Width = pstBitmap->u32Width;
	pstCanvasInfo->stSize.u32Height = pstBitmap->u32Height;
	pstCanvasInfo->u32Stride = ALIGN(bytesperline, 32);

	CVI_TRACE_RGN(RGN_INFO, "canvas fmt(%d) size(%d * %d) stride(%d).\n"
		, pstCanvasInfo->enPixelFormat, pstCanvasInfo->stSize.u32Width
		, pstCanvasInfo->stSize.u32Height, pstCanvasInfo->u32Stride);

	CVI_TRACE_RGN(RGN_INFO, "canvas v_addr(0x%lx), p_addr(0x%llx) size(%x)\n"
		, (uintptr_t)pstCanvasInfo->pu8VirtAddr, pstCanvasInfo->u64PhyAddr
		, pstCanvasInfo->u32Stride * pstCanvasInfo->stSize.u32Height);

	CVI_TRACE_RGN(RGN_INFO, "pstBitmap->pData(0x%lx)\n", (uintptr_t)pstBitmap->pData);

	if ((canvasNum > 1) && (ctx->stChn.enModId == CVI_ID_VPSS) && pstCanvasInfo->bCompressed) {
		cb_param.stChn = ctx->stChn;
		cb_param.handle = Handle;
		cb_param.layer = (ctx->stCanvasInfo[ctx->canvas_idx].bCompressed) ?
					RGN_ODEC_LAYER_VPSS : RGN_NORMAL_LAYER_VPSS;

		if (ctx->odec_data_valid) {
			do {
				if (_rgn_call_cb(E_MODULE_VPSS, VPSS_CB_GET_RGN_OW_ADDR, &cb_param) != 0) {
					CVI_TRACE_RGN(RGN_ERR, "VPSS_CB_GET_RGN_OW_ADDR is failed\n");
						return CVI_ERR_RGN_ILLEGAL_PARAM;
				}
				cnt++;
				usleep_range(1000, 2000);
			} while ((cb_param.addr == pstCanvasInfo->u64PhyAddr) && (cnt <= 500));

			CVI_TRACE_RGN(RGN_INFO, "VPSS_CB_GET_RGN_OW_ADDR PhyAaddr:%llx cnt:%d.\n",
				cb_param.addr, cnt);
			if (cb_param.addr == pstCanvasInfo->u64PhyAddr) {
				CVI_TRACE_RGN(RGN_WARN, "get a using canvas!\n");
				ctx->canvas_idx = 1 - ctx->canvas_idx;
				return CVI_ERR_RGN_BUSY;
			}
		}
	}

	if (pstCanvasInfo->bCompressed &&
		pstCanvasInfo->enOSDCompressMode == OSD_COMPRESS_MODE_SW) {
		if (copy_from_user(pstCanvasInfo->pu8VirtAddr, pstBitmap->pData, pstCanvasInfo->u32CompressedSize)) {
			CVI_TRACE_RGN(RGN_ERR, "Compressed pstBitmap->pData, copy_from_user failed.\n");
			return CVI_ERR_RGN_ILLEGAL_PARAM;
		}
		ctx->odec_data_valid = true;
	} else {
		for (h = 0; h < pstCanvasInfo->stSize.u32Height; ++h) {
			if (copy_from_user(pstCanvasInfo->pu8VirtAddr + pstCanvasInfo->u32Stride * h,
					   pstBitmap->pData + bytesperline * h, bytesperline)) {
				CVI_TRACE_RGN(RGN_ERR, "pstBitmap->pData, copy_from_user failed.\n");
				return CVI_ERR_RGN_ILLEGAL_PARAM;
			}
		}
	}

	// update rgn proc canvas info
	if (canvasNum == 1)
		rgn_prc_ctx[proc_idx].stCanvasInfo[0] = ctx->stCanvasInfo[0];
	else {
		rgn_prc_ctx[proc_idx].canvas_idx = ctx->canvas_idx;
		rgn_prc_ctx[proc_idx].stCanvasInfo[ctx->canvas_idx]
			= ctx->stCanvasInfo[ctx->canvas_idx];
	}

	// update hw.
	return _rgn_update_hw(ctx, RGN_OP_UPDATE);
}

CVI_S32 rgn_attach_to_chn(RGN_HANDLE Handle, const MMF_CHN_S *pstChn, const RGN_CHN_ATTR_S *pstChnAttr)
{
	struct cvi_rgn_ctx *ctx = NULL;
	CVI_U32 proc_idx;
	CVI_S32 s32Ret;
	CVI_U8 i;

	s32Ret = CHECK_RGN_HANDLE(&ctx, Handle);
	if (s32Ret != CVI_SUCCESS)
		return s32Ret;

	proc_idx = _rgn_proc_get_idx(Handle);

	if (ctx->stRegion.enType != pstChnAttr->enType) {
		CVI_TRACE_RGN(RGN_ERR, "type(%d) is different with chn type(%d).\n"
			, ctx->stRegion.enType, pstChnAttr->enType);
		return CVI_ERR_RGN_ILLEGAL_PARAM;
	}

	if (pstChn->enModId != CVI_ID_VPSS && pstChn->enModId != CVI_ID_VO) {
		CVI_TRACE_RGN(RGN_ERR, "rgn can only be attached to vpss or vo.\n");
		return CVI_ERR_RGN_NOT_SUPPORT;
	}

	if (ctx->stChn.enModId != CVI_ID_BASE) {
		CVI_TRACE_RGN(RGN_ERR, "RGN_HANDLE(%d) has been attached to CHN(%s-%d-%d).\n", Handle
			, sys_get_modname(ctx->stChn.enModId), ctx->stChn.s32DevId, ctx->stChn.s32ChnId);
		return CVI_ERR_RGN_NOT_CONFIG;
	}

	if (ctx->stRegion.enType == OVERLAYEX_RGN) {
		if (pstChn->enModId != CVI_ID_VPSS || (sys_get_vpssmode() != VPSS_MODE_RGNEX)) {
			CVI_TRACE_RGN(RGN_ERR, "rgn extension only support on vpss rgnex mode.\n");
			return CVI_ERR_RGN_NOT_SUPPORT;
		}
	}

	if (_rgn_check_chn_attr(Handle, pstChn, pstChnAttr) != CVI_SUCCESS)
		return CVI_ERR_RGN_ILLEGAL_PARAM;

	mutex_lock(&g_rgnlock);
	if (pstChnAttr->enType == COVER_RGN) {
		s32Ret = _rgn_update_cover_canvas(ctx, pstChnAttr);

		if (s32Ret != CVI_SUCCESS) {
			CVI_TRACE_RGN(RGN_ERR, "RGN_HANDLE(%d) fill cover failed.\n", Handle);
			mutex_unlock(&g_rgnlock);
			return s32Ret;
		}
	}
	ctx->stChnAttr = rgn_prc_ctx[proc_idx].stChnAttr = *pstChnAttr;
	ctx->stChn = rgn_prc_ctx[proc_idx].stChn = *pstChn;

	if (ctx->stRegion.enType == OVERLAY_RGN || ctx->stRegion.enType == OVERLAYEX_RGN) {
		PIXEL_FORMAT_E pixelFormat;
		CVI_U32 canvasNum, bgColor, compressed_size;
		SIZE_S rgnSize;
		CVI_U32 bytesperline = 0;

		if (ctx->stRegion.enType == OVERLAY_RGN) {
			pixelFormat = ctx->stRegion.unAttr.stOverlay.enPixelFormat;
			canvasNum = ctx->stRegion.unAttr.stOverlay.u32CanvasNum;
			rgnSize = ctx->stRegion.unAttr.stOverlay.stSize;
			bgColor = ctx->stRegion.unAttr.stOverlay.u32BgColor;
			switch (ctx->stRegion.unAttr.stOverlay.stCompressInfo.enOSDCompressMode) {
			case OSD_COMPRESS_MODE_SW:
				compressed_size = ctx->stRegion.unAttr.stOverlay.stCompressInfo.u32EstCompressedSize;
				break;
			case OSD_COMPRESS_MODE_HW:
				compressed_size = ctx->stRegion.unAttr.stOverlay.stCompressInfo.u32CompressedSize;
				break;
			default:
				compressed_size = 0;
				break;
			}
		} else {
			pixelFormat = ctx->stRegion.unAttr.stOverlayEx.enPixelFormat;
			canvasNum = ctx->stRegion.unAttr.stOverlayEx.u32CanvasNum;
			rgnSize = ctx->stRegion.unAttr.stOverlayEx.stSize;
			bgColor = ctx->stRegion.unAttr.stOverlayEx.u32BgColor;
			switch (ctx->stRegion.unAttr.stOverlayEx.stCompressInfo.enOSDCompressMode) {
			case OSD_COMPRESS_MODE_SW:
				compressed_size = ctx->stRegion.unAttr.stOverlayEx.stCompressInfo.u32EstCompressedSize;
				break;
			case OSD_COMPRESS_MODE_HW:
				compressed_size = ctx->stRegion.unAttr.stOverlayEx.stCompressInfo.u32CompressedSize;
				break;
			default:
				compressed_size = 0;
				break;
			}
		}

		ctx->canvas_idx = 0;

		s32Ret = _rgn_get_bytesperline(pixelFormat, rgnSize.u32Width, &bytesperline);
		if (s32Ret != CVI_SUCCESS) {
			goto RGN_FMT_INCORRECT;
		}

		ctx->stCanvasInfo[0].enPixelFormat = pixelFormat;
		ctx->stCanvasInfo[0].stSize = rgnSize;
		ctx->stCanvasInfo[0].u32Stride = ALIGN(bytesperline, 32);
		if (ctx->stRegion.unAttr.stOverlay.stCompressInfo.enOSDCompressMode == OSD_COMPRESS_MODE_NONE) {
			ctx->stCanvasInfo[0].bCompressed = false;
			ctx->stCanvasInfo[0].enOSDCompressMode = OSD_COMPRESS_MODE_NONE;
			ctx->stCanvasInfo[0].u32CompressedSize = 0;
			ctx->ion_len = ctx->u32MaxNeedIon =
				ctx->stCanvasInfo[0].u32Stride * ctx->stCanvasInfo[0].stSize.u32Height;
		} else {
			ctx->stCanvasInfo[0].bCompressed = true;
			ctx->stCanvasInfo[0].enOSDCompressMode =
				ctx->stRegion.unAttr.stOverlay.stCompressInfo.enOSDCompressMode;
			ctx->stCanvasInfo[0].u32CompressedSize = compressed_size;
			ctx->ion_len = ctx->u32MaxNeedIon = compressed_size;
		}

		CVI_TRACE_RGN(RGN_INFO, "RGN_HANDLE(%d) canvas fmt(%d) size(%d * %d) stride(%d).\n"
			, Handle, ctx->stCanvasInfo[0].enPixelFormat, ctx->stCanvasInfo[0].stSize.u32Width
			, ctx->stCanvasInfo[0].stSize.u32Height, ctx->stCanvasInfo[0].u32Stride);

		if (ctx->stCanvasInfo[0].bCompressed)
			CVI_TRACE_RGN(RGN_INFO, "RGN_HANDLE(%d) compressed canvas size(%d).\n"
				, Handle, ctx->stCanvasInfo[0].u32CompressedSize);

		for (i = 1; i < canvasNum; ++i)
			ctx->stCanvasInfo[i] = ctx->stCanvasInfo[0];

		for (i = 0; i < canvasNum; ++i) {
			if (sys_ion_alloc(&ctx->stCanvasInfo[i].u64PhyAddr,
							(void **)&ctx->stCanvasInfo[i].pu8VirtAddr,
							"rgn_canvas", ctx->ion_len, false) != CVI_SUCCESS) {
				CVI_TRACE_RGN(RGN_ERR, "Region(%d) Can't acquire ion for Canvas-%d.\n", Handle, i);
				s32Ret = CVI_ERR_RGN_NOBUF;
				goto RGN_ION_ALLOC_FAIL;
			}
			_rgn_fill_pattern(ctx->stCanvasInfo[i].pu8VirtAddr, ctx->ion_len, bgColor
				, (pixelFormat == PIXEL_FORMAT_ARGB_8888) ? 4 :
				((pixelFormat == PIXEL_FORMAT_8BIT_MODE) ? 1 : 2));
		}

		 // update rgn proc canvas info
		if (ctx->stRegion.enType == OVERLAY_RGN) {
			rgn_prc_ctx[proc_idx].canvas_idx = ctx->canvas_idx;
			for (i = 0; i < ctx->stRegion.unAttr.stOverlay.u32CanvasNum; ++i)
				rgn_prc_ctx[proc_idx].stCanvasInfo[i] = ctx->stCanvasInfo[i];
		} else if (ctx->stRegion.enType == OVERLAYEX_RGN) {
			rgn_prc_ctx[proc_idx].canvas_idx = ctx->canvas_idx;
			for (i = 0; i < ctx->stRegion.unAttr.stOverlayEx.u32CanvasNum; ++i)
				rgn_prc_ctx[proc_idx].stCanvasInfo[i] = ctx->stCanvasInfo[i];
		}
		rgn_prc_ctx[proc_idx].u32MaxNeedIon = ctx->u32MaxNeedIon;
	}

	s32Ret = _rgn_update_hw(ctx, RGN_OP_INSERT);
	if (s32Ret == CVI_SUCCESS) {
		// only update rgn_prc_ctx after _rgn_update_hw success
		rgn_prc_ctx[proc_idx].bUsed = true;
	} else {
		goto RGN_UPDATE_HW_FAIL;
	}
	mutex_unlock(&g_rgnlock);

	return s32Ret;
RGN_UPDATE_HW_FAIL:
RGN_ION_ALLOC_FAIL:
	if (ctx->stRegion.enType == OVERLAY_RGN) {
		for (i = 0; i < ctx->stRegion.unAttr.stOverlay.u32CanvasNum; ++i)
			if (ctx->stCanvasInfo[i].u64PhyAddr)
				sys_ion_free(ctx->stCanvasInfo[i].u64PhyAddr);
	} else if (ctx->stRegion.enType == OVERLAYEX_RGN) {
		for (i = 0; i < ctx->stRegion.unAttr.stOverlayEx.u32CanvasNum; ++i)
			if (ctx->stCanvasInfo[i].u64PhyAddr)
				sys_ion_free(ctx->stCanvasInfo[i].u64PhyAddr);
	}
RGN_FMT_INCORRECT:
	mutex_unlock(&g_rgnlock);
	return s32Ret;
}

CVI_S32 rgn_detach_from_chn(RGN_HANDLE Handle, const MMF_CHN_S *pstChn)
{
	struct cvi_rgn_ctx *ctx = NULL;
	CVI_S32 ret = -EINVAL;
	CVI_U32 proc_idx;
	CVI_S32 s32Ret;
	CVI_U8 i;

	s32Ret = CHECK_RGN_HANDLE(&ctx, Handle);
	if (s32Ret != CVI_SUCCESS)
		return s32Ret;

	proc_idx = _rgn_proc_get_idx(Handle);

	if (ctx->stChn.enModId == CVI_ID_BASE) {
		CVI_TRACE_RGN(RGN_ERR, "RGN_HANDLE(%d) not attached to any chn yet.\n", Handle);
		return CVI_ERR_RGN_NOT_CONFIG;
	}

	mutex_lock(&g_rgnlock);
	ret = _rgn_update_hw(ctx, RGN_OP_REMOVE);
	if (ret == CVI_SUCCESS) {
		ctx->stChn.enModId = rgn_prc_ctx[proc_idx].stChn.enModId = CVI_ID_BASE;
		rgn_prc_ctx[proc_idx].bUsed = false;
	}
	ctx->odec_data_valid = false;

	if (ctx->stChnAttr.enType == OVERLAY_RGN) {
		for (i = 0; i < ctx->stRegion.unAttr.stOverlay.u32CanvasNum; ++i)
			if (ctx->stCanvasInfo[i].u64PhyAddr)
				sys_ion_free(ctx->stCanvasInfo[i].u64PhyAddr);
	} else if (ctx->stChnAttr.enType == OVERLAYEX_RGN) {
		for (i = 0; i < ctx->stRegion.unAttr.stOverlayEx.u32CanvasNum; ++i)
			if (ctx->stCanvasInfo[i].u64PhyAddr)
				sys_ion_free(ctx->stCanvasInfo[i].u64PhyAddr);
	} else {
		if (ctx->stCanvasInfo[0].u64PhyAddr)
			sys_ion_free(ctx->stCanvasInfo[0].u64PhyAddr);
	}

	mutex_unlock(&g_rgnlock);

	return ret;
}

CVI_S32 rgn_set_display_attr(RGN_HANDLE Handle, const MMF_CHN_S *pstChn, const RGN_CHN_ATTR_S *pstChnAttr)
{
	struct cvi_rgn_ctx *ctx = NULL;
	CVI_U32 proc_idx;
	RGN_CHN_ATTR_S stChnAttr;
	CVI_S32 ret = -EINVAL;

	ret = CHECK_RGN_HANDLE(&ctx, Handle);
	if (ret != CVI_SUCCESS)
		return ret;

	proc_idx = _rgn_proc_get_idx(Handle);

	if (ctx->stChn.enModId == CVI_ID_BASE
		|| (ctx->stChn.enModId != pstChn->enModId
		|| ctx->stChn.s32DevId != pstChn->s32DevId
		|| ctx->stChn.s32ChnId != pstChn->s32ChnId)) {
		CVI_TRACE_RGN(RGN_ERR, "RGN_HANDLE(%d) is not attached on ModId(%d) s32DevId(%d) s32ChnId(%d)\n",
			Handle, pstChn->enModId, pstChn->s32DevId, pstChn->s32ChnId);
		return CVI_ERR_RGN_NOT_CONFIG;
	}
	if (ctx->stChnAttr.enType != pstChnAttr->enType) {
		CVI_TRACE_RGN(RGN_ERR, "RGN_HANDLE(%d) enType not allowed to changed.\n", Handle);
		return CVI_ERR_RGN_NOT_PERM;
	}

	if (_rgn_check_chn_attr(Handle, pstChn, pstChnAttr) != CVI_SUCCESS)
		return CVI_ERR_RGN_ILLEGAL_PARAM;

	if (ctx->stChnAttr.enType == COVER_RGN) {
		ret = _rgn_update_cover_canvas(ctx, pstChnAttr);

		if (ret != CVI_SUCCESS) {
			CVI_TRACE_RGN(RGN_ERR, "RGN_HANDLE(%d) fill cover failed.\n", Handle);
			return ret;
		}
	}

	memcpy(&stChnAttr, &ctx->stChnAttr, sizeof(RGN_CHN_ATTR_S));
	ctx->stChnAttr = rgn_prc_ctx[proc_idx].stChnAttr = *pstChnAttr;

	ret = _rgn_update_hw(ctx, RGN_OP_UPDATE);
	if (ret != CVI_SUCCESS) {
		ctx->stChnAttr = rgn_prc_ctx[proc_idx].stChnAttr = stChnAttr;
	}

	return ret;
}

CVI_S32 rgn_get_display_attr(RGN_HANDLE Handle, const MMF_CHN_S *pstChn, RGN_CHN_ATTR_S *pstChnAttr)
{
	struct cvi_rgn_ctx *ctx = NULL;
	CVI_S32 s32Ret;

	s32Ret = CHECK_RGN_HANDLE(&ctx, Handle);
	if (s32Ret != CVI_SUCCESS)
		return s32Ret;

	*pstChnAttr = ctx->stChnAttr;

	return CVI_SUCCESS;
}

CVI_S32 rgn_get_canvas_info(RGN_HANDLE Handle, RGN_CANVAS_INFO_S *pstCanvasInfo)
{
	struct cvi_rgn_ctx *ctx = NULL;
	CVI_U32 proc_idx;
	CVI_U32 canvasNum;
	CVI_S32 s32Ret;
	struct _rgn_get_ow_addr_cb_param cb_param;
	CVI_S32 cnt = 0;

	s32Ret = CHECK_RGN_HANDLE(&ctx, Handle);
	if (s32Ret != CVI_SUCCESS)
		return s32Ret;

	proc_idx = _rgn_proc_get_idx(Handle);

	if (ctx->stRegion.enType != OVERLAY_RGN && ctx->stRegion.enType != OVERLAYEX_RGN) {
		CVI_TRACE_RGN(RGN_ERR, "Only Overlay/OverlayEx support. type(%d).\n", ctx->stRegion.enType);
		return CVI_ERR_RGN_NOT_SUPPORT;
	}
	if (ctx->canvas_get) {
		CVI_TRACE_RGN(RGN_ERR, "Need CVI_RGN_UpdateCanvas() first.\n");
		return CVI_ERR_RGN_NOT_PERM;
	}

	if (ctx->stRegion.enType == OVERLAY_RGN)
		canvasNum = ctx->stRegion.unAttr.stOverlay.u32CanvasNum;
	else
		canvasNum = ctx->stRegion.unAttr.stOverlayEx.u32CanvasNum;

	if (canvasNum == 1)
		*pstCanvasInfo = ctx->stCanvasInfo[0];
	else {
		ctx->canvas_idx = rgn_prc_ctx[proc_idx].canvas_idx = 1 - ctx->canvas_idx;
		*pstCanvasInfo = ctx->stCanvasInfo[ctx->canvas_idx];
	}
	CVI_TRACE_RGN(RGN_INFO, "RGN_HANDLE(%d) canvas fmt(%d) size(%d * %d) stride(%d) compressed(%d).\n"
		, Handle, pstCanvasInfo->enPixelFormat, pstCanvasInfo->stSize.u32Width
		, pstCanvasInfo->stSize.u32Height, pstCanvasInfo->u32Stride, pstCanvasInfo->bCompressed);

	if ((canvasNum > 1) && (ctx->stChn.enModId == CVI_ID_VPSS) && pstCanvasInfo->bCompressed) {
		CVI_TRACE_RGN(RGN_INFO, "canvas p_addr(%llx) v_addr(%lx).\n"
			, pstCanvasInfo->u64PhyAddr, (uintptr_t)pstCanvasInfo->pu8VirtAddr);

		cb_param.stChn = ctx->stChn;
		cb_param.handle = Handle;
		cb_param.layer = (ctx->stCanvasInfo[ctx->canvas_idx].bCompressed) ?
					RGN_ODEC_LAYER_VPSS : RGN_NORMAL_LAYER_VPSS;

		if (ctx->odec_data_valid) {
			do {
				if (_rgn_call_cb(E_MODULE_VPSS, VPSS_CB_GET_RGN_OW_ADDR, &cb_param) != 0) {
					CVI_TRACE_RGN(RGN_ERR, "VPSS_CB_GET_RGN_OW_ADDR is failed\n");
						return CVI_ERR_RGN_ILLEGAL_PARAM;
				}
				cnt++;
				usleep_range(1000, 2000);
			} while ((cb_param.addr == pstCanvasInfo->u64PhyAddr) && (cnt <= 500));

			CVI_TRACE_RGN(RGN_WARN, "VPSS_CB_GET_RGN_OW_ADDR PhyAaddr:%llx cnt:%d.\n",
				cb_param.addr, cnt);
			CVI_TRACE_RGN(RGN_INFO, "ow addr(%llx).\n", cb_param.addr);
			if (cb_param.addr == pstCanvasInfo->u64PhyAddr) {
				CVI_TRACE_RGN(RGN_WARN, "get a using canvas!\n");
				ctx->canvas_idx = rgn_prc_ctx[proc_idx].canvas_idx = 1 - ctx->canvas_idx;
				return CVI_ERR_RGN_BUSY;
			}
		}
	}

	ctx->canvas_get = CVI_TRUE;

	return CVI_SUCCESS;
}

CVI_S32 rgn_update_canvas(RGN_HANDLE Handle)
{
	struct cvi_rgn_ctx *ctx = NULL;
	CVI_S32 ret = -EINVAL;
	CVI_S32 s32Ret, i;
	CVI_U32 proc_idx;

	s32Ret = CHECK_RGN_HANDLE(&ctx, Handle);
	if (s32Ret != CVI_SUCCESS)
		return s32Ret;

	proc_idx = _rgn_proc_get_idx(Handle);

	if (ctx->stRegion.enType != OVERLAY_RGN && ctx->stRegion.enType != OVERLAYEX_RGN) {
		CVI_TRACE_RGN(RGN_ERR, "Only Overlay/OverlayEx support. type(%d).\n", ctx->stRegion.enType);
		return CVI_ERR_RGN_NOT_SUPPORT;
	}
	if (!ctx->canvas_get) {
		CVI_TRACE_RGN(RGN_ERR, "CVI_RGN_GetCanvasInfo() first.\n");
		return CVI_ERR_RGN_NOT_SUPPORT;
	}

	if (ctx->stCanvasInfo[0].bCompressed) {
		if (ctx->stCanvasInfo[0].enOSDCompressMode == OSD_COMPRESS_MODE_HW) {
			RGN_CANVAS_INFO_S *pstCanvasInfo = NULL;
			RGN_CANVAS_CMPR_ATTR_S *pstCanvasCmprAttr = NULL, stCanvasCmprAttrTmp;
			RGN_CMPR_OBJ_ATTR_S *pstObjAttr = NULL;
			struct cmdqu_rgn_cb_param rgn_cb_param;

			if (ctx->stCanvasInfo[ctx->canvas_idx].pu8VirtAddr == NULL) {
				CVI_TRACE_RGN(RGN_ERR, "ctx->stCanvasInfo[ctx->canvas_idx].pu8VirtAddr NULL!\n");
					return CVI_ERR_RGN_SYS_NOTREADY;
			}
			pstCanvasInfo = &ctx->stCanvasInfo[ctx->canvas_idx];
			pstCanvasCmprAttr = (RGN_CANVAS_CMPR_ATTR_S *)pstCanvasInfo->pu8VirtAddr;
			pstObjAttr = (RGN_CMPR_OBJ_ATTR_S *)(pstCanvasInfo->pu8VirtAddr +
				sizeof(RGN_CANVAS_CMPR_ATTR_S));
			memcpy(&stCanvasCmprAttrTmp, pstCanvasCmprAttr, sizeof(stCanvasCmprAttrTmp));
			#if 0
			sys_cache_invalidate(pstCanvasInfo->u64PhyAddr, pstCanvasInfo->pu8VirtAddr,
				pstCanvasInfo->u32Stride * pstCanvasInfo->stSize.u32Height);
			#endif

			CVI_TRACE_RGN(RGN_DEBUG, "PAddr(%llx) Width(%d) Height(%d) bsSize(%d)\n",
				pstCanvasInfo->u64PhyAddr, pstCanvasCmprAttr->u32Width,
				pstCanvasCmprAttr->u32Height, pstCanvasCmprAttr->u32BsSize);
			CVI_TRACE_RGN(RGN_DEBUG, "BgColor(0x%x) Pixelformat(%d)\n",
				pstCanvasCmprAttr->u32BgColor, pstCanvasCmprAttr->enPixelFormat);
			for (i = 0; i < pstCanvasCmprAttr->u32ObjNum; ++i) {
				if (pstObjAttr[i].enObjType == RGN_CMPR_LINE) {
					CVI_TRACE_RGN(RGN_DEBUG, "start(%d %d) end(%d %d) Thick(%d) Color(0x%x)\n",
						pstObjAttr[i].stLine.stPointStart.s32X,
						pstObjAttr[i].stLine.stPointStart.s32Y,
						pstObjAttr[i].stLine.stPointEnd.s32X,
						pstObjAttr[i].stLine.stPointEnd.s32Y,
						pstObjAttr[i].stLine.u32Thick,
						pstObjAttr[i].stLine.u32Color);
				} else if (pstObjAttr[i].enObjType == RGN_CMPR_RECT) {
					CVI_TRACE_RGN(RGN_DEBUG,
						"xywh(%d %d %d %d) Thick(%d) Color(0x%x) is_fill(%d)\n",
						pstObjAttr[i].stRgnRect.stRect.s32X,
						pstObjAttr[i].stRgnRect.stRect.s32Y,
						pstObjAttr[i].stRgnRect.stRect.u32Width,
						pstObjAttr[i].stRgnRect.stRect.u32Height,
						pstObjAttr[i].stRgnRect.u32Thick,
						pstObjAttr[i].stRgnRect.u32Color,
						pstObjAttr[i].stRgnRect.u32IsFill);
				} else if (pstObjAttr[i].enObjType == RGN_CMPR_BIT_MAP) {
					CVI_TRACE_RGN(RGN_DEBUG, "xywh(%d %d %d %d) u32BitmapPAddr(%x)\n",
						pstObjAttr[i].stBitmap.stRect.s32X,
						pstObjAttr[i].stBitmap.stRect.s32Y,
						pstObjAttr[i].stBitmap.stRect.u32Width,
						pstObjAttr[i].stBitmap.stRect.u32Height,
						pstObjAttr[i].stBitmap.u32BitmapPAddr);
				}
			}

			#if 0
			sys_cache_flush(pstCanvasInfo->u64PhyAddr, pstCanvasInfo->pu8VirtAddr,
				pstCanvasInfo->u32Stride * pstCanvasInfo->stSize.u32Height);
			#endif
			rgn_cb_param.u8CmdId = CMDQU_CB_RGN_COMPRESS;
			rgn_cb_param.u32RgnCmprParamPAddr = (CVI_U32)pstCanvasInfo->u64PhyAddr;

			if (pstCanvasCmprAttr->u32BsSize != ctx->ion_len)
				pstCanvasCmprAttr->u32BsSize = ctx->ion_len;
			ret = _rgn_call_cb(E_MODULE_RTOS_CMDQU, CMDQU_CB_RGN_COMPRESS, (void *)&rgn_cb_param);

			if (ret) {
				CVI_TRACE_RGN(RGN_ERR, "Region(%d) OSDC failed!.\n", Handle);
				return CVI_ERR_RGN_SYS_NOTREADY;
			}

			// if C906L osdc status error,
			// status and real bs_size will be returned in pstCanvasInfo->pu8VirtAddr
			if (*(CVI_U32 *)pstCanvasInfo->pu8VirtAddr == 0xFFFFFFFF) {
				CVI_TRACE_RGN(RGN_ERR, "Region(%d) needs ion size(%d), current size(%d).\n",
					Handle, *((CVI_U32 *)pstCanvasInfo->pu8VirtAddr + 1), ctx->ion_len);
				ctx->u32MaxNeedIon = rgn_prc_ctx[proc_idx].u32MaxNeedIon =
					*((CVI_U32 *)pstCanvasInfo->pu8VirtAddr + 1);
				stCanvasCmprAttrTmp.u32ObjNum = 0;
				//pstCanvasCmprAttr is modified in C906L
				memcpy(pstCanvasCmprAttr, &stCanvasCmprAttrTmp, sizeof(stCanvasCmprAttrTmp));
				_rgn_call_cb(E_MODULE_RTOS_CMDQU, CMDQU_CB_RGN_COMPRESS, (void *)&rgn_cb_param);
			} else {
				// first 8 bytes restores compress data header, original header:
				// bit[0:7] version
				// bit[8:11] osd_format
				// bit[12:14] reserved
				// bit[15:22] palette_cache_size
				// bit[23:24] alpha truncate
				// bit[25:26] reserved
				// bit[27:28] rgb truncate
				// bit[29:30] reserved
				// bit[31:46] image_width minus 1
				// bit[47:62] image_height minus 1

				// bitstream size is saved in bit[32:63], after bitstream size is get,
				// restore it to image width and height
				// bit[0:7] version
				// bit[8:11] osd_format
				// bit[12:14] reserved
				// bit[15:22] palette_cache_size
				// bit[23:24] alpha truncate
				// bit[25:26] reserved
				// bit[27:28] rgb truncate
				// bit[29:30] reserved
				// bit[32:63] bitstream size
				ctx->stCanvasInfo[ctx->canvas_idx].u32CompressedSize =
					*((CVI_U32 *)pstCanvasInfo->pu8VirtAddr + 1);
				*((CVI_U32 *)pstCanvasInfo->pu8VirtAddr) |= ((stCanvasCmprAttrTmp.u32Width - 1) & 0x01) << 31;
				*((CVI_U32 *)pstCanvasInfo->pu8VirtAddr + 1) =
					(((stCanvasCmprAttrTmp.u32Width - 1) >> 1) & 0x7FFF) |
					(((stCanvasCmprAttrTmp.u32Height - 1) << 15) & 0x7FFF8000);
			}
		}
		ctx->odec_data_valid = true;
	}

	ret = _rgn_update_hw(ctx, RGN_OP_UPDATE);
	ctx->canvas_get = CVI_FALSE;
	return ret;
}

/* CVI_RGN_Invert_Color - invert color per luma statistics of video content
 *   Chns' pixel-format should be YUV.
 *   RGN's pixel-format should be ARGB1555.
 *
 * @param Handle: RGN Handle
 * @param pstChn: the chn which rgn attached
 * @param pu32Color: rgn's content
 */
CVI_S32 rgn_invert_color(RGN_HANDLE Handle, MMF_CHN_S *pstChn, CVI_U32 *pu32Color)
{
	CVI_S32 ret = -EINVAL;
	struct cvi_rgn_ctx *ctx = NULL;
	RGN_CANVAS_INFO_S stCanvasInfo;
	CVI_S32 s32StrLen;
	CVI_U32 u32LumaThresh;
	CVI_U32 proc_idx;
	CVI_U32 canvasNum;
	OVERLAY_INVERT_COLOR_S invertColor;
	POINT_S point;
	CVI_S32 s32Ret;

	s32Ret = CHECK_RGN_HANDLE(&ctx, Handle);
	if (s32Ret != CVI_SUCCESS)
		return s32Ret;

	proc_idx = _rgn_proc_get_idx(Handle);

	if ((pstChn->enModId != ctx->stChn.enModId) || (pstChn->s32DevId != ctx->stChn.s32DevId) ||
		(pstChn->s32ChnId != ctx->stChn.s32ChnId)) {
		CVI_TRACE_RGN(RGN_ERR, "RGN_HANDLE(%d) not attached to chn(%d-%d-%d) yet.\n",
			Handle,  pstChn->enModId, pstChn->s32DevId, pstChn->s32ChnId);
		return CVI_ERR_RGN_ILLEGAL_PARAM;
	}

	if (ctx->stRegion.enType != OVERLAY_RGN && ctx->stRegion.enType != OVERLAYEX_RGN) {
		CVI_TRACE_RGN(RGN_ERR, "CVI_RGN_Invert_Color only support Overlay/OverlayEx (%d).\n"
			, ctx->stRegion.enType);
		return CVI_ERR_RGN_NOT_SUPPORT;
	}

	if (pstChn->enModId != CVI_ID_VPSS) {
		CVI_TRACE_RGN(RGN_ERR, "CVI_RGN_Invert_Color GRN did not attach to vpss.\n");
		return CVI_ERR_RGN_NOT_SUPPORT;
	}

	if (ctx->stRegion.enType == OVERLAY_RGN) {
		canvasNum = ctx->stRegion.unAttr.stOverlay.u32CanvasNum;
		invertColor = ctx->stChnAttr.unChnAttr.stOverlayChn.stInvertColor;
		point = ctx->stChnAttr.unChnAttr.stOverlayChn.stPoint;
	} else {
		canvasNum = ctx->stRegion.unAttr.stOverlayEx.u32CanvasNum;
		invertColor = ctx->stChnAttr.unChnAttr.stOverlayExChn.stInvertColor;
		point = ctx->stChnAttr.unChnAttr.stOverlayExChn.stPoint;
	}

	if (invertColor.bInvColEn != CVI_TRUE) {
		CVI_TRACE_RGN(RGN_ERR, "CVI_RGN_Invert_Color bInvColEn hasn't been set\n");
		return CVI_ERR_RGN_SYS_NOTREADY;
	}

#if 0 //Need to try to use GOP HW invert function in 2x first.
	VIDEO_FRAME_INFO_S stVideoFrame;
	CVI_U8 *pstVirAddr;
	CVI_U32 u32MainStride;
	CVI_S32 s32StartX, s32StartY;
	SIZE_S Font;
	CVI_U32 u32Sum, u32Num;
	CVI_U32 u32AverLuma;
	//CVI_FLOAT proportion_x, proportion_y;
	CVI_U32 scale, proportion_x, proportion_y;
	CVI_S32 i;
	CVI_U32 x, y;
	VPSS_CROP_INFO_S pstGrpCropInfo;
	VPSS_CROP_INFO_S pstChnCropInfo;
	size_t Luma_size;
	VPSS_CHN_ATTR_S stVpssChnAttr;

	ret = CVI_VPSS_GetChnAttr(0, 0, &stVpssChnAttr);

	if (stVpssChnAttr.bFlip != CVI_FALSE || stVpssChnAttr.bMirror != CVI_FALSE) {
		if (stVpssChnAttr.stAspectRatio.enMode != ASPECT_RATIO_AUTO) {
		CVI_TRACE_RGN(RGN_ERR, "CVI_RGN_Invert_Color only support keep aspect ratio\n");
		return ret;
	}

	ret = CVI_VPSS_GetGrpCrop(pstChn->s32DevId, &pstGrpCropInfo);
	if (pstGrpCropInfo.stCropRect.s32X != 0 || pstGrpCropInfo.stCropRect.s32Y != 0
		|| pstGrpCropInfo.stCropRect.u32Width != 0 || pstGrpCropInfo.stCropRect.u32Height != 0) {
		CVI_TRACE_RGN(RGN_ERR, "CVI_RGN_Invert_Color not support vpss group crop yet\n");
		return ret;
	}

	ret = CVI_VPSS_GetChnCrop(pstChn->s32DevId, pstChn->s32ChnId, &pstChnCropInfo);
	if (pstChnCropInfo.stCropRect.s32X != 0 || pstChnCropInfo.stCropRect.s32Y != 0
		|| pstChnCropInfo.stCropRect.u32Width != 0 || pstChnCropInfo.stCropRect.u32Height != 0) {
		CVI_TRACE_RGN(RGN_ERR, "CVI_RGN_Invert_Color not support vpss chn crop yet\n");
		return ret;
	}

	ret = CVI_VI_GetChnFrame(0, 0, &stVideoFrame, 1000);
	if (MOD_CHECK_NULL_PTR(CVI_ID_RGN, &stVideoFrame)) {
		CVI_TRACE_RGN(RGN_ERR, "CVI_RGN_Invert_Color get vi chn frame failed with %#x\n", ret);
		return CVI_ERR_RGN_SYS_NOTREADY;
	}

	if (!IS_FMT_YUV(stVideoFrame.stVFrame.enPixelFormat)) {
		ret = CVI_VI_ReleaseChnFrame(0, 0, &stVideoFrame);

		if (ret != CVI_SUCCESS) {
			CVI_TRACE_RGN(RGN_ERR, "CVI_RGN_Invert_Color release vi chn frame failed with %#x\n", ret);
			return CVI_ERR_RGN_SYS_NOTREADY;
		}
		CVI_TRACE_RGN(RGN_ERR, "CVI_RGN_Invert_Color invert color only support yuv-fmt(%d).\n"
			, stVideoFrame.stVFrame.enPixelFormat);
		return CVI_ERR_RGN_NOT_SUPPORT;
	}

	Luma_size = stVideoFrame.stVFrame.u32Length[0];
	pstVirAddr = stVideoFrame.stVFrame.pu8VirAddr[0];
	//pstVirAddr = CVI_SYS_Mmap(stVideoFrame.stVFrame.u64PhyAddr[0], Luma_size);
	u32MainStride = stVideoFrame.stVFrame.u32Stride[0];
#endif

	if (canvasNum == 1)
		stCanvasInfo = ctx->stCanvasInfo[0];
	else {
		ctx->canvas_idx = rgn_prc_ctx[proc_idx].canvas_idx = 1 - ctx->canvas_idx;
		stCanvasInfo = ctx->stCanvasInfo[ctx->canvas_idx];
	}
	s32StrLen = stCanvasInfo.stSize.u32Width / invertColor.stInvColArea.u32Width;

	u32LumaThresh = invertColor.u32LumThresh;

#if 0 //Need to try to use GOP HW invert function in 2x first.
	//Original code in cvi_region.c.
	//proportion_x = (CVI_FLOAT)stVideoFrame.stVFrame.u32Width / stVpssChnAttr.u32Width;
	//proportion_y = (CVI_FLOAT)stVideoFrame.stVFrame.u32Height / stVpssChnAttr.u32Height;
	//=> After sinking into kernel to remove CVI_FLOAT usage
	//scale = stVpssChnAttr.u32Width * stVpssChnAttr.u32Height;
	//proportion_x = (stVideoFrame.stVFrame.u32Width * stVpssChnAttr.u32Height) / scale;
	//proportion_y = (stVideoFrame.stVFrame.u32Height * stVpssChnAttr.u32Width) / scale;
	//=> After sinking into kernel to remove CVI_FLOAT usage and callback function.
	scale = arg.stVpssChnAttr.u32Width * arg.stVpssChnAttr.u32Height;
	proportion_x = (stVideoFrame.stVFrame.u32Width * arg.stVpssChnAttr.u32Height) / scale;
	proportion_y = (stVideoFrame.stVFrame.u32Height * arg.stVpssChnAttr.u32Width) / scale;

	s32StartX = (CVI_S32)(point.s32X * proportion_x);
	s32StartY = (CVI_S32)(point.s32Y * proportion_y);

	Font.u32Width = (CVI_U32)(invertColor.stInvColArea.u32Width * proportion_x);
	Font.u32Height = (CVI_U32)(invertColor.stInvColArea.u32Height * proportion_y);

	for (i = 0; i < s32StrLen; i++) {
		u32Num = 0;
		u32Sum = 0;
		//get average Luma
		for (y = s32StartY; y < s32StartY + Font.u32Height; y++) {
			for (x = s32StartX + Font.u32Width * i; x < (s32StartX + Font.u32Width * (i + 1));
				x++) {
				u32Sum += *(pstVirAddr + x + y * u32MainStride);
				u32Num++;
			}
			y++;
		}
		u32AverLuma = u32Sum / u32Num;

		//invert color
		if (invertColor.enChgMod == MORETHAN_LUM_THRESH) {
			if (u32AverLuma > u32LumaThresh)
				pu32Color[i] = RGN_COLOR_DARK;
			else
				pu32Color[i] = RGN_COLOR_BRIGHT;
		} else {
			if (u32AverLuma > u32LumaThresh)
				pu32Color[i] = RGN_COLOR_BRIGHT;
			else
				pu32Color[i] = RGN_COLOR_DARK;
		}
	}

	CVI_SYS_Munmap(pstVirAddr, Luma_size);

	ret = CVI_VI_ReleaseChnFrame(0, 0, &stVideoFrame);
	if (ret != CVI_SUCCESS) {
		CVI_TRACE_RGN(RGN_ERR, "CVI_RGN_Invert_Color release vi chn frame failed with %#x\n", ret);
		return CVI_ERR_RGN_SYS_NOTREADY;
	}
#endif

	return ret;
}

RGN_COMP_INFO_S s_ConvertInfo[RGN_COLOR_FMT_BUTT] = {
		{ 0, 4, 4, 4 }, /*RGB444*/
		{ 4, 4, 4, 4 }, /*ARGB4444*/
		{ 0, 5, 5, 5 }, /*RGB555*/
		{ 0, 5, 6, 5 }, /*RGB565*/
		{ 1, 5, 5, 5 }, /*ARGB1555*/
		{ 0, 0, 0, 0 }, /*RESERVED*/
		{ 0, 8, 8, 8 }, /*RGB888*/
		{ 8, 8, 8, 8 }, /*ARGB8888*/
		{ 4, 4, 4, 4 }, /*ARGB4444*/
		{ 1, 5, 5, 5 }, /*ARGB1555*/
		{ 8, 8, 8, 8 }  /*ARGB8888*/
};

CVI_U16 RGN_MAKECOLOR_U16_A(CVI_U8 a, CVI_U8 r, CVI_U8 g, CVI_U8 b, RGN_COMP_INFO_S input_fmt)
{
	CVI_U8 a1, r1, g1, b1;
	CVI_U16 pixel;

	pixel = a1 = r1 = g1 = b1 = 0;
	a1 = ((input_fmt.alen - 4) > 0) ? a >> (input_fmt.alen - 4) :
		(input_fmt.alen ? a << (4 - input_fmt.alen) : a);
	r1 = ((input_fmt.rlen - 4) > 0) ? r >> (input_fmt.rlen - 4) : r;
	g1 = ((input_fmt.glen - 4) > 0) ? g >> (input_fmt.glen - 4) : g;
	b1 = ((input_fmt.blen - 4) > 0) ? b >> (input_fmt.blen - 4) : b;

	pixel = (b1 | (g1 << 4) | (r1 << 8 | (a1 << 12)));

	return pixel;
}

CVI_S32 rgn_set_chn_palette(RGN_HANDLE Handle, const MMF_CHN_S *pstChn, RGN_PALETTE_S *pstPalette,
			RGN_RGBQUARD_S *pstInputPixelTable)
{
	struct cvi_rgn_ctx *ctx;
	struct cvi_rgn_lut_cfg lut_cfg;
	CVI_S32 ret = CVI_SUCCESS;
	CVI_U32 idx, u32Pixel, osd_layer;
	CVI_U16 u16Pixel;
	CVI_U8 r, g, b, a;
	// RGN_RGBQUARD_S *pstInputPixelTable;
	CVI_U16 *pstOutputPixelTable;
	struct _rgn_lut_cb_param rgn_lut_arg;
	CVI_S32 s32Ret;

	s32Ret = CHECK_RGN_HANDLE(&ctx, Handle);
	if (s32Ret != CVI_SUCCESS)
		return s32Ret;

	if (pstPalette->lut_length > 256) {
		CVI_TRACE_RGN(RGN_ERR, "RGN_LUT_index(%d) is over maximum(256).\n", pstPalette->lut_length);
		return CVI_FAILURE;
	}

	osd_layer = (ctx->stCanvasInfo[ctx->canvas_idx].bCompressed) ?
				RGN_ODEC_LAYER_VPSS : RGN_NORMAL_LAYER_VPSS;

#if 0 /* todo---copy_from_user here is not right, why??? */
	pstInputPixelTable = (RGN_RGBQUARD_S *)kmalloc_array(pstPalette->lut_length,
		 sizeof(RGN_RGBQUARD_S), GFP_KERNEL);
	if (pstInputPixelTable == CVI_NULL) {
		CVI_TRACE_RGN(RGN_ERR, "kmalloc failed.\n");
		return CVI_ERR_RGN_NOMEM;
	}

	if (copy_from_user(pstInputPixelTable, (void *)(pstPalette->pstPaletteTable),
		lut_cfg.lut_length * sizeof(RGN_RGBQUARD_S))) {
		CVI_TRACE_RGN(RGN_ERR, "lut copy_from_user failed.\n");
		ret = CVI_ERR_RGN_NOMEM;
		kfree(pstInputPixelTable);
		return ret;
	}

	for (idx = 0; idx < pstPalette->lut_length ; idx++) {
		CVI_TRACE_RGN(RGN_INFO, "index:%d pixel:%#x\n",
			idx, (pstInputPixelTable[idx].argbBlue | pstInputPixelTable[idx].argbGreen << 8) |
				(pstInputPixelTable[idx].argbRed << 16 | pstInputPixelTable[idx].argbAlpha << 24));
	}
#endif
	pstOutputPixelTable = kmalloc_array(pstPalette->lut_length, sizeof(CVI_U16), GFP_KERNEL);
	if (pstOutputPixelTable == CVI_NULL) {
		CVI_TRACE_RGN(RGN_ERR, "kmalloc failed.\n");
		// kfree(pstInputPixelTable);
		ret = CVI_ERR_RGN_NOMEM;
		return ret;
	}

	/* start color convert to ARGB4444 */
	for (idx = 0 ; idx < pstPalette->lut_length ; idx++) {
		switch (pstPalette->pixelFormat) {
		case RGN_COLOR_FMT_RGB444:
		case RGN_COLOR_FMT_RGB555:
		case RGN_COLOR_FMT_RGB565:
		case RGN_COLOR_FMT_RGB888:
			a = 0xf; /*alpha*/
			r = pstInputPixelTable[idx].argbRed;
			g = pstInputPixelTable[idx].argbGreen;
			b = pstInputPixelTable[idx].argbBlue;
			break;

		case RGN_COLOR_FMT_RGB1555:
		case RGN_COLOR_FMT_RGB4444:
		case RGN_COLOR_FMT_RGB8888:
		case RGN_COLOR_FMT_ARGB8888:
		case RGN_COLOR_FMT_ARGB4444:
		case RGN_COLOR_FMT_ARGB1555:
		default:
			a = pstInputPixelTable[idx].argbAlpha;
			r = pstInputPixelTable[idx].argbRed;
			g = pstInputPixelTable[idx].argbGreen;
			b = pstInputPixelTable[idx].argbBlue;
			break;
		}

		u32Pixel = (b | g << 8 | r << 16 | a << 24);
		CVI_TRACE_RGN(RGN_INFO, "Input Table index(%d) (0x%x).\n", idx, u32Pixel);

		u16Pixel = RGN_MAKECOLOR_U16_A(a, r, g, b, s_ConvertInfo[pstPalette->pixelFormat]);
		CVI_TRACE_RGN(RGN_INFO, "Output data = (0x%x).\n", u16Pixel);

		pstOutputPixelTable[idx] = u16Pixel;
	}

	/* Write output_pixel_table and lut_length into cfg */
	lut_cfg.lut_addr = (void *)pstOutputPixelTable;
	lut_cfg.lut_length = pstPalette->lut_length;

	/* Get related device number to update LUT but RGNEX use device 0. */
	if (ctx->stRegion.enType == OVERLAY_RGN || ctx->stRegion.enType == COVER_RGN)
		lut_cfg.rgnex_en = false;
	else
		lut_cfg.rgnex_en = true;

	// Need to record for gop0 or gop1
	lut_cfg.lut_layer = osd_layer;

	/* Uupdate device LUT in kernel space. */
	rgn_lut_arg.stChn = ctx->stChn;
	rgn_lut_arg.lut_cfg = lut_cfg;
	if (_rgn_call_cb(E_MODULE_VPSS, VPSS_CB_SET_RGN_LUT_CFG, &rgn_lut_arg) != 0) {
		CVI_TRACE_RGN(RGN_ERR, "VPSS_CB_SET_LUT_CFG is failed\n");
		kfree(pstOutputPixelTable);
		return CVI_ERR_RGN_ILLEGAL_PARAM;
	}

	kfree(pstOutputPixelTable);

	return ret;
}

static int _rgn_sw_init(struct cvi_rgn_dev *rdev)
{
	return _rgn_init();
}

static int _rgn_release_op(struct cvi_rgn_dev *rdev)
{
	return _rgn_exit();
}

static int _rgn_create_proc(struct cvi_rgn_dev *rdev)
{
	int ret = -EINVAL;

	if (rgn_proc_init() < 0) {
		CVI_TRACE_RGN(RGN_ERR, "rgn proc init failed\n");
		goto err;
	}
	ret = CVI_SUCCESS;

err:
	return ret;
}

static void _rgn_destroy_proc(struct cvi_rgn_dev *rdev)
{
	rgn_proc_remove();
}

/*******************************************************
 *  File operations for core
 ******************************************************/
static long _rgn_s_ctrl(struct cvi_rgn_dev *rdev, struct rgn_ext_control *p)
{
	int ret = -EINVAL;
	RGN_ATTR_S stRegion;
	BITMAP_S stBitmap;
	MMF_CHN_S stChn;
	RGN_CHN_ATTR_S stChnAttr;
	CVI_U32 u32Color, id, sdk_id;
	RGN_PALETTE_S stPalette;
	RGN_HANDLE Handle;

	id	= p->id;
	sdk_id	= p->sdk_id;
	Handle	= p->handle;

	switch (id) {
	case RGN_IOCTL_SC_SET_RGN: {
#if 0 //copy from cvi_vip_sc.c for V4L2_CID_DV_VIP_SC_SET_RGN
		if (copy_from_user(&sdev->vpss_chn_cfg[0].rgn_cfg, ext_ctrls[i].ptr,
						   sizeof(sdev->vpss_chn_cfg[0].rgn_cfg))) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "ioctl-%#x, copy_from_user failed.\n", ext_ctrls[i].id);
			rc = -ENOMEM;
			break;
		}
		rc = 0;
#endif
	} //RGN_IOCTL_SC_SET_RGN:
	break;

	case RGN_IOCTL_DISP_SET_RGN: {
#if 0 //copy from cvi_vip_disp.c for V4L2_CID_DV_VIP_DISP_SET_RGN
		struct sclr_disp_timing *timing = sclr_disp_get_timing();
		struct sclr_size size;
		struct cvi_rgn_cfg cfg;

		if (copy_from_user(&cfg, ext_ctrls[i].ptr, sizeof(struct cvi_rgn_cfg))) {
			dprintk(VIP_ERR, "ioctl-%#x, copy_from_user failed.\n", ext_ctrls[i].id);
			break;
		}

		size.w = timing->hfde_end - timing->hfde_start + 1;
		size.h = timing->vfde_end - timing->vfde_start + 1;
		rc = cvi_vip_set_rgn_cfg(SCL_GOP_DISP, &cfg, &size);
#endif
	} //RGN_IOCTL_DISP_SET_RGN:
	break;

	case RGN_IOCTL_SDK_CTRL: {
		switch (sdk_id) {
		case RGN_SDK_CREATE: {
			if (copy_from_user(&stRegion, p->ptr1, sizeof(RGN_ATTR_S)) != 0)
				break;

			ret = rgn_create(Handle, &stRegion);
		}
		break;

		case RGN_SDK_DESTORY: {
			ret = rgn_destory(Handle);
		}
		break;

		case RGN_SDK_SET_ATTR: {
			if (copy_from_user(&stRegion, p->ptr1, sizeof(RGN_ATTR_S)) != 0)
				break;

			ret = rgn_set_attr(Handle, &stRegion);
		}
		break;

		case RGN_SDK_SET_BIT_MAP: {
			if (copy_from_user(&stBitmap, p->ptr1, sizeof(BITMAP_S)) != 0)
				break;

			ret = rgn_set_bit_map(Handle, &stBitmap);
		}
		break;

		case RGN_SDK_ATTACH_TO_CHN: {
			if ((copy_from_user(&stChn, p->ptr1, sizeof(MMF_CHN_S)) != 0) ||
				(copy_from_user(&stChnAttr, p->ptr2, sizeof(RGN_CHN_ATTR_S)) != 0))
				break;

			ret = rgn_attach_to_chn(Handle, &stChn, &stChnAttr);
		}
		break;

		case RGN_SDK_DETACH_FROM_CHN: {
			if (copy_from_user(&stChn, p->ptr1, sizeof(MMF_CHN_S)) != 0)
				break;

			ret = rgn_detach_from_chn(Handle, &stChn);
		}
		break;

		case RGN_SDK_SET_DISPLAY_ATTR: {
			if ((copy_from_user(&stChn, p->ptr1, sizeof(MMF_CHN_S)) != 0) ||
				(copy_from_user(&stChnAttr, p->ptr2, sizeof(RGN_CHN_ATTR_S)) != 0))
				break;

			ret = rgn_set_display_attr(Handle, &stChn, &stChnAttr);
		}
		break;

		case RGN_SDK_UPDATE_CANVAS: {
			ret = rgn_update_canvas(Handle);
		}
		break;

		case RGN_SDK_INVERT_COLOR: {
			if ((copy_from_user(&stChn, p->ptr1, sizeof(MMF_CHN_S)) != 0) ||
				(copy_from_user(&u32Color, p->ptr2, sizeof(CVI_U32)) != 0))
				break;

			ret = rgn_invert_color(Handle, &stChn, &u32Color);
		}
		break;

		case RGN_SDK_SET_CHN_PALETTE: {
			RGN_RGBQUARD_S *pstInputPixelTable;

			if ((copy_from_user(&stChn, p->ptr1, sizeof(MMF_CHN_S)) != 0) ||
			    (copy_from_user(&stPalette, p->ptr2, sizeof(RGN_PALETTE_S)) != 0))
				break;

			pstInputPixelTable = kmalloc_array(stPalette.lut_length, sizeof(RGN_RGBQUARD_S), GFP_KERNEL);
			if (pstInputPixelTable == CVI_NULL) {
				CVI_TRACE_RGN(RGN_ERR, "kmalloc failed.\n");
				return CVI_ERR_RGN_NOMEM;
			}
			if (copy_from_user(pstInputPixelTable, (void *)(stPalette.pstPaletteTable),
				stPalette.lut_length * sizeof(RGN_RGBQUARD_S))) {
				CVI_TRACE_RGN(RGN_ERR, "lut copy_from_user failed.\n");
				ret = CVI_ERR_RGN_NOMEM;
				kfree(pstInputPixelTable);
				break;
			}

			ret = rgn_set_chn_palette(Handle, &stChn, &stPalette, pstInputPixelTable);
			kfree(pstInputPixelTable);
		}
		break;

		default:
			break;
		} //switch (sdk_id)

	} //case RGN_IOCTL_SDK_CTRL:
	break;

	default:
	break;
	} //switch (id)

	return ret;
}

static long _rgn_g_ctrl(struct cvi_rgn_dev *rdev, struct rgn_ext_control *p)
{
	int ret = -EINVAL;
	RGN_ATTR_S stRegion;
	MMF_CHN_S stChn;
	RGN_CHN_ATTR_S stChnAttr;
	RGN_CANVAS_INFO_S stCanvasInfo;
	CVI_U32 id, sdk_id;
	RGN_HANDLE Handle;

	id	= p->id;
	sdk_id	= p->sdk_id;
	Handle	= p->handle;

	switch (id) {
	case RGN_IOCTL_SDK_CTRL: {
		switch (sdk_id) {
		case RGN_SDK_GET_ATTR: {
			ret = rgn_get_attr(Handle, &stRegion);

			if (copy_to_user(p->ptr1, &stRegion, sizeof(RGN_ATTR_S)) != 0)
				break;
		}
		break;

		case RGN_SDK_GET_DISPLAY_ATTR: {
			if (copy_from_user(&stChn, p->ptr1, sizeof(MMF_CHN_S)) != 0)
				break;

			ret = rgn_get_display_attr(Handle, &stChn, &stChnAttr);

			if (copy_to_user(p->ptr2, &stChnAttr, sizeof(RGN_CHN_ATTR_S)) != 0)
				break;
		}
		break;

		case RGN_SDK_GET_CANVAS_INFO: {
			ret = rgn_get_canvas_info(Handle, &stCanvasInfo);

			if (copy_to_user(p->ptr1, &stCanvasInfo, sizeof(RGN_CANVAS_INFO_S)) != 0)
				break;
		}
		break;

		default:
			break;
		} //switch (sdk_id)

	} //case RGN_IOCTL_SDK_CTRL:
	break;

	default:
	break;
	} //switch (id)

	return ret;
}

long rgn_ioctl(struct file *file, u_int cmd, u_long arg)
{
	int ret = -EINVAL;
	struct cvi_rgn_dev *rdev = file->private_data;
	struct rgn_ext_control p;

	if (copy_from_user(&p, (void __user *)arg, sizeof(struct rgn_ext_control)))
		return ret;

	switch (cmd) {
	case RGN_IOC_S_CTRL:
		ret = _rgn_s_ctrl(rdev, &p);
		break;
	case RGN_IOC_G_CTRL:
		ret = _rgn_g_ctrl(rdev, &p);
		break;
	default:
		ret = -ENOTTY;
		break;
	}

	if (copy_to_user((void __user *)arg, &p, sizeof(struct rgn_ext_control)))
		return ret;

	return ret;
}

int rgn_open(struct inode *inode, struct file *file)
{
	int ret = 0;

	// only open once
	if (!atomic_read(&dev_open_cnt)) {
		struct cvi_rgn_dev *rdev =
		container_of(file->private_data, struct cvi_rgn_dev, miscdev);

		file->private_data = rdev;
		_rgn_sw_init(rdev);

		CVI_TRACE_RGN(RGN_INFO, "-\n");

		ret = CVI_SUCCESS;
	}

	atomic_inc(&dev_open_cnt);

	return ret;
}

int rgn_release(struct inode *inode, struct file *file)
{
	int ret = 0;

	// only exit once
	if (atomic_dec_and_test(&dev_open_cnt)) {
		struct cvi_rgn_dev *rdev =
		container_of(file->private_data, struct cvi_rgn_dev, miscdev);

		/* This should move to stop streaming */
		_rgn_release_op(rdev);

		CVI_TRACE_RGN(RGN_INFO, "-\n");

		ret = CVI_SUCCESS;
	}

	if (atomic_read(&dev_open_cnt) < 0)
		atomic_set(&dev_open_cnt, 0);

	return ret;
}

int rgn_cb(void *dev, enum ENUM_MODULES_ID caller, u32 cmd, void *arg)
{
	int ret = 0;

	switch (cmd) {
	default:
		break;
	}

	return ret;
}

/*******************************************************
 *  Common interface for core
 ******************************************************/
int rgn_create_instance(struct platform_device *pdev)
{
	int ret = -EINVAL;
	struct cvi_rgn_dev *rdev;

	rdev = dev_get_drvdata(&pdev->dev);
	if (!rdev) {
		CVI_TRACE_RGN(RGN_ERR, "invalid data\n");
		goto err;
	}

	if (_rgn_create_proc(rdev)) {
		CVI_TRACE_RGN(RGN_ERR, "Failed to create proc\n");
		goto err;
	}
	ret = CVI_SUCCESS;

err:
	return ret;
}

int rgn_destroy_instance(struct platform_device *pdev)
{
	int ret = -EINVAL;
	struct cvi_rgn_dev *rdev;

	rdev = dev_get_drvdata(&pdev->dev);
	if (!rdev) {
		CVI_TRACE_RGN(RGN_ERR, "invalid data\n");
		goto err;
	}

	_rgn_destroy_proc(rdev);
	ret = CVI_SUCCESS;

err:
	return ret;
}
