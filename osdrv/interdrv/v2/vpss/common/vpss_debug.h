#ifndef _VPSS_DEBUG_H_
#define _VPSS_DEBUG_H_

#include <linux/debugfs.h>

extern u32 vpss_log_lv;

#define CVI_DBG_ERR        1   /* error conditions                     */
#define CVI_DBG_WARN       3   /* warning conditions                   */
#define CVI_DBG_NOTICE     7   /* normal but significant condition     */
#define CVI_DBG_INFO       0xf   /* informational                        */
#define CVI_DBG_DEBUG      0xff   /* debug-level messages                 */

#if defined(CONFIG_CVI_LOG)
#define CVI_TRACE_VPSS(level, fmt, ...) \
	do { \
		if (level <= vpss_log_lv) { \
			if (level == CVI_DBG_ERR) \
				pr_err("%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == CVI_DBG_WARN) \
				pr_warn("%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == CVI_DBG_NOTICE) \
				pr_notice("%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == CVI_DBG_INFO) \
				pr_info("%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == CVI_DBG_DEBUG) \
				printk(KERN_DEBUG "%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
		} \
	} while (0)
#else
#define CVI_TRACE_VPSS(level, fmt, ...)
#endif


#endif /* _VPSS_DEBUG_H_ */
