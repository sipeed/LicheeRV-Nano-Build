SHELL = /bin/bash
SDK_VER? := 64bit

ifeq ($(CHIP_ARCH), MARS)
	CHIP_ARCH:=CV181X
else ifeq ($(CHIP_ARCH), PHOBOS)
	CHIP_ARCH:=CV180X
endif

ifeq ($(PARAM_FILE), )
     PARAM_FILE:=Makefile.param
     include $(PARAM_FILE)
endif

CHIP_DIR := $(MW_PATH)/chips/$(shell echo $(CHIP_ARCH) | tr A-Z a-z)

ifeq ($(DESTDIR),)
    DESTDIR := $(shell pwd)/install
endif

define package_mw
    @install -d $(1)
    @tar -c --exclude='chips' --exclude='modules' --exclude='$(1)' --exclude='.git' --exclude='tool' --exclude='component/panel/cv1835/customer' -f - . | (cd $(1); tar xf -)
    @rm $(1)/3rdparty/gtest -r
    @rm $(1)/sample/ -r
    @make -C sample/ clean
    @cp sample $(1)/sample -r
endef

.PHONY: clean all module component sample install uninstall package 3rdparty

all: prepare 3rdparty module component sample

module: prepare 3rdparty
	@make -C modules/

prepare:
	@mkdir -p include/isp
	@cp -rf modules/isp/include/* include/isp
3rdparty:
	@make -C 3rdparty/

sample: module component
	@rm -rf mod_tmp
	@cp -rf modules mod_tmp
	@make -C sample/

component:
	@make -C component/isp/ all

ifneq ($(SUBTYPE), fpga)
install:
	@mkdir -p $(DESTDIR)/usr/bin
	@mkdir -p $(DESTDIR)/usr/lib/3rd
ifneq ($(FLASH_SIZE_SHRINK),y)
	# copy sample_xxx
	@cp -f sample/mipi_tx/sample_dsi $(DESTDIR)/usr/bin
	@cp -f sample/vio/sample_vio $(DESTDIR)/usr/bin
	@cp -f sample/sensor_test/sensor_test $(DESTDIR)/usr/bin
	@cp -f sample/audio/sample_audio $(DESTDIR)/usr/bin
	#@cp -f sample/cipher/sample_cipher $(DESTDIR)/usr/bin
	#@cp -f sample/cvg/sample_cvg $(DESTDIR)/usr/bin
	@cp -f sample/venc/sample_venc $(DESTDIR)/usr/bin
	@cp -f sample/venc/sample_vcodec $(DESTDIR)/usr/bin
	@cp -f sample/vdec/sample_vdec $(DESTDIR)/usr/bin
	@cp -f sample/ive/sample_* $(DESTDIR)/usr/bin
	@cp -f sample/ive/ive_stress $(DESTDIR)/usr/bin
	@cp -f sample/region/sample_region $(DESTDIR)/usr/bin
	@cp -f sample/vdecvo/sample_vdecvo $(DESTDIR)/usr/bin
	@cp -f sample/overlay/sample_overlay $(DESTDIR)/usr/bin
	@cp -f sample/fisheye/sample_fisheye $(DESTDIR)/usr/bin
endif

ifneq ($(FLASH_SIZE_SHRINK),y)
	# copy venc
	#@cp -f modules/venc/vc_lib/bin/cvi_h265_enc_test $(DESTDIR)/usr/bin
	#@cp -f modules/venc/vc_lib/bin/cvi_h265_dec $(DESTDIR)/usr/bin
	#@cp -f modules/venc/vc_lib/bin/cvi_h264_dec $(DESTDIR)/usr/bin
	#@cp -f modules/venc/vc_lib/bin/cvi_jpg_codec $(DESTDIR)/usr/bin
endif

ifneq ($(FLASH_SIZE_SHRINK),y)
	# copy audio libs and elf
	@cp -f sample/audio/sample_audio*  $(DESTDIR)/usr/bin
	@if [ -e "sample/audio/cvi_mp3player" ]; then cp -f sample/audio/cvi_mp3player $(DESTDIR)/usr/bin; fi
endif

	# copy mw lib
	@cp -a lib/*.so*  $(DESTDIR)/usr/lib
	@cp -a lib/3rd/*.so*  $(DESTDIR)/usr/lib/3rd

uninstall:
	@rm $(DESTDIR) -rf

package:
	$(call package_mw,tmp)
	@install -d $(DESTDIR)
	@tar fcz $(DESTDIR)/mw.tar.gz -C tmp .
	@echo $(KERNEL_INC)
	@tar fcz $(DESTDIR)/kernel_header.tar.gz -C $(KERNEL_INC) ./
	@rm tmp -r
	@echo "package done"
endif
clean:
	@if [ -d mod_tmp ]; then mv mod_tmp modules; fi;
	@rm -rf include/isp
	@make -C modules/ clean
	@make -C 3rdparty/ clean
	@make -C sample/ clean
	@make -C component/isp/ clean
