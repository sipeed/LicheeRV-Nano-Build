#include <linux/types.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#include <linux/suspend.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/version.h>
#include <linux/ctype.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
#include <linux/sched/signal.h>
#endif
#include <linux/security.h>
#include <linux/cred.h>
#include "tee_cv_private.h"
#include "base.h"
#include "vb.h"
#include "cvi_vb_proc.h"
#include "cvi_log_proc.h"
#include "cvi_sys_proc.h"
#include <linux/cvi_base_ctx.h>
#include <linux/semaphore.h>
#include <base_cb.h>
#include <sys_k.h>
#include <vip_common.h>

#include "linux/cv180x_efuse.h"

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

#define BASE_CLASS_NAME "cvi-base"
#define BASE_DEV_NAME "cvi-base"

/* register bank */
#define TOP_BASE 0x03000000
#define TOP_REG_BANK_SIZE 0x10000
#define GP_REG3_OFFSET 0x8C
#define GP_REG_CHIP_ID_MASK 0xFFFF

#define CV183X_RTC_BASE 0x03005000
#define CV182X_RTC_BASE 0x05026000
#define RTC_REG_BANK_SIZE 0x140
#define RTC_ST_ON_REASON 0xF8

struct base_device {
	struct device *dev;
	struct miscdevice miscdev;
	void *shared_mem;
	u16 mmap_count;
	struct list_head ps_list;
	spinlock_t lock;
	enum base_state_e state;
	struct completion done;
	struct notifier_block notifier;
	u8 sig_hook;
};

struct base_state {
	struct list_head list;      /* state list */
	struct base_device *ndev;
	struct file *file;
	unsigned int state_signr;
	struct pid *state_pid;
	const struct cred *cred;
	void __user *state_context;
	u32 secid;
};

static void __iomem *top_base;
static void __iomem *rtc_base;
static struct proc_dir_entry *proc_dir;
struct class *pbase_class;

const char * const CB_MOD_STR[] = CB_FOREACH_MOD(CB_GENERATE_STRING);
static struct base_m_cb_info base_m_cb[E_MODULE_BUTT];

#ifdef DRV_FPGA_PORTING
#define FPGA_EARLY_PORTING_CHIP_ID E_CHIPID_CV1822
#endif

static int __init base_init(void);
static void __exit base_exit(void);

uint32_t base_log_lv = CVI_BASE_DBG_ERR;
module_param(base_log_lv, int, 0644);


static ssize_t base_efuse_shadow_show(struct class *class,
				      struct class_attribute *attr, char *buf)
{
	int ret = 0;
	UNUSED(class);
	UNUSED(attr);

	ret = cvi_efuse_read_buf(0, buf, PAGE_SIZE);

	return ret;
}

static ssize_t base_efuse_shadow_store(struct class *class,
				       struct class_attribute *attr,
				       const char *buf, size_t count)
{
	unsigned long addr;
	uint32_t value = 0xDEAFBEEF;
	int ret;
	UNUSED(class);
	UNUSED(attr);

	ret = kstrtoul(buf, 0, &addr);
	if (ret < 0) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "efuse_read: ret=%d\n", ret);
		return ret;
	}

	ret = cvi_efuse_read_buf(addr, &value, sizeof(value));
	CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "efuse_read: 0x%04lx=0x%08x ret=%d\n", addr, value, ret);

	return count;
}

static ssize_t base_efuse_prog_show(struct class *class,
				    struct class_attribute *attr, char *buf)
{
	UNUSED(class);
	UNUSED(attr);

	return scnprintf(buf, PAGE_SIZE, "%s\n", "PROG_SHOW");
}

static ssize_t base_efuse_prog_store(struct class *class,
				     struct class_attribute *attr,
				     const char *buf, size_t count)
{
	int err;
	uint32_t addr = 0, value = 0;
	UNUSED(class);
	UNUSED(attr);

	if (sscanf(buf, "0x%x=0x%x", &addr, &value) != 2)
		return -ENOMEM;

	CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "%s: addr=0x%02x value=0x%08x\n", __func__, addr, value);
	err = cvi_efuse_write(addr, value);
	if (err < 0)
		return err;

	return count;
}

static ssize_t base_uid_show(struct class *class,
			     struct class_attribute *attr, char *buf)
{
	uint32_t uid_3 = 0xDEAFBEEF;
	uint32_t uid_4 = 0xDEAFBEEF;
	UNUSED(class);
	UNUSED(attr);

