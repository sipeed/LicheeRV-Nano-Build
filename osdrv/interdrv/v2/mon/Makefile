include $(PWD)/../Makefile.interdrv.param

PWD := $(shell pwd)

ccflags-y += -I$(src)/common -I$(src)/hal/$(CHIP_CODE)

obj-m += soph_mon.o
soph_mon-y += common/cvi_mon_interface.o
soph_mon-y += hal/$(CHIP_CODE)/mon_platform.o

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
