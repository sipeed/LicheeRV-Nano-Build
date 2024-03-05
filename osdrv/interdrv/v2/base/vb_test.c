#include "vb_test.h"

#ifdef DRV_TEST

static uint32_t test_cnt;

int32_t vb_test_cb(MMF_CHN_S chn)
{
	CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "vb_test_cb!\n");
	test_cnt++;
	return 0;
}

static void _memcpy_test(void *dst, void *src, uint32_t size, uint32_t *duration)
{
	struct timespec64 time[2];
	uint64_t timeUs[2];

	ktime_get_ts64(&time[0]);
	memcpy(dst, src, size);
	ktime_get_ts64(&time[1]);
	timeUs[0] = (u64)time[0].tv_sec * USEC_PER_SEC + time[0].tv_nsec / NSEC_PER_USEC;
	timeUs[1] = (u64)time[1].tv_sec * USEC_PER_SEC + time[1].tv_nsec / NSEC_PER_USEC;
	*duration = (uint32_t)(timeUs[1] - timeUs[0]);
}

static int32_t _vb_get_block_test(void)
{
	struct cvi_vb_cfg cfg;
	int32_t ret, i;
	VB_BLK blk = VB_INVALID_HANDLE, tmp_blk;
	VB_POOL pool;
	uint32_t usr_cnt = 0;
	uint64_t phy_addr = 0;
	void *pVirAddr = NULL;
	void *pTestBuf = NULL;
	uint32_t buf_len;
	uint32_t duration0, duration1;

	ret = vb_get_config(&cfg);
	if (ret) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_get_config fail, ret:%d\n", ret);
		goto GET_BLOCK_TEST_FAIL;
	}

	for (i = 0; i < cfg.comm_pool_cnt; ++i) {
		buf_len = cfg.comm_pool[i].blk_size;
		blk = vb_get_block_with_id(VB_INVALID_POOLID, buf_len, CVI_ID_VPSS);
		if (blk == VB_INVALID_HANDLE) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_get_block_with_id fail\n");
			goto GET_BLOCK_TEST_FAIL;
		}

		pool = vb_handle2PoolId(blk);
		if (pool != (uint32_t)i) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "Error:pool id:%d, expected pool id should be:%d\n", pool, i);
			goto GET_BLOCK_TEST_FAIL;
		}

		ret = vb_inquireUserCnt(blk, &usr_cnt);
		if (ret || usr_cnt != 1) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_inquireUserCnt fail, ret:%d, usr_cnt:%d\n", ret, usr_cnt);
			goto GET_BLOCK_TEST_FAIL;
		}

		phy_addr = vb_handle2PhysAddr(blk);
		if (phy_addr == 0) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_handle2PhysAddr fail\n");
			goto GET_BLOCK_TEST_FAIL;
		}
		tmp_blk = vb_physAddr2Handle(phy_addr);
		if (tmp_blk != blk) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "Error: should be the same blk\n");
			goto GET_BLOCK_TEST_FAIL;
		}

		pVirAddr = vb_handle2VirtAddr(blk);
		if (!pVirAddr) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_handle2VirtAddr fail\n");
			goto GET_BLOCK_TEST_FAIL;
		}
		memset(pVirAddr, 0xab, buf_len);

		pTestBuf = vmalloc(buf_len);
		memset(pTestBuf, 0, buf_len);
		_memcpy_test(pTestBuf, pVirAddr, buf_len, &duration0);
		_memcpy_test(pTestBuf, pVirAddr, buf_len, &duration1);
		if (memcmp(pTestBuf, pVirAddr, buf_len) != 0) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "memcpy test fail\n");
			vfree(pTestBuf);
			goto GET_BLOCK_TEST_FAIL;
		}
		vfree(pTestBuf);
		CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "memcpy(%d bytes)  %d - %d\n", buf_len, duration0, duration1);

		ret = vb_release_block(blk);
		if (ret) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_release_block fail\n");
			goto GET_BLOCK_TEST_FAIL;
		}
		ret = vb_inquireUserCnt(blk, &usr_cnt);
		if (ret || usr_cnt != 0) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_inquireUserCnt fail, ret:%d, usr_cnt:%d\n", ret, usr_cnt);
			goto GET_BLOCK_TEST_FAIL;
		}
	}
	CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "vb get block test SUCCESS!\n");
	return 0;