	uid_3 = cvi_efuse_read_from_shadow(0x0c);
	uid_4 = cvi_efuse_read_from_shadow(0x10);

	return scnprintf(buf, PAGE_SIZE, "UID: %08x_%08x\n", uid_3, uid_4);
}

static ssize_t base_rosc_show(struct class *class,
			      struct class_attribute *attr, char *buf)
{
	int count = 0;
	void __iomem *rosc_base;
	UNUSED(class);
	UNUSED(attr);

	rosc_base = ioremap(0x030D0000, 0x2028);

	count += scnprintf(buf + count, PAGE_SIZE - count,
			   "ROSC[0x030D0010]=0x%08X\n", ioread32(rosc_base + 0x0010));
	count += scnprintf(buf + count, PAGE_SIZE - count,
			   "ROSC[0x030D0024]=0x%08X\n", ioread32(rosc_base + 0x0024));
	count += scnprintf(buf + count, PAGE_SIZE - count,
			   "ROSC[0x030D0028]=0x%08X\n", ioread32(rosc_base + 0x0028));
	count += scnprintf(buf + count, PAGE_SIZE - count,
			   "ROSC[0x030D2010]=0x%08X\n", ioread32(rosc_base + 0x2010));
	count += scnprintf(buf + count, PAGE_SIZE - count,
			   "ROSC[0x030D2024]=0x%08X\n", ioread32(rosc_base + 0x2024));
	count += scnprintf(buf + count, PAGE_SIZE - count,
			   "ROSC[0x030D2028]=0x%08X\n", ioread32(rosc_base + 0x2028));

	iounmap(rosc_base);

	return count;
}

static ssize_t base_rosc_store(struct class *class,
			       struct class_attribute *attr,
			       const char *buf, size_t count)
{
	uint32_t chip_id;
	void __iomem *rosc_base;
	UNUSED(class);
	UNUSED(attr);

	if (sysfs_streq(buf, "set")) {
		chip_id = cvi_base_read_chip_id();
		rosc_base = ioremap(0x030D0000, 0x2028);
		if (IS_CHIP_CV182X(chip_id) || IS_CHIP_CV181X(chip_id)) {
			iowrite32(0x00000000, rosc_base + 0x0004);
			iowrite32(0x00000040, rosc_base + 0x000C);
			iowrite32(0x01020009, rosc_base + 0x0014);
			iowrite32(0x00FFFFFF, rosc_base + 0x001C);
			iowrite32(0x00000001, rosc_base + 0x0004);

			iowrite32(0x00000000, rosc_base + 0x2004);
			iowrite32(0x00000040, rosc_base + 0x200C);
			iowrite32(0x01020009, rosc_base + 0x2014);
			iowrite32(0x00FFFFFF, rosc_base + 0x201C);
			iowrite32(0x00000001, rosc_base + 0x2004);
		} else {
			iowrite32(0x01020009, rosc_base + 0x0014);
			iowrite32(0x00FFFFFF, rosc_base + 0x001C);
			iowrite32(0x00000000, rosc_base + 0x0004);
			iowrite32(0x00000001, rosc_base + 0x0004);

			iowrite32(0x01020009, rosc_base + 0x2014);
			iowrite32(0x00FFFFFF, rosc_base + 0x201C);
			iowrite32(0x00000000, rosc_base + 0x2004);
			iowrite32(0x00000001, rosc_base + 0x2004);
		}
		iounmap(rosc_base);
	}

	return count;
}

CLASS_ATTR_RW(base_efuse_shadow);
CLASS_ATTR_RW(base_efuse_prog);
CLASS_ATTR_RO(base_uid);
CLASS_ATTR_RW(base_rosc);

