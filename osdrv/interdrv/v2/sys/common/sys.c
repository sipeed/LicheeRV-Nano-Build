#include <linux/dma-buf.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <asm/cacheflush.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#include <linux/dma-map-ops.h>
#endif

#include "sys_context.h"
#include "sys.h"
#include <linux/sys_uapi.h>
#include <linux/cvi_comm_sys.h>
#include <linux/cvi_defines.h>
#include <base_cb.h>
#include <vi_cb.h>
#include <vpss_cb.h>

enum enum_cache_op {
	enum_cache_op_invalid,
	enum_cache_op_flush,
};

#define CVI_SYS_DEV_NAME   "cvi-sys"
#define CVI_SYS_CLASS_NAME "cvi-sys"

struct cvi_sys_device {
	struct device *dev;
	struct miscdevice miscdev;

	struct mutex dev_lock;
	spinlock_t close_lock;
	int use_count;
};

static struct base_m_cb_info *sys_m_cb;

#ifdef DRV_TEST
extern int32_t sys_test_proc_init(void);
extern int32_t sys_test_proc_deinit(void);
#endif

static int ion_debug_alloc_free;
module_param(ion_debug_alloc_free, int, 0644);

static int32_t _sys_ion_alloc_nofd(uint64_t *addr_p, void **addr_v, uint32_t u32Len,
	uint32_t is_cached, uint8_t *name)
{
	int32_t ret = 0;
	struct ion_buffer *ionbuf;
	uint8_t *owner_name = NULL;
	void *vmap_addr = NULL;
	struct mem_mapping mem_info;

	ionbuf = cvi_ion_alloc_nofd(ION_HEAP_TYPE_CARVEOUT, u32Len, is_cached);
	if (IS_ERR(ionbuf)) {
		pr_err("ion allocated len=0x%x failed\n", u32Len);
		return -ENOMEM;
	}

	owner_name = vmalloc(MAX_ION_BUFFER_NAME);
	if (name)
		strncpy(owner_name, name, MAX_ION_BUFFER_NAME);
	else
		strncpy(owner_name, "anonymous", MAX_ION_BUFFER_NAME);

	ionbuf->name = owner_name;

	ret = ion_buf_begin_cpu_access(ionbuf);
	if (ret < 0) {
		pr_err("cvi_ion_alloc() ion_buf_begin_cpu_access failed\n");
		cvi_ion_free_nofd(ionbuf);
		return ret;
	}

	vmap_addr = ionbuf->vaddr;
	pr_debug("_sys_ion_alloc_nofd v=%p\n", vmap_addr);
	if (IS_ERR(vmap_addr)) {
		ret = -EINVAL;
		ion_buf_end_cpu_access(ionbuf);
		cvi_ion_free_nofd(ionbuf);
		return ret;
	}

	//push into memory manager
	mem_info.dmabuf = NULL;
	mem_info.dmabuf_fd = -1;
	mem_info.vir_addr = vmap_addr;
	mem_info.phy_addr = ionbuf->paddr;
	mem_info.fd_pid = current->pid;
	mem_info.ionbuf = ionbuf;
	if (sys_ctx_mem_put(&mem_info)) {
		pr_err("allocate mm put failed\n");
		ion_buf_end_cpu_access(ionbuf);
		cvi_ion_free_nofd(ionbuf);
		return -ENOMEM;
	}

	if (ion_debug_alloc_free) {
		pr_info("%s: ionbuf->name=%s\n", __func__, ionbuf->name);
		pr_info("%s: mem_info.dmabuf=%p\n", __func__, mem_info.dmabuf);
		pr_info("%s: mem_info.dmabuf_fd=%d\n", __func__, mem_info.dmabuf_fd);
		pr_info("%s: mem_info.vir_addr=%p\n", __func__, mem_info.vir_addr);
		pr_info("%s: mem_info.phy_addr=0x%llx\n", __func__, mem_info.phy_addr);
		pr_info("%s: mem_info.fd_pid=%d\n", __func__, mem_info.fd_pid);
		pr_info("%s: current->pid=%d\n", __func__, current->pid);
		pr_info("%s: current->comm=%s\n", __func__, current->comm);
	}
	*addr_p = ionbuf->paddr;
	*addr_v = vmap_addr;
	return ret;
}