GET_BLOCK_TEST_FAIL:
	if (blk != VB_INVALID_HANDLE)
		vb_release_block(blk);
	return -1;
}

static int32_t _vb_create_pool_test(void)
{
	struct cvi_vb_pool_cfg config;
	int32_t ret;
	const uint32_t buf_len = 0x80000;
	const uint32_t blk_size = 0x100000;
	VB_BLK blk = VB_INVALID_HANDLE;
	VB_POOL pool = VB_INVALID_POOLID, tmp_pool;

	memset(&config, 0, sizeof(config));
	config.blk_cnt = 2;
	config.blk_size = blk_size;
	config.remap_mode = VB_REMAP_MODE_CACHED;
	strncpy(config.pool_name, "vb_test", sizeof(config.pool_name));
	ret = vb_create_pool(&config);
	if (ret) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_create_pool fail, ret:%d\n", ret);
		goto GREATE_POOL_TEST_FAIL;
	}

	pool = config.pool_id;
	blk = vb_get_block_with_id(pool, buf_len, CVI_ID_VPSS);
	if (blk == VB_INVALID_HANDLE) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_get_block_with_id fail\n");
		goto GREATE_POOL_TEST_FAIL;
	}

	tmp_pool = vb_handle2PoolId(blk);
	if (tmp_pool != pool) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "Error:pool id:%d, expected pool id should be:%d\n", tmp_pool, pool);
		goto GREATE_POOL_TEST_FAIL;
	}

	ret = vb_release_block(blk);
	if (ret) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_release_block fail\n");
		goto GREATE_POOL_TEST_FAIL;
	}

	ret = vb_destroy_pool(pool);
	if (ret) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_destroy_pool fail, pool id:%d\n", pool);
		goto GREATE_POOL_TEST_FAIL;
	}
	CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "vb create pool test SUCCESS!\n");
	return 0;

GREATE_POOL_TEST_FAIL:
	if (blk != VB_INVALID_HANDLE)
		vb_release_block(blk);
	if (pool != VB_INVALID_POOLID)
		vb_destroy_pool(pool);
	return -1;
}

