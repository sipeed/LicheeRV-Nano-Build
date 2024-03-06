#include "cvi_json_struct_comm.h"
#include "cvi_vpss.h"
#include "vpss_json_struct.h"

/**************************************************************************
 *   Json struct related APIs.
 **************************************************************************/
//VPSS_PARAMETER_BUFFER json & struct
static void VPSS_BIN_DATA_JSON(int r_w_flag, JSON *j, char *key, VPSS_BIN_DATA *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_S32, proc_amp, PROC_AMP_MAX);

	JSON_END(r_w_flag);
}
void VPSS_PARAMETER_BUFFER_JSON(int r_w_flag, JSON *j, char *key, VPSS_PARAMETER_BUFFER *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, VPSS_BIN_DATA, vpss_bin_data, VPSS_MAX_GRP_NUM);

	JSON_END(r_w_flag);
}
