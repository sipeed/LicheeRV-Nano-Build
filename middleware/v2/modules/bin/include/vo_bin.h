#ifndef _VO_BIN_H
#define _VO_BIN_H

#include <linux/cvi_type.h>
#include "cvi_bin.h"
#include <linux/cvi_common.h>
#include <linux/cvi_comm_vo.h>

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

CVI_S32 vo_bin_getbinsize(CVI_U32 *size);
CVI_S32 vo_bin_getparamfrombin(CVI_U8 *addr, CVI_U32 size);
CVI_S32 vo_bin_setparamtobuf(CVI_U8 *buffer);
CVI_S32 vo_bin_setparamtobin(FILE *fp);
#ifndef DISABLE_PQBIN_JSON
CVI_S32 vo_json_getParamFromJsonbuffer(const char *buffer, enum CVI_BIN_SECTION_ID id);
CVI_S32 vo_json_setParamToJsonbuffer(CVI_S8 **buffer, enum CVI_BIN_SECTION_ID id, CVI_S32 *len);
#endif

VO_BIN_INFO_S *get_vo_bin_info_addr(void);
CVI_U32 get_vo_bin_guardmagic_code(void);

#endif
