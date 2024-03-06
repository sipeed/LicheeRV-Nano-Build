SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
     PARAM_FILE:=../../Makefile.param
     include $(PARAM_FILE)
endif

SDIR = $(PWD)/src
SRCS = $(wildcard $(SDIR)/*.c)
INCS = -I$(MW_INC) -I$(KERNEL_INC) -I$(SYS_INC) -I$(PWD)/include
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
TARGET_A = $(MW_LIB)/libvdec.a
TARGET_SO = $(MW_LIB)/libvdec.so

EXTRA_CFLAGS = $(INCS)

ifeq ($(DEBUG), 1)
EXTRA_CFLAGS += -g -O0
endif

.PHONY : clean all
all : prepare $(TARGET_A) $(TARGET_SO)

$(SDIR)/%.o: $(SDIR)/%.c
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(OBJS)
	@$(AR) $(ARFLAGS) $@ $(OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $@)

$(TARGET_SO): $(OBJS)
	@$(LD) $(LDFLAGS) $(LEXTRA_LDFLAGS) -o $@ --start-group $(OBJS) $(LIBS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $@)

clean:
	@rm -f $(OBJS) $(DEPS) $(TARGET_A) $(TARGET_SO)

prepare:
	@echo "SDK_VER = ${SDK_VER}"
	@head=`git rev-parse --verify --short HEAD`
	@date=`git show -s --format=%cd --date=short ${head}`

-include $(DEPS)
