include $(PWD)/../Makefile.interdrv.param

ifneq ($(KERNELRELEASE),)

obj-m += soph_ive.o
soph_ive-y += common/cvi_ive_interface.o
soph_ive-y += hal/$(CHIP_CODE)/cvi_reg.o
soph_ive-y += hal/$(CHIP_CODE)/cvi_ive_platform.o

ccflags-y += -I$(PWD)/../include/common/uapi -I$(PWD)/../include/chip/$(CHIP_CODE)/uapi/
ccflags-y += -I$(PWD)/../include/common/kapi -I$(PWD)/../sys/common/uapi/
ccflags-y += -I$(src)/common -I$(src)/hal/$(CHIP_CODE) -I$(src)/hal/$(CHIP_CODE)/include
ccflags-y += -I$(PWD)/../base/ -I$(srctree)/drivers/staging/android
ccflags-y += -I$(PWD)/../base/chip/$(CHIP_CODE)

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

KBUILD_EXTRA_SYMBOLS += $(PWD)/../sys/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../base/Module.symvers

else
all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c *.mod
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
