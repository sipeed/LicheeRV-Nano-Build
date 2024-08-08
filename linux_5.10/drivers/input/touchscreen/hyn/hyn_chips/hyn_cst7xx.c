#include "../hyn_core.h"
#include "cst7xx_fw.h"

#define CUSTOM_SENSOR_NUM  	(10)

#define BOOT_I2C_ADDR   (0x6A)
#define MAIN_I2C_ADDR   (0x15)
#define RW_REG_LEN   (2)
#define CST7XX_BIN_SIZE    (15*1024)
static struct hyn_ts_data *hyn_7xxdata = NULL;

static int cst7xx_updata_judge(u8 *p_fw, u16 len);
static u32 cst7xx_read_checksum(void);
static int cst7xx_updata_tpinfo(void);
static int cst7xx_enter_boot(void);
static int cst7xx_set_workmode(enum work_mode mode,u8 enable);
static void cst7xx_rst(void);

static int cst7xx_init(struct hyn_ts_data* ts_data)
{
    int ret = 0;
    u8 buf[4];
    HYN_ENTER();
    hyn_7xxdata = ts_data;
    ret = cst7xx_enter_boot();
    if(ret == FALSE){
        HYN_ERROR("cst7xx_enter_boot failed");
        return FALSE;
    }
    hyn_7xxdata->fw_updata_addr = (u8*)fw_bin;
    hyn_7xxdata->fw_updata_len = CST7XX_BIN_SIZE;

    hyn_7xxdata->hw_info.ic_fw_checksum = cst7xx_read_checksum();
    if(hyn_7xxdata->need_updata_fw ==0){
        hyn_wr_reg(hyn_7xxdata,0xA006EE,3,buf,0); //exit boot
        cst7xx_rst();
        mdelay(50);
        hyn_set_i2c_addr(hyn_7xxdata,MAIN_I2C_ADDR);
        ret = cst7xx_updata_tpinfo();
        cst7xx_set_workmode(NOMAL_MODE,0);
        hyn_7xxdata->need_updata_fw = cst7xx_updata_judge((u8*)fw_bin,CST7XX_BIN_SIZE);
    }
    if(hyn_7xxdata->need_updata_fw){
        HYN_INFO("need updata FW !!!");
    }
    return TRUE;
}


static int  cst7xx_enter_boot(void)
{
    uint8_t t;
    hyn_set_i2c_addr(hyn_7xxdata,BOOT_I2C_ADDR);
    for (t = 5;; t += 2)
    {
        int ok = FALSE;
        uint8_t i2c_buf[4] = {0};

        if (t >= 15){
            return FALSE;
        }
        cst7xx_rst();
        mdelay(t);
        ok = hyn_wr_reg(hyn_7xxdata, 0xA001AA, 3, i2c_buf, 0);
        if(ok == FALSE){
            continue;
        }
        ok = hyn_wr_reg(hyn_7xxdata, 0xA003,  2, i2c_buf, 1);
        if(ok == FALSE){
            continue;
        }
        if (i2c_buf[0] != 0x55){
            continue;
        }
        break;
    }
    return TRUE;
}


static int write_code(u8 *bin_addr,uint8_t retry)
{
    uint16_t i,t;//,j;
    int ok = FALSE;
    u8 i2c_buf[512+2];
	
	bin_addr+=6;
	
    for ( i = 0;i < CST7XX_BIN_SIZE; i += 512)
    {
        i2c_buf[0] = 0xA0;
        i2c_buf[1] = 0x14;

        i2c_buf[2] = i;
        i2c_buf[3] = i >> 8;
        ok = hyn_write_data(hyn_7xxdata, i2c_buf,RW_REG_LEN, 4);
        if (ok == FALSE){
            break;
        }

        i2c_buf[0] = 0xA0;
        i2c_buf[1] = 0x18;
		if(0 == hyn_7xxdata->fw_file_name[0]){
			memcpy(i2c_buf + 2, bin_addr + i, 512); 
		}
		else{
			ok = copy_for_updata(hyn_7xxdata,i2c_buf + 2,i+6,512);
			if(ok)break;
		}
        ok = hyn_write_data(hyn_7xxdata, i2c_buf,RW_REG_LEN, 514);
        if (ok == FALSE){
            break;
        }

        ok = hyn_wr_reg(hyn_7xxdata, 0xA004EE, 3,i2c_buf,0);
        if (ok == FALSE){
            break;
        }

        mdelay(100 * retry);

        for (t = 0;; t ++)
        {
            if (t >= 50){
                return FALSE;
            }
            mdelay(5);

            ok = hyn_wr_reg(hyn_7xxdata,0xA005,2,i2c_buf,1);
            if (ok == FALSE){
                continue;
            }
            if (i2c_buf[0] != 0x55){
                continue;
            }
            break;
        }
    }
    return ok;
}


static uint32_t cst7xx_read_checksum(void)
{
    int ok = FALSE,t;
    uint8_t i2c_buf[4] = {0};
    uint32_t value = 0;
    int chip_checksum_ok = FALSE;
    // firmware checksum
    ok = hyn_wr_reg(hyn_7xxdata, 0xA00300,  3, i2c_buf, 0);
    if (ok == FALSE){
        return value;
    }
    mdelay(100);
    for (t = 0;; t += 10)
    {
        if (t >= 1000){
            //return FALSE;
            break;
        }

        mdelay(10);

        ok = hyn_wr_reg(hyn_7xxdata, 0xA000,  2, i2c_buf, 1);
        if (ok == FALSE){
            continue;
        }

        if (i2c_buf[0] == 1){
            chip_checksum_ok = TRUE;
            break;
        }
        else if (i2c_buf[0] == 2){
            chip_checksum_ok = FALSE;
            continue;
        }
    }

    if(chip_checksum_ok == FALSE){
        hyn_7xxdata->need_updata_fw = 1;
    }
    else{
        ok = hyn_wr_reg(hyn_7xxdata, 0xA008,  2, i2c_buf, 2);
        if (ok == FALSE){
            //return FALSE;
            return value;
        }
        value = i2c_buf[0];
        value |= (uint16_t)(i2c_buf[1]) << 8;
    }

    return value;
}


static int cst7xx_updata_fw(u8 *bin_addr, u16 len)
{ 
    int retry = 0;
    int ok_copy = TRUE;
    int ok = FALSE;
    u8 i2c_buf[4];
    u32 fw_checksum = 0;
    // len = len;
    HYN_ENTER();
    if(0 == hyn_7xxdata->fw_file_name[0]){
        fw_checksum =U8TO16(bin_addr[5],bin_addr[4]);
    }
    else{
        ok = copy_for_updata(hyn_7xxdata,i2c_buf,4,2);
        if(ok)  goto UPDATA_END;
        fw_checksum = U8TO16(i2c_buf[1],i2c_buf[0]);
    }

    hyn_irq_set(hyn_7xxdata,DISABLE);

    for(retry = 1; retry<10; retry++){
        ok = cst7xx_enter_boot();
        if (ok == FALSE){
            continue;
        }

        ok = write_code(bin_addr,retry);
        if (ok == FALSE){
            continue;
        }

        hyn_7xxdata->hw_info.ic_fw_checksum = cst7xx_read_checksum();
        if(fw_checksum != hyn_7xxdata->hw_info.ic_fw_checksum){
            continue;
        }
            
        if(retry>=5){
            ok_copy = FALSE;
            break;
        }
        break;
    }
    UPDATA_END:
    hyn_wr_reg(hyn_7xxdata,0xA006EE,3,i2c_buf,0); //exit boot
    mdelay(2);
    cst7xx_rst();
    mdelay(50);

    hyn_set_i2c_addr(hyn_7xxdata,MAIN_I2C_ADDR);   

    if(ok_copy == TRUE){
        cst7xx_updata_tpinfo();
        HYN_INFO("updata_fw success");
    }
    else{
        HYN_ERROR("updata_fw failed");
    }
    hyn_irq_set(hyn_7xxdata,ENABLE);

    return ok_copy;
}


static int cst7xx_updata_tpinfo(void)
{
    u8 buf[8];
    struct tp_info *ic = &hyn_7xxdata->hw_info;
    int ret = 0;

    ret = hyn_wr_reg(hyn_7xxdata,0xA7,1,buf,4);
    if(ret == FALSE){
        HYN_ERROR("cst7xx_updata_tpinfo failed");
        return FALSE;
    }

    ic->fw_sensor_txnum = CUSTOM_SENSOR_NUM;
    ic->fw_sensor_rxnum = 0;
    ic->fw_key_num = 0;
    ic->fw_res_y = 0;
    ic->fw_res_x = 0;
    ic->fw_project_id = buf[1];
    ic->fw_chip_type = buf[0];
    ic->fw_ver = buf[2];

    HYN_INFO("IC_info fw_project_id:%04x ictype:%04x fw_ver:%x checksum:%#x",ic->fw_project_id,ic->fw_chip_type,ic->fw_ver,ic->ic_fw_checksum);
    return TRUE;
}