#ifndef FPGA_EARLY_PORTING_CHIP_ID
static unsigned int _cvi_base_read_by_kernel(unsigned int chip_id)
{
#ifdef __riscv
	switch (chip_id) {
	case 0x1810C:
		return E_CHIPID_CV1810C;
	case 0x1811C:
		return E_CHIPID_CV1811C;
	case 0x1812C:
		return E_CHIPID_CV1812C;
	case 0x1811F:
		return E_CHIPID_CV1811H;
	case 0x1812F:
		return E_CHIPID_CV1812H;
	case 0x1813F:
		return E_CHIPID_CV1813H;
	}
#else
	switch (chip_id) {
	case 0x1810C:
		return E_CHIPID_CV1820A;
	case 0x1811C:
		return E_CHIPID_CV1821A;
	case 0x1812C:
		return E_CHIPID_CV1822A;
	case 0x1811F:
		return E_CHIPID_CV1823A;
	case 0x1812F:
		return E_CHIPID_CV1825A;
	case 0x1813F:
		return E_CHIPID_CV1826A;
	}
#endif
	//cv181x default cv1810c
	return E_CHIPID_CV1810C;
}
#endif

unsigned int cvi_base_read_chip_id(void)
{
#ifndef FPGA_EARLY_PORTING_CHIP_ID

	unsigned int chip_id = ioread32(top_base + GP_REG3_OFFSET);

	CVI_TRACE_BASE(CVI_BASE_DBG_DEBUG, "chip_id=0x%x\n", chip_id);

	switch (chip_id) {
	case 0x1821:
		return E_CHIPID_CV1821;
	case 0x1822:
		return E_CHIPID_CV1822;
	case 0x1826:
		return E_CHIPID_CV1826;
	case 0x1832:
		return E_CHIPID_CV1832;
	case 0x1838:
		return E_CHIPID_CV1838;
	case 0x1829:
		return E_CHIPID_CV1829;
	case 0x1820:
		return E_CHIPID_CV1820;
	case 0x1823:
		return E_CHIPID_CV1823;
	case 0x1825:
		return E_CHIPID_CV1825;
	case 0x1835:
		return E_CHIPID_CV1835;

	case 0x1810C:
	case 0x1811C:
	case 0x1812C:
	case 0x1811F:
	case 0x1812F:
	case 0x1813F:
		return _cvi_base_read_by_kernel(chip_id);

	// cv180x
	case 0x1800B:
		return E_CHIPID_CV1800B;
	case 0x1801B:
		return E_CHIPID_CV1801B;
	case 0x1800C:
		return E_CHIPID_CV1800C;
	case 0x1801C:
		return E_CHIPID_CV1801C;

	//default cv1835
	default:
		return E_CHIPID_CV1835;
	}
#else
	return FPGA_EARLY_PORTING_CHIP_ID;
#endif
}
EXPORT_SYMBOL_GPL(cvi_base_read_chip_id);

unsigned int cvi_base_read_chip_version(void)
{
	unsigned int chip_version = 0;

	chip_version = ioread32(top_base);

	CVI_TRACE_BASE(CVI_BASE_DBG_DEBUG, "chip_version=0x%x\n", chip_version);

	switch (chip_version) {
	case 0x18802000:
	case 0x18220000:
	case 0x18100000:
		return E_CHIPVERSION_U01;
	case 0x18802001:
	case 0x18220001:
	case 0x18100001:
		return E_CHIPVERSION_U02;
	default:
		return E_CHIPVERSION_U01;
	}
}
EXPORT_SYMBOL_GPL(cvi_base_read_chip_version);

unsigned int cvi_base_read_chip_pwr_on_reason(void)
{
	unsigned int reason = 0;

	reason = ioread32(rtc_base + RTC_ST_ON_REASON);

	CVI_TRACE_BASE(CVI_BASE_DBG_DEBUG, "pwr on reason = 0x%x\n", reason);

	switch (reason) {
	case 0x800d0000:
	case 0x800f0000:
		return E_CHIP_PWR_ON_COLDBOOT;
	case 0x880d0003:
	case 0x880f0003:
		return E_CHIP_PWR_ON_WDT;
	case 0x80050009:
	case 0x800f0009:
		return E_CHIP_PWR_ON_SUSPEND;
	case 0x840d0003:
	case 0x840f0003:
		return E_CHIP_PWR_ON_WARM_RST;
	default:
		return E_CHIP_PWR_ON_COLDBOOT;
	}
}
EXPORT_SYMBOL_GPL(cvi_base_read_chip_pwr_on_reason);

