################################################################################
#
# nanokvm-server
#
################################################################################

NANOKVM_SERVER_VERSION = a0f0699cdca848af6229947f119bcc9a11c7adc1
NANOKVM_SERVER_SITE = $(call github,sipeed,NanoKVM,$(NANOKVM_SERVER_VERSION))

NANOKVM_SERVER_DEPENDENCIES = host-go host-nodejs host-python3 opencv4

GO_BIN = $(HOST_DIR)/bin/go

HOST_NODEJS_BIN_ENV = $(HOST_CONFIGURE_OPTS) \
	LDFLAGS="$(NODEJS_LDFLAGS)" \
	LD="$(HOST_CXX)" \
	PATH=$(BR_PATH) \
	npm_config_build_from_source=true \
	npm_config_nodedir=$(HOST_DIR)/usr \
	npm_config_prefix=$(HOST_DIR)/usr \
	npm_config_cache=$(BUILD_DIR)/.npm-cache

HOST_COREPACK = $(HOST_NODEJS_BIN_ENV) $(HOST_DIR)/bin/corepack

NANOKVM_SERVER_GOMOD = server

NANOKVM_SERVER_EXT_MIDDLEWARE = ../../../../middleware/v2
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

# todo: build kvm_stream and kvm_system from source
define NANOKVM_SERVER_BUILD_CMDS
	mkdir -pv $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/
	if [ -e ${@D}/$(NANOKVM_SERVER_EXT_MIDDLEWARE)/$(NANOKVM_SERVER_EXT_MAIXCAM_LIB) ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/$(NANOKVM_SERVER_EXT_MIDDLEWARE)/$(NANOKVM_SERVER_EXT_MAIXCAM_LIB) $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/ ; \
	fi
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/$(NANOKVM_SERVER_EXT_MIDDLEWARE)/lib/3rd/libini.so $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/
	for l in $(NANOKVM_SERVER_REQUIRED_LIBS) ; do \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/$(NANOKVM_SERVER_EXT_MIDDLEWARE)/lib/$$l $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/ ; \
	done
	for l in $(NANOKVM_SERVER_VPU_LIBS) ; do \
		if [ -e ${@D}/$(NANOKVM_SERVER_EXT_MIDDLEWARE)/lib/$$l ]; then \
			rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/$(NANOKVM_SERVER_EXT_MIDDLEWARE)/lib/$$l $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/ ; \
		fi ; \
	done
	for l in $(NANOKVM_SERVER_UNUSED_LIBS) ; do \
		rm -f $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/$$l ; \
		ln -s libmisc.so $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/$$l ; \
	done
	if [ -e ${@D}/$(NANOKVM_SERVER_EXT_MIDDLEWARE)/lib/libcvi_dummy.so ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/$(NANOKVM_SERVER_EXT_MIDDLEWARE)/lib/libcvi_dummy.so $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/ ; \
		for l in $(NANOKVM_SERVER_DUMMY_LIBS) ; do \
			rm -f $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/$$l ; \
			ln -s libcvi_dummy.so $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/$$l ; \
		done ; \
	fi
	if [ -e ${@D}/$(NANOKVM_SERVER_EXT_MIDDLEWARE)/lib/libcvi_bin_light.so -a \
	     -e ${@D}/$(NANOKVM_SERVER_EXT_MIDDLEWARE)/lib/libisp_light.so ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/$(NANOKVM_SERVER_EXT_MIDDLEWARE)/lib/libcvi_bin_light.so $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/ ; \
		for l in libcvi_bin.so ; do \
			rm -f $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/$$l ; \
			ln -s libcvi_bin_light.so $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/$$l ; \
		done ; \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/$(NANOKVM_SERVER_EXT_MIDDLEWARE)/lib/libisp_light.so $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/ ; \
		for l in libcvi_bin_isp.so libisp.so ; do \
			rm -f $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/$$l ; \
			ln -s libisp_light.so $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/$$l ; \
		done ; \
		for l in libisp_algo.so ; do \
			rm -f $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/$$l ; \
			ln -s libmisc.so $(@D)/$(NANOKVM_SERVER_GOMOD)/dl_lib/$$l ; \
		done ; \
	fi
	cd $(@D)/$(NANOKVM_SERVER_GOMOD) ; \
	GOPROXY=direct $(GO_BIN) mod tidy
	cd $(@D)/$(NANOKVM_SERVER_GOMOD) ; \
	sed -i 's|-L../dl_lib -lkvm|-L../dl_lib -L$(TARGET_DIR)/usr/lib -lkvm|g' common/cgo.go ; \
	sed -i s/' -lkvm$$'/' -lkvm -lmaixcam_lib -latomic -lae -laf -lawb -lcvi_bin -lcvi_bin_isp -lini -lisp -lisp_algo -lsys -lvdec -lvenc -lvpu'/g common/cgo.go ; \
	sed -i s/'-lmaixcam_lib -latomic'/'-lmaixcam_lib -ljpeg -lopencv_calib3d -lopencv_core -lopencv_dnn -lopencv_features2d -lopencv_flann -lopencv_gapi -lopencv_imgcodecs -lopencv_imgproc -lopencv_objdetect -lopencv_video -lpng -lprotobuf -lsharpyuv -ltbb -ltiff -lwebp -lz -latomic'/g common/cgo.go ; \
	CGO_ENABLED=1 GOARCH=riscv64 GOOS=linux $(GO_BIN) build -x -ldflags="-extldflags '-Wl,-rpath,\$$ORIGIN/dl_lib'"
	cd $(@D)/web ; \
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
	mkdir -pv $(TARGET_DIR)/kvmapp/server/dl_lib/
	rsync -r --verbose --links --safe-links --hard-links ${@D}/server/dl_lib/ $(TARGET_DIR)/kvmapp/server/dl_lib/
	mkdir -pv $(TARGET_DIR)/kvmapp/server/web/
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/web/dist/ $(TARGET_DIR)/kvmapp/server/web/
endef

$(eval $(generic-package))
