include $(PWD)/../Makefile.interdrv.param

soph_base-objs = base.o \
				chip/$(CHIP_CODE)/vip_sys.o \
				reg.o \
				cvi_vb_proc.o \
				cvi_log_proc.o \
				cvi_sys_proc.o \
				base_context.o \
				vb.o

ifneq ($(INTRERDRV_FLAGS), )
soph_base-objs += vb_test.o
endif

obj-m += soph_base.o

ccflags-y += -I$(srctree)/drivers/tee \
             -I$(PWD)/../include/common/uapi/ \
             -I$(PWD)/../include/common/kapi/ \
             -I$(PWD)/../base/ \
             -I$(PWD)/../include/chip/$(CHIP_CODE)/uapi/ \
             -I$(PWD)/../sys/common/ \
             -I$(PWD)/../sys/common/uapi \
             -I$(srctree)/drivers/staging/android \
             -I$(PWD)/chip/$(CHIP_CODE)


ifeq ($(SUBTYPE), fpga)
ccflags-y += -DDRV_FPGA_PORTING
endif

ccflags-y += $(INTRERDRV_FLAGS)

ccflags-y +=-Wall -Wextra -Wno-unused-parameter -Wno-sign-compare

KBUILD_EXTRA_SYMBOLS += $(PWD)/../sys/Module.symvers

all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
