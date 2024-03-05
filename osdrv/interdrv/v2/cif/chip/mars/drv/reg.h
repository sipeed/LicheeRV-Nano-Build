#ifndef _CVI_REG_H_
#define _CVI_REG_H_

#include <linux/io.h>

#define _reg_read(addr) readl((void __iomem *)addr)
#define _reg_write(addr, data) writel(data, (void __iomem *)addr)

#endif //_CVI_REG_H_
