#ifndef _VO_JSON_STRUCT_H
#define _VO_JSON_STRUCT_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <linux/cvi_comm_vo.h>
#include "cvi_json_struct_comm.h"

void VO_GAMMA_INFO_S_JSON(int r_w_flag, JSON*j, char *key, VO_GAMMA_INFO_S*data);
void VO_BIN_INFO_S_JSON(int r_w_flag, JSON *j, char *key, VO_BIN_INFO_S *data);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif
