#ifdef DRV_TEST
#include <linux/types.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include "sys.h"
#include "sys_context.h"

static struct proc_dir_entry *sys_test_proc_dir;

static uint32_t sys_test_ion(void)
{
	uint64_t p_start = 0, p_start1 = 0, p_start2 = 0, p_start3 = 0;
	uint32_t ret = 0, i;
	uint32_t len = 0x10000;
	uint32_t bytelen = len >> 2;
	uint32_t *ion_v = NULL, *ion_v1 = NULL, *ion_v2 = NULL, *ion_v3 = NULL;

	ret = sys_ion_alloc(&p_start, (void *)&ion_v, "sys_test_ion", len, true);
	pr_err("sys_ion_alloc() ret=%d, p_start=0x%llx\n", ret, p_start);

	ret = sys_ion_alloc(&p_start1, (void *)&ion_v1, "sys_test_ion1", len, true);
	pr_err("sys_ion_alloc() ret=%d, p_start1=0x%llx\n", ret, p_start1);

	ret = sys_ion_alloc(&p_start2, (void *)&ion_v2, "sys_test_ion2", len, true);
	pr_err("sys_ion_alloc() ret=%d, p_start2=0x%llx\n", ret, p_start2);

	ret = sys_ion_alloc(&p_start3, (void *)&ion_v3, "sys_test_ion3", len, true);
	pr_err("sys_ion_alloc() ret=%d, p_start3=0x%llx\n", ret, p_start3);


	sys_cache_invalidate(p_start, ion_v, len);
	for (i = 0; i < bytelen; ++i) {
		ion_v[i] = i;
	}
	sys_cache_flush(p_start, ion_v, len);
	for (i = 0; i < bytelen; ++i) {
		if (ion_v[i] != i)
			pr_err("sys_test_ion() ion_v[%d]!=%d\n", ion_v[i], i);
	}

	sys_cache_invalidate(p_start, ion_v, len);
	for (i = 0; i < bytelen; ++i) {
		ion_v[i] = i;
	}
	sys_cache_flush(p_start, ion_v, len);
	for (i = 0; i < bytelen; ++i) {
		if (ion_v[i] != i)
			pr_err("sys_test_ion() ion_v[%d]!=%d\n", ion_v[i], i);
	}

	sys_cache_invalidate(p_start, ion_v, len);
	for (i = 0; i < bytelen; ++i) {
		ion_v[i] = i;
	}
	sys_cache_flush(p_start, ion_v, len);
	for (i = 0; i < bytelen; ++i) {
		if (ion_v[i] != i)
			pr_err("sys_test_ion() ion_v[%d]!=%d\n", ion_v[i], i);
	}

	ret = sys_ion_free(p_start);
	pr_err("sys_ion_free() ret=%d, p_start=0x%llx\n", ret, p_start);

	ret = sys_ion_free(p_start1);
	pr_err("sys_ion_free() ret=%d, p_start1=0x%llx\n", ret, p_start);

	ret = sys_ion_free(p_start2);
	pr_err("sys_ion_free() ret=%d, p_start2=0x%llx\n", ret, p_start);

	ret = sys_ion_free(p_start3);
	pr_err("sys_ion_free() ret=%d, p_start3=0x%llx\n", ret, p_start);

	return 0;
}

static int sys_test_proc_show(struct seq_file *m, void *v)
{
	return 0;
}

static int sys_test_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sys_test_proc_show, PDE_DATA(inode));
}

static ssize_t sys_test_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	uint32_t input_param = 0;

	if (kstrtouint_from_user(user_buf, count, 0, &input_param)) {
		pr_err("input parameter incorrect\n");
		return count;
	}

	//reset related info
	pr_err("input_param=%d\n", input_param);

	switch (input_param) {
	case 100:
		sys_test_ion();
		break;
	}

	return count;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops sys_test_proc_ops = {
	.proc_open = sys_test_proc_open,
	.proc_read = seq_read,
	.proc_write = sys_test_proc_write,
	.proc_release = single_release,
};
#else
static const struct file_operations sys_test_proc_ops = {
	.owner = THIS_MODULE,
	.open = sys_test_proc_open,
	.read = seq_read,
	.write = sys_test_proc_write,
	.release = single_release,
};
#endif

int32_t sys_test_proc_init(void)
{
	if (proc_create("sys_test_sel", 0644, sys_test_proc_dir, &sys_test_proc_ops) == NULL)
		pr_err("sys_test_proc_init() failed\n");

	return 0;
}

int32_t sys_test_proc_deinit(void)
{
	remove_proc_entry("sys_test_sel", sys_test_proc_dir);
	proc_remove(sys_test_proc_dir);
	return 0;
}

#endif