static int32_t _sys_ion_alloc(int32_t *p_fd, uint64_t *addr_p, void **addr_v, uint32_t u32Len,
	uint32_t is_cached, uint8_t *name)
{
	int32_t dmabuf_fd = 0, ret = 0;
	struct dma_buf *dmabuf;
	struct ion_buffer *ionbuf;
	uint8_t *owner_name = NULL;
	void *vmap_addr = NULL;
	struct mem_mapping mem_info;

	dmabuf_fd = cvi_ion_alloc(ION_HEAP_TYPE_CARVEOUT, u32Len, is_cached);
	if (dmabuf_fd < 0) {
		pr_err("ion allocated len=0x%x failed\n", u32Len);
		return -ENOMEM;
	}

	dmabuf = dma_buf_get(dmabuf_fd);
	if (!dmabuf) {
		pr_err("allocated get dmabuf failed\n");
		cvi_ion_free(current->pid, dmabuf_fd);
		return -ENOMEM;
	}

	ionbuf = (struct ion_buffer *)dmabuf->priv;
	owner_name = vmalloc(MAX_ION_BUFFER_NAME);
	if (name)
		strncpy(owner_name, name, MAX_ION_BUFFER_NAME);
	else
		strncpy(owner_name, "anonymous", MAX_ION_BUFFER_NAME);

	ionbuf->name = owner_name;

	ret = dma_buf_begin_cpu_access(dmabuf, DMA_TO_DEVICE);
	if (ret < 0) {
		pr_err("cvi_ion_alloc() dma_buf_begin_cpu_access failed\n");
		dma_buf_put(dmabuf);
		cvi_ion_free(current->pid, dmabuf_fd);
		return ret;
	}

	vmap_addr = ionbuf->vaddr;
	pr_debug("_sys_ion_alloc v=%p\n", vmap_addr);
	if (IS_ERR(vmap_addr)) {
		dma_buf_end_cpu_access(dmabuf, DMA_TO_DEVICE);
		dma_buf_put(dmabuf);
		cvi_ion_free(current->pid, dmabuf_fd);
		ret = -EINVAL;
		return ret;
	}

	//push into memory manager
	mem_info.dmabuf = (void *)dmabuf;
	mem_info.dmabuf_fd = dmabuf_fd;
	mem_info.vir_addr = vmap_addr;
	mem_info.phy_addr = ionbuf->paddr;
	mem_info.fd_pid = current->pid;
	if (sys_ctx_mem_put(&mem_info)) {
		pr_err("allocate mm put failed\n");
		dma_buf_end_cpu_access(dmabuf, DMA_TO_DEVICE);
		dma_buf_put(dmabuf);
		cvi_ion_free(current->pid, dmabuf_fd);
		return -ENOMEM;
	}

	if (ion_debug_alloc_free) {
		pr_info("%s: ionbuf->name=%s\n", __func__, ionbuf->name);
		pr_info("%s: mem_info.dmabuf=%p\n", __func__, mem_info.dmabuf);
		pr_info("%s: mem_info.dmabuf_fd=%d\n", __func__, mem_info.dmabuf_fd);
		pr_info("%s: mem_info.vir_addr=%p\n", __func__, mem_info.vir_addr);
		pr_info("%s: mem_info.phy_addr=0x%llx\n", __func__, mem_info.phy_addr);
		pr_info("%s: mem_info.fd_pid=%d\n", __func__, mem_info.fd_pid);
		pr_info("%s: current->pid=%d\n", __func__, current->pid);
		pr_info("%s: current->comm=%s\n", __func__, current->comm);
	}
	*p_fd = dmabuf_fd;
	*addr_p = ionbuf->paddr;
	*addr_v = vmap_addr;
	return ret;
}

