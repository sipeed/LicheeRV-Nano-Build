################################################################################
#
# nanokvm-server
#
################################################################################

NANOKVM_SERVER_VERSION = 23b19015f468634e54d116a84b8f7d8194b7ef3e
NANOKVM_SERVER_SITE = $(call github,sipeed,NanoKVM,$(NANOKVM_SERVER_VERSION))
NANOKVM_SERVER_UPDATE_URL = https://cdn.sipeed.com/nanokvm

NANOKVM_SERVER_DEPENDENCIES = host-go host-nodejs host-python3

ifeq ($(BR2_PACKAGE_MAIX_CDK),y)
# Use MaixCDK to build kvm_system.
NANOKVM_SERVER_DEPENDENCIES += maix-cdk
endif

NANOKVM_SERVER_TOOLCHAIN_ARCH := $(BR2_ARCH)
NANOKVM_SERVER_TOOLCHAIN_LIBC := $(findstring musl,$(realpath $(TOOLCHAIN_EXTERNAL_BIN)))

GO_BIN = $(HOST_DIR)/bin/go

ifeq ($(BR2_PACKAGE_HOST_GO_SRC),y)
HOST_GO_CROSS_ENV ?= $(HOST_GO_SRC_CROSS_ENV)
endif

NANOKVM_SERVER_GO_ENV = $(HOST_GO_CROSS_ENV)

HOST_NODEJS_BIN_ENV = $(HOST_CONFIGURE_OPTS) \
	LDFLAGS="$(NODEJS_LDFLAGS)" \
	LD="$(HOST_CXX)" \
	PATH=$(BR_PATH) \
	npm_config_build_from_source=true \
	npm_config_nodedir=$(HOST_DIR)/usr \
	npm_config_prefix=$(HOST_DIR)/usr \
	npm_config_cache=$(BUILD_DIR)/.npm-cache

HOST_COREPACK = $(HOST_NODEJS_BIN_ENV) $(HOST_DIR)/bin/corepack

NANOKVM_SERVER_PNPM_VERSION = 9.15.5
NANOKVM_SERVER_PNPM_SHA_SUM = cb1f6372ef64e2ba352f2f46325adead1c99ff8f

NANOKVM_SERVER_GOMOD = server

NANOKVM_SERVER_EXT_MIDDLEWARE = $(realpath $(TOPDIR)/../middleware/v2)
NANOKVM_SERVER_EXT_KVM_MMF = sample/test_mmf/kvm_mmf/release.linux/libkvm_mmf.so
NANOKVM_SERVER_EXT_KVM_VISION = sample/test_mmf/kvm_vision/release.linux/libkvm.so
NANOKVM_SERVER_EXT_MAIXCAM_LIB = sample/test_mmf/maixcam_lib/release.linux/libmaixcam_lib.so

NANOKVM_SERVER_REQUIRED_LIBS = \
	libae.so \
	libaf.so \
	libawb.so \
	libcvi_bin_isp.so \
	libcvi_bin.so \
	libcvi_ive.so \
	libini.so \
	libisp_algo.so \
	libisp.so \
	libmipi_tx.so \
	libmisc.so \
	libraw_dump.so \
	libsys.so \
	libvdec.so \
	libvenc.so

NANOKVM_SERVER_VPU_LIBS = \
	libosdc.so \
	libvpu.so \
	libgdc.so \
	librgn.so \
	libvi.so \
	libvo.so \
	libvpss.so

NANOKVM_SERVER_UNUSED_LIBS = \
	libcli.so \
	libjson-c.so.5 \
	libcvi_ispd2.so \
	libaaccomm2.so \
	libaacdec2.so \
	libaacenc2.so \
	libaacsbrdec2.so \
	libaacsbrenc2.so \
	libcvi_RES1.so \
	libcvi_VoiceEngine.so \
	libcvi_audio.so \
	libcvi_ssp.so \
	libcvi_vqe.so \
	libdnvqe.so \
	libtinyalsa.so

NANOKVM_SERVER_DUMMY_LIBS = \
	libae.so \
	libaf.so \
	libawb.so

