SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
PARAM_FILE:=../../Makefile.param
include $(PARAM_FILE)
endif


PREBUILT_LIB = lib_$(SDK_VER)

all :
	@rm -rf lib
	@ln -s $(PREBUILT_LIB) lib
	@cp -a include/* $(MW_INC)
	@cp -a lib/* $(MW_3RD_LIB)

clean :
	@rm -rf lib
	@rm -rf $(MW_INC)/cli.h
	@rm -f $(MW_3RD_LIB)/libcli*
