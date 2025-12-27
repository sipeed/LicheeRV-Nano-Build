include $(PWD)/../Makefile.interdrv.param

COMMON_PATH = common
VCODEC_DRIVER_PATH = vcodec
JPEG_DRIVER_PATH = jpeg
VCODEC_MODULE_PATH = module

ifneq ($(KERNELRELEASE),)
# kbuild part of makefile

VCODEC_DRIVER_HEADER := -I$(PWD)/	\
			-I$(PWD)/include/	\
			-I$(PWD)/$(COMMON_PATH)/	\
			-I$(PWD)/$(VCODEC_DRIVER_PATH)/	\
			-I$(PWD)/$(VCODEC_DRIVER_PATH)/sample/	\
			-I$(PWD)/$(VCODEC_DRIVER_PATH)/sample/helper/	\
			-I$(PWD)/$(VCODEC_DRIVER_PATH)/sample/helper/misc/	\
			-I$(PWD)/$(VCODEC_DRIVER_PATH)/vpuapi/	\
			-I$(PWD)/$(VCODEC_DRIVER_PATH)/vdi/	\
			-I$(PWD)/$(VCODEC_DRIVER_PATH)/cvi/	\
			-I$(PWD)/$(JPEG_DRIVER_PATH)/	\
			-I$(PWD)/$(JPEG_DRIVER_PATH)/include/	\
			-I$(PWD)/$(JPEG_DRIVER_PATH)/jdi/	\
			-I$(PWD)/$(JPEG_DRIVER_PATH)/jdi/linux/	\
			-I$(PWD)/$(JPEG_DRIVER_PATH)/jpuapi/	\
			-I$(PWD)/$(JPEG_DRIVER_PATH)/src/	\
			-I$(PWD)/$(JPEG_DRIVER_PATH)/cvi_jpg_interface/	\
			-I$(PWD)/$(JPEG_DRIVER_PATH)/cvi_jpg_interface/src/	\
			-I$(PWD)/$(JPEG_DRIVER_PATH)/cvi_jpg_interface/inc/	\
			-I$(PWD)/$(VCODEC_MODULE_PATH)/include/

cvi_vc_driver-objs :=	$(COMMON_PATH)/rcKernel/cvi_rc_kernel.o	\
			$(COMMON_PATH)/rcKernel/cvi_float_point/cvi_float_point.o	\
			$(COMMON_PATH)/rcKernel/cvi_float_point/cvi_soft_float.o	\
			$(VCODEC_DRIVER_PATH)/sample/main_enc_cvitest.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/main_helper.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/vpuhelper.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/bitstream/bitstreamfeeder.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/bitstream/bitstreamreader.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/bitstream/bsfeeder_fixedsize_impl.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/bitstream/bsfeeder_es_in_impl.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/comparator/comparator.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/display/simplerenderer.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/misc/cfgParser.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/misc/cnm_video_helper.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/misc/datastructure.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/misc/platform.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/misc/debug.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/misc/pbu.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/yuv/yuvfeeder.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/yuv/yuvAddrfeeder.o	\
			$(VCODEC_DRIVER_PATH)/vdi/linux/vdi.o	\
			$(VCODEC_DRIVER_PATH)/vdi/linux/vdi_osal.o	\
			$(VCODEC_DRIVER_PATH)/vpuapi/product.o	\
			$(VCODEC_DRIVER_PATH)/vpuapi/vpuapifunc.o	\
			$(VCODEC_DRIVER_PATH)/vpuapi/vpuapi.o	\
			$(VCODEC_DRIVER_PATH)/vpuapi/coda9/coda9.o	\
			$(VCODEC_DRIVER_PATH)/vpuapi/wave/common/common.o	\
			$(VCODEC_DRIVER_PATH)/vpuapi/wave/wave4/wave4.o	\
			$(VCODEC_DRIVER_PATH)/cvi/cvi_enc_rc.o	\
			$(VCODEC_DRIVER_PATH)/cvi/cvi_vcodec_lib.o	\
			$(VCODEC_DRIVER_PATH)/cvi/cvi_enc_internal.o	\
			$(VCODEC_DRIVER_PATH)/cvi/cvi_dec_internal.o	\
			$(VCODEC_DRIVER_PATH)/cvi/cvi_vdec_api.o	\
			$(VCODEC_DRIVER_PATH)/cvi/cvi_h265_dec.o	\
			$(VCODEC_DRIVER_PATH)/cvi/cvi_h264_dec.o	\
			$(JPEG_DRIVER_PATH)/jdi/linux/jdi.o	\
			$(JPEG_DRIVER_PATH)/jpuapi/jpuapi.o	\
			$(JPEG_DRIVER_PATH)/jpuapi/jpuapifunc.o	\
			$(JPEG_DRIVER_PATH)/src/jpuhelper.o	\
			$(JPEG_DRIVER_PATH)/src/jpulog.o	\
			$(JPEG_DRIVER_PATH)/src/mixer.o	\
			$(JPEG_DRIVER_PATH)/cvi_jpg_interface/src/cvi_jpg_interface.o	\
			$(JPEG_DRIVER_PATH)/cvi_jpg_interface/src/cvi_jpg_dec_internal.o	\
			$(JPEG_DRIVER_PATH)/cvi_jpg_interface/src/cvi_jpg_enc_internal.o	\
			$(JPEG_DRIVER_PATH)/cvi_jpg_interface/test/cvi_jpg_codec.o	\
			$(VCODEC_MODULE_PATH)/src/enc_ctx.o	\
			$(VCODEC_MODULE_PATH)/src/cvi_venc.o	\
			$(VCODEC_MODULE_PATH)/src/cvi_vdec.o	\
			cvi_vc_drv.o	\
			cvi_vc_drv_proc.o