define NANOKVM_SERVER_BUILD_CMDS
	if [ -e $(@D)/kvmapp ]; then \
		rm -f $(@D)/kvmapp/jpg_stream/jpg_stream ; \
		rm -rf $(@D)/kvmapp/kvm_system/ ; \
		rm -rf $(@D)/kvmapp/system/ko/ ; \
	fi
	mkdir -pv $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/
	rm -f $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/libopencv_*.so*
	if [ -e $(NANOKVM_SERVER_EXT_MIDDLEWARE)/$(NANOKVM_SERVER_EXT_KVM_MMF) ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(NANOKVM_SERVER_EXT_MIDDLEWARE)/$(NANOKVM_SERVER_EXT_KVM_MMF) $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/ ; \
	elif [ -e $(NANOKVM_SERVER_EXT_MIDDLEWARE)/$(NANOKVM_SERVER_EXT_MAIXCAM_LIB) ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(NANOKVM_SERVER_EXT_MIDDLEWARE)/$(NANOKVM_SERVER_EXT_MAIXCAM_LIB) $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/ ; \
	fi
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(NANOKVM_SERVER_EXT_MIDDLEWARE)/lib/3rd/libini.so $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/
	for l in $(NANOKVM_SERVER_REQUIRED_LIBS) ; do \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(NANOKVM_SERVER_EXT_MIDDLEWARE)/lib/$$l $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/ ; \
	done
	for l in $(NANOKVM_SERVER_VPU_LIBS) ; do \
		if [ -e $(NANOKVM_SERVER_EXT_MIDDLEWARE)/lib/$$l ]; then \
			rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(NANOKVM_SERVER_EXT_MIDDLEWARE)/lib/$$l $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/ ; \
		fi ; \
	done
	for l in $(NANOKVM_SERVER_UNUSED_LIBS) ; do \
		rm -f $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/$$l ; \
	done
	if [ -e $(NANOKVM_SERVER_EXT_MIDDLEWARE)/lib/libcvi_dummy.so ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(NANOKVM_SERVER_EXT_MIDDLEWARE)/lib/libcvi_dummy.so $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/ ; \
		for l in $(NANOKVM_SERVER_DUMMY_LIBS) ; do \
			rm -f $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/$$l ; \
			ln -s libcvi_dummy.so $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/$$l ; \
		done ; \
	fi
	if [ -e $(NANOKVM_SERVER_EXT_MIDDLEWARE)/lib/libcvi_bin_light.so -a \
	     -e $(NANOKVM_SERVER_EXT_MIDDLEWARE)/lib/libisp_light.so ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(NANOKVM_SERVER_EXT_MIDDLEWARE)/lib/libcvi_bin_light.so $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/ ; \
		for l in libcvi_bin.so ; do \
			rm -f $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/$$l ; \
			ln -s libcvi_bin_light.so $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/$$l ; \
		done ; \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(NANOKVM_SERVER_EXT_MIDDLEWARE)/lib/libisp_light.so $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/ ; \
		for l in libcvi_bin_isp.so libisp.so ; do \
			rm -f $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/$$l ; \
			ln -s libisp_light.so $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/$$l ; \
		done ; \
		for l in libisp_algo.so ; do \
			rm -f $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/$$l ; \
			ln -s libmisc.so $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/$$l ; \
		done ; \
	fi
	if [ -e $(NANOKVM_SERVER_EXT_MIDDLEWARE)/$(NANOKVM_SERVER_EXT_KVM_VISION) -a \
	     -e $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/libkvm.so ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(NANOKVM_SERVER_EXT_MIDDLEWARE)/$(NANOKVM_SERVER_EXT_KVM_VISION) $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/ ; \
		chmod ugo+rx $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/libkvm.so ; \
	fi
	if [ -e $(@D)/support/sg2002/additional -a -e $(HOST_DIR)/bin/maixcdk ]; then \
		if [ ! -e $(@D)/../maix-cdk-$(MAIX_CDK_VERSION)/_off/vision ]; then \
			mkdir $(@D)/../maix-cdk-$(MAIX_CDK_VERSION)/_off ; \
			mv $(@D)/../maix-cdk-$(MAIX_CDK_VERSION)/components/vision $(@D)/../maix-cdk-$(MAIX_CDK_VERSION)/_off/ ; \
		else \
			rm -rf $(@D)/../maix-cdk-$(MAIX_CDK_VERSION)/components/vision ; \
		fi ; \
		rsync -avpPxH $(@D)/support/sg2002/additional/ $(@D)/../maix-cdk-$(MAIX_CDK_VERSION)/components/ ; \
		rm -rf $(@D)/../maix-cdk-$(MAIX_CDK_VERSION)/examples/kvm_vision_test ; \
		rsync -avpPxH $(@D)/support/sg2002/kvm_vision_test $(@D)/../maix-cdk-$(MAIX_CDK_VERSION)/examples/ ; \
		cd $(@D)/../maix-cdk-$(MAIX_CDK_VERSION)/examples/kvm_vision_test/ ; \
		PATH=$(BR_PATH) $(HOST_DIR)/bin/maixcdk build -p maixcam ; \
		rm -rf $(@D)/../maix-cdk-$(MAIX_CDK_VERSION)/components/vision ; \
		mv $(@D)/../maix-cdk-$(MAIX_CDK_VERSION)/_off/vision  $(@D)/../maix-cdk-$(MAIX_CDK_VERSION)/components/ ; \
		rmdir $(@D)/../maix-cdk-$(MAIX_CDK_VERSION)/_off ; \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(@D)/../maix-cdk-$(MAIX_CDK_VERSION)/examples/kvm_vision_test/dist/kvm_vision_test_release/dl_lib/libkvm*.so ${@D}/server/dl_lib/ ; \
	fi
	if [ -e $(@D)/support/sg2002/kvm_system -a -e $(HOST_DIR)/bin/maixcdk ]; then \
		rm -rf $(@D)/../maix-cdk-$(MAIX_CDK_VERSION)/examples/kvm_system ; \
		rsync -avpPxH $(@D)/support/sg2002/kvm_system $(@D)/../maix-cdk-$(MAIX_CDK_VERSION)/examples/ ; \
		cd $(@D)/../maix-cdk-$(MAIX_CDK_VERSION)/examples/kvm_system/ ; \
		PATH=$(BR_PATH) $(HOST_DIR)/bin/maixcdk build -p maixcam ; \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(@D)/../maix-cdk-$(MAIX_CDK_VERSION)/examples/kvm_system/dist/kvm_system_release/kvm_system $(@D)/support/sg2002/kvm_system/ ; \
	fi
	cd $(@D)/$(NANOKVM_SERVER_GOMOD) ; \
	GOPROXY=direct GOSUMDB="sum.golang.org" $(GO_BIN) mod tidy
	cd $(@D)/$(NANOKVM_SERVER_GOMOD) ; \
	sed -i 's|-L../dl_lib -lkvm|-L../dl_lib -L$(TARGET_DIR)/usr/lib -lkvm|g' common/cgo.go ; \
	sed -i s/' -lkvm$$'/' -lkvm -lmaixcam_lib -latomic -lae -laf -lawb -lcvi_bin -lcvi_bin_isp -lini -lisp -lisp_algo -lsys -lvdec -lvenc -lvpu'/g common/cgo.go
	if [ -e $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/libkvm_mmf.so ]; then \
		cd $(@D)/$(NANOKVM_SERVER_GOMOD) ; \
		sed -i s/'maixcam_lib'/'kvm_mmf'/g common/cgo.go ; \
	fi
	cd $(@D)/$(NANOKVM_SERVER_GOMOD) ; \
	CGO_ENABLED=1 $(NANOKVM_SERVER_GO_ENV) $(GO_BIN) build -x -ldflags="-extldflags '-Wl,-rpath,\$$ORIGIN/dl_lib'"
	cd $(@D)/web ; \
	$(HOST_COREPACK) install -g pnpm@$(NANOKVM_SERVER_PNPM_VERSION)+sha1.$(NANOKVM_SERVER_PNPM_SHA_SUM) ; \
	$(HOST_COREPACK) use pnpm@$(NANOKVM_SERVER_PNPM_VERSION) ; \
	$(HOST_COREPACK) pnpm install
	cd $(@D)/web ; \
	$(HOST_COREPACK) pnpm build
endef

define NANOKVM_SERVER_INSTALL_TARGET_CMDS
	if [ "X$(BR2_PACKAGE_TAILSCALE_RISCV64)" = "Xy" ]; then \
		rm -f $(TARGET_DIR)/etc/tailscale_disabled ; \
	else \
		mkdir -pv $(TARGET_DIR)/etc/ ; \
		touch $(TARGET_DIR)/etc/tailscale_disabled ; \
	fi
	mkdir -pv $(TARGET_DIR)/kvmapp/
	#touch $(TARGET_DIR)/kvmapp/force_dl_lib
	mkdir -pv $(TARGET_DIR)/kvmapp/server/
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/server/NanoKVM-Server $(TARGET_DIR)/kvmapp/server/
	if [ -e ${@D}/support/sg2002/kvm_system/kvm_system ]; then \
		mkdir -pv $(TARGET_DIR)/kvmapp/kvm_system/ ; \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/support/sg2002/kvm_system/kvm_system $(TARGET_DIR)/kvmapp/kvm_system/ ; \
	else \
		rm -f $(TARGET_DIR)/kvmapp/kvm_system/kvm_system ; \
	fi
	mkdir -pv $(TARGET_DIR)/kvmapp/server/dl_lib/
	rsync -r --verbose --links --safe-links --hard-links ${@D}/server/dl_lib/ $(TARGET_DIR)/kvmapp/server/dl_lib/
	mkdir -pv $(TARGET_DIR)/kvmapp/server/web/
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/web/dist/ $(TARGET_DIR)/kvmapp/server/web/
endef

$(eval $(generic-package))