static int32_t _sys_ion_free(uint64_t addr_p)
{
	struct mem_mapping mem_info;
	struct ion_buffer *ionbuf;
	struct dma_buf *dmabuf;

	//get from memory manager
	memset(&mem_info, 0, sizeof(struct mem_mapping));
	mem_info.phy_addr = addr_p;
	if (sys_ctx_mem_get(&mem_info)) {
		pr_err("dmabuf_fd get failed, addr:0x%llx\n", addr_p);
		return -ENOMEM;
	}

	dmabuf = (struct dma_buf *)(mem_info.dmabuf);
#if 1
	ionbuf = (struct ion_buffer *)dmabuf->priv;
	if (ionbuf->name && ion_debug_alloc_free) {
		pr_info("%s: ionbuf->name: %s\n", __func__, ionbuf->name);
		vfree(ionbuf->name);
		ionbuf->name = NULL;
	}
#endif

	if (ion_debug_alloc_free) {
		pr_info("%s: mem_info.dmabuf=%p\n", __func__, mem_info.dmabuf);
		pr_info("%s: mem_info.dmabuf_fd=%d\n", __func__, mem_info.dmabuf_fd);
		pr_info("%s: mem_info.vir_addr=%p\n", __func__, mem_info.vir_addr);
		pr_info("%s: mem_info.phy_addr=0x%llx\n", __func__, mem_info.phy_addr);
		pr_info("%s: mem_info.fd_pid=%d\n", __func__, mem_info.fd_pid);
		pr_info("%s: current->pid=%d\n", __func__, current->pid);
		pr_info("%s: current->comm=%s\n", __func__, current->comm);
	}
	dma_buf_end_cpu_access(dmabuf, DMA_TO_DEVICE);
	dma_buf_put(dmabuf);

	cvi_ion_free(mem_info.fd_pid, mem_info.dmabuf_fd);
	return 0;
}

static int32_t _sys_ion_free_nofd(uint64_t addr_p)
{
	struct mem_mapping mem_info;
	struct ion_buffer *ionbuf;

	//get from memory manager
	memset(&mem_info, 0, sizeof(struct mem_mapping));
	mem_info.phy_addr = addr_p;
	if (sys_ctx_mem_get(&mem_info)) {
		pr_err("dmabuf_fd get failed, addr:0x%llx\n", addr_p);
		return -ENOMEM;
	}

	ionbuf = (struct ion_buffer *)(mem_info.ionbuf);
	pr_debug("%s: ionbuf->name: %s\n", __func__, ionbuf->name);

	if (ion_debug_alloc_free) {
		pr_info("%s: mem_info.vir_addr=%p\n", __func__, mem_info.vir_addr);
		pr_info("%s: mem_info.phy_addr=0x%llx\n", __func__, mem_info.phy_addr);
		pr_info("%s: mem_info.fd_pid=%d\n", __func__, mem_info.fd_pid);
		pr_info("%s: current->pid=%d\n", __func__, current->pid);
		pr_info("%s: current->comm=%s\n", __func__, current->comm);
	}

	if (ionbuf->name) {
		vfree(ionbuf->name);
		ionbuf->name = NULL;
	}

	ion_buf_end_cpu_access(ionbuf);
	cvi_ion_free_nofd(ionbuf);

	return 0;
}

static int32_t sys_init_user(struct cvi_sys_device *ndev, unsigned long arg)
{
	return 0;
}

static int32_t sys_exit_user(struct cvi_sys_device *ndev, unsigned long arg)
{
	return 0;
}

int32_t sys_init()
{
	sys_ctx_init();
#ifdef DRV_TEST
	sys_test_proc_init();
#endif
	return 0;
}

int32_t sys_exit()
{
#ifdef DRV_TEST
	sys_test_proc_deinit();
#endif
	return 0;
}

int32_t sys_ion_free(uint64_t u64PhyAddr)
{
	return _sys_ion_free(u64PhyAddr);
}
EXPORT_SYMBOL_GPL(sys_ion_free);

int32_t sys_ion_free_nofd(uint64_t u64PhyAddr)
{
	return _sys_ion_free_nofd(u64PhyAddr);
}
EXPORT_SYMBOL_GPL(sys_ion_free_nofd);

