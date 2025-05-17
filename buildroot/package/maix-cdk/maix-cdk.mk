################################################################################
#
# maix-cdk
#
################################################################################

MAIX_CDK_VERSION = da86eddb2c9551faa0cfd248447c53072e35555e
MAIX_CDK_SITE = $(call github,sipeed,MaixCDK,$(MAIX_CDK_VERSION))

MAIX_CDK_SAMPLE = rtsp_demo

MAIX_CDK_DEPENDENCIES =\
	host-cmake \
	host-pkgconf \
	host-python3 \
	host-python-pip \
	host-python-setuptools

ifeq ($(BR2_PACKAGE_MAIX_CDK_ALL_DEPENDENCIES),y)
MAIX_CDK_DEPENDENCIES +=\
	alsa-lib \
	ffmpeg \
	harfbuzz \
	opencv4
endif

ifeq ($(BR2_TOOLCHAIN_BUILDROOT),y)
MAIX_CDK_TOOLCHAIN_BIN := $(HOST_DIR)/bin
MAIX_CDK_TOOLCHAIN_PREFIX := $(BR2_ARCH)-buildroot-linux-gnu
else
MAIX_CDK_TOOLCHAIN_BIN := $(TOOLCHAIN_EXTERNAL_BIN)
MAIX_CDK_TOOLCHAIN_PREFIX := $(TOOLCHAIN_EXTERNAL_PREFIX)
endif

# maixcam pre-built binaries are only for riscv64
# MaixCDK searches for "musl" or "glibc" in toolchain path
MAIX_CDK_TOOLCHAIN_ARCH := $(BR2_ARCH)
MAIX_CDK_TOOLCHAIN_LIBC := $(findstring musl,$(realpath $(MAIX_CDK_TOOLCHAIN_BIN)))

MAIX_CDK_HARFBUZZ_VER = 8.2.1
MAIX_CDK_OPENCV_VER = 4.9.0

MAIX_CDK_EXT_MIDDLEWARE = $(realpath $(TOPDIR)/../middleware)
MAIX_CDK_EXT_MAIXCAM_LIB = sample/test_mmf/maixcam_lib/release.linux/libmaixcam_lib.so
MAIX_CDK_EXT_OSDRV = $(realpath $(TOPDIR)/../osdrv)

MAIX_CDK_MIDDLEWARE = components/3rd_party/sophgo-middleware/sophgo-middleware

MAIX_CDK_MAIXCAM_DIST = examples/$(MAIX_CDK_SAMPLE)/dist/$(MAIX_CDK_SAMPLE)_release

