#ifndef __VO_COMMON_H__
#define __VO_COMMON_H__

#ifdef __cplusplus
	extern "C" {
#endif
#define VIP_ALIGNMENT 0x40
#define GOP_ALIGNMENT 0x10

extern u32 vo_log_lv;
#define _reg_read(addr) readl((void __iomem *)addr)
#define _reg_write(addr, data) writel(data, (void __iomem *)addr)

#define vo_pr(level, fmt, arg...) \
	do { \
		if (vo_log_lv & level) { \
			if (level == VO_ERR) \
				pr_err("%d:%s(): " fmt, __LINE__, __func__, ## arg); \
			else if (level == VO_WARN) \
				pr_warn("%d:%s(): " fmt, __LINE__, __func__, ## arg); \
			else if (level == VO_NOTICE) \
				pr_notice("%d:%s(): " fmt, __LINE__, __func__, ## arg); \
			else if (level == VO_INFO) \
				pr_info("%d:%s(): " fmt, __LINE__, __func__, ## arg); \
			else if (level == VO_DBG) \
				pr_debug("%d:%s(): " fmt, __LINE__, __func__, ## arg); \
		} \
	} while (0)

enum vo_msg_pri {
	VO_ERR = 3,
	VO_WARN = 7,
	VO_NOTICE = 0xf,
	VO_INFO = 0xff,
	VO_DBG = 0xfff,
};

#ifdef __cplusplus
}
#endif

#endif /* __VO_COMMON_H__ */