static int base_open(struct inode *inode, struct file *filp)
{
	struct base_device *ndev = container_of(filp->private_data, struct base_device, miscdev);
	struct base_state *ps;
	int ret;
	UNUSED(inode);

	if (!ndev) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "cannot find base private data\n");
		return -ENODEV;
	}
	ps = kzalloc(sizeof(struct base_state), GFP_KERNEL);
	if (!ps) {
		ret = -ENOMEM;
		goto out_free_ps;
	}

	device_lock(ndev->miscdev.this_device);
	ps->ndev = ndev;
	INIT_LIST_HEAD(&ps->list);
	ps->state_pid = get_pid(task_pid(current));
	ps->cred = get_current_cred();
	security_task_getsecid(current, &ps->secid);
	/* memory barrier in smp case. */
	smp_wmb();
	/* replace the private data with base state */
	filp->private_data = ps;
	list_add_tail(&ps->list, &ndev->ps_list);

	device_unlock(ndev->miscdev.this_device);
	CVI_TRACE_BASE(CVI_BASE_DBG_DEBUG, "base open ok\n");

	return 0;

 out_free_ps:
	kfree(ps);
	return ret;
}

static int base_release(struct inode *inode, struct file *filp)
{
	struct base_state *ps = filp->private_data;
	struct base_device *ndev = ps->ndev;
	UNUSED(inode);

	if (!ndev) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "cannot find base private data\n");
		return -ENODEV;
	}
	device_lock(ndev->miscdev.this_device);
	list_del_init(&ps->list);
	device_unlock(ndev->miscdev.this_device);
	put_pid(ps->state_pid);
	put_cred(ps->cred);
	kfree(ps);

	return 0;
}

static void signal_state_change(struct base_device *ndev, enum base_state_e state)
{
	struct base_state *ps;
	struct siginfo sinfo;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	sigval_t addr;
#endif

	spin_lock(&ndev->lock);
	ndev->state = state;
	list_for_each_entry(ps, &ndev->ps_list, list) {
		if (ps->state_signr) {
			memset(&sinfo, 0, sizeof(sinfo));
			sinfo.si_signo = ps->state_signr;
			sinfo.si_errno = 0;
			sinfo.si_code = SI_ASYNCIO;
			sinfo.si_addr = ps->state_context;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
			addr.sival_ptr = sinfo.si_addr;
			kill_pid_usb_asyncio(ps->state_signr, sinfo.si_errno, addr,
				ps->state_pid, ps->cred);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
			kill_pid_info_as_cred(ps->state_signr, &sinfo,
				ps->state_pid, ps->cred);
#else
			kill_pid_info_as_cred(ps->state_signr, &sinfo,
				ps->state_pid, ps->cred, ps->secid);
#endif
		}
	}
	spin_unlock(&ndev->lock);
}

static int base_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct base_state *ps = filp->private_data;
	struct base_device *ndev = ps->ndev;
	unsigned long vm_start = vma->vm_start;
	unsigned int vm_size = vma->vm_end - vma->vm_start;
	unsigned int offset = vma->vm_pgoff << PAGE_SHIFT;
	void *pos = ndev->shared_mem;

	if ((vm_size + offset) > BASE_SHARE_MEM_SIZE)
		return -EINVAL;

	while (vm_size > 0) {
		if (remap_pfn_range(vma, vm_start, virt_to_pfn(pos), PAGE_SIZE, vma->vm_page_prot))
			return -EAGAIN;
		CVI_TRACE_BASE(CVI_BASE_DBG_DEBUG, "mmap vir(%p) phys(%#llx)\n", pos, (u64)virt_to_phys((void *) pos));
		vm_start += PAGE_SIZE;
		pos += PAGE_SIZE;
		vm_size -= PAGE_SIZE;
	}

	return 0;
}

