include $(PWD)/../Makefile.interdrv.param

PWD := $(shell pwd)

ccflags-y += -I$(src) -I$(src)/$(CHIP_CODE)

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

soph_clock_cooling-objs = $(CHIP_CODE)/clock_cooling.o

obj-m += soph_clock_cooling.o

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c *.mod
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
