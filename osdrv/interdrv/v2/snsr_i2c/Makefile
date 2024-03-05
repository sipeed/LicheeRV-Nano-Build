include $(PWD)/../Makefile.interdrv.param

ifneq ($(KERNELRELEASE),)
soph_snsr_i2c-objs += common/snsr_i2c.o

obj-m += soph_snsr_i2c.o

ccflags-y += -I$(PWD)/chip/$(CHIP_CODE)/ -I$(PWD)/common/ -I$(PWD)/../include/chip/$(CHIP_CODE)/uapi
ccflags-y += -I$(PWD)/../include/common/kapi/

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

KBUILD_EXTRA_SYMBOLS = $(PWD)/../base/Module.symvers
else
all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c *.mod
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
