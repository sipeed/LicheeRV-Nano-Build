include $(PWD)/../Makefile.interdrv.param

ifneq ($(KERNELRELEASE),)
include $(PWD)/chip/$(CHIP_CODE)/chip.mk

obj-m += soph_mipi_rx.o

ccflags-y += -I$(PWD)/chip/$(CHIP_CODE)/ -I$(PWD)/common/
ccflags-y += -I$(PWD)/../include/common/uapi/ -I$(PWD)/../include/chip/$(CHIP_CODE)/uapi/
ccflags-y += -I$(srctree)/drivers/pinctrl/cvitek/
ccflags-y += -I$(PWD)/../include/common/kapi/
ccflags-y += -I$(PWD)/../base/chip/$(CHIP_CODE)/

ifeq ($(SUBTYPE), fpga)
export FPGA_PORTING := true
ccflags-y += -DFPGA_PORTING
endif

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

KBUILD_EXTRA_SYMBOLS = $(PWD)/../base/Module.symvers
else
all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
