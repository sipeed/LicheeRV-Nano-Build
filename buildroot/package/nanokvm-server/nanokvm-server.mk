################################################################################
#
# nanokvm-server
#
################################################################################

NANOKVM_SERVER_VERSION = 97a9b376c79611ca1cceb8f6df282670bb597379
NANOKVM_SERVER_SITE = $(call github,sipeed,NanoKVM,$(NANOKVM_SERVER_VERSION))

NANOKVM_SERVER_DEPENDENCIES = host-go host-nodejs host-python3

ifeq ($(BR2_PACKAGE_MAIX_CDK),y)
# Use MaixCDK to build kvm_system.
NANOKVM_SERVER_DEPENDENCIES += maix-cdk
endif

GO_BIN = $(HOST_DIR)/bin/go

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
	mkdir -pv $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/
	rm -f $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/libopencv_*.so*
	if [ -e $(NANOKVM_SERVER_EXT_MIDDLEWARE)/$(NANOKVM_SERVER_EXT_MAIXCAM_LIB) ]; then \
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
		ln -s libmisc.so $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/$$l ; \
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
	cd $(@D)/$(NANOKVM_SERVER_GOMOD) ; \
	GOPROXY=direct GOSUMDB="sum.golang.org" $(GO_BIN) mod tidy
	cd $(@D)/$(NANOKVM_SERVER_GOMOD) ; \
	sed -i 's|-L../dl_lib -lkvm|-L../dl_lib -L$(TARGET_DIR)/usr/lib -lkvm|g' common/cgo.go ; \
	sed -i s/' -lkvm$$'/' -lkvm -lmaixcam_lib -latomic -lae -laf -lawb -lcvi_bin -lcvi_bin_isp -lini -lisp -lisp_algo -lsys -lvdec -lvenc -lvpu'/g common/cgo.go
	cd $(@D)/$(NANOKVM_SERVER_GOMOD) ; \
	CGO_ENABLED=1 $(NANOKVM_SERVER_GO_ENV) $(GO_BIN) build -x -ldflags="-extldflags '-Wl,-rpath,\$$ORIGIN/dl_lib'"
	cd $(@D)/web ; \
	$(HOST_COREPACK) install -g pnpm@$(NANOKVM_SERVER_PNPM_VERSION)+sha1.$(NANOKVM_SERVER_PNPM_SHA_SUM) ; \
	$(HOST_COREPACK) use pnpm@$(NANOKVM_SERVER_PNPM_VERSION) ; \
	$(HOST_COREPACK) pnpm install
	cd $(@D)/web ; \
	$(HOST_COREPACK) pnpm build
	if [ -e $(@D)/support/kvm_system -a -e $(HOST_DIR)/bin/maixcdk ]; then \
		rm -rf $(@D)/../maix-cdk-$(MAIX_CDK_VERSION)/examples/kvm_system ; \
		rsync -avpPxH $(@D)/support/kvm_system $(@D)/../maix-cdk-$(MAIX_CDK_VERSION)/examples/ ; \
		cd $(@D)/../maix-cdk-$(MAIX_CDK_VERSION)/examples/kvm_system/ ; \
		PATH=$(BR_PATH) $(HOST_DIR)/bin/maixcdk build -p maixcam ; \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(@D)/../maix-cdk-$(MAIX_CDK_VERSION)/examples/kvm_system/dist/kvm_system_release/kvm_system $(@D)/support/kvm_system/ ; \
	fi
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
	if [ -e ${@D}/support/kvm_system/kvm_system ]; then \
		mkdir -pv $(TARGET_DIR)/kvmapp/kvm_system/ ; \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/support/kvm_system/kvm_system $(TARGET_DIR)/kvmapp/kvm_system/ ; \
	else \
		rm -f $(TARGET_DIR)/kvmapp/kvm_system/kvm_system ; \
	fi
	mkdir -pv $(TARGET_DIR)/kvmapp/server/dl_lib/
	rsync -r --verbose --links --safe-links --hard-links ${@D}/server/dl_lib/ $(TARGET_DIR)/kvmapp/server/dl_lib/
	mkdir -pv $(TARGET_DIR)/kvmapp/server/web/
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/web/dist/ $(TARGET_DIR)/kvmapp/server/web/
endef

$(eval $(generic-package))