static int32_t _vb_create_blk_test(void)
{
	VB_BLK blk = VB_INVALID_HANDLE, tmp_blk;
	const uint32_t buf_len = 0x80000;
	struct vb_s *vb;
	int32_t ret;
	uint64_t phy_addr = 0;
	void *ion_v = NULL;

	// get static blk test
	// ------------------------------------------
	blk = vb_get_block_with_id(VB_STATIC_POOLID, buf_len, CVI_ID_VPSS);
	if (blk == VB_INVALID_HANDLE) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb get static blk fail\n");
		goto GREATE_BLK_TEST_FAIL;
	}

	vb = (struct vb_s *)blk;
	if (vb->vb_pool != VB_STATIC_POOLID) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "Error: pool id:%d, should be VB_STATIC_POOLID\n", vb->vb_pool);
		goto GREATE_BLK_TEST_FAIL;
	}

	phy_addr = vb->phy_addr;
	tmp_blk = vb_physAddr2Handle(phy_addr);
	if (tmp_blk != blk) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "Error: Unexpected blk-phy_addr pair\n");
		goto GREATE_BLK_TEST_FAIL;
	}

	ret = vb_release_block(blk);
	if (ret) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_release_block fail\n");
		goto GREATE_BLK_TEST_FAIL;
	}

	tmp_blk = vb_physAddr2Handle(phy_addr);
	if (tmp_blk != VB_INVALID_HANDLE) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "Error: vb_physAddr2Handle should fail after release static blk\n");
		goto GREATE_BLK_TEST_FAIL;
	}
	blk = VB_INVALID_HANDLE;
	phy_addr = 0;


	// create external vb test
	// ------------------------------------------
	ret = sys_ion_alloc(&phy_addr, &ion_v, "ext_vb", buf_len, true);
	if (ret) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "sys_ion_alloc fail! ret(%d)\n", ret);
		goto GREATE_BLK_TEST_FAIL;
	}

	blk = vb_create_block(phy_addr, ion_v, VB_EXTERNAL_POOLID, true);
	if (blk == VB_INVALID_HANDLE) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_create_block fail\n");
		goto GREATE_BLK_TEST_FAIL;
	}

	tmp_blk = vb_physAddr2Handle(phy_addr);
	if (tmp_blk != blk) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "Error: Unexpected blk-phy_addr pair\n");
		goto GREATE_BLK_TEST_FAIL;
	}

	ret = vb_release_block(blk);
	if (ret) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_release_block fail\n");
		goto GREATE_BLK_TEST_FAIL;
	}

	tmp_blk = vb_physAddr2Handle(phy_addr);
	if (tmp_blk != VB_INVALID_HANDLE) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "Error: vb_physAddr2Handle should fail after release ext blk\n");
		goto GREATE_BLK_TEST_FAIL;
	}
	sys_ion_free(phy_addr);
	CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "vb create blk test SUCCESS!\n");
	return 0;

GREATE_BLK_TEST_FAIL:
	if (blk != VB_INVALID_HANDLE)
		vb_release_block(blk);
	if (phy_addr)
		sys_ion_free(phy_addr);
	return -1;
}

static int32_t _vb_qbuf_test(void)
{
	MMF_CHN_S chn = {.enModId = CVI_ID_VI, .s32DevId = 0, .s32ChnId = 0};
	VB_BLK blk[2] = {VB_INVALID_HANDLE, VB_INVALID_HANDLE};
	VB_BLK workq_blk, doneq_blk, tmp_blk;
	const uint32_t buf_len = 0x100000;
	CVI_S32 ret;
	struct vb_s *vb;
	uint32_t i;

	base_mod_jobs_init(chn, CHN_TYPE_OUT, 0, 2, 1);

	// get blk and qbuf to workq
	// ------------------------------------------
	for (i = 0; i < 2; ++i) {
		blk[i] = vb_get_block_with_id(VB_INVALID_POOLID, buf_len, CVI_ID_VI);
		if (blk[i] == VB_INVALID_HANDLE) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_get_block_with_id fail, blk idx(%d)\n", i);
			goto QBUF_TEST_FAIL;
		}
		ret = vb_qbuf(chn, CHN_TYPE_OUT, blk[i]);
		if (ret != CVI_SUCCESS) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_qbuf fail\n");
			goto QBUF_TEST_FAIL;
		}
		vb_release_block(blk[i]);
	}

	tmp_blk = vb_get_block_with_id(VB_INVALID_POOLID, buf_len, CVI_ID_VI);
	if (tmp_blk == VB_INVALID_HANDLE) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_get_block_with_id fail\n");
		goto QBUF_TEST_FAIL;
	}
	ret = vb_qbuf(chn, CHN_TYPE_OUT, tmp_blk);
	if (ret == CVI_SUCCESS) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_qbuf should fail due to workq full\n");
		goto QBUF_TEST_FAIL;
	}
	vb_release_block(tmp_blk);

	// test dqbuf, done_handler & get_chn_buf from doneq
	// --------------------------------------------------
	for (i = 0; i < 2; ++i) {
		vb_dqbuf(chn, CHN_TYPE_OUT, &workq_blk);
		if (workq_blk != blk[i]) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_dqbuf fail, blk idx(%d)\n", i);
			goto QBUF_TEST_FAIL;
		}

		vb_done_handler(chn, CHN_TYPE_OUT, blk[i]);
		ret = base_get_chn_buffer(chn, &doneq_blk, -1);
		if (ret) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "base_get_chn_buffer fail, blk idx(%d)\n", i);
			goto QBUF_TEST_FAIL;
		}
		if (doneq_blk != blk[i]) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "get unexpected blk from doneq, blk idx(%d)\n", i);
			goto QBUF_TEST_FAIL;
		}
		vb_release_block(blk[i]);

		vb = (struct vb_s *)blk[i];
		if (vb->usr_cnt.counter != 0) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "usr_cnt should be zero after release blk, blk idx(%d)\n", i);
			goto QBUF_TEST_FAIL;
		}
	}

	base_mod_jobs_exit(chn, CHN_TYPE_OUT);
	CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "vb qbuf test SUCCESS!\n");
	return 0;

