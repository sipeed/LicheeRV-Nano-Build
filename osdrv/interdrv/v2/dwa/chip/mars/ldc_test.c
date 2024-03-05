#include <linux/types.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/dma-buf.h>
#include <linux/version.h>

#include "ion/ion.h"
#include "ion/cvitek/cvitek_ion_alloc.h"

#include "ldc_test.h"
#include "ldc.h"
#include "ldc_reg.h"
#include "reg_ldc.h"
#include "cmdq.h"
#include "dwa_debug.h"
#include "reg.h"

#define PROC_NAME	"cvitek/ldc_test"

uint8_t ldc_test_enabled;

#ifdef PORTING_TEST
void ldc_dump_register(void)
{
	u32 val;
	void *base;

	ldc_get_base_addr(&base);
	uintptr_t ldc_base = (uintptr_t)base + REG_LDC_REG_BASE;

	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_FORMAT=0x%08x\n", _reg_read(ldc_base + LDC_FORMAT));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_RAS_MODE=0x%08x\n", _reg_read(ldc_base + LDC_RAS_MODE));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_RAS_XSIZE=0x%08x\n", _reg_read(ldc_base + LDC_RAS_XSIZE));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_RAS_YSIZE=0x%08x\n", _reg_read(ldc_base + LDC_RAS_YSIZE));

	val = _reg_read(ldc_base + LDC_MAP_BASE);
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_MAP_BASE=0x%08x\n", val);
	CVI_TRACE_DWA(CVI_DBG_INFO, "    addr=0x%08x\n", val << LDC_BASE_ADDR_SHIFT);

	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_MAP_BYPASS=0x%08x\n", _reg_read(ldc_base + LDC_MAP_BYPASS));

	val = _reg_read(ldc_base + LDC_SRC_BASE_Y);
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_SRC_BASE_Y=0x%08x\n", val);
	CVI_TRACE_DWA(CVI_DBG_INFO, "    addr=0x%08x\n", val << LDC_BASE_ADDR_SHIFT);

	val = _reg_read(ldc_base + LDC_SRC_BASE_C);
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_SRC_BASE_C=0x%08x\n", val);
	CVI_TRACE_DWA(CVI_DBG_INFO, "    addr=0x%08x\n", val << LDC_BASE_ADDR_SHIFT);

	val = _reg_read(ldc_base + LDC_SRC_XSIZE);
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_SRC_XSIZE=0x%08x\n", val);
	CVI_TRACE_DWA(CVI_DBG_INFO, "    src_xsize=%d\n",
		(val & LDC_REG_SRC_XSIZE_MASK) >> LDC_REG_SRC_XSIZE_OFFSET);

	val = _reg_read(ldc_base + LDC_SRC_YSIZE);
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_SRC_YSIZE=0x%08x\n", val);
	CVI_TRACE_DWA(CVI_DBG_INFO, "    src_ysize=%d\n",
		(val & LDC_REG_SRC_YSIZE_MASK) >> LDC_REG_SRC_YSIZE_OFFSET);

	val = _reg_read(ldc_base + LDC_SRC_XSTR);
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_SRC_XSTR=0x%08x\n", val);
	CVI_TRACE_DWA(CVI_DBG_INFO, "    src_xstr=%d\n",
		(val & LDC_REG_SRC_XSTR_MASK) >> LDC_REG_SRC_XSTR_OFFSET);

	val = _reg_read(ldc_base + LDC_SRC_XEND);
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_SRC_XEND=0x%08x\n", val);
	CVI_TRACE_DWA(CVI_DBG_INFO, "    src_xend=%d\n",
		(val & LDC_REG_SRC_XEND_MASK) >> LDC_REG_SRC_XEND_OFFSET);

	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_SRC_BG=0x%08x\n", _reg_read(ldc_base + LDC_SRC_BG));

	val = _reg_read(ldc_base + LDC_DST_BASE_Y);
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_DST_BASE_Y=0x%08x\n", val);
	CVI_TRACE_DWA(CVI_DBG_INFO, "    addr=0x%08x\n", val << LDC_BASE_ADDR_SHIFT);

	val = _reg_read(ldc_base + LDC_DST_BASE_C);
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_DST_BASE_C=0x%08x\n", val);
	CVI_TRACE_DWA(CVI_DBG_INFO, "    addr=0x%08x\n", val << LDC_BASE_ADDR_SHIFT);

	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_DST_MODE=0x%08x\n", _reg_read(ldc_base + LDC_DST_MODE));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_IRQEN=0x%08x\n", _reg_read(ldc_base + LDC_IRQEN));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_START=0x%08x\n", _reg_read(ldc_base + LDC_START));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_IRQSTAT=0x%08x\n", _reg_read(ldc_base + LDC_IRQSTAT));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_IRQCLR=0x%08x\n", _reg_read(ldc_base + LDC_IRQCLR));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_SB_MODE=0x%08x\n", _reg_read(ldc_base + LDC_SB_MODE));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_SB_NB=0x%08x\n", _reg_read(ldc_base + LDC_SB_NB));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_SB_SIZE=0x%08x\n", _reg_read(ldc_base + LDC_SB_SIZE));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_SB_SET_STR=0x%08x\n", _reg_read(ldc_base + LDC_SB_SET_STR));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_SB_SW_WPTR=0x%08x\n", _reg_read(ldc_base + LDC_SB_SW_WPTR));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_SB_SW_CLR=0x%08x\n", _reg_read(ldc_base + LDC_SB_SW_CLR));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_SB_FULL=0x%08x\n", _reg_read(ldc_base + LDC_SB_FULL));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_SB_EMPTY=0x%08x\n", _reg_read(ldc_base + LDC_SB_EMPTY));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_SB_WPTR_RO=0x%08x\n", _reg_read(ldc_base + LDC_SB_WPTR_RO));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_SB_DPTR_RO=0x%08x\n", _reg_read(ldc_base + LDC_SB_DPTR_RO));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_SB_STRIDE=0x%08x\n", _reg_read(ldc_base + LDC_SB_STRIDE));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_LDC_DIR=0x%08x\n", _reg_read(ldc_base + LDC_LDC_DIR));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_CMDQ_IRQ_EN=0x%08x\n", _reg_read(ldc_base + LDC_REG_CMDQ_IRQ_EN));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_FORCE_IN_RANGE=0x%08x\n", _reg_read(ldc_base + LDC_LDC_FORCE_IN_RANGE));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_OUT_RANGE=0x%08x\n", _reg_read(ldc_base + LDC_LDC_OUT_RANGE));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_OUT_RANGE_DST_X=0x%08x\n", _reg_read(ldc_base + LDC_LDC_OUT_RANGE_DST_X));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_OUT_RANGE_DST_Y=0x%08x\n", _reg_read(ldc_base + LDC_LDC_OUT_RANGE_DST_Y));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_OUT_RANGE_SRC_X=0x%08x\n", _reg_read(ldc_base + LDC_LDC_OUT_RANGE_SRC_X));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_OUT_RANGE_SRC_Y=0x%08x\n", _reg_read(ldc_base + LDC_LDC_OUT_RANGE_SRC_Y));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_DST_TI_CNT_X=0x%08x\n", _reg_read(ldc_base + LDC_LDC_DST_TI_CNT_X));
	CVI_TRACE_DWA(CVI_DBG_INFO, "LDC_DST_TI_CNT_Y=0x%08x\n", _reg_read(ldc_base + LDC_LDC_DST_TI_CNT_Y));
}