obj-m = soph_vc_driver.o
soph_vc_driver-y = $(cvi_vc_driver-objs)

ccflags-y += -I$(PWD)/../include/common/uapi/ -I$(PWD)/../include/chip/$(CHIP_CODE)/uapi/
ccflags-y += -I$(PWD)/../include/common/kapi/
ccflags-y += -I$(PWD)/../base/
ccflags-y += -I$(PWD)/../base/chip/$(CHIP_CODE)
ccflags-y += -I$(PWD)/../sys/common/ -I$(PWD)/../sys/common/uapi
ccflags-y += -I$(srctree)/drivers/tee
ccflags-y += -I$(srctree)/drivers/staging/android
ccflags-y += -I$(PWD)/../base/$(CHIP_CODE)
ccflags-y += -I$(LDDINCDIR) -Wno-date-time -Wno-error=date-time
ccflags-y += $(VCODEC_DRIVER_HEADER)
ccflags-y += -I$(PWD)/../include/
ccflags-y += -DUSE_KERNEL_MODE -DCVI_H26X_USE_ION_MEM -DCVI_JPG_USE_ION_MEM -DBITSTREAM_ION_CACHED_MEM -DMJPEG_INTERFACE_API -DCVI_H26X_ES_BUFFER_QUEUE_ENABLE -DCVI_H26X_USE_ION_FW_BUFFER -DENV_SET_ADDR_REMAP
#ccflags-y += -DVC_DRIVER_TEST
ccflags-y += -DENABLE_DEC
ifeq ($(CVIARCH_L),cv183x)
ccflags-y += -DARCH_CV183X
else
ccflags-y += -DARCH_CV182X
endif

ccflags-y += -Werror

ccflags-y += -DVC_DEBUG_BASIC_LEVEL

KBUILD_EXTRA_SYMBOLS = $(PWD)/../sys/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../base/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../vcodec/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../jpeg/Module.symvers

else
# normal makefile

ifeq ($(SDK_VER), 64bit)
ROIRC_STATIC_LIB=./$(VCODEC_DRIVER_PATH)/vpuapi/coda9/980_roi_rc_lib/libroirc_aarch64.a
else
ROIRC_STATIC_LIB=./$(VCODEC_DRIVER_PATH)/vpuapi/coda9/980_roi_rc_lib/libroirc_arm.a
endif

