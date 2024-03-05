// $Module: isp_rdma $
// $RegisterBank Version: V 1.0.00 $
// $Author: Siwi $
// $Date: Sun, 26 Sep 2021 04:20:44 PM $
//

#ifndef __REG_ISP_RDMA_STRUCT_H__
#define __REG_ISP_RDMA_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*Shadow reg read select
		0 : read active reg
		1 : read shadow reg;*/
		uint32_t reg_shadow_rd_sel:1;
	};
	uint32_t val;
} ISP_RDMA_SHADOW_RD_SEL_C;
typedef union {
	struct {
		/*IP Disable;*/
		uint32_t reg_ip_disable:32;
	};
	uint32_t val;
} ISP_RDMA_IP_DISABLE_C;
typedef union {
	struct {
		/*Abort done flag;*/
		uint32_t reg_abort_done:1;
		uint32_t rsv_1_3:3;
		/*Error flag : AXI response error;*/
		uint32_t reg_error_axi:1;
		uint32_t rsv_5_7:3;
		/*Error client ID;*/
		uint32_t reg_error_id:5;
		uint32_t rsv_13_15:3;
		/*Date of latest update;*/
		uint32_t reg_dma_version:16;
	};
	uint32_t val;
} ISP_RDMA_NORM_STATUS0_C;
typedef union {
	struct {
		/*reg_id_idle[id] : idle status of client id
		(RDMA is not active or it has transferred all data to the client);*/
		uint32_t reg_id_done:32;
	};
	uint32_t val;
} ISP_RDMA_NORM_STATUS1_C;
typedef union {
	struct {
		/*Bandwidth limiter window size;*/
		uint32_t reg_bwlwin:10;
		/*Bandwidth limiter transaction number;*/
		uint32_t reg_bwltxn:6;
	};
	uint32_t val;
} ISP_RDMA_NORM_PERF_C;
typedef struct {
	ISP_RDMA_SHADOW_RD_SEL_C SHADOW_RD_SEL;
	ISP_RDMA_IP_DISABLE_C IP_DISABLE;
	uint32_t _NORM_STATUS0_0; // 0x08
	uint32_t _NORM_STATUS0_1; // 0x0C
	ISP_RDMA_NORM_STATUS0_C NORM_STATUS0;
	ISP_RDMA_NORM_STATUS1_C NORM_STATUS1;
	uint32_t _NORM_PERF_0; // 0x18
	uint32_t _NORM_PERF_1; // 0x1C
	ISP_RDMA_NORM_PERF_C NORM_PERF;
} ISP_RDMA_C;
#ifdef __cplusplus
#error "removed."
#else /* !ifdef __cplusplus */
#define _DEFINE_ISP_RDMA_C \
	{\
		.SHADOW_RD_SEL.reg_shadow_rd_sel = 0x1,\
		.IP_DISABLE.reg_ip_disable = 0x0,\
		.NORM_STATUS0.reg_abort_done = 0x0,\
		.NORM_STATUS0.reg_error_axi = 0x0,\
		.NORM_STATUS0.reg_error_id = 0x1f,\
		.NORM_STATUS0.reg_dma_version = 0x0423,\
		.NORM_STATUS1.reg_id_done = 0x0,\
		.NORM_PERF.reg_bwlwin = 0x0,\
		.NORM_PERF.reg_bwltxn = 0x0,\
	}
//#define DEFINE_ISP_RDMA_C(X) ISP_RDMA_C X = _DEFINE_ISP_RDMA_C
#endif /* ifdef __cplusplus */
#endif //__REG_ISP_RDMA_STRUCT_H__