static int32_t sys_ion_free_user(struct cvi_sys_device *ndev, unsigned long arg)
{
	int32_t ret = 0;
	struct sys_ion_data ioctl_arg;

	ret = copy_from_user(&ioctl_arg,
			     (struct sys_ion_data __user *)arg,
			     sizeof(struct sys_ion_data));
	if (ret) {
		pr_err("copy_from_user failed, sys_ion_free_user\n");
		return ret;
	}

	return _sys_ion_free(ioctl_arg.addr_p);
}

int32_t sys_ion_alloc(uint64_t *p_paddr, void **pp_vaddr, uint8_t *buf_name, uint32_t buf_len, bool is_cached)
{
	int32_t dma_fd = 0;

	return _sys_ion_alloc(&dma_fd, p_paddr, pp_vaddr, buf_len, is_cached, buf_name);
}
EXPORT_SYMBOL_GPL(sys_ion_alloc);

int32_t sys_ion_alloc_nofd(uint64_t *p_paddr, void **pp_vaddr, uint8_t *buf_name, uint32_t buf_len, bool is_cached)
{
	return _sys_ion_alloc_nofd(p_paddr, pp_vaddr, buf_len, is_cached, buf_name);
}
EXPORT_SYMBOL_GPL(sys_ion_alloc_nofd);

static int32_t sys_ion_alloc_user(struct cvi_sys_device *ndev, unsigned long arg)
{
	int32_t ret = 0, dma_fd = 0;
	uint64_t addr_p = 0;
	void *addr_v = NULL;
	struct sys_ion_data ioctl_arg;

	ret = copy_from_user(&ioctl_arg,
			     (struct sys_ion_data __user *)arg,
			     sizeof(struct sys_ion_data));
	if (ret) {
		pr_err("copy_from_user failed, sys_ion_alloc_user\n");
		return ret;
	}

	ret = _sys_ion_alloc(&dma_fd, &addr_p, &addr_v,
					ioctl_arg.size, ioctl_arg.cached, ioctl_arg.name);
	if (ret < 0) {
		pr_err("_sys_ion_alloc failed\n");
		return ret;
	}

	ioctl_arg.addr_p = addr_p;
	ioctl_arg.dmabuf_fd = dma_fd;

	ret = copy_to_user((struct sys_ion_data __user *)arg,
			     &ioctl_arg,
			     sizeof(struct sys_ion_data));
	if (ret) {
		pr_err("copy_to_user fail, cvi_tpu_loadcmdbuf_sec\n");
		return ret;
	}

	return 0;
}

int32_t sys_cache_invalidate(uint64_t addr_p, void *addr_v, uint32_t u32Len)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)) && defined(__riscv)
	arch_sync_dma_for_device(addr_p, u32Len, DMA_FROM_DEVICE);
#else
	__dma_map_area(addr_v, u32Len, DMA_FROM_DEVICE);
#endif

	/*	*/
	smp_mb();
	return 0;
}
EXPORT_SYMBOL_GPL(sys_cache_invalidate);

int32_t sys_cache_flush(uint64_t addr_p, void *addr_v, uint32_t u32Len)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)) && defined(__riscv)
	arch_sync_dma_for_device(addr_p, u32Len, DMA_TO_DEVICE);
#else
	__dma_map_area(addr_v, u32Len, DMA_TO_DEVICE);
#endif

	/*  */
	smp_mb();
	return 0;
}
EXPORT_SYMBOL_GPL(sys_cache_flush);

static int32_t sys_cache_op_userv(unsigned long arg, enum enum_cache_op op_code)
{
	int ret = 0;
	struct sys_cache_op ioctl_arg;

	ret = copy_from_user(&ioctl_arg, (struct sys_cache_op __user *)arg, sizeof(struct sys_cache_op));
	if (ret) {
		pr_err("copy_from_user faile, sys_cache_op_userv\n");
		return ret;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)) && defined(__riscv)
	if (op_code == enum_cache_op_invalid)
		arch_sync_dma_for_device(ioctl_arg.addr_p, ioctl_arg.size, DMA_FROM_DEVICE);
	else if (op_code == enum_cache_op_flush)
		arch_sync_dma_for_device(ioctl_arg.addr_p, ioctl_arg.size, DMA_TO_DEVICE);