QBUF_TEST_FAIL:
	for (i = 0; i < 2; ++i) {
		if (blk[i] != VB_INVALID_HANDLE)
			vb_release_block(blk[i]);
	}
	base_mod_jobs_exit(chn, CHN_TYPE_OUT);
	return -1;
}

static int32_t _vb_jobs_test(void)
{
	MMF_CHN_S chn = {.enModId = CVI_ID_VI, .s32DevId = 0, .s32ChnId = 0};
	CVI_S32 ret;
	VB_BLK blk = VB_INVALID_HANDLE, tmp_blk;
	const uint32_t buf_len = 0x100000;

	base_mod_jobs_init(chn, CHN_TYPE_IN, 1, 1, 0);

	blk = vb_get_block_with_id(VB_INVALID_POOLID, buf_len, CVI_ID_VPSS);
	if (blk == VB_INVALID_HANDLE) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_get_block_with_id fail\n");
		goto JOBS_TEST_FAIL;
	}

	// waitq api test
	// --------------------------------------------------
	ret = vb_qbuf(chn, CHN_TYPE_IN, blk);
	if (ret != CVI_SUCCESS) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_qbuf fail\n");
		goto JOBS_TEST_FAIL;
	}
	if (base_mod_jobs_waitq_empty(chn, CHN_TYPE_IN)) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "waitq should not be empty\n");
		goto JOBS_TEST_FAIL;
	}

	tmp_blk = base_mod_jobs_waitq_pop(chn, CHN_TYPE_IN);
	if (tmp_blk != blk) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "base_mod_jobs_waitq_pop fail\n");
		goto JOBS_TEST_FAIL;
	}
	vb_release_block(blk);
	if (!base_mod_jobs_waitq_empty(chn, CHN_TYPE_IN)) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "waitq should be empty\n");
		goto JOBS_TEST_FAIL;
	}

	// enque_work & workq api test
	// --------------------------------------------------
	ret = vb_qbuf(chn, CHN_TYPE_IN, blk);
	if (ret != CVI_SUCCESS) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_qbuf fail\n");
		goto JOBS_TEST_FAIL;
	}
	base_mod_jobs_enque_work(chn, CHN_TYPE_IN);
	if (base_mod_jobs_workq_empty(chn, CHN_TYPE_IN)) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "workq should not be empty\n");
		goto JOBS_TEST_FAIL;
	}
	if (!base_mod_jobs_waitq_empty(chn, CHN_TYPE_IN)) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "waitq should be empty\n");
		goto JOBS_TEST_FAIL;
	}

	tmp_blk = base_mod_jobs_workq_pop(chn, CHN_TYPE_IN);
	if (tmp_blk != blk) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "base_mod_jobs_workq_pop fail\n");
		goto JOBS_TEST_FAIL;
	}
	vb_release_block(blk);
	if (!base_mod_jobs_workq_empty(chn, CHN_TYPE_IN)) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "workq should be empty\n");
		goto JOBS_TEST_FAIL;
	}

	vb_release_block(blk);
	base_mod_jobs_exit(chn, CHN_TYPE_IN);
	CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "vb jobs test SUCCESS!\n");
	return 0;