VCODEC_DRIVER_HEADER := -I$(PWD)/	\
			-I$(PWD)/include/	\
			-I$(PWD)/$(VCODEC_DRIVER_PATH)/	\
			-I$(PWD)/$(VCODEC_DRIVER_PATH)/sample/	\
			-I$(PWD)/$(VCODEC_DRIVER_PATH)/sample/helper/	\
			-I$(PWD)/$(VCODEC_DRIVER_PATH)/sample/helper/misc/	\
			-I$(PWD)/$(VCODEC_DRIVER_PATH)/vpuapi/	\
			-I$(PWD)/$(VCODEC_DRIVER_PATH)/vdi/	\
			-I$(PWD)/$(VCODEC_DRIVER_PATH)/cvi/	\
			-I$(PWD)/$(JPEG_DRIVER_PATH)/	\
			-I$(PWD)/$(JPEG_DRIVER_PATH)/include/	\
			-I$(PWD)/$(JPEG_DRIVER_PATH)/jdi/	\
			-I$(PWD)/$(JPEG_DRIVER_PATH)/jdi/linux/	\
			-I$(PWD)/$(JPEG_DRIVER_PATH)/jpuapi/	\
			-I$(PWD)/$(JPEG_DRIVER_PATH)/src/	\
			-I$(PWD)/$(JPEG_DRIVER_PATH)/cvi_jpg_interface/	\
			-I$(PWD)/$(JPEG_DRIVER_PATH)/cvi_jpg_interface/src/	\
			-I$(PWD)/$(JPEG_DRIVER_PATH)/cvi_jpg_interface/inc/	\
			-I$(PWD)/$(VCODEC_MODULE_PATH)/include/	\
			-I$(srctree)/drivers/staging/android/	\

cvi_vc_driver-objs :=	$(VCODEC_DRIVER_PATH)/sample/main_enc_cvitest.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/main_helper.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/vpuhelper.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/bitstream/bitstreamfeeder.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/bitstream/bitstreamreader.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/bitstream/bsfeeder_fixedsize_impl.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/bitstream/bsfeeder_es_in_impl.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/comparator/comparator.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/display/simplerenderer.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/misc/cfgParser.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/misc/cnm_video_helper.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/misc/datastructure.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/misc/platform.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/misc/debug.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/misc/pbu.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/yuv/yuvfeeder.o	\
			$(VCODEC_DRIVER_PATH)/sample/helper/yuv/yuvAddrfeeder.o	\
			$(VCODEC_DRIVER_PATH)/vdi/linux/vdi.o	\
			$(VCODEC_DRIVER_PATH)/vdi/linux/vdi_osal.o	\
			$(VCODEC_DRIVER_PATH)/vpuapi/product.o	\
			$(VCODEC_DRIVER_PATH)/vpuapi/vpuapifunc.o	\
			$(VCODEC_DRIVER_PATH)/vpuapi/vpuapi.o	\
			$(VCODEC_DRIVER_PATH)/vpuapi/coda9/coda9.o	\
			$(VCODEC_DRIVER_PATH)/vpuapi/wave/common/common.o	\
			$(VCODEC_DRIVER_PATH)/vpuapi/wave/wave4/wave4.o	\
			$(VCODEC_DRIVER_PATH)/cvi/cvi_enc_rc.o	\
			$(VCODEC_DRIVER_PATH)/cvi/cvi_vcodec_lib.o	\
			$(VCODEC_DRIVER_PATH)/cvi/cvi_enc_internal.o	\
			$(VCODEC_DRIVER_PATH)/cvi/cvi_dec_internal.o	\
			$(VCODEC_DRIVER_PATH)/cvi/cvi_vdec_api.o	\
			$(JPEG_DRIVER_PATH)/jdi/linux/jdi.o	\
			$(JPEG_DRIVER_PATH)/jdi/mm.o	\
			$(JPEG_DRIVER_PATH)/jpuapi/jpuapi.o	\
			$(JPEG_DRIVER_PATH)/jpuapi/jpuapifunc.o	\
			$(JPEG_DRIVER_PATH)/src/jpuhelper.o	\
			$(JPEG_DRIVER_PATH)/src/mixer.o	\
			$(JPEG_DRIVER_PATH)/cvi_jpg_interface/src/cvi_jpg_interface.o	\
			$(JPEG_DRIVER_PATH)/cvi_jpg_interface/src/cvi_jpg_dec_internal.o	\
			$(JPEG_DRIVER_PATH)/cvi_jpg_interface/src/cvi_jpg_enc_internal.o	\
			$(VCODEC_MODULE_PATH)/src/enc_ctx.o	\
			$(VCODEC_MODULE_PATH)/src/cvi_venc.o	\
			$(VCODEC_MODULE_PATH)/src/cvi_vdec.o	\
			vcodec_drv.o

#cvi_vc_drv-objs +=	$(ROIRC_STATIC_LIB)

VCODEC_DRIVER_DEFINES = -DUSE_KERNEL_MODE -DCVI_H26X_USE_ION_MEM -DBITSTREAM_ION_CACHED_MEM -DCVI_JPG_USE_ION_MEM -DBITSTREAM_ION_CACHED_MEM -DMJPEG_INTERFACE_API -DCVI_H26X_ES_BUFFER_QUEUE_ENABLE