#else
	if (op_code == enum_cache_op_invalid)
		__dma_map_area(ioctl_arg.addr_v, ioctl_arg.size, DMA_FROM_DEVICE);
	else if (op_code == enum_cache_op_flush)
		__dma_map_area(ioctl_arg.addr_v, ioctl_arg.size, DMA_TO_DEVICE);
#endif
	/*	*/
	smp_mb();
	return 0;
}

uint32_t sys_get_chipid(void)
{
	struct sys_info *p_info = (struct sys_info *)sys_ctx_get_sysinfo();
	return p_info->chip_id;
}
EXPORT_SYMBOL_GPL(sys_get_chipid);

uint8_t *sys_get_version(void)
{
	struct sys_info *p_info = (struct sys_info *)sys_ctx_get_sysinfo();
	return p_info->version;
}

static int32_t sys_get_sysinfo(struct cvi_sys_device *ndev, unsigned long arg)
{
	int32_t ret = 0;
	struct sys_info *ioctl_arg;

	ioctl_arg = (struct sys_info *)sys_ctx_get_sysinfo();
	ret = copy_to_user((struct sys_info __user *)arg,
			     ioctl_arg,
			     sizeof(struct sys_info));
	if (ret) {
		pr_err("copy_to_user fail, sys_get_sysinfo\n");
		return ret;
	}
	return 0;
}

static int32_t sys_set_bind_cfg(struct cvi_sys_device *ndev, unsigned long arg)
{
	int32_t ret = 0;
	struct sys_bind_cfg ioctl_arg;

	ret = copy_from_user(&ioctl_arg,
			     (struct sys_bind_cfg __user *)arg,
			     sizeof(struct sys_bind_cfg));
	if (ret) {
		pr_err("copy_from_user failed, sys_set_mod_cfg\n");
		return ret;
	}

	if (ioctl_arg.is_bind)
		ret = sys_ctx_bind(&ioctl_arg.mmf_chn_src, &ioctl_arg.mmf_chn_dst);
	else
		ret = sys_ctx_unbind(&ioctl_arg.mmf_chn_src, &ioctl_arg.mmf_chn_dst);

	return ret;
}

static int32_t sys_get_bind_cfg(struct cvi_sys_device *ndev, unsigned long arg)
{
	int32_t ret = 0;
	struct sys_bind_cfg ioctl_arg;

	ret = copy_from_user(&ioctl_arg,
			     (struct sys_bind_cfg __user *)arg,
			     sizeof(struct sys_bind_cfg));
	if (ret) {
		pr_err("copy_from_user failed, sys_set_mod_cfg\n");
		return ret;
	}

	if (ioctl_arg.get_by_src)
		ret = sys_ctx_get_bindbysrc(&ioctl_arg.mmf_chn_src, &ioctl_arg.bind_dst);
	else
		ret = sys_ctx_get_bindbydst(&ioctl_arg.mmf_chn_dst, &ioctl_arg.mmf_chn_src);

	if (ret)
		pr_err("sys_ctx_getbind failed\n");

	ret = copy_to_user((struct sys_bind_cfg __user *)arg,
			     &ioctl_arg,
			     sizeof(struct sys_bind_cfg));

	if (ret)
		pr_err("copy_to_user fail, sys_get_bind_cfg\n");

	return ret;
}

int32_t sys_bind(MMF_CHN_S *pstSrcChn, MMF_CHN_S *pstDestChn)
{
	return sys_ctx_bind(pstSrcChn, pstDestChn);
}
EXPORT_SYMBOL_GPL(sys_bind);

int32_t sys_unbind(MMF_CHN_S *pstSrcChn, MMF_CHN_S *pstDestChn)
{
	return sys_ctx_unbind(pstSrcChn, pstDestChn);
}
EXPORT_SYMBOL_GPL(sys_unbind);

int32_t sys_ion_dump(void)
{
	return sys_ctx_mem_dump();
}
EXPORT_SYMBOL_GPL(sys_ion_dump);

VPSS_MODE_E sys_get_vpssmode(void)
{
	return sys_ctx_get_vpssmode();
}
EXPORT_SYMBOL_GPL(sys_get_vpssmode);

