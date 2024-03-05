#ifndef _CVI_LDC_REG_H_
#define _CVI_LDC_REG_H_

#if defined(ENV_CVITEST) || defined(ENV_EMU)
#define REG_LDC_TOP_BASE 0x0A0C0000
#else
#define REG_LDC_TOP_BASE 0
#endif

#define REG_LDC_REG_BASE                (REG_LDC_TOP_BASE)
#define REG_LDC_CMDQ_BASE               (REG_LDC_TOP_BASE + 0x100)

#define REG_LDC_DATA_FORMAT             (REG_LDC_REG_BASE + 0x00)
#define REG_LDC_RAS_MODE                (REG_LDC_REG_BASE + 0x04)
#define REG_LDC_RAS_XSIZE               (REG_LDC_REG_BASE + 0x08)
#define REG_LDC_RAS_YSIZE               (REG_LDC_REG_BASE + 0x0C)
#define REG_LDC_MAP_BASE                (REG_LDC_REG_BASE + 0x10)
#define REG_LDC_MAP_BYPASS              (REG_LDC_REG_BASE + 0x14)

#define REG_LDC_SRC_BASE_Y              (REG_LDC_REG_BASE + 0x20)
#define REG_LDC_SRC_BASE_C              (REG_LDC_REG_BASE + 0x24)
#define REG_LDC_SRC_XSIZE               (REG_LDC_REG_BASE + 0x28)
#define REG_LDC_SRC_YSIZE               (REG_LDC_REG_BASE + 0x2C)
#define REG_LDC_SRC_XSTART              (REG_LDC_REG_BASE + 0x30)
#define REG_LDC_SRC_XEND                (REG_LDC_REG_BASE + 0x34)
#define REG_LDC_SRC_BG                  (REG_LDC_REG_BASE + 0x38)

#define REG_LDC_DST_BASE_Y              (REG_LDC_REG_BASE + 0x40)
#define REG_LDC_DST_BASE_C              (REG_LDC_REG_BASE + 0x44)
#define REG_LDC_DST_MODE                (REG_LDC_REG_BASE + 0x48)

#define REG_LDC_IRQEN                   (REG_LDC_REG_BASE + 0x50)
#define REG_LDC_START                   (REG_LDC_REG_BASE + 0x54)
#define REG_LDC_IRQSTAT                 (REG_LDC_REG_BASE + 0x58)
#define REG_LDC_IRQCLR                  (REG_LDC_REG_BASE + 0x5C)

#define REG_SB_MODE                     (REG_LDC_REG_BASE + 0x60)
#define REG_SB_NB                       (REG_LDC_REG_BASE + 0x64)
#define REG_SB_SIZE                     (REG_LDC_REG_BASE + 0x68)
#define REG_SB_SET_STR                  (REG_LDC_REG_BASE + 0x6C)
#define REG_SB_SW_WPTR                  (REG_LDC_REG_BASE + 0x70)
#define REG_SB_SW_CLR                   (REG_LDC_REG_BASE + 0x74)
#define REG_SB_FULL                     (REG_LDC_REG_BASE + 0x78)
#define REG_SB_EMPTY                    (REG_LDC_REG_BASE + 0x7C)
#define REG_SB_WPTR_RO                  (REG_LDC_REG_BASE + 0x80)
#define REG_SB_DPTR_RO                  (REG_LDC_REG_BASE + 0x84)
#define REG_SB_STRIDE                   (REG_LDC_REG_BASE + 0x88)

#define REG_LDC_DIR                     (REG_LDC_REG_BASE + 0x8c)
#define REG_LDC_INT_SEL                 (REG_LDC_REG_BASE + 0x90)

/* Command queue */
#define REG_CMDQ_INT_EVENT              (REG_LDC_CMDQ_BASE + 0x00)
#define REG_CMDQ_INT_EN                 (REG_LDC_CMDQ_BASE + 0x04)
#define REG_CMDQ_DMA_ADDR_L             (REG_LDC_CMDQ_BASE + 0x08)
#define REG_CMDQ_DMA_ADDR_H             (REG_LDC_CMDQ_BASE + 0x0C)
#define REG_CMDQ_DMA_CNT                (REG_LDC_CMDQ_BASE + 0x10)
#define REG_CMDQ_DMA_CONFIG             (REG_LDC_CMDQ_BASE + 0x14)
#define REG_CMDQ_DMA_PARA               (REG_LDC_CMDQ_BASE + 0x18)
#define REG_CMDQ_JOB_CTL                (REG_LDC_CMDQ_BASE + 0x1C)
#define REG_CMDQ_STATUS                 (REG_LDC_CMDQ_BASE + 0x20)
#define REG_CMDQ_APB_PARA               (REG_LDC_CMDQ_BASE + 0x24)
#define REG_CMDQ_SOFT_RST               (REG_LDC_CMDQ_BASE + 0x28)
#define REG_CMDQ_DEBUG                  (REG_LDC_CMDQ_BASE + 0x2c)
#define REG_CMDQ_DUMMY                  (REG_LDC_CMDQ_BASE + 0x30)

#endif  // _CVI_LDC_REG_H_
