SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
     PARAM_FILE:=../../Makefile.param
     include $(PARAM_FILE)
endif

SDIR = src
SRCS = $(wildcard $(SDIR)/*.c)
INCS = -I$(MW_INC) -I$(KERNEL_INC) -I$(SYS_INC) -Iinclude
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
TARGET_A = $(MW_LIB)/libvenc.a
TARGET_SO = $(MW_LIB)/libvenc.so
ifeq ($(MULTI_PROCESS_SUPPORT), 1)
DEFS += -DRPC_MULTI_PROCESS
endif

EXTRA_CFLAGS = $(INCS) $(DEFS)

ifeq ($(DEBUG), 1)
EXTRA_CFLAGS += -g -O0
endif

ldflags-y = -lsys


ifeq ($(CLI_DEBUG_SUPPORT), 1)
DEFS += -DCLI_DEBUG_SUPPORT
ldflags-y += -lcli
LIB_A_FLAGS += $(MW_3RD_LIB)/libcli.a
LIB_SO_FLAGS += $(MW_3RD_LIB)/libcli.so
endif

TMP_INC = $(MW_INC)/cvi_jpg* $(MW_INC)/cvi_h265*

.PHONY : clean all
all : prepare $(TARGET_A) $(TARGET_SO)

$(SDIR)/%.o: $(SDIR)/%.c
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(OBJS) $(LIB_A_FLAGS)
	@$(AR) $(ARFLAGS) $@ $(OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $@)

$(TARGET_SO): $(OBJS) $(LIB_SO_FLAGS)
	@$(LD) $(LDFLAGS) $(ldflags-y) -o $@ --start-group $(OBJS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $@)

clean:
	@rm -f $(OBJS) $(DEPS) $(TARGET_A) $(TARGET_SO)

prepare:
	@echo "SDK_VER = ${SDK_VER}"
	@head=`git rev-parse --verify --short HEAD`
	@date=`git show -s --format=%cd --date=short ${head}`

-include $(DEPS)