int32_t sys_get_bindbysrc(MMF_CHN_S *pstSrcChn, MMF_BIND_DEST_S *pstBindDest)
{
	return sys_ctx_get_bindbysrc(pstSrcChn, pstBindDest);
}
EXPORT_SYMBOL_GPL(sys_get_bindbysrc);

int32_t sys_get_bindbydst(MMF_CHN_S *pstDestChn, MMF_CHN_S *pstSrcChn)
{
	return sys_ctx_get_bindbydst(pstSrcChn, pstSrcChn);
}
EXPORT_SYMBOL_GPL(sys_get_bindbydst);

#define GENERATE_STRING(STRING) (#STRING),
static const char *const MOD_STRING[] = FOREACH_MOD(GENERATE_STRING);
const uint8_t *sys_get_modname(MOD_ID_E id)
{
	return (id < CVI_ID_BUTT) ? MOD_STRING[id] : "UNDEF";
}
EXPORT_SYMBOL_GPL(sys_get_modname);

void sys_save_modules_cb(void *base_m_cb)
{
	sys_m_cb = (struct base_m_cb_info *)base_m_cb;
}
EXPORT_SYMBOL_GPL(sys_save_modules_cb);

int _sys_exe_module_cb(struct base_exe_m_cb *exe_cb)
{
	struct base_m_cb_info *cb_info;

	if (exe_cb->caller < 0 || exe_cb->caller >= E_MODULE_BUTT) {
		pr_err("sys exe cb error: wrong caller\n");
		return -1;
	}

	if (exe_cb->callee < 0 || exe_cb->callee >= E_MODULE_BUTT) {
		pr_err("sys exe cb error: wrong callee\n");
		return -1;
	}

	if (!sys_m_cb) {
		pr_err("sys_m_cb/base_m_cb not ready yet\n");
		return -1;
	}

	cb_info = &sys_m_cb[exe_cb->callee];
	if (!cb_info->cb) {
		pr_err("sys exe cb error\n");
		return -1;
	}

	return cb_info->cb(cb_info->dev, exe_cb->caller, exe_cb->cmd_id, exe_cb->data);
}

static int _sys_call_cb(u32 m_id, u32 cmd_id, void *data)
{
	struct base_exe_m_cb exe_cb;

	exe_cb.callee = m_id;
	exe_cb.caller = E_MODULE_SYS;
	exe_cb.cmd_id = cmd_id;
	exe_cb.data   = (void *)data;

	return _sys_exe_module_cb(&exe_cb);
}

static long _sys_s_ctrl(struct cvi_sys_device *dev, struct sys_ext_control *p)
{
	u32 id = p->id;
	long rc = -EINVAL;
	struct sys_ctx_info *sys_ctx = NULL;

	sys_ctx = sys_get_ctx();

	mutex_lock(&dev->dev_lock);

	switch (id) {
	case SYS_IOCTL_SET_VIVPSSMODE:
	{
		VI_VPSS_MODE_S *stVIVPSSMode;

		stVIVPSSMode = &sys_ctx->mode_cfg.vivpss_mode;

		if (copy_from_user(stVIVPSSMode, p->ptr, sizeof(VI_VPSS_MODE_S)) != 0) {
			pr_err("SYS_IOCTL_SET_VIVPSSMODE, copy_from_user failed.\n");
			break;
		}

		if (_sys_call_cb(E_MODULE_VI, VI_CB_SET_VIVPSSMODE, stVIVPSSMode) != 0) {
			pr_err("VI_CB_SET_VIVPSSMODE failed\n");
			break;
		}

		if (_sys_call_cb(E_MODULE_VPSS, VPSS_CB_SET_VIVPSSMODE, stVIVPSSMode) != 0) {
			pr_err("VPSS_CB_SET_VIVPSSMODE failed\n");
			break;
		}

		rc = 0;
		break;
	}
	case SYS_IOCTL_SET_VPSSMODE:
	{
		VPSS_MODE_E enVPSSMode;

		sys_ctx->mode_cfg.vpss_mode.enMode = enVPSSMode = (VPSS_MODE_E)p->value;

		if (_sys_call_cb(E_MODULE_VPSS, VPSS_CB_SET_VPSSMODE, (void *)&enVPSSMode) != 0) {
			pr_err("VPSS_CB_SET_VPSSMODE failed\n");
			break;
		}

		rc = 0;
		break;
	}
	case SYS_IOCTL_SET_VPSSMODE_EX:
	{
		VPSS_MODE_S *stVPSSMode;

		stVPSSMode = &sys_ctx->mode_cfg.vpss_mode;

		if (copy_from_user(stVPSSMode, p->ptr, sizeof(VPSS_MODE_S)) != 0) {
			pr_err("SYS_IOCTL_SET_VPSSMODE_EX, copy_from_user failed.\n");
			break;
		}

		if (_sys_call_cb(E_MODULE_VPSS, VPSS_CB_SET_VPSSMODE_EX, (void *)stVPSSMode) != 0) {
			pr_err("VPSS_CB_SET_VPSSMODE_EX failed\n");
			break;
		}

		rc = 0;
		break;
	}
	case SYS_IOCTL_SET_SYS_INIT:
	{
		atomic_set(&sys_ctx->sys_inited, 1);

		rc = 0;
		break;
	}
	default:
		break;
	}

	mutex_unlock(&dev->dev_lock);

	return rc;
}

