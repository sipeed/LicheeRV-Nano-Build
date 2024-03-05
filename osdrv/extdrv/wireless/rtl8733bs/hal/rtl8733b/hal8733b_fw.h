/******************************************************************************
*
* Copyright(c) 2012 - 2017 Realtek Corporation.
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of version 2 of the GNU General Public License as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
******************************************************************************/

#ifdef CONFIG_RTL8733B

#ifndef _FW_HEADER_8733B_H
#define _FW_HEADER_8733B_H

#ifdef LOAD_FW_HEADER_FROM_DRIVER
#if (DM_ODM_SUPPORT_TYPE & (ODM_WIN)) || (DM_ODM_SUPPORT_TYPE & (ODM_CE))
extern u8 ccv_array_mp_8733b_fw_nic[122232];
extern u32 ccv_array_length_mp_8733b_fw_nic;
extern u8 array_mp_8733b_fw_nic[121856];
extern u32 array_length_mp_8733b_fw_nic;
#ifdef CONFIG_WOWLAN
extern u8 array_mp_8733b_fw_wowlan[114752];
extern u32 array_length_mp_8733b_fw_wowlan;
#endif /*CONFIG_WOWLAN*/
#ifdef CONFIG_WOWLAN
extern u8 ccv_array_mp_8733b_fw_wowlan[115480];
extern u32 ccv_array_length_mp_8733b_fw_wowlan;
#endif /*CONFIG_WOWLAN*/
#endif
#endif /* end of LOAD_FW_HEADER_FROM_DRIVER */

#endif

#endif

