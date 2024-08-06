
#include "../hyn_core.h"

#if HYN_APK_DEBUG_EN

#define HYN_PROC_DIR_NAME		"hyn_apk_tool"
#define HYN_PROC_NODE_NAME		"fops_nod"

static struct hyn_ts_data *hyn_tool_data = NULL;
static const struct hyn_ts_fuc* hyn_tool_fun = NULL;

static struct proc_dir_entry *g_tool_dir = NULL;
static struct proc_dir_entry *g_tool_node = NULL;

#define READ_TPINF 	    (0x0A)
#define WRITE_IIC 	    (0x1A)  //len
#define READ_REG 	    (0x2A)
#define READ_IIC 	    (0x3A)
#define SET_WORK_MODE   (0x7A)
#define RESET_IC 	    (0xCA)
#define UPDATA_FW	    (0xAC)


// #undef HYN_INFO
// #define HYN_INFO(...)

static ssize_t hyn_proc_tool_read(struct file *page,char __user *user_buf, size_t count, loff_t *f_pos)
{
	unsigned char *buf = NULL;
	int ret_len = 0,ret = 0;	
	int mt_total_len = hyn_tool_data->hw_info.fw_sensor_rxnum*hyn_tool_data->hw_info.fw_sensor_txnum*2
					+ (hyn_tool_data->hw_info.fw_sensor_rxnum + hyn_tool_data->hw_info.fw_sensor_txnum)*2;
	u8 *cmd = hyn_tool_data->host_cmd_save;
	mutex_lock(&hyn_tool_data->mutex_fs);

	if(*f_pos != 0){
		goto tool_read_end;
	}
	ret_len = (mt_total_len + sizeof(struct ts_frame))*2;
	if(ret_len > count){
		ret_len = count;
	}
	if(hyn_tool_data->work_mode == FAC_TEST_MODE){
		ret_len += mt_total_len;
	}
    buf = kzalloc(ret_len, GFP_KERNEL);
	// buf = kzalloc(ret_len, GFP_KERNEL);
    if(buf == NULL){
        HYN_ERROR("zalloc GFP_KERNEL memory failed.\n");
        goto tool_read_end;
    }
	if(DIFF_MODE <= hyn_tool_data->work_mode && FAC_TEST_MODE >= hyn_tool_data->work_mode){
		wait_event_interruptible_timeout(hyn_tool_data->wait_irq,(atomic_read(&hyn_tool_data->hyn_irq_flg)==1),msecs_to_jiffies(500)); //  
		atomic_set(&hyn_tool_data->hyn_irq_flg,0);
		if(hyn_tool_data->work_mode == FAC_TEST_MODE){
			if(NULL != hyn_tool_fun->tp_get_test_result){
				u8 restut[4];
				if(buf == NULL){
					HYN_ERROR("zalloc GFP_KERNEL memory failed.\n");
					goto tool_read_end;
				}
				*(int*)&restut = hyn_tool_fun->tp_get_test_result(buf,ret_len);
				HYN_INFO("test ret = %d",ret);
				ret = copy_to_user(user_buf,restut,4);
				ret = copy_to_user(user_buf+4,buf,count-4);
			}
		}
		else{
			ret_len = hyn_tool_fun->tp_get_dbg_data(buf,count);
			if(ret_len > 0)
				ret = copy_to_user(user_buf,buf,ret_len);
		}
	}
	else{
		if(cmd[0] == READ_REG){  //0:head 1~4:reg 5:reg_len  6~7:read_len
			u32 reg_addr = U8TO32(cmd[4],cmd[3],cmd[2],cmd[1]); //eg (read A1 A2 len 256):2A 00 00 A1 A2 02 FF 00
			ret = hyn_wr_reg(hyn_tool_data,reg_addr,cmd[5],buf,count);
			ret |= copy_to_user(user_buf,buf,count);
		}
		else if(cmd[0] == READ_IIC){ //0:head 1~2:read_len
			ret = hyn_read_data(hyn_tool_data,buf,count);
			ret |= copy_to_user(user_buf,buf,count);
		}
		else if(cmd[0] == READ_TPINF){
			ret_len = sizeof(hyn_tool_data->hw_info);
			ret = copy_to_user(user_buf,&hyn_tool_data->hw_info,ret_len);
		}
		else if(cmd[0] == UPDATA_FW){
			ret_len = sizeof(hyn_tool_data->fw_updata_process);
			ret = copy_to_user(user_buf,&hyn_tool_data->fw_updata_process,ret_len);
			msleep(1);
		}
	}
tool_read_end:
	if(!IS_ERR_OR_NULL(buf)){	
		kfree(buf);
	}
	mutex_unlock(&hyn_tool_data->mutex_fs);
	// HYN_INFO("count = %d copy len = %d ret = %d\r\n",(int)count,ret_len,ret);
	return count;
}

