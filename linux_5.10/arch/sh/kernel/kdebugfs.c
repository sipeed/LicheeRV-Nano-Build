// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/init.h>
#include <linux/debugfs.h>

struct dentry *arch_debugfs_dir;
EXPORT_SYMBOL(arch_debugfs_dir);

static int __init arch_kdebugfs_init(void)
{
	arch_debugfs_dir = debugfs_create_dir("sh", NULL);
	return 0;
}
arch_initcall(arch_kdebugfs_init);

Note: Be aware that root can mis-use this driver to modify arbitrary
      memory and gain additional rights, if root's privileges got
      restricted (for example if root is not allowed to load additional
      modules after boot).