JOBS_TEST_FAIL:
	if (blk != VB_INVALID_HANDLE)
		vb_release_block(blk);
	base_mod_jobs_exit(chn, CHN_TYPE_IN);
	return -1;
}

static int32_t _vb_acquire_block_test(void)
{
	struct cvi_vb_cfg cfg;
	int32_t ret, i, loop;
	VB_POOL pool = 0; // use pool0
	VB_BLK *blk, tmp_blk;
	uint32_t blk_cnt, blk_size, old_test_cnt;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = 0, .s32ChnId = 0};

	for (loop = 0; loop < 2; ++loop) {
		old_test_cnt = test_cnt;

		ret = vb_get_config(&cfg);
		if (ret) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_get_config fail, ret:%d\n", ret);
			goto ACQUIRE_BLK_TEST_FAIL;
		}
		blk_cnt = cfg.comm_pool[pool].blk_cnt;
		blk_size = cfg.comm_pool[pool].blk_size;
		blk = kcalloc(1, sizeof(VB_BLK) * blk_cnt, GFP_KERNEL);

		for (i = 0; i < blk_cnt; ++i) {
			blk[i] = vb_get_block_with_id(pool, blk_size, CVI_ID_USER);
			if (blk[i] == VB_INVALID_HANDLE) {
				CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_get_block_with_id fail, idx(%d)\n", i);
				goto ACQUIRE_BLK_TEST_FAIL;
			}
		}

		tmp_blk = vb_get_block_with_id(pool, blk_size, CVI_ID_USER);
		if (tmp_blk != VB_INVALID_HANDLE) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_get_block_with_id should be failed due to no free blk\n");
			goto ACQUIRE_BLK_TEST_FAIL;
		}
		vb_acquire_block(vb_test_cb, chn, blk_size, pool);
		if (loop == 1)
			vb_cancel_block(chn, blk_size, pool);

		for (i = 0; i < blk_cnt; ++i)
			vb_release_block(blk[i]);
		kfree(blk);

		if (loop == 1) {
			if (test_cnt != old_test_cnt) {
				CVI_TRACE_BASE(CVI_BASE_DBG_ERR,
					"fail, cnt(%d) old_cnt(%d) loop(%d)\n",
					test_cnt, old_test_cnt, loop);
				return -1;
			}
		} else {
			if (test_cnt != (old_test_cnt + 1)) {
				CVI_TRACE_BASE(CVI_BASE_DBG_ERR,
					"fail, cnt(%d) old_cnt(%d) loop(%d)\n",
					test_cnt, old_test_cnt, loop);
				return -1;
			}
		}
	}
	CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "vb acquire block test SUCCESS!\n");
	return 0;

ACQUIRE_BLK_TEST_FAIL:
	for (i = 0; i < blk_cnt; ++i) {
		if (blk[i] != VB_INVALID_HANDLE)
			vb_release_block(blk[i]);
	}
	kfree(blk);
	return -1;
}

int32_t vb_unit_test(int32_t op)
{
	int32_t ret = 0;

	switch (op) {
	case 1: {
		CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "vb get block test\n");
		ret = _vb_get_block_test();
		break;
	}
	case 2: {
		CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "vb create pool test\n");
		ret = _vb_create_pool_test();
		break;
	}
	case 3: {
		CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "vb create blk test\n");
		ret = _vb_create_blk_test();
		break;
	}
	case 4: {
		CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "vb qbuf test\n");
		ret = _vb_qbuf_test();
		break;
	}
	case 5: {
		CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "vb jobs test\n");
		ret = _vb_jobs_test();
		break;
	}
	case 6: {
		CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "vb acquire blk test\n");
		ret = _vb_acquire_block_test();
	}
	default:
		break;
	}
	return ret;
}

#endif