static int cst7xx_updata_judge(u8 *p_fw, u16 len)
{
    u32 f_checksum,f_fw_ver,f_ictype,f_fw_project_id;
    u8 *p_data = p_fw ; 
    struct tp_info *ic = &hyn_7xxdata->hw_info;

    f_fw_project_id = 0;
    f_ictype = 0;
    f_fw_ver = 0;

    f_checksum = U8TO16(p_data[5],p_data[4]);

    HYN_INFO("Bin_info fw_project_id:%04x ictype:%04x fw_ver:%x checksum:%#x",f_fw_project_id,f_ictype,f_fw_ver,f_checksum);

    if(f_checksum != ic->ic_fw_checksum ){
        return 1; //need updata
    }
    return 0;
}

//------------------------------------------------------------------------------//
static int cst7xx_set_workmode(enum work_mode mode,u8 enable)
{
    hyn_7xxdata->work_mode = mode;
    if(mode != NOMAL_MODE){
        hyn_esdcheck_switch(hyn_7xxdata,DISABLE);
    }
    switch(mode){
        case NOMAL_MODE:
            hyn_esdcheck_switch(hyn_7xxdata,ENABLE);
            hyn_irq_set(hyn_7xxdata,ENABLE);
            hyn_wr_reg(hyn_7xxdata,0xFE00,2,NULL,0);
            break;
        case GESTURE_MODE:
            hyn_wr_reg(hyn_7xxdata,0xD001,2,NULL,0);
            break;
        case LP_MODE:
            break;
        case DIFF_MODE:
        case RAWDATA_MODE:
            hyn_wr_reg(hyn_7xxdata,0xFEF8,2,NULL,0);
            break;
        case FAC_TEST_MODE:
            //hyn_wr_reg(hyn_7xxdata,0xD119,2,NULL,0);
            break;
        case DEEPSLEEP:
            hyn_irq_set(hyn_7xxdata,DISABLE);
            hyn_wr_reg(hyn_7xxdata,0xA503,2,NULL,0);
            break;
        default :
            //hyn_esdcheck_switch(hyn_7xxdata,ENABLE);
            hyn_7xxdata->work_mode = NOMAL_MODE;
            break;
    }
    return 0;
}

static int cst7xx_prox_handle(u8 cmd)
{
    int ret = 0;
    switch(cmd){
        case 1: //enable
            hyn_7xxdata->prox_is_enable = 1;
            hyn_7xxdata->prox_state = 0;
            ret = hyn_wr_reg(hyn_7xxdata,0xB001,2,NULL,0);
            break;
        case 0: //disable
            hyn_7xxdata->prox_is_enable = 0;
            hyn_7xxdata->prox_state = 0;
            ret = hyn_wr_reg(hyn_7xxdata,0xB000,2,NULL,0);
            break;
        default: 
            break;
    }
    return ret;
}

static void cst7xx_rst(void)
{
    gpio_set_value(hyn_7xxdata->plat_data.reset_gpio,0);
    msleep(5);
    gpio_set_value(hyn_7xxdata->plat_data.reset_gpio,1);
}



static int cst7xx_supend(void)
{
    HYN_ENTER();
    cst7xx_set_workmode(DEEPSLEEP,0);
    return 0;
}

static int cst7xx_resum(void)
{
    cst7xx_rst();
    msleep(50);
    cst7xx_set_workmode(NOMAL_MODE,0);
    return 0;
}

