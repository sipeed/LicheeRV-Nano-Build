SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
     PARAM_FILE:=../../Makefile.param
     include $(PARAM_FILE)
endif

SDIR = $(PWD)/src
SRCS := $(wildcard $(SDIR)/*.c)

# 3rdparty lib
OSS_MODULE_INSTALL_PATH = $(OUTPUT_DIR)/mw_$(SDK_VER)/cvitek_mw_sdk
OSS_MODULE_INCLUDE_PATH = $(OSS_MODULE_INSTALL_PATH)/include
OSS_MODULE_LIB_PATH = $(OSS_MODULE_INSTALL_PATH)/lib

ISP_BIN_DIR = $(MW_PATH)/modules/isp/$(shell echo $(CHIP_ARCH) | tr A-Z a-z)/isp_bin/inc

SRCS_ASM = $(wildcard $(SDIR)/*.S)
INCS = -I$(MW_INC) -I$(KERNEL_INC) -I$(SYS_INC) -I$(VPU_INC) -I$(ISP_INC) -Iinclude
INCS += -I$(SYS_INC) -I$(OSS_MODULE_INCLUDE_PATH)/cvi_json-c -I$(ISP_BIN_DIR) -I$(OSS_MODULE_INCLUDE_PATH)/cvi_miniz
OBJS = $(SRCS:.c=.o)
OBJS_ASM = $(SRCS_ASM:.S=.o)
DEPS = $(SRCS:.c=.d)

TARGET_A = $(MW_LIB)/libcvi_bin.a
TARGET_SO = $(MW_LIB)/libcvi_bin.so

EXTRA_CFLAGS = $(INCS) -fpack-struct=8
EXTRA_CFLAGS += -DMMF_VERSION=\"$(shell git describe --always)\"
EXTRA_CFLAGS += -DSDK_VER=\"$(SDK_VER)\"
EXTRA_CFLAGS += -DISP_COMMIT=\"$(shell git rev-parse --short HEAD)\"
EXTRA_CFLAGS += -DISP_GERRIT=\"$(shell git log -n1 | grep Reviewed-on | tail -n1 | sed "s/.*\///g")\"
ifeq ($(MULTI_PROCESS_SUPPORT), 1)
EXTRA_CFLAGS += -DRPC_MULTI_PROCESS
endif

.PHONY : clean all prepare_3rdparty_module
all : prepare_3rdparty_module $(TARGET_A) $(TARGET_SO)

OSS_BUILD_SRCIPT = $(TOP_DIR)/oss/run_build.sh
OSS_TARBALL_PATH = $(OUTPUT_DIR)/tpu_$(SDK_VER)/third_party
OSS_TARBALL_CVI_JSONC_PATH = $(OSS_TARBALL_PATH)/cvi-json-c.tar.gz
OSS_TARBALL_CVI_MINIZ_PATH = $(OSS_TARBALL_PATH)/cvi-miniz.tar.gz
OSS_MODULE_LIB_CVI_JSONC_CHECK_PATH = $(OSS_MODULE_LIB_PATH)/libcvi_json-c*
OSS_MODULE_LIB_CVI_MINIZ_CHECK_PATH = $(OSS_MODULE_LIB_PATH)/libcvi_miniz*

prepare_3rdparty_module:
# cvi_json-c
ifeq "$(wildcard $(OSS_MODULE_LIB_CVI_JSONC_CHECK_PATH))" ""
ifeq "$(wildcard $(OSS_TARBALL_CVI_JSONC_PATH))" ""
	$(error libcvi_json-c source tarball not exist, please run build_3rd_party first!!)
endif
	@mkdir -p $(OSS_MODULE_INSTALL_PATH)
	@$(OSS_BUILD_SRCIPT) -n cvi-json-c -e -t "$(OSS_TARBALL_PATH)" -i "$(OSS_MODULE_INSTALL_PATH)"
endif

# cvi_miniz
ifeq "$(wildcard $(OSS_MODULE_LIB_CVI_MINIZ_CHECK_PATH))" ""
ifeq "$(wildcard $(OSS_TARBALL_CVI_MINIZ_PATH))" ""
	$(error libcvi_miniz source tarball not exist, please run build_3rd_party first!!)
endif
	@mkdir -p $(OSS_MODULE_INSTALL_PATH)
	@$(OSS_BUILD_SRCIPT) -n cvi-miniz -e -t "$(OSS_TARBALL_PATH)" -i "$(OSS_MODULE_INSTALL_PATH)"
endif

$(SDIR)/%.o: $(SDIR)/%.c
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(SDIR)/%.o: $(SDIR)/%.S
	$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(OBJS) $(OBJS_ASM)
	@$(AR) $(ARFLAGS) $(TARGET_A) $(OBJS) $(OBJS_ASM)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $(TARGET_A))
	@echo "$$AR_MRI" | $(AR) -M

$(TARGET_SO): $(OBJS) $(OBJS_ASM)
	@$(LD) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $(TARGET_SO) --start-group $(OBJS) $(OBJS_ASM) $(LIBS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $(TARGET_SO))
	@$(STRIP) $(TARGET_SO)

clean:
	@rm -f $(OBJS) $(OBJS_ASM) $(DEPS) $(TARGET_A) $(TARGET_SO)
	@rm -rf $(OSS_MODULE_INSTALL_PATH)

-include $(DEPS)
