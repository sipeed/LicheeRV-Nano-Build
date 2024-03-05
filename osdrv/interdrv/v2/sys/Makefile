include $(PWD)/../Makefile.interdrv.param

ccflags-y += -I$(PWD)/../include/common/uapi/ -I$(PWD)/../include/common/kapi/
ccflags-y += -I$(src)/../base/mw/ \
             -I$(src)/../base/ \
             -I$(src)/common/uapi \
             -I$(src)/common/kapi \
             -I$(src)/common \
             -I$(src)/hal/$(CHIP_CODE)/uapi \
             -I$(src)/hal/$(CHIP_CODE) \
             -I$(srctree)/drivers/staging/android \
             -I$(src)/../include/chip/$(CHIP_CODE)/uapi

ccflags-y += $(INTRERDRV_FLAGS)

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

obj-m += soph_sys.o
soph_sys-y += common/sys.o \
              common/sys_context.o

ifneq ($(INTRERDRV_FLAGS), )
soph_sys-y += common/sys_test.o
endif

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o* *.ko
	@rm -f Module.symvers modules.order
	@rm -f *.mod.c *.mod
	@rm -rf .tmp_versions
	@rm -f .*cmd
	@rm -f $(CHIP)/*.o*
	@rm -f $(CHIP)/.*cmd
	@find . -name \*.o* -type f -delete
