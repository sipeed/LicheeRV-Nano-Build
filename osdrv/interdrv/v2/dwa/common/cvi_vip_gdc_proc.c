#include "cvi_vip_gdc_proc.h"

#define GENERATE_STRING(STRING)	(#STRING),
#define GDC_PROC_NAME "cvitek/gdc"

static void *gdc_shared_mem;
static const char *const MOD_STRING[] = FOREACH_MOD(GENERATE_STRING);
/*************************************************************************
 *	GDC proc functions
 *************************************************************************/
static int gdc_proc_show(struct seq_file *m, void *v)
{
	struct cvi_gdc_proc_ctx *pgdcCtx = NULL;
	int i, j, idx, total_hwTime, total_busyTime;
	char c[32];

	pgdcCtx = (struct cvi_gdc_proc_ctx *)(gdc_shared_mem);
	if (!pgdcCtx) {
		seq_puts(m, "gdc shm = NULL\n");
		return -1;
	}

	seq_printf(m, "\nModule: [GDC], Build Time[%s]\n", UTS_VERSION);
	// recent job info
	seq_puts(m, "\n-------------------------------RECENT JOB INFO----------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%20s%20s%20s%20s\n",
		"SeqNo", "ModName", "TaskNum", "State",
		"InSize(pixel)", "OutSize(pixel)", "CostTime(us)", "HwTime(us)");

	idx = pgdcCtx->u16JobInfoIdx;
	for (i = 0 ; i < 8; ++i) {
		if (!pgdcCtx->stJobInfo[idx].hHandle)
			break;

		memset(c, 0, sizeof(c));
		if (pgdcCtx->stJobInfo[idx].eState == GDC_JOB_SUCCESS)
			strncpy(c, "SUCCESS", sizeof(c));
		else if (pgdcCtx->stJobInfo[idx].eState == GDC_JOB_FAIL)
			strncpy(c, "FAIL", sizeof(c));
		else if (pgdcCtx->stJobInfo[idx].eState == GDC_JOB_WORKING)
			strncpy(c, "WORKING", sizeof(c));
		else
			strncpy(c, "UNKNOWN", sizeof(c));

		seq_printf(m, "%8s%2d%10s%10d%10s%20d%20d%20d%20d\n",
				"#",
				i,
				(pgdcCtx->stJobInfo[idx].enModId == CVI_ID_BUTT) ?
					"USER" : MOD_STRING[pgdcCtx->stJobInfo[idx].enModId],
				pgdcCtx->stJobInfo[idx].u32TaskNum,
				c,
				pgdcCtx->stJobInfo[idx].u32InSize,
				pgdcCtx->stJobInfo[idx].u32OutSize,
				pgdcCtx->stJobInfo[idx].u32CostTime,
				pgdcCtx->stJobInfo[idx].u32HwTime);

		idx = (--idx < 0) ? (GDC_PROC_JOB_INFO_NUM - 1) : idx;
	}

	// Max waste time job info
	seq_puts(m, "\n-------------------------------MAX WASTE TIME JOB INFO--------------------\n");
	seq_printf(m, "%10s%10s%10s%20s%20s%20s%20s\n",
		"ModName", "TaskNum", "State",
		"InSize(pixel)", "OutSize(pixel)", "CostTime(us)", "HwTime(us)");

	idx = i = pgdcCtx->u16JobInfoIdx;
	for (j = 1; j < GDC_PROC_JOB_INFO_NUM; ++j) {
		i = (--i < 0) ? (GDC_PROC_JOB_INFO_NUM - 1) : i;
		if (!pgdcCtx->stJobInfo[i].hHandle)
			break;

		if (pgdcCtx->stJobInfo[i].u32CostTime > pgdcCtx->stJobInfo[idx].u32CostTime)
			idx = i;
	}

	if (pgdcCtx->stJobInfo[idx].hHandle) {
		memset(c, 0, sizeof(c));
		if (pgdcCtx->stJobInfo[idx].eState == GDC_JOB_SUCCESS)
			strncpy(c, "SUCCESS", sizeof(c));
		else if (pgdcCtx->stJobInfo[idx].eState == GDC_JOB_FAIL)
			strncpy(c, "FAIL", sizeof(c));
		else if (pgdcCtx->stJobInfo[idx].eState == GDC_JOB_WORKING)
			strncpy(c, "WORKING", sizeof(c));
		else
			strncpy(c, "UNKNOWN", sizeof(c));

		seq_printf(m, "%10s%10d%10s%20d%20d%20d%20d\n",
					MOD_STRING[pgdcCtx->stJobInfo[idx].enModId],
					pgdcCtx->stJobInfo[idx].u32TaskNum,
					c,
					pgdcCtx->stJobInfo[idx].u32InSize,
					pgdcCtx->stJobInfo[idx].u32OutSize,
					pgdcCtx->stJobInfo[idx].u32CostTime,
					pgdcCtx->stJobInfo[idx].u32HwTime);
	}

	// GDC job status
	seq_puts(m, "\n-------------------------------GDC JOB STATUS-----------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%20s\n",
		"Success", "Fail", "Cancel", "BeginNum", "BusyNum", "ProcingNum");

	seq_printf(m, "%10d%10d%10d%10d%10d%20d\n",
				pgdcCtx->stJobStatus.u32Success,
				pgdcCtx->stJobStatus.u32Fail,
				pgdcCtx->stJobStatus.u32Cancel,
				pgdcCtx->stJobStatus.u32BeginNum,
				pgdcCtx->stJobStatus.u32BusyNum,
				pgdcCtx->stJobStatus.u32ProcingNum);

	// GDC task status
	seq_puts(m, "\n-------------------------------GDC TASK STATUS----------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s\n", "Success", "Fail", "Cancel", "BusyNum");
	seq_printf(m, "%10d%10d%10d%10d\n",
				pgdcCtx->stTaskStatus.u32Success,
				pgdcCtx->stTaskStatus.u32Fail,
				pgdcCtx->stTaskStatus.u32Cancel,
				pgdcCtx->stTaskStatus.u32BusyNum);

	// GDC interrupt status
	seq_puts(m, "\n-------------------------------GDC INT STATUS-----------------------------\n");
	seq_printf(m, "%10s%20s%20s\n", "IntNum", "IntTm(us)", "HalProcTm(us)");

	total_hwTime = total_busyTime = 0;
	for (i = 0; i < GDC_PROC_JOB_INFO_NUM; ++i) {
		total_hwTime += pgdcCtx->stJobInfo[i].u32HwTime;
		total_busyTime += pgdcCtx->stJobInfo[i].u32BusyTime;
	}

	seq_printf(m, "%10d%20d%20d\n",
				pgdcCtx->stJobStatus.u32Success,
				total_hwTime / GDC_PROC_JOB_INFO_NUM,
				total_busyTime / GDC_PROC_JOB_INFO_NUM);

	// GDC call correction status
	seq_puts(m, "\n-------------------------------GDC CALL CORRECTION STATUS-----------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s\n", "TaskSuc", "TaskFail", "EndSuc", "EndFail", "CbCnt");
	seq_printf(m, "%10d%10d%10d%10d%10d\n",
				pgdcCtx->stFishEyeStatus.u32AddTaskSuc,
				pgdcCtx->stFishEyeStatus.u32AddTaskFail,
				pgdcCtx->stFishEyeStatus.u32EndSuc,
				pgdcCtx->stFishEyeStatus.u32EndFail,
				pgdcCtx->stFishEyeStatus.u32CbCnt);

	return 0;
}

static int gdc_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, gdc_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops gdc_proc_fops = {
	.proc_open = gdc_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations gdc_proc_fops = {
	.owner = THIS_MODULE,
	.open = gdc_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int gdc_proc_init(void *shm)
{
	int rc = 0;

	/* create the /proc file */
	if (proc_create_data(GDC_PROC_NAME, 0644, NULL, &gdc_proc_fops, NULL) == NULL) {
		pr_err("gdc proc creation failed\n");
		rc = -1;
	}

	gdc_shared_mem = shm;
	return rc;
}

int gdc_proc_remove(void)
{
	remove_proc_entry(GDC_PROC_NAME, NULL);
	gdc_shared_mem = NULL;

	return 0;
}