static long base_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct base_state *ps = filp->private_data;
	struct base_device *ndev = ps->ndev;
	long ret = 0;

	switch (cmd) {
	case IOCTL_READ_CHIP_ID: {
		unsigned long chip_id = 0;

		chip_id = cvi_base_read_chip_id();
		if (copy_to_user((uint32_t *) arg, &chip_id, sizeof(unsigned int)))
			return -EFAULT;
		break;
	}
	case IOCTL_READ_CHIP_VERSION: {
		unsigned long chip_version = 0;

		chip_version = cvi_base_read_chip_version();
		if (copy_to_user((uint32_t *) arg, &chip_version, sizeof(unsigned int)))
			return -EFAULT;
		break;
	}
	case IOCTL_READ_CHIP_PWR_ON_REASON: {
		unsigned long reason = 0;

		reason = cvi_base_read_chip_pwr_on_reason();
		if (copy_to_user((uint32_t *) arg, &reason, sizeof(unsigned int)))
			return -EFAULT;
		break;
	}
	case IOCTL_STATESIG: {
		struct base_statesignal ds;

		if (copy_from_user(&ds, (void __user *)arg, sizeof(ds)))
			return -EFAULT;
		ps->state_signr = ds.signr;
		ps->state_context = ds.context;
		ndev->sig_hook = 1;
		break;
	}
#ifdef CONFIG_COMPAT
	case IOCTL_STATESIG32: {
		struct base_statesignal32 ds;

		if (copy_from_user(&ds, arg, sizeof(ds)))
			return -EFAULT;
		ps->state_signr = ds.signr;
		ps->state_context = compat_ptr(ds.context);
		ndev->sig_hook = 1;
		break;
	}
#endif
	case IOCTL_READ_STATE: {
		unsigned int state;

		spin_lock(&ndev->lock);
		state = ndev->state;
		spin_unlock(&ndev->lock);
		if (copy_to_user((void __user *)arg, &state, sizeof(unsigned int)))
			return -EFAULT;
		break;
	}
	case IOCTL_USER_SUSPEND_DONE: {
		spin_lock(&ndev->lock);
		ndev->state = BASE_STATE_SUSPEND;
		spin_unlock(&ndev->lock);
		/* memory barrier in smp case. */
		smp_wmb();
		/* release the notifier */
		complete(&ndev->done);
		break;
	}
	case IOCTL_USER_RESUME_DONE: {
		spin_lock(&ndev->lock);
		ndev->state = BASE_STATE_NORMAL;
		spin_unlock(&ndev->lock);
		/* memory barrier in smp case. */
		smp_wmb();
		/* release the notifier */
		complete(&ndev->done);
		break;
	}
	case IOCTL_VB_CMD: {
		struct vb_ext_control p;

		if (copy_from_user(&p, (void __user *)arg, sizeof(struct vb_ext_control)))
			return -EINVAL;

		ret = vb_ctrl(&p);
		if (copy_to_user((void __user *)arg, &p, sizeof(struct vb_ext_control)))
			return -EINVAL;
		break;
	}
	default:
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "Not support functions");
		return -ENOTTY;
	}
	return ret;
}

#ifdef CONFIG_COMPAT
static long compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static const struct file_operations base_fops = {
	.owner = THIS_MODULE,
	.open = base_open,
	.release = base_release,
	.mmap = base_mmap,
	.unlocked_ioctl = base_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = compat_ptr_ioctl,
#endif
};

static int _register_dev(struct base_device *ndev)
{
	int rc;

	ndev->miscdev.minor = MISC_DYNAMIC_MINOR;
	ndev->miscdev.name = BASE_DEV_NAME;
	ndev->miscdev.fops = &base_fops;

	rc = misc_register(&ndev->miscdev);
	if (rc) {
		dev_err(ndev->dev, "cvi_base: failed to register misc device.\n");
		return rc;
	}

	return 0;
}

#define POWER_PROC_NAME			"power"
#define POWER_PROC_PERMS		(0644)

static u8 *sel_state[] = {
	"suspend",
	"resume",
};

static int proc_power_show(struct seq_file *m, void *v)
{
	UNUSED(v);
	seq_puts(m, "suspend resume\n");

	return 0;
}

static int set_power_hdler(struct base_device *ndev, char const *input)
{
	u32 num;
	u8 str[80] = {0};
	u8 t = 0;
	u8 i, n;
	u8 *p;

	num = sscanf(input, "%s", str);
	if (num > 1) {
		return -EINVAL;
	}

	/* convert to lower case for following type compare */
	p = str;
	for (; *p; ++p)
		*p = tolower(*p);
	n = ARRAY_SIZE(sel_state);
	for (i = 0; i < n; i++) {
		if (!strcmp(str, sel_state[i])) {
			t = i;
			break;
		}
	}
	if (i == n)
		return -EINVAL;

	signal_state_change(ndev, t ? BASE_STATE_RESUME : BASE_STATE_SUSPEND_PREPARE);

	return 0;
}

static ssize_t power_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	struct base_device *ndev = PDE_DATA(file_inode(file));
	UNUSED(ppos);

	set_power_hdler(ndev, user_buf);

	return count;
}

