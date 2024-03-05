#include "cvi_vb_proc.h"

#define VB_PROC_NAME			"vb"
#define VB_PROC_PERMS			(0644)

/*************************************************************************
 *	VB proc functions
 *************************************************************************/
static int32_t _get_vb_modIds(struct vb_pool *pool, uint32_t blk_idx, uint64_t *modIds)
{
	uint64_t phyAddr;
	VB_BLK blk;

	phyAddr = pool->memBase + (blk_idx * pool->blk_size);
	blk = vb_physAddr2Handle(phyAddr);
	if (blk == VB_INVALID_HANDLE)
		return -EINVAL;

	*modIds = ((struct vb_s*)blk)->mod_ids;
	return 0;
}

static void _show_vb_status(struct seq_file *m)
{
	uint32_t i, j, k;
	uint32_t mod_sum[CVI_ID_BUTT];
	int32_t ret;
	uint64_t modIds;
	struct vb_pool *pstVbPool = NULL;

	seq_printf(m, "\nModule: [VB], Build Time[%s]\n", UTS_VERSION);
	seq_puts(m, "-----VB PUB CONFIG-----------------------------------------------------------------------------------------------------------------\n");
	seq_printf(m, "%10s(%3d), %10s(%3d)\n", "MaxPoolCnt", vb_max_pools, "MaxBlkCnt", vb_pool_max_blk);
	ret = vb_get_pool_info(&pstVbPool);
	if (ret != 0) {
		seq_puts(m, "vb_pool has not inited yet\n");
		return;
	}

	seq_puts(m, "\n-----COMMON POOL CONFIG------------------------------------------------------------------------------------------------------------\n");
	for (i = 0; i < vb_max_pools ; ++i) {
		if (pstVbPool[i].memBase != 0) {
			seq_printf(m, "%10s(%3d)\t%10s(%12d)\t%10s(%3d)\n"
			, "PoolId", i, "Size", pstVbPool[i].blk_size, "Count", pstVbPool[i].blk_cnt);
		}
	}

	seq_puts(m, "\n-----------------------------------------------------------------------------------------------------------------------------------\n");
	for (i = 0; i < vb_max_pools; ++i) {
		if (pstVbPool[i].memBase != 0) {
			mutex_lock(&pstVbPool[i].lock);
			seq_printf(m, "%-10s: %s\n", "PoolName", pstVbPool[i].acPoolName);
			seq_printf(m, "%-10s: %d\n", "PoolId", pstVbPool[i].poolID);
			seq_printf(m, "%-10s: 0x%llx\n", "PhysAddr", pstVbPool[i].memBase);
			seq_printf(m, "%-10s: 0x%lx\n", "VirtAddr", (uintptr_t)pstVbPool[i].vmemBase);
			seq_printf(m, "%-10s: %d\n", "IsComm", pstVbPool[i].bIsCommPool);
			seq_printf(m, "%-10s: %d\n", "Owner", pstVbPool[i].ownerID);
			seq_printf(m, "%-10s: %d\n", "BlkSz", pstVbPool[i].blk_size);
			seq_printf(m, "%-10s: %d\n", "BlkCnt", pstVbPool[i].blk_cnt);
			seq_printf(m, "%-10s: %d\n", "Free", pstVbPool[i].u32FreeBlkCnt);
			seq_printf(m, "%-10s: %d\n", "MinFree", pstVbPool[i].u32MinFreeBlkCnt);
			seq_puts(m, "\n");

			memset(mod_sum, 0, sizeof(mod_sum));
			seq_puts(m, "BLK\tBASE\tVB\tSYS\tRGN\tCHNL\tVDEC\tVPSS\tVENC\tH264E\tJPEGE\tMPEG4E\tH265E\tJPEGD\tVO\tVI\tDIS\n");
			seq_puts(m, "RC\tAIO\tAI\tAO\tAENC\tADEC\tAUD\tVPU\tISP\tIVE\tUSER\tPROC\tLOG\tH264D\tGDC\tPHOTO\tFB\n");
			for (j = 0; j < pstVbPool[i].blk_cnt; ++j) {
				seq_printf(m, "%s%d\t", "#", j);
				if (_get_vb_modIds(&pstVbPool[i], j, &modIds) != 0) {
					for (k = 0; k < CVI_ID_BUTT; ++k) {
						seq_puts(m, "e\t");
						if (k == CVI_ID_DIS || k == CVI_ID_FB)
							seq_puts(m, "\n");
					}
					continue;
				}

				for (k = 0; k < CVI_ID_BUTT; ++k) {
					if (modIds & BIT(k)) {
						seq_puts(m, "1\t");
						mod_sum[k]++;
					} else
						seq_puts(m, "0\t");

					if (k == CVI_ID_DIS || k == CVI_ID_FB)
						seq_puts(m, "\n");
				}
			}

			seq_puts(m, "Sum\t");
			for (k = 0; k < CVI_ID_BUTT; ++k) {
				seq_printf(m, "%d\t", mod_sum[k]);
				if (k == CVI_ID_DIS || k == CVI_ID_FB)
					seq_puts(m, "\n");
			}
			mutex_unlock(&pstVbPool[i].lock);
			seq_puts(m, "\n-----------------------------------------------------------------------------------------------------------------------------------\n");
		}
	}
}

static int _vb_proc_show(struct seq_file *m, void *v)
{
	_show_vb_status(m);
	return 0;
}

static int _vb_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, _vb_proc_show, NULL);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops _vb_proc_fops = {
	.proc_open = _vb_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations _vb_proc_fops = {
	.owner = THIS_MODULE,
	.open = _vb_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int vb_proc_init(struct proc_dir_entry *_proc_dir)
{
	int rc = 0;

	/* create the /proc file */
	if (proc_create_data(VB_PROC_NAME, VB_PROC_PERMS, _proc_dir, &_vb_proc_fops, NULL) == NULL) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb proc creation failed\n");
		rc = -1;
	}
	return rc;
}

int vb_proc_remove(struct proc_dir_entry *_proc_dir)
{
	remove_proc_entry(VB_PROC_NAME, _proc_dir);

	return 0;
}
