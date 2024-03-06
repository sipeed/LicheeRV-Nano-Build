ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../$(shell echo $(MW_VER))/Makefile.param
	include $(PARAM_FILE)
endif
