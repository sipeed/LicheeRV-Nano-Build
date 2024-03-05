#ifndef __VB_TEST_H__
#define __VB_TEST_H__

#ifdef DRV_TEST

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <linux/types.h>
#include <linux/slab.h>
#include "vb.h"
#include "sys.h"

int32_t vb_unit_test(int32_t op);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif

#endif /* __VB_TEST_H__ */