static int cst7xx_report(void)
{
    uint8_t i = 0;
    uint8_t i2c_buf[3+6*MAX_POINTS_REPORT] = {0};
    uint16_t x,y;
    uint8_t id = 0,index = 0;
    struct hyn_plat_data *dt = &hyn_7xxdata->plat_data;

    memset(&hyn_7xxdata->rp_buf,0,sizeof(hyn_7xxdata->rp_buf));
    hyn_7xxdata->rp_buf.report_need = REPORT_NONE;

    
    if(hyn_7xxdata->work_mode == GESTURE_MODE){
        static const uint8_t ges_map[][2] = {{0x24,IDX_POWER},{0x22,IDX_UP},{0x23,IDX_DOWN},{0x20,IDX_LEFT},{0x21,IDX_RIGHT},
        {0x34,IDX_C},{0x33,IDX_e},{0x32,IDX_M},{0x30,IDX_O},{0x46,IDX_S},{0x54,IDX_V},{0x31,IDX_W},{0x65,IDX_Z}};
        if(hyn_wr_reg(hyn_7xxdata,0xD3,1,i2c_buf,1)){
            goto FAILD_END;
        }
        index = sizeof(ges_map)/2;
        hyn_7xxdata->gesture_id  = IDX_NULL;
        for(i=0; i<index; i++){
            if(ges_map[i][0] == i2c_buf[0]){
                hyn_7xxdata->gesture_id = ges_map[i][1];
                hyn_7xxdata->rp_buf.report_need = REPORT_GES;
                break;
            }
        }
        return TRUE;
    }
    else{
        if(hyn_wr_reg(hyn_7xxdata,0x00,1,i2c_buf,(3+6*2))){
            goto FAILD_END;
        }
        if(hyn_7xxdata->prox_is_enable){
            u8 state=0;
            if(i2c_buf[1]==0xE0 || i2c_buf[1]==0){
                state = PS_FAR_AWAY;
            }
            else if(i2c_buf[1]==0xC0){
                state = PS_NEAR;
            }
            if(hyn_7xxdata->prox_state != state){
                hyn_7xxdata->rp_buf.report_need = REPORT_PROX;
                return TRUE;
            }
        }
        hyn_7xxdata->rp_buf.rep_num  = i2c_buf[2];
        for(i = 0 ; i < 2 ; i++){
            id = (i2c_buf[5 + i*6] & 0xf0)>>4;
            if(id > 1) continue;
            x = (i2c_buf[3 + i*6] & 0x0f);
            x = (x<<8) + i2c_buf[4 + i*6];

            y = (i2c_buf[5 + i*6] & 0x0f);
            y = (y<<8) + i2c_buf[6 + i*6];

            hyn_7xxdata->rp_buf.pos_info[index].pos_id = id;
            hyn_7xxdata->rp_buf.pos_info[index].event = (i2c_buf[3 + i*6] & 0x40) ? 0:1;
            hyn_7xxdata->rp_buf.pos_info[index].pos_x = x ;
            hyn_7xxdata->rp_buf.pos_info[index].pos_y = y ;
            // hyn_7xxdata->rp_buf.pos_info[index].pres_z = (i2c_buf[7 + i*6] <<8) + i2c_buf[8 + i*6] ;
            hyn_7xxdata->rp_buf.pos_info[index].pres_z = 3+(x&0x03); //press mast chang
            index++;
            hyn_7xxdata->rp_buf.report_need = REPORT_POS;
        }

        if(dt->key_num){
            i = dt->key_num;
            while(i){
                i--;
                if(dt->key_y_coords ==hyn_7xxdata->rp_buf.pos_info[0].pos_y && dt->key_x_coords[i] == hyn_7xxdata->rp_buf.pos_info[0].pos_x){
                    hyn_7xxdata->rp_buf.key_id = i;
                    hyn_7xxdata->rp_buf.key_state = hyn_7xxdata->rp_buf.pos_info[0].event;
                    hyn_7xxdata->rp_buf.report_need = REPORT_KEY;
                }
            }
        }
    }
    return TRUE;
    FAILD_END:
    HYN_ERROR("read report data failed");
    return FALSE;
}

static u32 cst7xx_check_esd(void)
{
    return 0;
}



static int cst7xx_get_dbg_data(u8 *buf, u16 len)
{
    int ret = -1,read_len = hyn_7xxdata->hw_info.fw_sensor_txnum*2;
    if(len < hyn_7xxdata->hw_info.fw_sensor_txnum*2){
        HYN_ERROR("buf too small");
        return -1;
    }
    switch (hyn_7xxdata->work_mode){
        case DIFF_MODE:
            ret = hyn_wr_reg(hyn_7xxdata, 0x41, 1,buf,read_len);
            break;
        case RAWDATA_MODE:
            ret = hyn_wr_reg(hyn_7xxdata, 0x61, 1,buf,read_len); 
            break;
        default:
            HYN_ERROR("work_mode:%d",hyn_7xxdata->work_mode);
            break;
    }
    return ret==0 ? read_len:-1;
}


static int cst7xx_get_test_result(u8 *buf, u16 len)
{
    return 0;
}



const struct hyn_ts_fuc cst7xx_fuc = {
    .tp_rest = cst7xx_rst,
    .tp_report = cst7xx_report,
    .tp_supend = cst7xx_supend,
    .tp_resum = cst7xx_resum,
    .tp_chip_init = cst7xx_init,
    .tp_updata_fw = cst7xx_updata_fw,
    .tp_set_workmode = cst7xx_set_workmode,
    .tp_check_esd = cst7xx_check_esd,
    .tp_prox_handle = cst7xx_prox_handle,
    .tp_get_dbg_data = cst7xx_get_dbg_data,
    .tp_get_test_result = cst7xx_get_test_result
};


