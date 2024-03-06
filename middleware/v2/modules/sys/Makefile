SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
     PARAM_FILE:=../../Makefile.param
     include $(PARAM_FILE)
endif

SDIR = $(PWD)/src
SRCS := $(wildcard $(SDIR)/*.c)
ifneq ($(MULTI_PROCESS_SUPPORT), 1)
SRCS := $(filter-out $(SDIR)/cvi_rpc.c, $(SRCS))
endif
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

ifeq ($(SDK_VER), 32bit)
SRCS_ASM = $(wildcard $(SDIR)/*.S)
else ifeq ($(SDK_VER), 64bit)
SRCS_ASM = $(wildcard $(SDIR)/*.S)
else
SRCS_ASM =
endif

OBJS_ASM = $(SRCS_ASM:.S=.o)
DEPS_ASM = $(SRCS_ASM:.S=.d)

TARGET_A = $(MW_LIB)/libsys.a
TARGET_SO = $(MW_LIB)/libsys.so

INCS = -I$(MW_INC) -I$(KERNEL_INC) -I$(SYS_INC) -I$(VPU_INC)

EXTRA_CFLAGS = $(INCS) -fpack-struct=8
EXTRA_CFLAGS += -DMMF_VERSION=\"$(shell git describe --always)\"
EXTRA_CFLAGS += -DSDK_VER=\"$(SDK_VER)\"
ifeq ($(SDK_VER), 32bit)
EXTRA_CFLAGS += -D_FILE_OFFSET_BITS=64
endif

LDFLAGS  = -shared -fPIC -Wl,--gc-sections -export-dynamic -L$(MW_LIB) -L$(MW_3RD_LIB) --sysroot $(SYSROOT) $(CVI_TARGET_PACKAGES_LIBDIR)

.PHONY : clean all
all : $(TARGET_A) $(TARGET_SO)

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
	@$(CC) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $(TARGET_SO) -Wl,--start-group $(OBJS) $(OBJS_ASM) $(LIBS) -Wl,--end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $(TARGET_SO))

clean:
	@rm -f $(OBJS) $(OBJS_ASM) $(DEPS) $(DEPS_ASM) $(TARGET_A) $(TARGET_SO)

-include $(DEPS)
