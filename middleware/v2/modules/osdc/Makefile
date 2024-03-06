SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
     PARAM_FILE:=../../Makefile.param
     include $(PARAM_FILE)
endif

SDIR = $(PWD)/src
CSRCS := $(wildcard $(SDIR)/*.c)
COBJS = $(CSRCS:.c=.o)
CDEPS = $(CSRCS:.c=.d)
CXXSRCS := $(wildcard $(SDIR)/*.cpp)
CXXOBJS = $(CXXSRCS:.cpp=.o)
CXXDEPS = $(CXXSRCS:.cpp=.d)

TARGET_A = $(MW_LIB)/libosdc.a
TARGET_SO = $(MW_LIB)/libosdc.so

INCS = -I$(MW_INC) -I$(KERNEL_INC) -I$(OSDC_INC)

EXTRA_CFLAGS = $(INCS) -fpack-struct=8
EXTRA_CFLAGS += -DMMF_VERSION=\"$(shell git describe --always)\"
EXTRA_CFLAGS += -DSDK_VER=\"$(SDK_VER)\"
ifeq ($(SDK_VER), 32bit)
EXTRA_CFLAGS += -D_FILE_OFFSET_BITS=64

endif
EXTRA_CXXFLAGS = $(INCS) -fpack-struct=8
EXTRA_CXXFLAGS += -DMMF_VERSION=\"$(shell git describe --always)\"
EXTRA_CXXFLAGS += -DSDK_VER=\"$(SDK_VER)\"
ifeq ($(SDK_VER), 32bit)
EXTRA_CXXFLAGS += -D_FILE_OFFSET_BITS=64
endif

.PHONY : clean all
all : prepare $(TARGET_A) $(TARGET_SO)

prepare:
	cp include/cmpr_canvas.h $(MW_INC)
	cp include/cvi_osdc.h $(MW_INC)
	cp include/osd_cmpr.h $(MW_INC)

$(SDIR)/%.o: $(SDIR)/%.c
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(SDIR)/%.o: $(SDIR)/%.cpp
	@$(CXX) $(DEPFLAGS) $(CXXFLAGS) $(EXTRA_CXXFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(COBJS) $(CXXOBJS)
	@$(AR) $(ARFLAGS) $(TARGET_A) $(COBJS) $(CXXOBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $(TARGET_A))
	@echo "$$AR_MRI" | $(AR) -M

$(TARGET_SO): $(OBJS) $(CXXOBJS)
	@$(LD) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $(TARGET_SO) --start-group $(COBJS) $(CXXOBJS) $(LIBS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $(TARGET_SO))

clean:
	@rm -f $(COBJS) $(CXXOBJS) $(CDEPS) $(CXXDEPS) $(TARGET_A) $(TARGET_SO)
	@rm -f $(MW_INC)/cmpr_canvas.h
	@rm -f $(MW_INC)/cvi_osdc.h
	@rm -f $(MW_INC)/osd_cmpr.h

-include $(DEPS)
