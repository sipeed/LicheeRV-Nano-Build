#include "cvi_sys_proc.h"

#define SYS_PROC_NAME			"sys"
#define SYS_PROC_PERMS			(0644)
#define GENERATE_STRING(STRING)	(#STRING),

static void *shared_mem;
static const char *const MOD_STRING[] = FOREACH_MOD(GENERATE_STRING);

extern struct _BIND_NODE_S bind_nodes[BIND_NODE_MAXNUM];

#define CHN_MATCH(x, y) (((x)->enModId == (y)->enModId) && ((x)->s32DevId == (y)->s32DevId)             \
	&& ((x)->s32ChnId == (y)->s32ChnId))

/*************************************************************************
 *	sys proc functions
 *************************************************************************/
static bool _is_fisrt_level_bind_node(BIND_NODE_S *node)
{
	int i, j;
	BIND_NODE_S *bindNodes;

	bindNodes = bind_nodes;
	for (i = 0; i < BIND_NODE_MAXNUM; ++i) {
		if ((bindNodes[i].bUsed) && (bindNodes[i].dsts.u32Num != 0)
			&& !CHN_MATCH(&bindNodes[i].src, &node->src)) {
			for (j = 0; j < bindNodes[i].dsts.u32Num; ++j) {
				if (CHN_MATCH(&bindNodes[i].dsts.astMmfChn[j], &node->src))
					// find other source in front of this node
					return false;
			}
		}
	}

	return true;
}

static BIND_NODE_S *_find_next_bind_node(const MMF_CHN_S *pstSrcChn)
{
	int i;
	BIND_NODE_S *bindNodes;

	bindNodes = bind_nodes;
	for (i = 0; i < BIND_NODE_MAXNUM; ++i) {
		if ((bindNodes[i].bUsed) && CHN_MATCH(pstSrcChn, &bindNodes[i].src)
			&& (bindNodes[i].dsts.u32Num != 0)) {
			return &bindNodes[i];
		}
	}

	return NULL; // didn't find next bind node
}

static void _show_sys_status(struct seq_file *m)
{
	int i, j, k;
	MMF_VERSION_S *mmfVersion;
	BIND_NODE_S *bindNodes, *nextBindNode;
	MMF_CHN_S *first, *second, *third;

	mmfVersion = (MMF_VERSION_S *)(shared_mem + BASE_VERSION_INFO_OFFSET);
	bindNodes = bind_nodes;

	seq_printf(m, "\nModule: [SYS], Version[%s], Build Time[%s]\n", mmfVersion->version, UTS_VERSION);
	seq_puts(m, "-----BIND RELATION TABLE-----------------------------------------------------------------------------------------------------------\n");
	seq_printf(m, "%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s\n",
		"1stMod", "1stDev", "1stChn", "2ndMod", "2ndDev", "2ndChn", "3rdMod", "3rdDev", "3rdChn");

	for (i = 0; i < BIND_NODE_MAXNUM; ++i) {
		//Check if the bind node is used / has destination / first level of bind chain
		if ((bindNodes[i].bUsed) && (bindNodes[i].dsts.u32Num != 0)
			&& (_is_fisrt_level_bind_node(&bindNodes[i]))) {

			first = &bindNodes[i].src; //bind chain first level

			for (j = 0; j < bindNodes[i].dsts.u32Num; ++j) {
				second = &bindNodes[i].dsts.astMmfChn[j]; //bind chain second level

				nextBindNode = _find_next_bind_node(second);
				if (nextBindNode != NULL) {
					for (k = 0; k < nextBindNode->dsts.u32Num; ++k) {
					third = &nextBindNode->dsts.astMmfChn[k]; //bind chain third level
					seq_printf(m, "%-10s%-10d%-10d%-10s%-10d%-10d%-10s%-10d%-10d\n",
						MOD_STRING[first->enModId], first->s32DevId, first->s32ChnId,
						MOD_STRING[second->enModId], second->s32DevId, second->s32ChnId,
						MOD_STRING[third->enModId], third->s32DevId, third->s32ChnId);
					}
				} else { //level 3 node not found
					seq_printf(m, "%-10s%-10d%-10d%-10s%-10d%-10d%-10s%-10d%-10d\n",
						MOD_STRING[first->enModId], first->s32DevId, first->s32ChnId,
						MOD_STRING[second->enModId], second->s32DevId, second->s32ChnId,
						"null", 0, 0);
				}
			}
		}
	}
	seq_puts(m, "\n-----------------------------------------------------------------------------------------------------------------------------------\n");
}

static int _sys_proc_show(struct seq_file *m, void *v)
{
	_show_sys_status(m);
	return 0;
}

static int _sys_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, _sys_proc_show, NULL);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops _sys_proc_fops = {
	.proc_open = _sys_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations _sys_proc_fops = {
	.owner = THIS_MODULE,
	.open = _sys_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int sys_proc_init(struct proc_dir_entry *_proc_dir, void *shm)
{
	int rc = 0;

	/* create the /proc file */
	if (proc_create_data(SYS_PROC_NAME, SYS_PROC_PERMS, _proc_dir, &_sys_proc_fops, NULL) == NULL) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "sys proc creation failed\n");
		rc = -1;
	}

	shared_mem = shm;
	return rc;
}

int sys_proc_remove(struct proc_dir_entry *_proc_dir)
{
	remove_proc_entry(SYS_PROC_NAME, _proc_dir);
	shared_mem = NULL;

	return 0;
}