static int proc_power_open(struct inode *inode, struct file *file)
{
	struct base_device *ndev = PDE_DATA(inode);

	return single_open(file, proc_power_show, ndev);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops _power_proc_fops = {
	.proc_open = proc_power_open,
	.proc_read = seq_read,
	.proc_write = power_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations _power_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= proc_power_open,
	.read		= seq_read,
	.write		= power_proc_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#endif

static int power_proc_init(struct proc_dir_entry *_proc_dir, void *ndev)
{
	int rc = 0;

	/* create the /proc file */
	if (proc_create_data(POWER_PROC_NAME, POWER_PROC_PERMS, _proc_dir, &_power_proc_fops, ndev) == NULL) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "power proc creation failed\n");
		rc = -1;
	}

	return rc;
}

static int power_proc_remove(struct proc_dir_entry *_proc_dir)
{
	remove_proc_entry(POWER_PROC_NAME, _proc_dir);

	return 0;
}

static int base_suspend_user(struct base_device *ndev)
{
	int ret;

	if (!ndev->sig_hook)
		return 0;
	signal_state_change(ndev, BASE_STATE_SUSPEND_PREPARE);
	ret = wait_for_completion_timeout(&ndev->done, usecs_to_jiffies(500000));
	if (ret < 0) {
		CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "user space suspend expired, state = %d\n", ndev->state);
		ndev->state = BASE_STATE_SUSPEND;
		return 0;
	}
	if (ndev->state != BASE_STATE_SUSPEND) {
		CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "expect suspend(2) but state = %d\n", ndev->state);
		ndev->state = BASE_STATE_SUSPEND;
	}
	return 0;
}

static int base_resume_user(struct base_device *ndev)
{
	int ret;

	if (!ndev->sig_hook)
		return 0;
	signal_state_change(ndev, BASE_STATE_RESUME);
	ret = wait_for_completion_timeout(&ndev->done, usecs_to_jiffies(500000));
	if (ret < 0) {
		CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "user space resume expired, state = %d\n", ndev->state);
		ndev->state = BASE_STATE_NORMAL;
		return 0;
	}
	if (ndev->state != BASE_STATE_NORMAL) {
		CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "expect normal(0) but state = %d\n", ndev->state);
		ndev->state = BASE_STATE_NORMAL;
	}
	return 0;
}

static int base_pm_notif(struct notifier_block *b, unsigned long v, void *d)
{
	struct base_device *ndev = container_of(b, struct base_device, notifier);
	UNUSED(d);

	CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "pm notif %lu\n", v);
	switch (v) {
	case PM_SUSPEND_PREPARE:
	case PM_HIBERNATION_PREPARE:
	case PM_RESTORE_PREPARE:
		CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "suspending displays\n");
		return base_suspend_user(ndev);

	case PM_POST_SUSPEND:
	case PM_POST_HIBERNATION:
	case PM_POST_RESTORE:
		CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "resuming displays\n");
		return base_resume_user(ndev);

	default:
		return 0;
	}
}

static int base_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct base_device *ndev;
	struct resource *res;
	void __iomem *reg_base;
	int ret;

	CVI_TRACE_BASE(CVI_BASE_DBG_DEBUG, "start\n");

	ndev = devm_kzalloc(&pdev->dev, sizeof(*ndev), GFP_KERNEL);
	if (!ndev)
		return -ENOMEM;

	ndev->shared_mem = kzalloc(BASE_SHARE_MEM_SIZE, GFP_KERNEL);
	if (!ndev->shared_mem)
		return -ENOMEM;

	proc_dir = proc_mkdir("cvitek", NULL);
	if (vb_proc_init(proc_dir) < 0) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb proc init failed\n");
	}

	if (log_proc_init(proc_dir, ndev->shared_mem) < 0) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "log proc init failed\n");
	}

	if (sys_proc_init(proc_dir, ndev->shared_mem) < 0) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "sys proc init failed\n");
	}

	if (power_proc_init(proc_dir, ndev) < 0) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "power proc init failed\n");
	}

	spin_lock_init(&ndev->lock);
	INIT_LIST_HEAD(&ndev->ps_list);
	ndev->state = BASE_STATE_NORMAL;
	ndev->dev = dev;
	init_completion(&ndev->done);

	/* Get vip_sys base address */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	reg_base = devm_ioremap_resource(&pdev->dev, res);
	CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%p).\n",
			res->start, res->end, reg_base);
	if (IS_ERR(reg_base)) {
		ret = PTR_ERR(reg_base);
		return ret;
	}

	vip_set_base_addr(reg_base);

	vip_sys_set_offline(VIP_SYS_AXI_BUS_SC_TOP, true);
	vip_sys_set_offline(VIP_SYS_AXI_BUS_ISP_RAW, true);
	vip_sys_set_offline(VIP_SYS_AXI_BUS_ISP_YUV, true);

	ndev->notifier.notifier_call = base_pm_notif;
	register_pm_notifier(&ndev->notifier);
	ret = _register_dev(ndev);
	if (ret < 0) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "regsiter base chrdev error\n");
		return ret;
	}

	memset(base_m_cb, 0, sizeof(struct base_m_cb_info) * E_MODULE_BUTT);

	platform_set_drvdata(pdev, ndev);
	CVI_TRACE_BASE(CVI_BASE_DBG_DEBUG, "%s DONE\n", __func__);

	return 0;
}

