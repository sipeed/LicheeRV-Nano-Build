SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../Makefile.param
	include $(PARAM_FILE)
endif
include ../sample.mk

SDIR = $(PWD)
SRCS = $(wildcard $(SDIR)/*.c)
INCS = -I$(MW_INC) -I$(ISP_INC) -I../common/ -I$(KERNEL_INC) -I$(MW_INC)/linux -I$(SDIR)

SRCS_CPP += $(wildcard $(SDIR)/media_server-1.0.1/*.cpp) \
$(wildcard $(SDIR)/media_server-1.0.1/media/*.cpp)

CFLAGS += -I$(SDIR)/media_server-1.0.1
CPPFLAGS += -I$(SDIR)/media_server-1.0.1/include \
-I$(SDIR)/media_server-1.0.1/librtsp/source/server \
-I$(SDIR)/media_server-1.0.1/sdk/include \
-I$(SDIR)/media_server-1.0.1/sdk/libhttp/include \
-I$(SDIR)/media_server-1.0.1/librtp/include \
-I$(SDIR)/media_server-1.0.1/libmpeg/include \
-I$(SDIR)/media_server-1.0.1/libflv/include \
-I$(SDIR)/media_server-1.0.1/libmkv/include \
-I$(SDIR)/media_server-1.0.1 \
-I$(SDIR)/media_server-1.0.1/media \
-I$(SDIR)/media_server-1.0.1/avcodec/avcodec/include \
-I$(SDIR)/media_server-1.0.1/avcodec/avbsf/include \
-I$(SDIR)/media_server-1.0.1/libmov/include \
-D__ERROR__=00*10000000+__LINE__*1000


OBJS = $(SRCS:.c=.o)
OBJS += $(SRCS_CPP:.cpp=.o)
DEPS = $(SRCS:.c=.d)

TARGET = test_mmf
ifeq ($(CONFIG_ENABLE_SDK_ASAN), y)
TARGET = test_mmf_asan
endif

PKG_CONFIG_PATH = $(MW_PATH)/pkgconfig
REQUIRES = cvi_common cvi_sample cvi_ive

MW_LIBS = $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs --define-variable=mw_dir=$(MW_PATH) $(REQUIRES))

LIBS = $(MW_LIBS)
ifeq ($(MULTI_PROCESS_SUPPORT), 1)
DEFS += -DRPC_MULTI_PROCESS
LIBS += -lnanomsg
endif

EXTRA_CFLAGS = $(INCS) $(DEFS)
EXTRA_LDFLAGS = $(LIBS) -lpthread -lm -lini

LIBS += -L$(SDIR)/media_server-1.0.1/libs -L. -lrtsp -lhttp -lflv -lmov -lrtp -lmpeg -lsdk -lavcodec -lavbsf

# IVE_SUPPORT = 1
ifeq ($(IVE_SUPPORT), 1)
CFLAGS += -DIVE_SUPPORT

IVE_PATH = $(MW_PATH)/../install/soc_cv1835_wevb_0002a_spinand/tpu_64/cvitek_ive_sdk
EXTRA_CFLAGS += -I$(IVE_PATH)/include/ive
EXTRA_LDFLAGS += -L$(IVE_PATH)/lib -lcvi_ive_tpu-static

TPU_PATH = $(MW_PATH)/../install/soc_cv1835_wevb_0002a_spinand/tpu_64/cvitek_tpu_sdk
EXTRA_CFLAGS += -I$(TPU_PATH)/include
EXTRA_LDFLAGS += -L$(TPU_PATH)/lib -lcviruntime-static -lcvimath-static -lcvikernel-static -lcnpy -lglog -lz
endif

.PHONY : clean all
all: $(TARGET)

$(COMMON_DIR)/%.o: $(COMMON_DIR)/%.c
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(SDIR)/%.o: $(SDIR)/%.c
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET): $(COMM_OBJ) $(OBJS) $(ISP_OBJ) $(MW_LIB)/libvpu.a $(MW_LIB)/libsys.a
	@$(CXX) -o $@ -Wl,--start-group $(OBJS) $(COMM_OBJS) -lsys $(MW_LIB)/libsys.a -Wl,--end-group -lmaix_mmf $(ELFFLAGS) $(EXTRA_LDFLAGS)
	@echo -e $(BLUE)[LINK]$(END)[$(notdir $(CXX))] $(notdir $@)

clean:
	@rm -f $(OBJS) $(DEPS) $(COMM_OBJ) $(COMM_DEPS) $(TARGET)

-include $(DEPS)