void ldc_test_irq_handler(uint32_t intr_status)
{
	if (ldc_tst_enable_cmdq) {
		u8 status = cmdQ_intr_status(REG_LDC_CMDQ_BASE);

		if (status) {
			cmdQ_intr_clr(REG_LDC_CMDQ_BASE, status);
			CVI_TRACE_DWA(CVI_DBG_INFO, "status(%#x), cmdq-status(0x%x)\n", intr_status, status);

			if ((status & 0x04) && cmdQ_is_sw_restart(REG_LDC_CMDQ_BASE)) {
				CVI_TRACE_DWA(CVI_DBG_INFO, "cmdq-sw restart\n", __func__);
				cmdQ_sw_restart(REG_LDC_CMDQ_BASE);
			}

			if (status & 0x02) {
				ldc_tst_mark++;  // cmdq end
				ldc_tst_enable_cmdq = false;
			}
		} else {
			CVI_TRACE_DWA(CVI_DBG_INFO, "status(%#x), cmdq-status not found\n", intr_status);
		}
	} else {
		ldc_intr_clr(intr_status);
		++ldc_tst_mark;
		CVI_TRACE_DWA(CVI_DBG_INFO, "status(%#x)\n", intr_status);
	}
}

static int ldc_check_irq_handler(unsigned int timeout)
{
	unsigned int i;
	const unsigned int step = 100;

	/* OS tick(10 ms) -> usec */
	timeout *= 10000;
	timeout = (timeout >= step) ? timeout : step;

	for (i = 0; i < timeout; i += step) {
		if (ldc_tst_mark > 0) {
			ldc_tst_mark = 0;
			CVI_TRACE_DWA(CVI_DBG_INFO, "check tick:%d\n", i);
			return 0;
		}
		udelay(step);
	}

	return -1;
}