static long _sys_g_ctrl(struct cvi_sys_device *dev, struct sys_ext_control *p)
{
	u32 id = p->id;
	long rc = -EINVAL;
	struct sys_ctx_info *sys_ctx = NULL;

	sys_ctx = sys_get_ctx();

	mutex_lock(&dev->dev_lock);

	switch (id) {
	case SYS_IOCTL_GET_VIVPSSMODE:
	{
		if (copy_to_user(p->ptr, &sys_ctx->mode_cfg.vivpss_mode, sizeof(VI_VPSS_MODE_S)) != 0) {
			pr_err("SYS_IOCTL_GET_VIVPSSMODE, copy_to_user failed.\n");
			break;
		}

		rc = 0;
		break;
	}
	case SYS_IOCTL_GET_VPSSMODE:
	{
		p->value = sys_ctx->mode_cfg.vpss_mode.enMode;

		rc = 0;
		break;
	}
	case SYS_IOCTL_GET_VPSSMODE_EX:
	{
		if (copy_to_user(p->ptr, &sys_ctx->mode_cfg.vpss_mode, sizeof(VPSS_MODE_S)) != 0) {
			pr_err("SYS_IOCTL_GET_VPSSMODE_EX, copy_to_user failed.\n");
			break;
		}

		rc = 0;
		break;
	}
	case SYS_IOCTL_GET_SYS_INIT:
	{
		p->value = atomic_read(&sys_ctx->sys_inited);

		rc = 0;
		break;
	}
	default:
		break;
	}

	mutex_unlock(&dev->dev_lock);

	return rc;
}

static long _cvi_sys_sg_ctrl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct cvi_sys_device *dev = filp->private_data;
	long ret = 0;
	struct sys_ext_control p;

	if (copy_from_user(&p, (void __user *)arg, sizeof(struct sys_ext_control)))
		return -EINVAL;

	switch (cmd) {
	case SYS_IOC_S_CTRL:
		ret = _sys_s_ctrl(dev, &p);
		break;
	case SYS_IOC_G_CTRL:
		ret = _sys_g_ctrl(dev, &p);
		break;
	default:
		ret = -ENOTTY;
		break;
	}

	if (copy_to_user((void __user *)arg, &p, sizeof(struct sys_ext_control)))
		return -EINVAL;

	return ret;

}

