# the share variable is exported from wifi, the following is required
ccflags-y += -DCONFIG_COMBO_MULTISDIO_EXPORT_FROM_RTW
KBUILD_EXTRA_SYMBOLS=/lib/firmware/Module.symvers
# please mark the above two lines when testing btrtksdio driver without wifi driver

ifneq ($(KERNELRELEASE),)
	obj-m += btrtksdio.o
	btrtksdio-y := btrtl.o btrtk_sdio.o rtk_coex.o
else
	PWD := $(shell pwd)
	KVER := $(shell uname -r)
	KDIR := /lib/modules/$(KVER)/build

all:
	make ${KBUILD_EXTRA_SYMBOLS} -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean

endif