static int base_remove(struct platform_device *pdev)
{
	struct base_device *ndev = platform_get_drvdata(pdev);

	unregister_pm_notifier(&ndev->notifier);
	vb_proc_remove(proc_dir);
	log_proc_remove(proc_dir);
	sys_proc_remove(proc_dir);
	power_proc_remove(proc_dir);
	proc_remove(proc_dir);
	proc_dir = NULL;
	kfree(ndev->shared_mem);
	ndev->shared_mem = NULL;

	misc_deregister(&ndev->miscdev);
	platform_set_drvdata(pdev, NULL);
	CVI_TRACE_BASE(CVI_BASE_DBG_DEBUG, "%s DONE\n", __func__);

	return 0;
}

static const struct of_device_id cvi_base_dt_match[] = { { .compatible = "cvitek,base" }, {} };

static struct platform_driver base_driver = {
	.probe = base_probe,
	.remove = base_remove,
	.driver = {
		.name = BASE_DEV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = cvi_base_dt_match,
	},
};

static void base_cleanup(void)
{
	class_remove_file(pbase_class, &class_attr_base_efuse_shadow);
	class_remove_file(pbase_class, &class_attr_base_efuse_prog);
	class_remove_file(pbase_class, &class_attr_base_uid);
	class_remove_file(pbase_class, &class_attr_base_rosc);
	class_destroy(pbase_class);
}

static int __init base_init(void)
{
	int rc;
	uint32_t chip_id;

	top_base = ioremap(TOP_BASE, TOP_REG_BANK_SIZE);

	pbase_class = class_create(THIS_MODULE, BASE_CLASS_NAME);
	if (IS_ERR(pbase_class)) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "create class failed\n");
		rc = PTR_ERR(pbase_class);
		goto cleanup;
	}

	rc = class_create_file(pbase_class, &class_attr_base_efuse_shadow);
	if (rc) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "base: can't create sysfs base_efuse_shadow file\n");
		goto cleanup;
	}

	rc = class_create_file(pbase_class, &class_attr_base_efuse_prog);
	if (rc) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "base: can't create sysfs base_efuse_prog) file\n");
		goto cleanup;
	}

	rc = class_create_file(pbase_class, &class_attr_base_uid);
	if (rc) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "base: can't create sysfs base_uid file\n");
		goto cleanup;
	}

	rc = class_create_file(pbase_class, &class_attr_base_rosc);
	if (rc) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "base: can't create sysfs base_rosc file\n");
		goto cleanup;
	}

	rc = platform_driver_register(&base_driver);
	chip_id = cvi_base_read_chip_id();
	pr_notice("CVITEK CHIP ID = %d\n", chip_id);

	sys_save_modules_cb(&base_m_cb[0]);

	if (IS_CHIP_CV182X(chip_id) || IS_CHIP_CV181X(chip_id))
		rtc_base = ioremap(CV182X_RTC_BASE, RTC_REG_BANK_SIZE);
	else
		rtc_base = ioremap(CV183X_RTC_BASE, RTC_REG_BANK_SIZE);

	return 0;

cleanup:
	base_cleanup();

	return rc;
}

static void __exit base_exit(void)
{
	platform_driver_unregister(&base_driver);
	vb_cleanup();
	base_cleanup();

	iounmap(top_base);
	iounmap(rtc_base);
}

