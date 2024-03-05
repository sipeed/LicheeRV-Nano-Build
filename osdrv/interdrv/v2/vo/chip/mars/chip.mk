soph_vo-objs += chip/$(CHIP_CODE)/vo.o \
				chip/$(CHIP_CODE)/vo_sdk_layer.o \
				chip/$(CHIP_CODE)/proc/vo_disp_proc.o \
				chip/$(CHIP_CODE)/proc/vo_proc.o

soph_mipi_tx-objs := chip/$(CHIP_CODE)/vo_mipi_tx.o \
					chip/$(CHIP_CODE)/proc/vo_mipi_tx_proc.o
