#ifndef __RGN_CB_H__
#define __RGN_CB_H__

#ifdef __cplusplus
	extern "C" {
#endif

enum RGN_CB_CMD {
	RGN_CB_MAX
};

struct _rgn_chn_size_cb_param {
	MMF_CHN_S stChn;
	RECT_S rect;
};

struct _rgn_hdls_cb_param {
	MMF_CHN_S stChn;
	RGN_HANDLE *hdls;
	RGN_TYPE_E enType;
	__u32 layer;
};

struct _rgn_cfg_cb_param {
	MMF_CHN_S stChn;
	struct cvi_rgn_cfg rgn_cfg;
	__u32 layer;
};

struct _rgn_ex_cfg_cb_param {
	MMF_CHN_S stChn;
	struct cvi_rgn_ex_cfg rgn_ex_cfg;
};

struct _rgn_coverex_cfg_cb_param {
	MMF_CHN_S stChn;
	struct cvi_rgn_coverex_cfg rgn_coverex_cfg;
};

struct _rgn_mosaic_cfg_cb_param {
	MMF_CHN_S stChn;
	struct cvi_rgn_mosaic_cfg rgn_mosaic_cfg;
};


struct _rgn_lut_cb_param {
	MMF_CHN_S stChn;
	struct cvi_rgn_lut_cfg lut_cfg;
};

struct _rgn_get_ow_addr_cb_param {
	MMF_CHN_S stChn;
	RGN_HANDLE handle;
	__u32 layer;
	__u64 addr;
};

#ifdef __cplusplus
}
#endif

#endif /* __RGN_CB_H__ */
