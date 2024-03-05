include $(PWD)/../Makefile.interdrv.param

HAL_PATH = hal/$(CHIP_CODE)

ccflags-y += -I$(PWD)/../include/common/uapi/ -I$(PWD)/../include/chip/$(CHIP_CODE)/uapi/
ccflags-y += -I$(PWD)/../include/common/kapi/
ccflags-y += -I$(PWD) -I$(LDDINCDIR) -DCNM_FPGA_PLATFORM -DCNM_FPGA_PCI_INTERFACE -DCVI_H26X_USE_ION_FW_BUFFER -DUSE_KERNEL_MODE -Wno-date-time -Werror

PWD	:= $(shell pwd)
obj-m += soph_vcodec.o
soph_vcodec-y += cvi_vcodec.o vcodec_common.o
soph_vcodec-y += hal/$(CHIP_CODE)/cvi_vcodec_cfg.o

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c *.mod
	rm -rf .tmp_versions Module.symvers
	rm -rf modules.order
	rm -rf $(HAL_PATH)/*.o $(HAL_PATH)/.*.cmd

install:
	install -d $(INSTALL_DIR)
	install -d $(INSTALL_DIR)/drv
	install -m 755 load_vcodec.sh $(INSTALL_DIR)/drv
	install -m 755 unload_vcodec.sh $(INSTALL_DIR)/drv
	install -m 644 vcodec.ko $(INSTALL_DIR)/drv

uninstall:
	$(RM) $(INSTALL_DIR)/drv/vcodec.ko
	$(RM) $(INSTALL_DIR)/drv/load_vcodec.sh
	$(RM) $(INSTALL_DIR)/drv/unload_vcodec.sh

depend .depend dep:
	$(CC) $(CFLAGS)	-M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include	.depend
endif
