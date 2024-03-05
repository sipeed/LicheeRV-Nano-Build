SHELL=/bin/bash
-include $(BUILD_PATH)/.config
#
export CHIP_CODE := $(shell echo $(CHIP_CODE) | tr A-Z a-z)
#
INTERDRV_PATH := interdrv/$(shell echo $(MW_VER))

ifeq ($(KERNEL_DIR), )
$(info Please set KERNEL_DIR global variable!!)
endif

ifeq ($(INSTALL_DIR), )
INSTALL_DIR = ko
endif
CUR_DIR = $(PWD)

$(info ** [ KERNEL_DIR ] ** = $(KERNEL_DIR))
$(info ** [ INSTALL_DIR ] ** = $(INSTALL_DIR))

export INTRERDRV_FLAGS :=
ifeq ($(CONFIG_BUILD_FOR_DEBUG), y)
INTRERDRV_FLAGS += -DDRV_DEBUG -DDRV_TEST
endif

define COPY_KO
	( cd $(1) && cp -f *.ko $(INSTALL_DIR); )
endef

define MAKE_KO
	( cd $(1) && $(MAKE) KERNEL_DIR=$(KERNEL_DIR) all -j$(shell nproc))
	$(call COPY_KO, $(1))
endef

MAKE_EXT_KO_CP :=
ifneq (${FLASH_SIZE_SHRINK},y)
define MAKE_EXT_KO_CP
	find $(1) -name '*.ko' -print -exec cp {} $(INSTALL_DIR)/3rd/ \;;
endef
endif

define MAKE_EXT_KO
	( cd $(1) && $(MAKE) KERNEL_DIR=$(KERNEL_DIR) all -j$(shell nproc))
	$(call MAKE_EXT_KO_CP, $(1))
endef

SUBDIRS = $(shell find ./interdrv -maxdepth 2 -mindepth 2 -type d | grep -v "git")
SUBDIRS += $(shell find ./extdrv -maxdepth 1 -mindepth 1 -type d | grep -v "git")
exclude_dirs = ./interdrv/v1/include ./interdrv/v2/include
SUBDIRS := $(filter-out $(exclude_dirs), $(SUBDIRS))

# prepare ko list
KO_LIST = base vcodec jpeg pwm rtc wdt tpu mon clock_cooling saradc wiegand wiegand-gpio

ifneq ($(CONFIG_USB_OSDRV_CVITEK_GADGET),)
KO_LIST += usb
endif

ifeq ($(CHIP_ARCH), $(filter $(CHIP_ARCH), CV183X CV182X))
	KO_LIST += vip
	KO_LIST += spacc
	FB_DEP = vip
endif

ifeq ($(CHIP_ARCH), $(filter $(CHIP_ARCH), CV181X))
	KO_LIST += sys vi snsr_i2c cif vpss dwa rgn vo rtos_cmdqu fast_image cvi_vc_drv ive
	BASE_DEP = sys
	FB_DEP = vpss
else ifeq ($(CHIP_ARCH), $(filter $(CHIP_ARCH), CV180X))
	KO_LIST += sys vi snsr_i2c cif vpss dwa rgn rtos_cmdqu fast_image cvi_vc_drv
	BASE_DEP = sys
	FB_DEP = vpss
else ifeq ($(CHIP_ARCH), $(filter $(CHIP_ARCH), SG200X))
	KO_LIST += sys vi snsr_i2c cif vpss dwa rgn vo rtos_cmdqu fast_image cvi_vc_drv ive
	BASE_DEP = sys
	FB_DEP = vpss
endif

ifeq (, ${CONFIG_NO_FB})
	KO_LIST += fb
endif

$(info ** [ KO_LIST ] ** = $(KO_LIST))

OTHERS :=

ifeq (y, ${CONFIG_CP_EXT_WIRELESS})
KO_LIST += wireless
OTHERS += cp_ext_wireless
endif

ifeq (, ${CONFIG_NO_TP})
	KO_LIST += tp
	OTHERS += cp_ext_tp
endif

export CROSS_COMPILE=$(patsubst "%",%,$(CONFIG_CROSS_COMPILE_KERNEL))
export ARCH=$(patsubst "%",%,$(CONFIG_ARCH))

.PHONY : prepare clean all
all: prepare $(KO_LIST) $(OTHERS)

prepare:
	@mkdir -p $(INSTALL_DIR)/3rd

# osdrv/interdrv
fb: base $(FB_DEP)
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

base: $(BASE_DEP)
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

audio:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

vcodec:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

usb:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

fast_image: rtos_cmdqu
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

jpeg:
	@$(call COPY_KO, ${INTERDRV_PATH}/${@}/${CHIP_CODE}_${ARCH})

pwm:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

wdt:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

spacc:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

rtc:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

vip: base
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

tpu:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

mon:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

clock_cooling:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

saradc:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

wiegand:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

vi: base
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

snsr_i2c: base
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

cif: base
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

sys:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

vpss: base
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

dwa: base
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

rgn: base vpss
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

vo: base
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

ive:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

cvi_vc_drv:
	@$(call COPY_KO, ${INTERDRV_PATH}/${@}/${CHIP_CODE}_${ARCH})

rtos_cmdqu:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

# osdrv/extdrv
tp:
	@$(call MAKE_EXT_KO, extdrv/${@})

wireless:
	@$(call MAKE_EXT_KO, extdrv/${@})

wiegand-gpio:
	@$(call MAKE_EXT_KO, extdrv/${@})

gyro_i2c:
	@$(call MAKE_EXT_KO, extdrv/${@})

cp_ext_wireless:
	@find extdrv/wireless -name '*.ko' -print -exec cp {} $(INSTALL_DIR)/3rd/ \;

cp_ext_tp:
	@find extdrv/tp -name '*.ko' -print -exec cp {} $(INSTALL_DIR)/3rd/ \;

clean:
	@for subdir in $(SUBDIRS); do cd $$subdir && $(MAKE) clean && cd $(CUR_DIR); done
	@rm -f  $(INSTALL_DIR)/*.ko
	@rm -f  $(INSTALL_DIR)/3rd/*.ko
