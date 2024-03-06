#include <string.h>
#include "vo_json_struct.h"

/**************************************************************************
 *   Json struct related APIs.
 **************************************************************************/
void VO_GAMMA_INFO_S_JSON(int r_w_flag, JSON *j, char *key, VO_GAMMA_INFO_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, enable);
	JSON(r_w_flag, CVI_BOOL, osd_apply);
	JSON_A(r_w_flag, CVI_U8, value, VO_GAMMA_NODENUM);

	JSON_END(r_w_flag);
}
void VO_BIN_INFO_S_JSON(int r_w_flag, JSON *j, char *key, VO_BIN_INFO_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, VO_GAMMA_INFO_S, gamma_info);
	JSON(r_w_flag, CVI_U32, guard_magic);

	JSON_END(r_w_flag);
}

