#include "aic8800d80x2_compat.h"
#include "aic_bsp_driver.h"

#define USER_PWROFST_COVER_CALIB_FLAG           (0x01U << 0)
#define USER_CHAN_MAX_TXPWR_EN_FLAG             (0x01U << 1)
#define USER_TX_USE_ANA_F_FLAG                  (0x01U << 2)
#define USER_APM_PRBRSP_OFFLOAD_DISABLE_FLAG    (0x01U << 3)
#define USER_HE_MU_EDCA_UPDATE_DISABLE_FLAG     (0x01U << 4)

#define RAM_FMAC_FW_ADDR_8800D80X2           0x120000
#define RAM_FMAC_RF_FW_ADDR_8800D80X2        0x120000

#define CFG_USER_CHAN_MAX_TXPWR_EN  1
#define CFG_USER_TX_USE_ANA_F       0
#ifdef CONFIG_PRBREQ_REPORT
#define CFG_USER_APM_PRBRSP_OFFLOAD_DISABLE 1
#else
#define CFG_USER_APM_PRBRSP_OFFLOAD_DISABLE 0
#endif

extern int adap_test;
#define NEW_PATCH_BUFFER_MAP    1
#define AIC_PATCH_MAGIG_NUM     0x48435450 // "PTCH"
#define AIC_PATCH_MAGIG_NUM_2   0x50544348 // "HCTP"
#define AIC_PATCH_BLOCK_MAX     4
extern struct aicbsp_info_t aicbsp_info;
extern int adap_test;

typedef u32 (*array2_tbl_t)[2];

typedef struct {
    uint32_t magic_num;
    uint32_t pair_start;
    uint32_t magic_num_2;
    uint32_t pair_count;
    uint32_t block_dst[AIC_PATCH_BLOCK_MAX];
    uint32_t block_src[AIC_PATCH_BLOCK_MAX];
    uint32_t block_size[AIC_PATCH_BLOCK_MAX]; // word count
} aic_patch_t;

#define AIC_PATCH_OFST(mem) ((size_t) &((aic_patch_t *)0)->mem)
#define AIC_PATCH_ADDR(mem) ((u32) (aic_patch_str_base + AIC_PATCH_OFST(mem)))


u32 patch_tbl_d80x2[][2] =
{
    {0x021c, 0x04000000},//hs amsdu
    {0x0220, 0x04010101},//ss amsdu
    {0x0224, 0x50000a01},//hs aggr
    {0x0228, 0x50000a00},//ss aggr

    {0x01f0, 0x00000001
        #if CFG_USER_CHAN_MAX_TXPWR_EN
        | USER_CHAN_MAX_TXPWR_EN_FLAG
        #endif
        #if CFG_USER_TX_USE_ANA_F
        | USER_TX_USE_ANA_F_FLAG
        #endif
        #if CFG_USER_APM_PRBRSP_OFFLOAD_DISABLE
        | USER_APM_PRBRSP_OFFLOAD_DISABLE_FLAG
        #endif
    }, // user_ext_flags
};

//adap test
u32 adaptivity_patch_tbl_d80x2[][2] = {
    {0x000C, 0x0000320A}, //linkloss_thd
    {0x009C, 0x00000000}, //ac_param_conf
    {0x01D0, 0x00010000}, //tx_adaptivity_en
};


