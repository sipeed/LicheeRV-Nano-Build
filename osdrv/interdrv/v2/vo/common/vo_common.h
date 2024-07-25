#ifndef __VO_COMMON_H__
#define __VO_COMMON_H__

#include <linux/debugfs.h>

#ifdef __cplusplus
	extern "C" {
#endif
#define VIP_ALIGNMENT 0x40
#define GOP_ALIGNMENT 0x10

extern u32 vo_log_lv;
#define _reg_read(addr) readl((void __iomem *)addr)
#define _reg_write(addr, data) writel(data, (void __iomem *)addr)

#define CVI_DBG_ERR        1   /* error conditions                     */
#define CVI_DBG_WARN       2   /* warning conditions                   */
#define CVI_DBG_NOTICE     3   /* normal but significant condition     */
#define CVI_DBG_INFO       4   /* informational                        */
#define CVI_DBG_DEBUG      5   /* debug-level messages                 */

#if defined(CONFIG_CVI_LOG)
#define CVI_TRACE_VO(level, fmt, ...) \
	do { \
		if (vo_log_lv >= level) { \
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
#define CVI_TRACE_VO(level, fmt, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __VO_COMMON_H__ */