static ssize_t hyn_proc_tool_write(struct file *file, const char __user *buffer,size_t count, loff_t *data)
{
    u8 *cmd = NULL;
	int ret = 0;
	HYN_ENTER();
	mutex_lock(&hyn_tool_data->mutex_fs);
	#define MAX_W_SIZE (1024+8)
	if(count > MAX_W_SIZE){
		ret = -1;
		HYN_ERROR("count > MAX_W_SIZE\n");
		goto ERRO_WEND;
	}
	cmd = kzalloc(count, GFP_KERNEL);
	if(IS_ERR_OR_NULL(cmd)){
		ret = -1;
		HYN_ERROR("kzalloc failed\n");
		goto ERRO_WEND;
	}
	if(copy_from_user(cmd, buffer, count)){
		ret = -1;
		HYN_ERROR("copy data from user space failed.\n");
		goto ERRO_WEND;
	}
	memcpy(hyn_tool_data->host_cmd_save,cmd,count <sizeof(hyn_tool_data->host_cmd_save) ? count:sizeof(hyn_tool_data->host_cmd_save));
	HYN_INFO("cmd:0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x***len:%d ", cmd[0], cmd[1],cmd[2], cmd[3],cmd[4], cmd[5],cmd[6], cmd[7],(u16)count);

	switch(cmd[0]){
		case UPDATA_FW: // apk download bin
			HYN_INFO("updata file_name =%s",&cmd[1]);
			strcpy(hyn_tool_data->fw_file_name,&cmd[1]);
			hyn_tool_data->fw_updata_process = 0;
			if(0==queue_work(hyn_tool_data->hyn_workqueue,&hyn_tool_data->work_updata_fw)){
				HYN_ERROR("queue_work work_updata_fw failed");
			}
			break;
		case SET_WORK_MODE://
			atomic_set(&hyn_tool_data->hyn_irq_flg,0);
			ret = hyn_tool_fun->tp_set_workmode(cmd[1],0);
			break;
		case RESET_IC:
			hyn_tool_fun->tp_rest();
			break;
		case WRITE_IIC: //
			ret = hyn_write_data(hyn_tool_data,&cmd[1],2,count-1);
			break;
		default:
			break;
	}
ERRO_WEND:
	if(!IS_ERR_OR_NULL(cmd)){
		kfree(cmd);
	}
	mutex_unlock(&hyn_tool_data->mutex_fs);
	return ret == 0 ? count:-1;
}

static const struct file_operations proc_tool_fops = {
	.owner		= THIS_MODULE,
	.read	    = hyn_proc_tool_read,
	.write		= hyn_proc_tool_write, 	
};

int hyn_tool_fs_int(struct hyn_ts_data *ts_data)
{
    hyn_tool_data = ts_data;
    hyn_tool_fun = ts_data->hyn_fuc_used;
	g_tool_dir = proc_mkdir(HYN_PROC_DIR_NAME, NULL);	
	if (IS_ERR_OR_NULL(g_tool_dir)){
		HYN_INFO("proc_mkdir %s failed.\n",HYN_PROC_DIR_NAME);
		return -1;
	}
	g_tool_node = proc_create_data(HYN_PROC_NODE_NAME, 0777 | S_IFREG, g_tool_dir, (void *)&proc_tool_fops, (void *)ts_data->client);
				//proc_create(HYN_PROC_NODE_NAME, 0777|S_IFREG, g_tool_dir, &proc_tool_fops);
	if (IS_ERR_OR_NULL(g_tool_node)) {
		HYN_INFO("proc_create %s failed.\n",HYN_PROC_NODE_NAME);
		remove_proc_entry(HYN_PROC_DIR_NAME, NULL);
		return -ENOMEM;
	}
	HYN_INFO("apk node creat success");
	return 0;
}

void hyn_tool_fs_exit(void)
{
	if(!IS_ERR_OR_NULL(g_tool_node)){
		remove_proc_entry(HYN_PROC_NODE_NAME, g_tool_dir);
	}
	if(!IS_ERR_OR_NULL(g_tool_dir)){
		remove_proc_entry(HYN_PROC_DIR_NAME, NULL);
	}
}

#endif