static long cvi_sys_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct cvi_sys_device *ndev = filp->private_data;
	long ret = 0;

	switch (cmd) {
	case SYS_IOC_S_CTRL:
	case SYS_IOC_G_CTRL:
		ret = _cvi_sys_sg_ctrl(filp, cmd, arg);
		break;

	case SYS_ION_ALLOC:
		ret = sys_ion_alloc_user(ndev, arg);
		break;
	case SYS_ION_FREE:
		ret = sys_ion_free_user(ndev, arg);
		break;
	case SYS_CACHE_INVLD:
		ret = sys_cache_op_userv(arg, enum_cache_op_invalid);
		break;
	case SYS_CACHE_FLUSH:
		ret = sys_cache_op_userv(arg, enum_cache_op_flush);
		break;

	case SYS_INIT_USER:
		ret = sys_init_user(ndev, arg);
		break;
	case SYS_EXIT_USER:
		ret = sys_exit_user(ndev, arg);
		break;

	case SYS_GET_SYSINFO:
		ret = sys_get_sysinfo(ndev, arg);
		break;

	case SYS_SET_BINDCFG:
		ret = sys_set_bind_cfg(ndev, arg);
		break;
	case SYS_GET_BINDCFG:
		ret = sys_get_bind_cfg(ndev, arg);
		break;

	default:
		return -ENOTTY;
	}
	return ret;
}

static int cvi_sys_open(struct inode *inode, struct file *filp)
{
	struct cvi_sys_device *ndev = container_of(filp->private_data, struct cvi_sys_device, miscdev);

	spin_lock(&ndev->close_lock);
	ndev->use_count++;
	spin_unlock(&ndev->close_lock);
	filp->private_data = ndev;
	return 0;
}

static int cvi_sys_close(struct inode *inode, struct file *filp)
{
	struct cvi_sys_device *ndev = filp->private_data;
	int cnt = 0;

	spin_lock(&ndev->close_lock);
	cnt = --ndev->use_count;
	spin_unlock(&ndev->close_lock);

	if (cnt == 0) {
		struct sys_ctx_info *sys_ctx = NULL;

		sys_ctx = sys_get_ctx();

		sys_ctx_release_bind();
		atomic_set(&sys_ctx->sys_inited, 0);
	}

	filp->private_data = NULL;
	return 0;
}

#ifdef CONFIG_COMPAT
static long compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static const struct file_operations cvi_sys_fops = {
	.owner = THIS_MODULE,
	.open = cvi_sys_open,
	.release = cvi_sys_close,
	.unlocked_ioctl = cvi_sys_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = compat_ptr_ioctl,
#endif
};

int cvi_sys_register_misc(struct cvi_sys_device *ndev)
{
	int rc;

	ndev->miscdev.minor = MISC_DYNAMIC_MINOR;
	ndev->miscdev.name = CVI_SYS_DEV_NAME;
	ndev->miscdev.fops = &cvi_sys_fops;

	rc = misc_register(&ndev->miscdev);
	if (rc) {
		dev_err(ndev->dev, "cvi_sys: failed to register misc device.\n");
		return rc;
	}

	return 0;
}

static int cvi_sys_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cvi_sys_device *ndev;
	int32_t ret;

	pr_debug("===cvitek_sys_probe start\n");
	ndev = devm_kzalloc(&pdev->dev, sizeof(*ndev), GFP_KERNEL);
	if (!ndev)
		return -ENOMEM;
	ndev->dev = dev;

	mutex_init(&ndev->dev_lock);
	spin_lock_init(&ndev->close_lock);
	ndev->use_count = 0;

	ret = cvi_sys_register_misc(ndev);
	if (ret < 0) {
		pr_err("register misc error\n");
		return ret;
	}

	sys_init();

	platform_set_drvdata(pdev, ndev);

	pr_debug("===cvitek_sys_probe end\n");
	return 0;
}

static int cvi_sys_remove(struct platform_device *pdev)
{
	struct cvi_sys_device *ndev = platform_get_drvdata(pdev);

	sys_exit();

	misc_deregister(&ndev->miscdev);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id cvitek_sys_match[] = {
	{ .compatible = "cvitek,sys" },
	{},
};
MODULE_DEVICE_TABLE(of, cvitek_sys_match);

static struct platform_driver cvitek_sys_driver = {
	.probe = cvi_sys_probe,
	.remove = cvi_sys_remove,
	.driver = {
			.owner = THIS_MODULE,
			.name = CVI_SYS_DEV_NAME,
			.of_match_table = cvitek_sys_match,
		},
};
module_platform_driver(cvitek_sys_driver);

MODULE_AUTHOR("Wellken Chen<wellken.chen@cvitek.com.tw>");
MODULE_DESCRIPTION("Cvitek SoC SYS driver");
MODULE_LICENSE("GPL");

