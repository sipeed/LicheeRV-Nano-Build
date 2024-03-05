include $(PWD)/../Makefile.interdrv.param

ifneq ($(KERNELRELEASE),)
include $(PWD)/chip/$(CHIP_CODE)/chip.mk

soph_vo-objs += common/vo_core.o
soph_vo-objs += common/vo_rgn_ctrl.o

obj-m += soph_vo.o
obj-m += soph_mipi_tx.o

ccflags-y +=-Wall -Wextra -Wno-unused-parameter -Wno-sign-compare

ccflags-y += -I$(PWD)/chip/$(CHIP_CODE)/ -I$(PWD)/common/
ccflags-y += -I$(PWD)/../include/common/uapi/ -I$(PWD)/../include/chip/$(CHIP_CODE)/uapi/
ccflags-y += -I$(PWD)/../include/common/kapi/
ccflags-y += -I$(PWD)/../vpss/chip/$(CHIP_CODE)/
ccflags-y += -I$(PWD)/../vpss/common
ccflags-y += -I$(PWD)/../sys/common/ -I$(PWD)/../sys/common/uapi
ccflags-y += -I$(PWD)/../base/chip/$(CHIP_CODE)/
ccflags-y += -I$(PWD)/../base/
ccflags-y += -I$(srctree)/drivers/staging/android
ccflags-$(CONFIG_CVITEK_PINCTRL_CV1835) += -I$(srctree)/drivers/pinctrl/cvitek/

KBUILD_EXTRA_SYMBOLS = $(PWD)/../base/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../vpss/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../sys/Module.symvers
else
all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c *.mod
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