int aicwifi_patch_config_8800d80x2(struct aic_sdio_dev *sdiodev)
{
    u32 rd_patch_addr;
    u32 aic_patch_addr;
    u32 config_base, aic_patch_str_base;
    #if (NEW_PATCH_BUFFER_MAP)
    u32 patch_buff_addr, patch_buff_base, rd_version_addr, rd_version_val;
    #endif
    uint32_t start_addr;
    u32 patch_addr;
    u32 patch_cnt = sizeof(patch_tbl_d80x2) / 4 / 2;
    struct dbg_mem_read_cfm rd_patch_addr_cfm;
    int ret = 0;
    int cnt = 0;
    //adap test
    int adap_patch_cnt = 0;

    if (adap_test) {
        AICWFDBG(LOGINFO, "%s adap test \r\n", __func__);
        adap_patch_cnt = sizeof(adaptivity_patch_tbl_d80x2)/sizeof(u32)/2;
    }

    rd_patch_addr = RAM_FMAC_FW_ADDR_8800D80X2 + 0x01A8;
    aic_patch_addr = rd_patch_addr + 8;

    AICWFDBG(LOGINFO, "Read FW mem: %08x\n", rd_patch_addr);
    if ((ret = rwnx_send_dbg_mem_read_req(sdiodev, rd_patch_addr, &rd_patch_addr_cfm))) {
        AICWFDBG(LOGERROR, "setting base[0x%x] rd fail: %d\n", rd_patch_addr, ret);
        return ret;
    }
    AICWFDBG(LOGINFO, "rd_patch_addr_cfm %x=%x\n", rd_patch_addr_cfm.memaddr, rd_patch_addr_cfm.memdata);
    config_base = rd_patch_addr_cfm.memdata;

    if ((ret = rwnx_send_dbg_mem_read_req(sdiodev, aic_patch_addr, &rd_patch_addr_cfm))) {
        AICWFDBG(LOGERROR, "patch_str_base[0x%x] rd fail: %d\n", aic_patch_addr, ret);
        return ret;
    }
    AICWFDBG(LOGINFO, "rd_patch_addr_cfm %x=%x\n", rd_patch_addr_cfm.memaddr, rd_patch_addr_cfm.memdata);
    aic_patch_str_base = rd_patch_addr_cfm.memdata;

    #if (NEW_PATCH_BUFFER_MAP)
    rd_version_addr = RAM_FMAC_FW_ADDR_8800D80X2 + 0x01C;
    if ((ret = rwnx_send_dbg_mem_read_req(sdiodev, rd_version_addr, &rd_patch_addr_cfm))) {
        AICWFDBG(LOGERROR, "version val[0x%x] rd fail: %d\n", rd_version_addr, ret);
        return ret;
    }
    rd_version_val = rd_patch_addr_cfm.memdata;
    AICWFDBG(LOGINFO, "rd_version_val=%08X\n", rd_version_val);
    sdiodev->fw_version_uint = rd_version_val;
    patch_buff_addr = rd_patch_addr + 12;
    ret = rwnx_send_dbg_mem_read_req(sdiodev, patch_buff_addr, &rd_patch_addr_cfm);
    if (ret) {
        AICWFDBG(LOGERROR, "patch buf rd fail\n");
        return ret;
    }
    AICWFDBG(LOGINFO, "rd_patch_addr_cfm %x=%x\n", rd_patch_addr_cfm.memaddr, rd_patch_addr_cfm.memdata);
    patch_buff_base = rd_patch_addr_cfm.memdata;
    patch_addr = start_addr = patch_buff_base;
    #endif

    if ((ret = rwnx_send_dbg_mem_write_req(sdiodev, AIC_PATCH_ADDR(magic_num), AIC_PATCH_MAGIG_NUM))) {
        AICWFDBG(LOGERROR, "maigic_num[0x%x] write fail: %d\n", AIC_PATCH_ADDR(magic_num), ret);
        return ret;
    }

    if ((ret = rwnx_send_dbg_mem_write_req(sdiodev, AIC_PATCH_ADDR(magic_num_2), AIC_PATCH_MAGIG_NUM_2))) {
        AICWFDBG(LOGERROR, "maigic_num[0x%x] write fail: %d\n", AIC_PATCH_ADDR(magic_num_2), ret);
        return ret;
    }

    if ((ret = rwnx_send_dbg_mem_write_req(sdiodev, AIC_PATCH_ADDR(pair_start), patch_addr))) {
        AICWFDBG(LOGERROR, "pair_start[0x%x] write fail: %d\n", AIC_PATCH_ADDR(pair_start), ret);
        return ret;
    }

    if ((ret = rwnx_send_dbg_mem_write_req(sdiodev, AIC_PATCH_ADDR(pair_count), patch_cnt + adap_patch_cnt))) {
        AICWFDBG(LOGERROR, "pair_count[0x%x] write fail: %d\n", AIC_PATCH_ADDR(pair_count), ret);
        return ret;
    }

    for (cnt = 0; cnt < patch_cnt; cnt++) {
        if ((ret = rwnx_send_dbg_mem_write_req(sdiodev, start_addr+8*cnt, patch_tbl_d80x2[cnt][0]+config_base))) {
            AICWFDBG(LOGERROR, "%x write fail\n", start_addr+8*cnt);
            return ret;
        }
        if ((ret = rwnx_send_dbg_mem_write_req(sdiodev, start_addr+8*cnt+4, patch_tbl_d80x2[cnt][1]))) {
            AICWFDBG(LOGERROR, "%x write fail\n", start_addr+8*cnt+4);
            return ret;
        }
    }

    if (adap_test){
        int tmp_cnt = patch_cnt + adap_patch_cnt;
        AICWFDBG(LOGINFO, "%s set adap_test patch \r\n", __func__);
        for (cnt = patch_cnt; cnt < tmp_cnt; cnt++) {
            int tbl_idx = cnt - patch_cnt;
            if ((ret = rwnx_send_dbg_mem_write_req(sdiodev, start_addr+8*cnt, adaptivity_patch_tbl_d80x2[tbl_idx][0]+config_base))) {
                AICWFDBG(LOGERROR, "%x write fail\n", start_addr+8*cnt);
            return ret;
            }
            if ((ret = rwnx_send_dbg_mem_write_req(sdiodev, start_addr+8*cnt+4, adaptivity_patch_tbl_d80x2[tbl_idx][1]))) {
                AICWFDBG(LOGERROR, "%x write fail\n", start_addr+8*cnt+4);
            return ret;
            }
        }
    }

    if ((ret = rwnx_send_dbg_mem_write_req(sdiodev, AIC_PATCH_ADDR(block_size[0]), 0))) {
        AICWFDBG(LOGERROR, "block_size[0x%x] write fail: %d\n", AIC_PATCH_ADDR(block_size[0]), ret);
        return ret;
    }
    if ((ret = rwnx_send_dbg_mem_write_req(sdiodev, AIC_PATCH_ADDR(block_size[1]), 0))) {
        AICWFDBG(LOGERROR, "block_size[0x%x] write fail: %d\n", AIC_PATCH_ADDR(block_size[1]), ret);
        return ret;
    }
    if ((ret = rwnx_send_dbg_mem_write_req(sdiodev, AIC_PATCH_ADDR(block_size[2]), 0))) {
        AICWFDBG(LOGERROR, "block_size[0x%x] write fail: %d\n", AIC_PATCH_ADDR(block_size[2]), ret);
        return ret;
    }
    if ((ret = rwnx_send_dbg_mem_write_req(sdiodev, AIC_PATCH_ADDR(block_size[3]), 0))) {
        AICWFDBG(LOGERROR, "block_size[0x%x] write fail: %d\n", AIC_PATCH_ADDR(block_size[3]), ret);
        return ret;
    }

    return 0;
}

