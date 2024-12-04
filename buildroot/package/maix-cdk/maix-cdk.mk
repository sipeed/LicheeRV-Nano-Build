################################################################################
#
# maix-cdk
#
################################################################################

MAIX_CDK_VERSION = 63735ac3d34c507b52ec530b558dc8e0d55de958
MAIX_CDK_SITE = $(call github,sipeed,MaixCDK,$(MAIX_CDK_VERSION))

MAIX_CDK_SAMPLE = rtsp_demo

MAIX_CDK_DEPENDENCIES =\
	host-cmake \
	host-pkgconf \
	host-python3 \
	host-python-pip \
	host-python-setuptools

MAIX_CDK_EXT_MIDDLEWARE = ../../../../middleware
MAIX_CDK_EXT_MAIXCAM_LIB = sample/test_mmf/maixcam_lib/release.linux/libmaixcam_lib.so
MAIX_CDK_EXT_OSDRV = ../../../../osdrv

MAIX_CDK_MIDDLEWARE = components/3rd_party/sophgo-middleware/sophgo-middleware

MAIX_CDK_MAIXCAM_DIST = examples/$(MAIX_CDK_SAMPLE)/dist/$(MAIX_CDK_SAMPLE)_release

define MAIX_CDK_POST_EXTRACT_FIXUP
	mv $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2 $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2-cdk
	mkdir $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2
	if [ -e $(@D)/$(MAIX_CDK_EXT_MIDDLEWARE)/Makefile -a ! -e $(@D)/$(MAIX_CDK_EXT_MIDDLEWARE)/v2/Makefile ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(@D)/$(MAIX_CDK_EXT_MIDDLEWARE)/ $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/ ; \
	else \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(@D)/$(MAIX_CDK_EXT_MIDDLEWARE)/v2/ $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/ ; \
	fi
	mkdir $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/uapi
	if [ -e $(@D)/$(MAIX_CDK_EXT_OSDRV)/interdrv/include -a ! -e $(@D)/$(MAIX_CDK_EXT_OSDRV)/interdrv/v2/include ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(@D)/$(MAIX_CDK_EXT_OSDRV)/interdrv/include/common/uapi/ $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/uapi/ ; \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(@D)/$(MAIX_CDK_EXT_OSDRV)/interdrv/include/chip/mars/uapi/ $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/uapi/ ; \
	else \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(@D)/$(MAIX_CDK_EXT_OSDRV)/interdrv/v2/include/common/uapi/ $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/uapi/ ; \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(@D)/$(MAIX_CDK_EXT_OSDRV)/interdrv/v2/include/chip/mars/uapi/ $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/uapi/ ; \
	fi
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2-cdk/sample/vio/ $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/sample/vio/
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
	sed -i s/'^    url: .*'/'    url:'/g $(@D)/platforms/maixcam.yaml
	sed -i s/'^    sha256sum: .*'/'    sha256sum:'/g $(@D)/platforms/maixcam.yaml
	sed -i s/'^    filename: .*'/'    filename:'/g $(@D)/platforms/maixcam.yaml
	sed -i s/'^    path: .*'/'    path:'/g $(@D)/platforms/maixcam.yaml
	sed -i 's|^    bin_path: .*|    bin_path: '$(realpath $(TOOLCHAIN_EXTERNAL_BIN))'|g' $(@D)/platforms/maixcam.yaml
	sed -i 's|COMMAND python |COMMAND '$(HOST_DIR)/bin/python3' |g' $(@D)/tools/cmake/*.cmake
	sed -i 's|COMMAND python3 |COMMAND '$(HOST_DIR)/bin/python3' |g' $(@D)/tools/cmake/*.cmake
	sed -i 's|set.$${python} python3 |set($${python} '$(HOST_DIR)/bin/python3' |g' $(@D)/tools/cmake/*.cmake
endef
MAIX_CDK_POST_EXTRACT_HOOKS += MAIX_CDK_POST_EXTRACT_FIXUP

define MAIX_CDK_BUILD_CMDS
	cd $(@D)/ ; \
	$(HOST_DIR)/bin/python3 -m pip install -r requirements.txt
	cd $(@D)/examples/$(MAIX_CDK_SAMPLE)/ ; \
	$(HOST_DIR)/bin/maixcdk build -p maixcam
	rm -rf $(@D)/projects/app_classifier/
	rm -rf $(@D)/projects/app_detector/
	rm -rf $(@D)/projects/app_self_learn_tracker/
	rm -rf $(@D)/projects/app_speech/
	if [ -e $(@D)/distapps.sh -a -e $(@D)/projects/build_all.sh ]; then \
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
	if [ -e $(@D)/distapps.sh -a -e $(@D)/test/test_examples/test_cases.sh ]; then \
		chmod +x $(@D)/test/test_examples/test_cases.sh ; \
		cd $(@D)/test/test_examples/ ; \
		PATH=$(BR_PATH) ./test_cases.sh maixcam 0 ; \
	fi
	if [ -e $(@D)/distapps.sh ]; then \
		chmod +x $(@D)/distapps.sh ; \
		cd $(@D)/ ; \
		PATH=$(BR_PATH) ./distapps.sh ; \
	fi
endef

define MAIX_CDK_INSTALL_TARGET_CMDS
	if [ ! -e ${@D}/$(MAIX_CDK_MAIXCAM_DIST)/dl_lib/libmaixcam_lib.so ] ; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/components/maixcam_lib/lib/libmaixcam_lib.so ${@D}/$(MAIX_CDK_MAIXCAM_DIST)/dl_lib/ ; \
	fi
	if [ -e ${@D}/dist/maixapp/lib -a ! -e ${@D}/dist/maixapp/lib/libmaixcam_lib.so ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/components/maixcam_lib/lib/libmaixcam_lib.so ${@D}/dist/maixapp/lib/ ; \
	fi
	#mkdir -pv $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/
	#rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/$(MAIX_CDK_MAIXCAM_DIST)/dl_lib/libmaixcam_lib.so $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/
	mkdir -pv $(TARGET_DIR)/maixapp/lib
	mkdir -pv $(TARGET_DIR)/maixapp/tmp
	if [ -e ${@D}/dist/maixapp ]; then \
		mkdir -pv $(TARGET_DIR)/maixapp/ ; \
		rsync -r --verbose --links --safe-links --hard-links ${@D}/dist/maixapp/ $(TARGET_DIR)/maixapp/ ; \
	else \
		mkdir -pv $(TARGET_DIR)/maixapp/$(MAIX_CDK_SAMPLE)/ ; \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/$(MAIX_CDK_MAIXCAM_DIST)/ $(TARGET_DIR)/maixapp/$(MAIX_CDK_SAMPLE)/ ; \
		rm -rf $(TARGET_DIR)/maixapp/$(MAIX_CDK_SAMPLE)/dl_lib ; \
		ln -s ../lib $(TARGET_DIR)/maixapp/$(MAIX_CDK_SAMPLE)/dl_lib ; \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/$(MAIX_CDK_MAIXCAM_DIST)/dl_lib/ $(TARGET_DIR)/maixapp/lib/ ; \
	fi
	#rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(MAIX_CDK_PKGDIR)/overlay/ $(TARGET_DIR)/
endef

$(eval $(generic-package))