ifeq ($(CHIP_ARCH),CV183X)
VCODEC_DRIVER_DEFINES += -DARCH_CV183X
else
VCODEC_DRIVER_DEFINES += -DARCH_CV182X
endif

ccflags-y += $(PWD) -I$(LDDINCDIR) $(VCODEC_DRIVER_HEADER) $(VCODEC_DRIVER_DEFINES) -Wno-date-time

PWD	:= $(shell pwd)
obj-m = cvi_vc_drv.o
cvi_vc_drv-y = $(cvi_vc_drv-objs)

KBUILD_EXTRA_SYMBOLS = $(PWD)/Module.symvers

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -rf .tmp_versions Module.symvers
	rm -rf modules.order
	rm -rf module/src/.*.o.cmd
	rm -rf common/rcKernel/cvi_float_point/.*.o.cmd
	rm -rf common/rcKernel/.*.o.cmd
	rm -rf $(VCODEC_DRIVER_PATH)/sample/*.o $(VCODEC_DRIVER_PATH)/sample/.*.cmd
	rm -rf $(VCODEC_DRIVER_PATH)/sample/helper/*.o $(VCODEC_DRIVER_PATH)/sample/helper/.*.cmd
	rm -rf $(VCODEC_DRIVER_PATH)/sample/helper/bitstream/*.o $(VCODEC_DRIVER_PATH)/sample/helper/bitstream/.*.cmd
	rm -rf $(VCODEC_DRIVER_PATH)/sample/helper/comparator/*.o $(VCODEC_DRIVER_PATH)/sample/helper/comparator/.*.cmd
	rm -rf $(VCODEC_DRIVER_PATH)/sample/helper/display/*.o $(VCODEC_DRIVER_PATH)/sample/helper/display/.*.cmd
	rm -rf $(VCODEC_DRIVER_PATH)/sample/helper/misc/*.o $(VCODEC_DRIVER_PATH)/sample/helper/misc/.*.cmd
	rm -rf $(VCODEC_DRIVER_PATH)/sample/helper/yuv/*.o $(VCODEC_DRIVER_PATH)/sample/helper/yuv/.*.cmd
	rm -rf $(VCODEC_DRIVER_PATH)/vdi/linux/*.o $(VCODEC_DRIVER_PATH)/vdi/linux/.*.cmd
	rm -rf $(VCODEC_DRIVER_PATH)/vpuapi/*.o $(VCODEC_DRIVER_PATH)/vpuapi/.*.cmd
	rm -rf $(VCODEC_DRIVER_PATH)/vpuapi/coda9/*.o $(VCODEC_DRIVER_PATH)/vpuapi/coda9/.*.cmd $(VCODEC_DRIVER_PATH)/vpuapi/coda9/.*.o.d
	rm -rf $(VCODEC_DRIVER_PATH)/vpuapi/wave/common/*.o $(VCODEC_DRIVER_PATH)/vpuapi/wave/common/.*.cmd
	rm -rf $(VCODEC_DRIVER_PATH)/vpuapi/wave/wave4/*.o $(VCODEC_DRIVER_PATH)/vpuapi/wave/wave4/.*.cmd
	rm -rf $(VCODEC_DRIVER_PATH)/cvi/*.o $(VCODEC_DRIVER_PATH)/cvi/.*.cmd
	rm -rf $(VCODEC_DRIVER_PATH)/*.o $(VCODEC_DRIVER_PATH)/.*.cmd
	rm -rf $(JPEG_DRIVER_PATH)/jdi/*.o $(JPEG_DRIVER_PATH)/jdi/.*.cmd
	rm -rf $(JPEG_DRIVER_PATH)/jdi/linux/*.o $(JPEG_DRIVER_PATH)/jdi/linux/.*.cmd
	rm -rf $(JPEG_DRIVER_PATH)/jpuapi/*.o $(JPEG_DRIVER_PATH)/jpuapi/.*.cmd
	rm -rf $(JPEG_DRIVER_PATH)/src/*.o $(JPEG_DRIVER_PATH)/src/.*.cmd
	rm -rf $(JPEG_DRIVER_PATH)/cvi_jpg_interface/src/*.o $(JPEG_DRIVER_PATH)/cvi_jpg_interface/src/.*.cmd
	rm -rf $(JPEG_DRIVER_PATH)/cvi_jpg_interface/test/.*.o.cmd

depend .depend dep:
	$(CC) $(CFLAGS)	-M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include	.depend
endif

endif
