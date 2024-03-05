// $Module: cmdq $
// $RegisterBank Version: V 1.0.00 $
// $Author: Test 123 $
// $Date: Sun, 05 Dec 2021 04:15:49 PM $
//

#ifndef __REG_CMDQ_STRUCT_H__
#define __REG_CMDQ_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*CMDQ INT event;*/
		uint32_t reg_cmdq_int:1;
		/*CMDQ end interrupt event when CMDQ decode is finish;*/
		uint32_t reg_cmdq_end:1;
		/*CMDQ wait interrupt event when wait for flag;*/
		uint32_t reg_cmdq_wait:1;
		/*isp apb error from ISP slave;*/
		uint32_t reg_isp_pslverr:1;
		/*Task end interrupt event when task is finish;*/
		uint32_t reg_task_end:1;
	};
	uint32_t val;
} CMDQ_INT_EVENT_C;
typedef union {
	struct {
		/*CMDQ INT event interrupt enable;*/
		uint32_t reg_cmdq_int_en:1;
		/*CMDQ end interupt enable;*/
		uint32_t reg_cmdq_end_en:1;
		/*CMDQ wait interrupt enable when wait for flag;*/
		uint32_t reg_cmdq_wait_en:1;
		/*isp pslverr interrupt enable;*/
		uint32_t reg_isp_pslverr_en:1;
		/*Task end interrupt enable;*/
		uint32_t reg_task_end_en:1;
	};
	uint32_t val;
} CMDQ_INT_EN_C;
typedef union {
	struct {
		/*DMA start address;*/
		uint32_t reg_dma_addr_l;
	};
	uint32_t val;
} CMDQ_DMA_ADDR_L_C;
typedef union {
	struct {
		/*DMA start address;*/
		uint32_t reg_dma_addr_h:8;
	};
	uint32_t val;
} CMDQ_DMA_ADDR_H_C;
typedef union {
	struct {
		/*DMA transfer count in bytes;*/
		uint32_t reg_dma_cnt:32;
	};
	uint32_t val;
} CMDQ_DMA_CNT_C;
typedef union {
	struct {
		/*DMA reserved bit;*/
		uint32_t reg_dma_rsv:1;
		/*Enable ADMA mode;*/
		uint32_t reg_adma_en:1;
		/*Enable Task Management mode;*/
		uint32_t reg_task_en:1;
	};
	uint32_t val;
} CMDQ_DMA_CONFIG_C;
typedef union {
	struct {
		/*Max burst leng for AXI;*/
		uint32_t reg_max_burst_len:8;
		/*Outstanding enable;*/
		uint32_t reg_ot_enable:1;
		/*Overwrite axi config by SW;*/
		uint32_t reg_sw_overwrite:1;
	};
	uint32_t val;
} CMDQ_AXI_CONFIG_C;
typedef union {
	struct {
		/*Job start;*/
		uint32_t reg_job_start:1;
		/*Restart next cmd;*/
		uint32_t reg_cmd_restart:1;
		/*restart cmd by
		1 : hw
		0 : sw;*/
		uint32_t reg_restart_hw_mod:1;
		/*enable cmdq_idle for power saving
		1 : enable
		0 : disable;*/
		uint32_t reg_cmdq_idle_en:1;
	};
	uint32_t val;
} CMDQ_JOB_CTL_C;
typedef union {
	struct {
		/*APB Base address;*/
		uint32_t reg_base_addr:16;
		/*APB pprot[2:0];*/
		uint32_t reg_apb_pprot:3;
	};
	uint32_t val;
} CMDQ_APB_PARA_C;
typedef union {
	struct {
		/*CMDQ debug bus 0;*/
		uint32_t reg_debus0:32;
	};
	uint32_t val;
} CMDQ_DEBUG_BUS0_C;
typedef union {
	struct {
		/*CMDQ debug bus 1;*/
		uint32_t reg_debus1:32;
	};
	uint32_t val;
} CMDQ_DEBUG_BUS1_C;
typedef union {
	struct {
		/*CMDQ debug bus 2;*/
		uint32_t reg_debus2:32;
	};
	uint32_t val;
} CMDQ_DEBUG_BUS2_C;
typedef union {
	struct {
		/*CMDQ debug bus 3;*/
		uint32_t reg_debus3:32;
	};
	uint32_t val;
} CMDQ_DEBUG_BUS3_C;
typedef union {
	struct {
		/*CMDQ debug bus selection;*/
		uint32_t reg_debus_sel:2;
	};
	uint32_t val;
} CMDQ_DEBUG_BUS_SEL_C;
typedef union {
	struct {
		/*Dummy register;*/
		uint32_t reg_dummy:32;
	};
	uint32_t val;
} CMDQ_DUMMY_C;
typedef union {
	struct {
		/*task done status;*/
		uint32_t reg_task_done:8;
	};
	uint32_t val;
} CMDQ_TASK_DONE_STS_C;
typedef union {
	struct {
		/*DMA start address for task 0;*/
		uint32_t reg_dma_addr_tsk0;
	};
	uint32_t val;
} CMDQ_DMA_ADDR_TSK0_C;
typedef union {
	struct {
		/*DMA transfer count in bytes for task 0;*/
		uint32_t reg_dma_cnt_tsk0:32;
	};
	uint32_t val;
} CMDQ_DMA_CNT_TSK0_C;
typedef union {
	struct {
		/*DMA start address for task 1;*/
		uint32_t reg_dma_addr_tsk1;
	};
	uint32_t val;
} CMDQ_DMA_ADDR_TSK1_C;
typedef union {
	struct {
		/*DMA transfer count in bytes for task 1;*/
		uint32_t reg_dma_cnt_tsk1:32;
	};
	uint32_t val;
} CMDQ_DMA_CNT_TSK1_C;
typedef union {
	struct {
		/*DMA start address for task 2;*/
		uint32_t reg_dma_addr_tsk2;
	};
	uint32_t val;
} CMDQ_DMA_ADDR_TSK2_C;
typedef union {
	struct {
		/*DMA transfer count in bytes for task 2;*/
		uint32_t reg_dma_cnt_tsk2:32;
	};
	uint32_t val;
} CMDQ_DMA_CNT_TSK2_C;
typedef union {
	struct {
		/*DMA start address for task 3;*/
		uint32_t reg_dma_addr_tsk3;
	};
	uint32_t val;
} CMDQ_DMA_ADDR_TSK3_C;
typedef union {
	struct {
		/*DMA transfer count in bytes for task 3;*/
		uint32_t reg_dma_cnt_tsk3:32;
	};
	uint32_t val;
} CMDQ_DMA_CNT_TSK3_C;
typedef union {
	struct {
		/*DMA start address for task 4;*/
		uint32_t reg_dma_addr_tsk4;
	};
	uint32_t val;
} CMDQ_DMA_ADDR_TSK4_C;
typedef union {
	struct {
		/*DMA transfer count in bytes for task 4;*/
		uint32_t reg_dma_cnt_tsk4:32;
	};
	uint32_t val;
} CMDQ_DMA_CNT_TSK4_C;
typedef union {
	struct {
		/*DMA start address for task 5;*/
		uint32_t reg_dma_addr_tsk5;
	};
	uint32_t val;
} CMDQ_DMA_ADDR_TSK5_C;
typedef union {
	struct {
		/*DMA transfer count in bytes for task 5;*/
		uint32_t reg_dma_cnt_tsk5:32;
	};
	uint32_t val;
} CMDQ_DMA_CNT_TSK5_C;
typedef union {
	struct {
		/*DMA start address for task 6;*/
		uint32_t reg_dma_addr_tsk6;
	};
	uint32_t val;
} CMDQ_DMA_ADDR_TSK6_C;
typedef union {
	struct {
		/*DMA transfer count in bytes for task 6;*/
		uint32_t reg_dma_cnt_tsk6:32;
	};
	uint32_t val;
} CMDQ_DMA_CNT_TSK6_C;
typedef union {
	struct {
		/*DMA start address for task 7;*/
		uint32_t reg_dma_addr_tsk7;
	};
	uint32_t val;
} CMDQ_DMA_ADDR_TSK7_C;
typedef union {
	struct {
		/*DMA transfer count in bytes for task 7;*/
		uint32_t reg_dma_cnt_tsk7:32;
	};
	uint32_t val;
} CMDQ_DMA_CNT_TSK7_C;
typedef struct _CMDQ_C {
	CMDQ_INT_EVENT_C INT_EVENT;
	CMDQ_INT_EN_C INT_EN;
	CMDQ_DMA_ADDR_L_C DMA_ADDR_L;
	CMDQ_DMA_ADDR_H_C DMA_ADDR_H;
	CMDQ_DMA_CNT_C DMA_CNT;
	CMDQ_DMA_CONFIG_C DMA_CONFIG;
	CMDQ_AXI_CONFIG_C AXI_CONFIG;
	CMDQ_JOB_CTL_C JOB_CTL;
	uint32_t _APB_PARA_0; // 0x20
	CMDQ_APB_PARA_C APB_PARA;
	CMDQ_DEBUG_BUS0_C DEBUG_BUS0;
	CMDQ_DEBUG_BUS1_C DEBUG_BUS1;
	CMDQ_DEBUG_BUS2_C DEBUG_BUS2;
	CMDQ_DEBUG_BUS3_C DEBUG_BUS3;
	CMDQ_DEBUG_BUS_SEL_C DEBUG_BUS_SEL;
	CMDQ_DUMMY_C DUMMY;
	CMDQ_TASK_DONE_STS_C TASK_DONE_STS;
	uint32_t _DMA_ADDR_TSK0_0;
	uint32_t _DMA_ADDR_TSK0_1;
	uint32_t _DMA_ADDR_TSK0_2;
	uint32_t _DMA_ADDR_TSK0_3;
	uint32_t _DMA_ADDR_TSK0_4;
	uint32_t _DMA_ADDR_TSK0_5;
	uint32_t _DMA_ADDR_TSK0_6;
	uint32_t _DMA_ADDR_TSK0_7;
	uint32_t _DMA_ADDR_TSK0_8;
	uint32_t _DMA_ADDR_TSK0_9;
	uint32_t _DMA_ADDR_TSK0_10;
	uint32_t _DMA_ADDR_TSK0_11;
	uint32_t _DMA_ADDR_TSK0_12;
	uint32_t _DMA_ADDR_TSK0_13;
	uint32_t _DMA_ADDR_TSK0_14;
	CMDQ_DMA_ADDR_TSK0_C DMA_ADDR_TSK0;
	uint32_t _DMA_CNT_TSK0_0;
	CMDQ_DMA_CNT_TSK0_C DMA_CNT_TSK0;
	uint32_t _DMA_ADDR_TSK1_0;
	CMDQ_DMA_ADDR_TSK1_C DMA_ADDR_TSK1;
	uint32_t _DMA_CNT_TSK1_0;
	CMDQ_DMA_CNT_TSK1_C DMA_CNT_TSK1;
	uint32_t _DMA_ADDR_TSK2_0;
	CMDQ_DMA_ADDR_TSK2_C DMA_ADDR_TSK2;
	uint32_t _DMA_CNT_TSK2_0;
	CMDQ_DMA_CNT_TSK2_C DMA_CNT_TSK2;
	uint32_t _DMA_ADDR_TSK3_0;
	CMDQ_DMA_ADDR_TSK3_C DMA_ADDR_TSK3;
	uint32_t _DMA_CNT_TSK3_0;
	CMDQ_DMA_CNT_TSK3_C DMA_CNT_TSK3;
	uint32_t _DMA_ADDR_TSK4_0;
	CMDQ_DMA_ADDR_TSK4_C DMA_ADDR_TSK4;
	uint32_t _DMA_CNT_TSK4_0;
	CMDQ_DMA_CNT_TSK4_C DMA_CNT_TSK4;
	uint32_t _DMA_ADDR_TSK5_0;
	CMDQ_DMA_ADDR_TSK5_C DMA_ADDR_TSK5;
	uint32_t _DMA_CNT_TSK5_0;
	CMDQ_DMA_CNT_TSK5_C DMA_CNT_TSK5;
	uint32_t _DMA_ADDR_TSK6_0;
	CMDQ_DMA_ADDR_TSK6_C DMA_ADDR_TSK6;
	uint32_t _DMA_CNT_TSK6_0;
	CMDQ_DMA_CNT_TSK6_C DMA_CNT_TSK6;
	uint32_t _DMA_ADDR_TSK7_0;
	CMDQ_DMA_ADDR_TSK7_C DMA_ADDR_TSK7;
	uint32_t _DMA_CNT_TSK7_0;
	CMDQ_DMA_CNT_TSK7_C DMA_CNT_TSK7;
} CMDQ_C;
#ifdef __cplusplus
#error "removed."
#else /* !ifdef __cplusplus */
#define _DEFINE_CMDQ_C \
{\
	.INT_EVENT.reg_cmdq_int = 0x0,\
	.INT_EVENT.reg_cmdq_end = 0x0,\
	.INT_EVENT.reg_cmdq_wait = 0x0,\
	.INT_EVENT.reg_isp_pslverr = 0x0,\
	.INT_EVENT.reg_task_end = 0x0,\
	.INT_EN.reg_cmdq_int_en = 0x0,\
	.INT_EN.reg_cmdq_end_en = 0x0,\
	.INT_EN.reg_cmdq_wait_en = 0x0,\
	.INT_EN.reg_isp_pslverr_en = 0x0,\
	.INT_EN.reg_task_end_en = 0x0,\
	.DMA_ADDR_L.reg_dma_addr_l = 0x0,\
	.DMA_ADDR_H.reg_dma_addr_h = 0x0,\
	.DMA_CNT.reg_dma_cnt = 0x0,\
	.DMA_CONFIG.reg_dma_rsv = 0x0,\
	.DMA_CONFIG.reg_adma_en = 0x0,\
	.DMA_CONFIG.reg_task_en = 0x0,\
	.AXI_CONFIG.reg_max_burst_len = 0x3,\
	.AXI_CONFIG.reg_ot_enable = 0x1,\
	.AXI_CONFIG.reg_sw_overwrite = 0x0,\
	.JOB_CTL.reg_job_start = 0x0,\
	.JOB_CTL.reg_cmd_restart = 0x0,\
	.JOB_CTL.reg_restart_hw_mod = 0x0,\
	.JOB_CTL.reg_cmdq_idle_en = 0x0,\
	.APB_PARA.reg_base_addr = 0x0,\
	.APB_PARA.reg_apb_pprot = 0x0,\
	.DEBUG_BUS0.reg_debus0 = 0x0,\
	.DEBUG_BUS1.reg_debus1 = 0x0,\
	.DEBUG_BUS2.reg_debus2 = 0x0,\
	.DEBUG_BUS3.reg_debus3 = 0x0,\
	.DEBUG_BUS_SEL.reg_debus_sel = 0x0,\
	.DUMMY.reg_dummy = 0xffff,\
	.TASK_DONE_STS.reg_task_done = 0x0,\
	.DMA_ADDR_TSK0.reg_dma_addr_tsk0 = 0x0,\
	.DMA_CNT_TSK0.reg_dma_cnt_tsk0 = 0x0,\
	.DMA_ADDR_TSK1.reg_dma_addr_tsk1 = 0x0,\
	.DMA_CNT_TSK1.reg_dma_cnt_tsk1 = 0x0,\
	.DMA_ADDR_TSK2.reg_dma_addr_tsk2 = 0x0,\
	.DMA_CNT_TSK2.reg_dma_cnt_tsk2 = 0x0,\
	.DMA_ADDR_TSK3.reg_dma_addr_tsk3 = 0x0,\
	.DMA_CNT_TSK3.reg_dma_cnt_tsk3 = 0x0,\
	.DMA_ADDR_TSK4.reg_dma_addr_tsk4 = 0x0,\
	.DMA_CNT_TSK4.reg_dma_cnt_tsk4 = 0x0,\
	.DMA_ADDR_TSK5.reg_dma_addr_tsk5 = 0x0,\
	.DMA_CNT_TSK5.reg_dma_cnt_tsk5 = 0x0,\
	.DMA_ADDR_TSK6.reg_dma_addr_tsk6 = 0x0,\
	.DMA_CNT_TSK6.reg_dma_cnt_tsk6 = 0x0,\
	.DMA_ADDR_TSK7.reg_dma_addr_tsk7 = 0x0,\
	.DMA_CNT_TSK7.reg_dma_cnt_tsk7 = 0x0,\
}
//#define DEFINE_CMDQ_C(X) CMDQ_C X = _DEFINE_CMDQ_C
#endif /* ifdef __cplusplus */
#endif //__REG_CMDQ_STRUCT_H__
