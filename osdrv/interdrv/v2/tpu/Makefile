include $(PWD)/../Makefile.interdrv.param

PWD := $(shell pwd)

ccflags-y +=	-I$(src)/../include/uapi \
				-I$(src)/common \
				-I$(src)/hal/$(CHIP_CODE) \
				-I$(srctree)/drivers/tee \
				-I$(srctree)/arch/arm64/include \
				-I$(srctree)/drivers/staging/android/ion \
				-I$(src)/../include/common/uapi/linux

obj-m += soph_tpu.o
soph_tpu-y += common/cvi_tpu_interface.o
soph_tpu-y += hal/$(CHIP_CODE)/tpu_platform.o
soph_tpu-y += hal/$(CHIP_CODE)/tpu_pmu.o

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