static int ldc_test1(void)
{
	u32 ldc_src_buf, ldc_dst_buf;
	char *src, *dst;
	struct ldc_cfg cfg;
	int ret = 0;
	u8 status;
	int i, len;
	int ion_fd;
	struct ion_buffer *ionbuf;
	struct dma_buf *dmabuf;
	u32 width = 64;		// 64X
	u32 height = 64;	// 64X

	len = 64 * 64 * 2; // in + out
	ion_fd = cvi_ion_alloc(ION_HEAP_TYPE_CARVEOUT, len, 1);
	if (ion_fd < 0) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "ion allocate fail\n");
		return -1;
	}

	dmabuf = dma_buf_get(ion_fd);
	if (!dmabuf) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "dma_buf_get fail\n");
		return -1;
	}

	/* Get kernel vaddr */
	ret = dma_buf_begin_cpu_access(dmabuf, DMA_TO_DEVICE);
	if (ret < 0) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "dma_buf_begin_cpu_access fail.\n");
		return ret;
	}

	ionbuf = (struct ion_buffer *)dmabuf->priv;
	ldc_src_buf = ionbuf->paddr;
	ldc_dst_buf = ldc_src_buf + width * height;
	src = ionbuf->vaddr;
	dst = ionbuf->vaddr + width * height;

	memset(&cfg, 0, sizeof(cfg));
	cfg.pixel_fmt = 0; // Y only
	cfg.dst_mode = 0; // flat
	cfg.map_bypass = 1;
	cfg.src_width = width;
	cfg.src_height = height;
	cfg.ras_width = cfg.src_width;
	cfg.ras_height = cfg.src_height;
	cfg.src_xstart = 0;
	cfg.src_xend = cfg.ras_width - 1;
	cfg.src_y_base = ldc_src_buf;
	cfg.dst_y_base = ldc_dst_buf;
	cfg.bgcolor = 0xff;

	CVI_TRACE_DWA(CVI_DBG_INFO, "src buf address Y(%#x)\n", cfg.src_y_base);
	CVI_TRACE_DWA(CVI_DBG_INFO, "dst buf address Y(%#x)\n", cfg.dst_y_base);

	for (i = 0; i < 32 * 32; i++) {
		src[i] = i;
		dst[i] = 0;
	}

	// Write 1 clear
	ldc_intr_clr(1);

	status = ldc_intr_status();
	if (status) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "ldc intr status(%d) doesn't cleared\n", status);
		ret = -1;
	}

	ldc_engine(&cfg);

	if (ldc_check_irq_handler(60) != 0) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "ldc end intr doesn't happens after 60 ms\n");
		ret = -1;
	} else {
		CVI_TRACE_DWA(CVI_DBG_INFO, "ldc done\n");
	}

	status = ldc_intr_status();
	CVI_TRACE_DWA(CVI_DBG_INFO, "ldc intr status(%d) after kicked\n", status);

	for (i = 0; i < 64 * 64; i++) {
		if (src[i] != dst[i]) {
			CVI_TRACE_DWA(CVI_DBG_ERR, "  [%d] src=%d, dst=%d, error !\n", i, src[i], dst[i]);
			ret = -1;
			break;
		}
	}

	CVI_TRACE_DWA(CVI_DBG_INFO, "%s\n", ret ? "fail":"pass");

	return ret;
}

static void ldc_test_usage(struct seq_file *m)
{
	seq_puts(m, "  0: 64x64, memcpy\n");
	seq_puts(m, "  1: 128x128, memcpy\n");
	seq_puts(m, "  2: 1920x1088, memcpy\n");
	seq_puts(m, "  3: 1920x1088, 2 pass ldc\n");
	seq_puts(m, "  4: 1984x1088, 2 pass ldc\n");
	seq_puts(m, "  5: 320x256, 2 pass ldc\n");
	seq_puts(m, "  6: 320x256, cmdq, bypass, flat\n");
	seq_puts(m, "100: dump register\n");
}

static int ldc_test_proc_show(struct seq_file *m, void *v)
{
	ldc_test_usage(m);

	return 0;
}

static int ldc_test_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ldc_test_proc_show, PDE_DATA(inode));
}

static ssize_t ldc_test_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	uint32_t input_param = 0;

	if (kstrtouint_from_user(user_buf, count, 0, &input_param)) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "input parameter incorrect\n");
		return count;
	}

	//reset related info
	CVI_TRACE_DWA(CVI_DBG_ERR, "input_param=%d\n", input_param);

	if (input_param < 100)
		ldc_test_enabled = 1;

	switch (input_param) {
	case 0:
		ldc_test1();
	break;

	case 100:
		ldc_dump_register();
	break;

	default:
	break;
	}

	if (input_param < 100)
		ldc_test_enabled = 0;

	return count;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops ldc_test_proc_ops = {
	.proc_open = ldc_test_proc_open,
	.proc_read = seq_read,
	.proc_write = ldc_test_proc_write,
	.proc_release = single_release,
};
#else
static const struct file_operations ldc_test_proc_ops = {
	.owner = THIS_MODULE,
	.open = ldc_test_proc_open,
	.read = seq_read,
	.write = ldc_test_proc_write,
	.release = single_release,
};
#endif
int32_t ldc_test_proc_init(void)
{
	int ret = 0;

	if (proc_create(PROC_NAME, 0644, NULL, &ldc_test_proc_ops) == NULL) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "sclr_test: : proc_init() failed\n");
		ret = -1;
	}

	return ret;
}

int32_t ldc_test_proc_deinit(void)
{
	remove_proc_entry(PROC_NAME, NULL);

	return 0;
}
#else
void ldc_dump_register(void)
{
}
int32_t ldc_test_proc_init(void)
{
	return 0;
}
int32_t ldc_test_proc_deinit(void)
{
	return 0;
}
void ldc_test_irq_handler(uint32_t intr_raw_status)
{
}
#endif
