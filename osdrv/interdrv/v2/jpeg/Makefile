include $(PWD)/../Makefile.interdrv.param

HAL_PATH = hal/$(CHIP_CODE)

CHIP_TYPE = CHIP_BM1880
SUB_TYPE = ASIC_EVB

# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
  DEBFLAGS = -O	-g # "-O" is needed to expand inlines
else
  DEBFLAGS = -O2
endif

ccflags-y += $(DEBFLAGS) -I$(PWD) -I$(LDDINCDIR) -D$(CHIP_TYPE) -D$(SUB_TYPE) -DUSE_KERNEL_MODE -Wno-date-time -Werror

KBUILD_EXTRA_SYMBOLS += $(PWD)/../vcodec/Module.symvers

# PWD	:= $(shell pwd)
obj-m += soph_jpeg.o
soph_jpeg-y += cvi_jpeg.o jpeg_common.o jmm.o hal/$(CHIP_CODE)/cvi_jpeg_cfg.o

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -rf .tmp_versions Module.symvers
	rm -rf modules.order
	rm -rf $(HAL_PATH)/*.o $(HAL_PATH)/.*.cmd

depend .depend dep:
	$(CC) $(CFLAGS)	-M *.c > .depend


ifeq (.depend,$(wildcard .depend))
include	.depend
endif