define MAIX_CDK_POST_EXTRACT_FIXUP
	mv $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2 $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2-cdk
	mkdir $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2
	if [ -e $(MAIX_CDK_EXT_MIDDLEWARE)/Makefile -a ! -e $(MAIX_CDK_EXT_MIDDLEWARE)/v2/Makefile ]; then \
		rsync -r --verbose --exclude=mod_tmp --copy-dirlinks --copy-links --hard-links $(MAIX_CDK_EXT_MIDDLEWARE)/ $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/ ; \
	else \
		rsync -r --verbose --exclude=mod_tmp --copy-dirlinks --copy-links --hard-links $(MAIX_CDK_EXT_MIDDLEWARE)/v2/ $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/ ; \
	fi
	mkdir $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/uapi
	if [ -e $(MAIX_CDK_EXT_OSDRV)/interdrv/include -a ! -e $(MAIX_CDK_EXT_OSDRV)/interdrv/v2/include ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(MAIX_CDK_EXT_OSDRV)/interdrv/include/common/uapi/ $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/uapi/ ; \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(MAIX_CDK_EXT_OSDRV)/interdrv/include/chip/mars/uapi/ $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/uapi/ ; \
	else \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(MAIX_CDK_EXT_OSDRV)/interdrv/v2/include/common/uapi/ $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/uapi/ ; \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(MAIX_CDK_EXT_OSDRV)/interdrv/v2/include/chip/mars/uapi/ $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/uapi/ ; \
	fi
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2-cdk/sample/vio/ $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/sample/vio/
	sed -i s/' dummy isp_light '/' '/g $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/modules/Makefile
	rm -rf $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/modules/dummy/
	rm -rf $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/modules/isp_light/
	if grep -q stSnsGc02m1_Obj $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/sample/common/sample_common_sensor.c ; then \
		if ! grep -q stSnsGc02m1_Obj $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/include/cvi_sns_ctrl.h ; then \
			sed -i s/stSnsGc02m1b_Obj/stSnsGc02m1_Obj/g $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/include/cvi_sns_ctrl.h ; \
		fi ; \
	fi
	if [ ! -e $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/lib/3rd/libcli.so ]; then \
		sed -i /'$${mmf_lib_dir}.3rd.libcli.so'/d $(@D)/components/3rd_party/sophgo-middleware/CMakeLists.txt ; \
		sed -i /'$${mmf_lib_dir}.3rd.libcli.so'/d $(@D)/components/maixcam_lib/CMakeLists.txt ; \
	fi
	if [ ! -e $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/lib/libjson-c.so.5 ]; then \
		sed -i /'$${mmf_lib_dir}.libjson-c.so.5'/d $(@D)/components/3rd_party/sophgo-middleware/CMakeLists.txt ; \
		sed -i /'$${mmf_lib_dir}.libjson-c.so.5'/d $(@D)/components/maixcam_lib/CMakeLists.txt ; \
	fi
	if [ -e $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/lib/libcvi_dnvqe.so -a ! -e $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/lib/libdnvqe.so ]; then \
		sed -i s/'libdnvqe.so'/'libcvi_dnvqe.so'/g $(@D)/components/3rd_party/sophgo-middleware/CMakeLists.txt ; \
		sed -i s/'libdnvqe.so'/'libcvi_dnvqe.so'/g $(@D)/components/maixcam_lib/CMakeLists.txt ; \
	fi
	if [ -e $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/lib/libcvi_ssp2.so ]; then \
		sed -i 's|$${mmf_lib_dir}/libcvi_dnvqe.so|\$${mmf_lib_dir}/libcvi_dnvqe.so $${mmf_lib_dir}/libcvi_ssp2.so|g' $(@D)/components/3rd_party/sophgo-middleware/CMakeLists.txt ; \
		sed -i 's|$${mmf_lib_dir}/libcvi_dnvqe.so|\$${mmf_lib_dir}/libcvi_dnvqe.so $${mmf_lib_dir}/libcvi_ssp2.so|g' $(@D)/components/maixcam_lib/CMakeLists.txt ; \
	fi
	if [ -e $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/lib/libgdc.so -a ! -e $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/lib/libosdc.so ]; then \
		sed -i s/'libosdc.so'/'libgdc.so'/g $(@D)/components/3rd_party/sophgo-middleware/CMakeLists.txt ; \
		sed -i s/'libosdc.so'/'libgdc.so'/g $(@D)/components/maixcam_lib/CMakeLists.txt ; \
	fi
	if [ -e $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/lib/libvi.so -a ! -e $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/lib/libvpu.so ]; then \
		sed -i s/'libvpu.so'/'libvi.so'/g $(@D)/components/3rd_party/sophgo-middleware/CMakeLists.txt ; \
		sed -i s/'libvpu.so'/'libvi.so'/g $(@D)/components/maixcam_lib/CMakeLists.txt ; \
	fi
	if [ -e $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/lib/libvo.so -a \
	     -e $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/lib/libvpss.so -a \
	     -e $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/lib/librgn.so ]; then \
		sed -i 's|$${mmf_lib_dir}/libvi.so|\$${mmf_lib_dir}/libvi.so $${mmf_lib_dir}/libvo.so $${mmf_lib_dir}/libvpss.so $${mmf_lib_dir}/librgn.so|g' $(@D)/components/3rd_party/sophgo-middleware/CMakeLists.txt ; \
		sed -i 's|$${mmf_lib_dir}/libvi.so|\$${mmf_lib_dir}/libvi.so $${mmf_lib_dir}/libvo.so $${mmf_lib_dir}/libvpss.so $${mmf_lib_dir}/librgn.so|g' $(@D)/components/maixcam_lib/CMakeLists.txt ; \
	fi
	if [ -e $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/uapi/linux/cvi_cv181x_defines.h ]; then \
		sed -i 's|^list.APPEND ADD_INCLUDE $${middleware_include_dir}.|list(APPEND ADD_INCLUDE $${middleware_include_dir})\n\nlist(APPEND ADD_DEFINITIONS -D__CV181X__)|g' $(@D)/components/3rd_party/sophgo-middleware/CMakeLists.txt ; \
		sed -i 's|^list.APPEND ADD_INCLUDE "include".|list(APPEND ADD_INCLUDE "include")\n\nlist(APPEND ADD_DEFINITIONS -D__CV181X__)|g' $(@D)/components/maixcam_lib/CMakeLists.txt ; \
	fi
	if [ -e $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/component/isp/common/sensor_list.h ]; then \
		sed -i 's|^    $${middleware_src_path}/v2/component/panel/sg200x|    $${middleware_src_path}/v2/component/isp/common\n    $${middleware_src_path}/v2/component/panel/sg200x|g' $(@D)/components/3rd_party/sophgo-middleware/CMakeLists.txt ; \
		sed -i 's|^append_srcs_dir(middleware_src_dir  $${middleware_src_path}/v2/sample/common|append_srcs_dir(middleware_src_dir  $${middleware_src_path}/v2/sample/common\n                                    $${middleware_src_path}/v2/component/isp/common|g' $(@D)/components/3rd_party/sophgo-middleware/CMakeLists.txt ; \
		sed -i 's|^        $${middleware_src_path}/v2/component/panel/sg200x|        $${middleware_src_path}/v2/component/isp/common\n        $${middleware_src_path}/v2/component/panel/sg200x|g' $(@D)/components/maixcam_lib/CMakeLists.txt ; \
		sed -i 's|^    append_srcs_dir(middleware_src_dir  $${middleware_src_path}/v2/sample/common|    append_srcs_dir(middleware_src_dir  $${middleware_src_path}/v2/component/isp/common\n                                        $${middleware_src_path}/v2/sample/common|g' $(@D)/components/maixcam_lib/CMakeLists.txt ; \
	fi
	if [ -e $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/$(MAIX_CDK_EXT_MAIXCAM_LIB) ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/$(MAIX_CDK_EXT_MAIXCAM_LIB) ${@D}/components/maixcam_lib/lib/ ; \
	fi
	@$(eval OPENCV4_SUFFIX=$(shell echo "$(OPENCV4_VERSION)" | cut -d '.' -f 1-2 | tr -d '.'))
	if [ "X$(BR2_PACKAGE_MAIX_CDK_ALL_DEPENDENCIES)" = "Xy" -a "$(MAIX_CDK_OPENCV_VER)-$(MAIX_CDK_TOOLCHAIN_ARCH)-$(MAIX_CDK_TOOLCHAIN_LIBC)" != "$(OPENCV4_VERSION)-riscv64-musl" ]; then \
		sed -i 's|set(alsa_lib_dir "lib")|set(alsa_lib_dir "$(TARGET_DIR)/usr/lib")|g' $(@D)/components/3rd_party/alsa_lib/CMakeLists.txt ; \
		sed -i 's|set(alsa_lib_include_dir "include")|set(alsa_lib_include_dir "$(TARGET_DIR)/usr/include")|g' $(@D)/components/3rd_party/alsa_lib/CMakeLists.txt ; \
		sed -i 's|set(src_path "$${ffmpeg_unzip_path}/ffmpeg")|set(src_path "$(TARGET_DIR)/usr")|g' $(@D)/components/3rd_party/FFmpeg/CMakeLists.txt ; \
		[ -e $(TARGET_DIR)/usr/lib/libavresample.so ] || sed -i /libavresample.so/d $(@D)/components/3rd_party/FFmpeg/CMakeLists.txt ; \
		sed -i 's|                            $${src_path}/lib/libswscale.so|                            $${src_path}/lib/libswscale.so\n                            $${src_path}/lib/liblzma.so\n                            $${src_path}/lib/libxml2.so\n                            $${src_path}/lib/libz.so\n                            $${src_path}/lib/libbz2.so\n                            $${src_path}/lib/libssl.so\n                            $${src_path}/lib/libcrypto.so|g' $(@D)/components/3rd_party/FFmpeg/CMakeLists.txt ; \
		sed -i s/'# list.APPEND ADD_REQUIREMENTS.$$'/'list(APPEND ADD_REQUIREMENTS alsa_lib)'/g $(@D)/components/3rd_party/FFmpeg/CMakeLists.txt ; \
		sed -i s/'default n'/'default y'/g $(@D)/components/3rd_party/opencv/Kconfig ; \
		sed -i s/'# list.APPEND ADD_REQUIREMENTS pthread dl atomic.$$'/'list(APPEND ADD_REQUIREMENTS pthread dl atomic)'/g $(@D)/components/3rd_party/opencv/CMakeLists.txt ; \
		sed -i s/'version_str "$(MAIX_CDK_OPENCV_VER)"'/'version_str "$(OPENCV4_VERSION)"'/g $(@D)/components/3rd_party/opencv/CMakeLists.txt ; \
		sed -i s/'so_suffix_number "4.."'/'so_suffix_number "$(OPENCV4_SUFFIX)"'/g $(@D)/components/3rd_party/opencv/CMakeLists.txt ; \
		mkdir -pv $(@D)/dl/extracted/harfbuzz_srcs/harfbuzz-$(MAIX_CDK_HARFBUZZ_VER)/ ; \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(@D)/../harfbuzz-$(HARFBUZZ_VERSION)/ $(@D)/dl/extracted/harfbuzz_srcs/harfbuzz-$(MAIX_CDK_HARFBUZZ_VER)/ ; \
		mkdir -pv $(@D)/dl/extracted/opencv/opencv4/opencv-$(OPENCV4_VERSION)/ ; \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(@D)/../opencv4-$(OPENCV4_VERSION)/ $(@D)/dl/extracted/opencv/opencv4/opencv-$(OPENCV4_VERSION)/ ; \
		sed -i /'list.APPEND ADD_REQUIREMENTS cvi_tpu.'/d $(@D)/components/maixcam_lib/CMakeLists.txt ; \
	fi
endef
MAIX_CDK_POST_EXTRACT_HOOKS += MAIX_CDK_POST_EXTRACT_FIXUP

define MAIX_CDK_BUILD_CMDS
	sed -i s/'^    url: .*'/'    url:'/g $(@D)/platforms/maixcam.yaml
	sed -i s/'^    sha256sum: .*'/'    sha256sum:'/g $(@D)/platforms/maixcam.yaml
	sed -i s/'^    filename: .*'/'    filename:'/g $(@D)/platforms/maixcam.yaml
	sed -i s/'^    path: .*'/'    path:'/g $(@D)/platforms/maixcam.yaml
	sed -i 's|^    bin_path: .*|    bin_path: '$(realpath $(MAIX_CDK_TOOLCHAIN_BIN))'|g' $(@D)/platforms/maixcam.yaml ; \
	sed -i 's|^    prefix: .*|    prefix: '$(MAIX_CDK_TOOLCHAIN_PREFIX)'-|g' $(@D)/platforms/maixcam.yaml
	sed -i "s|^    c_flags: .*|    c_flags: $(TARGET_LDFLAGS)|g" $(@D)/platforms/maixcam.yaml
	sed -i "s|^    cxx_flags: .*|    cxx_flags: $(TARGET_LDFLAGS)|g" $(@D)/platforms/maixcam.yaml
	sed -i 's|COMMAND python |COMMAND '$(HOST_DIR)/bin/python3' |g' $(@D)/tools/cmake/*.cmake
	sed -i 's|COMMAND python3 |COMMAND '$(HOST_DIR)/bin/python3' |g' $(@D)/tools/cmake/*.cmake
	sed -i 's|set.$${python} python3 |set($${python} '$(HOST_DIR)/bin/python3' |g' $(@D)/tools/cmake/*.cmake
	[ "X$(BR2_TOOLCHAIN_BUILDROOT)" != "Xy" ] || sed -i /'^    $${strip_cmd}'/d $(@D)/tools/cmake/gen_binary.cmake
	rm -rf $(@D)/components/3rd_party/ax620e_msp/
	cd $(@D)/ ; \
	$(HOST_DIR)/bin/python3 -m pip install -r requirements.txt
	if [ "X$(BR2_PACKAGE_MAIX_CDK_ALL_DEPENDENCIES)" = "Xy" ]; then \
		cd $(@D)/examples/$(MAIX_CDK_SAMPLE)/ ; \
		PATH=$(BR_PATH) $(HOST_DIR)/bin/maixcdk build -p maixcam ; \
	fi
	rm -rf $(@D)/projects/app_classifier/
	rm -rf $(@D)/projects/app_detector/
	rm -rf $(@D)/projects/app_self_learn_tracker/
	rm -rf $(@D)/projects/app_speech/
	if [ -e $(@D)/projects/app_uvc_camera/main/CMakeLists.txt ]; then \
		sed -i s/'basic nn vision'/'basic vision'/g $(@D)/projects/app_uvc_camera/main/CMakeLists.txt ; \
	fi
	if [ "X$(BR2_PACKAGE_MAIX_CDK_ALL_PROJECTS)" = "Xy" -a -e $(@D)/distapps.sh -a -e $(@D)/projects/build_all.sh ]; then \
		chmod +x $(@D)/projects/build_all.sh ; \
		cd $(@D)/projects/ ; \
		PATH=$(BR_PATH) ./build_all.sh ; \
	fi
	rm -rf $(@D)/examples/bytetrack_demo/
	rm -rf $(@D)/examples/nn_*/
	rm -rf $(@D)/examples/rtsp_yolo_demo/
	if [ -e $(@D)/examples/maix_bm8563/app.yaml ]; then \
		sed -i s/bm8653/bm8563/g $(@D)/examples/maix_bm8563/app.yaml ; \
	fi
	if [ -e $(@D)/examples/mlx90640/app.yaml ]; then \
		sed -i s/mlx90640_$$/mlx90640/g $(@D)/examples/mlx90640/app.yaml ; \
	fi
	if [ -e $(@D)/examples/i18n/app.yaml ]; then \
		if grep -q 'id: i18n_demo' $(@D)/examples/i18n/app.yaml ; then \
			mv $(@D)/examples/i18n $(@D)/examples/i18n_demo ; \
		fi ; \
	fi
	if [ -e $(@D)/examples/peripheral_gpio/app.yaml ]; then \
		if grep -q 'id: switch_led' $(@D)/examples/peripheral_gpio/app.yaml ; then \
			mv $(@D)/examples/peripheral_gpio $(@D)/examples/switch_led ; \
		fi ; \
	fi
	if [ "X$(BR2_PACKAGE_MAIX_CDK_ALL_EXAMPLES)" = "Xy" -a -e $(@D)/distapps.sh -a -e $(@D)/test/test_examples/test_cases.sh ]; then \
		chmod +x $(@D)/test/test_examples/test_cases.sh ; \
		cd $(@D)/test/test_examples/ ; \
		PATH=$(BR_PATH) ./test_cases.sh maixcam 0 ; \
	fi
	if [ "X$(BR2_PACKAGE_MAIX_CDK_ALL_DEPENDENCIES)" != "Xy" ]; then \
		rm -rf $(@D)/components/3rd_party/alsa_lib/ ; \
		rm -rf $(@D)/components/3rd_party/cvi_tpu/ ; \
		rm -rf $(@D)/components/3rd_party/FFmpeg/ ; \
		rm -rf $(@D)/components/3rd_party/harfbuzz/ ; \
		rm -rf $(@D)/components/3rd_party/opencv/ ; \
		rm -rf $(@D)/components/3rd_party/opencv_freetype/ ; \
		rm -rf $(@D)/components/nn/ ; \
		rm -rf $(@D)/components/vision_extra/ ; \
		rm -rf $(@D)/examples/*/ ; \
		rm -rf $(@D)/projects/*/ ; \
	elif [ -e $(@D)/distapps.sh ]; then \
		chmod +x $(@D)/distapps.sh ; \
		cd $(@D)/ ; \
		PATH=$(BR_PATH) ./distapps.sh ; \
	fi
endef

define MAIX_CDK_INSTALL_TARGET_CMDS
	if [ -e  ${@D}/$(MAIX_CDK_MAIXCAM_DIST) -a ! -e ${@D}/$(MAIX_CDK_MAIXCAM_DIST)/dl_lib/libmaixcam_lib.so ] ; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/components/maixcam_lib/lib/libmaixcam_lib.so ${@D}/$(MAIX_CDK_MAIXCAM_DIST)/dl_lib/ ; \
	fi
	if [ -e ${@D}/dist/maixapp/lib -a ! -e ${@D}/dist/maixapp/lib/libmaixcam_lib.so ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/components/maixcam_lib/lib/libmaixcam_lib.so ${@D}/dist/maixapp/lib/ ; \
	fi
	if [ -e ${@D}/dist/maixapp ]; then \
		mkdir -pv $(TARGET_DIR)/maixapp/tmp/ ; \
		rsync -r --verbose --links --safe-links --hard-links ${@D}/dist/maixapp/ $(TARGET_DIR)/maixapp/ ; \
	elif [ -e  ${@D}/$(MAIX_CDK_MAIXCAM_DIST) ]; then \
		mkdir -pv $(TARGET_DIR)/maixapp/lib/ ; \
		mkdir -pv $(TARGET_DIR)/maixapp/tmp/ ; \
		mkdir -pv $(TARGET_DIR)/maixapp/$(MAIX_CDK_SAMPLE)/ ; \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/$(MAIX_CDK_MAIXCAM_DIST)/ $(TARGET_DIR)/maixapp/$(MAIX_CDK_SAMPLE)/ ; \
		rm -rf $(TARGET_DIR)/maixapp/$(MAIX_CDK_SAMPLE)/dl_lib ; \
		ln -s ../lib $(TARGET_DIR)/maixapp/$(MAIX_CDK_SAMPLE)/dl_lib ; \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/$(MAIX_CDK_MAIXCAM_DIST)/dl_lib/ $(TARGET_DIR)/maixapp/lib/ ; \
	fi
endef

$(eval $(generic-package))
