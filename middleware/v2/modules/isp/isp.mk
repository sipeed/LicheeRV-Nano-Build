ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../$(shell echo $(MW_VER))/Makefile.param
	include $(PARAM_FILE)
endif

isp_chip_dir := $(shell echo $(CHIP_ARCH) | tr A-Z a-z)