/* sensor cmm extern function. */
enum vip_sys_cmm {
	VIP_CMM_I2C = 0,
	VIP_CMM_SSP,
	VIP_CMM_BUTT,
};

struct vip_sys_cmm_ops {
	long (*cb)(void *hdlr, unsigned int cmd, void *arg);
};

struct vip_sys_cmm_dev {
	enum vip_sys_cmm		cmm_type;
	void				*hdlr;
	struct vip_sys_cmm_ops		ops;
};
static struct vip_sys_cmm_dev cmm_ssp;
static struct vip_sys_cmm_dev cmm_i2c;

int vip_sys_register_cmm_cb(unsigned long cmm, void *hdlr, void *cb)
{
	struct vip_sys_cmm_dev *cmm_dev;

	if ((cmm >= VIP_CMM_BUTT) || !hdlr || !cb)
		return -1;

	cmm_dev = (cmm == VIP_CMM_I2C) ? &cmm_i2c : &cmm_ssp;

	cmm_dev->cmm_type = cmm;
	cmm_dev->hdlr = hdlr;
	cmm_dev->ops.cb = cb;

	return 0;
}
EXPORT_SYMBOL_GPL(vip_sys_register_cmm_cb);

int vip_sys_cmm_cb_i2c(unsigned int cmd, void *arg)
{
	struct vip_sys_cmm_dev *cmm_dev = &cmm_i2c;

	if (cmm_dev->cmm_type != VIP_CMM_I2C)
		return -1;

	return (cmm_dev->ops.cb) ? cmm_dev->ops.cb(cmm_dev->hdlr, cmd, arg) : (-1);
}
EXPORT_SYMBOL_GPL(vip_sys_cmm_cb_i2c);

int vip_sys_cmm_cb_ssp(unsigned int cmd, void *arg)
{
	struct vip_sys_cmm_dev *cmm_dev = &cmm_ssp;

	if (cmm_dev->cmm_type != VIP_CMM_SSP)
		return -1;

	return (cmm_dev->ops.cb) ? cmm_dev->ops.cb(cmm_dev->hdlr, cmd, arg) : (-1);
}
EXPORT_SYMBOL_GPL(vip_sys_cmm_cb_ssp);

int base_rm_module_cb(enum ENUM_MODULES_ID module_id)
{
	if (module_id < 0 || module_id >= E_MODULE_BUTT) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "base rm cb error: wrong module_id\n");
		return -1;
	}

	base_m_cb[module_id].dev = NULL;
	base_m_cb[module_id].cb  = NULL;

	return 0;
}
EXPORT_SYMBOL_GPL(base_rm_module_cb);

int base_reg_module_cb(struct base_m_cb_info *cb_info)
{
	if (!cb_info || !cb_info->dev || !cb_info->cb) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "base reg cb error: no data\n");
		return -1;
	}

	if (cb_info->module_id < 0 || cb_info->module_id >= E_MODULE_BUTT) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "base reg cb error: wrong module_id\n");
		return -1;
	}

	base_m_cb[cb_info->module_id] = *cb_info;

	return 0;
}
EXPORT_SYMBOL_GPL(base_reg_module_cb);

int base_exe_module_cb(struct base_exe_m_cb *exe_cb)
{
	struct base_m_cb_info *cb_info;

	if (exe_cb->caller < 0 || exe_cb->caller >= E_MODULE_BUTT) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "base exe cb error: wrong caller\n");
		return -1;
	}

	if (exe_cb->callee < 0 || exe_cb->callee >= E_MODULE_BUTT) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "base exe cb error: wrong callee\n");
		return -1;
	}

	cb_info = &base_m_cb[exe_cb->callee];

	if (!cb_info->cb) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "base exe cb error: cb of callee(%s) is null, caller(%s)\n",
			IDTOSTR(exe_cb->callee), IDTOSTR(exe_cb->caller));
		return -1;
	}

	return cb_info->cb(cb_info->dev, exe_cb->caller, exe_cb->cmd_id, exe_cb->data);
}
EXPORT_SYMBOL_GPL(base_exe_module_cb);


MODULE_DESCRIPTION("Cvitek base driver");
MODULE_LICENSE("GPL");
module_init(base_init);
module_exit(base_exit);
