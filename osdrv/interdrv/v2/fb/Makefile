include $(PWD)/../Makefile.interdrv.param

obj-m += soph_fb.o
soph_fb-y += fb.o

ccflags-y += -I$(srctree)/drivers/staging/android/
ccflags-y += -I$(PWD)/../base/
ccflags-y += -I$(PWD)/../base/chip/$(CHIP_CODE)/
ccflags-y += -I$(PWD)/../vpss/chip/$(CHIP_CODE)/
ccflags-y += -I$(PWD)/../include/common/uapi/
ccflags-y += -I$(PWD)/../include/chip/$(CHIP_CODE)/uapi/
ccflags-y += -I$(PWD)/../include/common/kapi/

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

KBUILD_EXTRA_SYMBOLS += $(PWD)/../base/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../vpss/Module.symvers

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c *.mod
	rm -rf .tmp_versions
