#ifndef _VPSS_JSON_STRUCT_H
#define _VPSS_JSON_STRUCT_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include "cvi_json_struct_comm.h"
#include "cvi_base.h"
#include "cvi_vpss.h"

void VPSS_PARAMETER_BUFFER_JSON(int r_w_flag, JSON *j, char *key, VPSS_PARAMETER_BUFFER *data);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif
