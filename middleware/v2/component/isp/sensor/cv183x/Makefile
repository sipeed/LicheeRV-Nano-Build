SHELL = /bin/bash

ifeq ($(PARAM_FILE), )
PARAM_FILE=../../../../Makefile.param
include $(PARAM_FILE)
endif
include ../../sensor.mk

define MAKE_SENSOR
	pushd $(1) && \
	$(MAKE) all && \
	popd
endef

.PHONY : prepare clean $(sensor-y)
all: prepare $(sensor-y) all_sensor

prepare:
	@echo "#################################################"
	@echo "#"
	@echo "# Compiling 'component libs' Configs as below..."
	@echo "# SENSOR_LIST=$(sensor-y)"
	@echo "#"
	@echo "#################################################"

gcore_gc2053:
	$(call MAKE_SENSOR, ${@})

gcore_gc2053_slave:
	$(call MAKE_SENSOR, ${@})

gcore_gc2053_1L:
	$(call MAKE_SENSOR, ${@})

gcore_gc2093:
	$(call MAKE_SENSOR, ${@})

gcore_gc2093_slave:
	$(call MAKE_SENSOR, ${@})

gcore_gc4653:
	$(call MAKE_SENSOR, ${@})

gcore_gc4653_slave:
	$(call MAKE_SENSOR, ${@})

nextchip_n5:
	$(call MAKE_SENSOR, ${@})

ov_os02d10:
	$(call MAKE_SENSOR, ${@})

ov_os02d10_slave:
	$(call MAKE_SENSOR, ${@})

ov_os02k10_slave:
	$(call MAKE_SENSOR, ${@})

ov_os04c10:
	$(call MAKE_SENSOR, ${@})

ov_os04c10_slave:
	$(call MAKE_SENSOR, ${@})

ov_os08a20:
	$(call MAKE_SENSOR, ${@})

ov_os08a20_slave:
	$(call MAKE_SENSOR, ${@})

pico_384:
	$(call MAKE_SENSOR, ${@})

pico_640:
	$(call MAKE_SENSOR, ${@})

pixelplus_pr2020:
	$(call MAKE_SENSOR, ${@})

pixelplus_pr2100:
	$(call MAKE_SENSOR, ${@})

sms_sc035hgs:
	$(call MAKE_SENSOR, ${@})

sms_sc200ai:
	$(call MAKE_SENSOR, ${@})

sms_sc401ai:
	$(call MAKE_SENSOR, ${@})

sms_sc850sl:
	$(call MAKE_SENSOR, ${@})

sms_sc3335:
	$(call MAKE_SENSOR, ${@})

sms_sc3335_slave:
	$(call MAKE_SENSOR, ${@})

sms_sc3336:
	$(call MAKE_SENSOR, ${@})

sms_sc4210:
	$(call MAKE_SENSOR, ${@})

sms_sc8238:
	$(call MAKE_SENSOR, ${@})

soi_f23:
	$(call MAKE_SENSOR, ${@})

soi_f35:
	$(call MAKE_SENSOR, ${@})

soi_f35_slave:
	$(call MAKE_SENSOR, ${@})

soi_h65:
	$(call MAKE_SENSOR, ${@})

sony_imx290_2L:
	$(call MAKE_SENSOR, ${@})

sony_imx307:
	$(call MAKE_SENSOR, ${@})

sony_imx307_slave:
	$(call MAKE_SENSOR, ${@})

sony_imx307_2L:
	$(call MAKE_SENSOR, ${@})

sony_imx307_sublvds:
	$(call MAKE_SENSOR, ${@})

sony_imx327:
	$(call MAKE_SENSOR, ${@})

sony_imx327_slave:
	$(call MAKE_SENSOR, ${@})

sony_imx327_2L:
	$(call MAKE_SENSOR, ${@})

sony_imx327_sublvds:
	$(call MAKE_SENSOR, ${@})

sony_imx334:
	$(call MAKE_SENSOR, ${@})

sony_imx335:
	$(call MAKE_SENSOR, ${@})

sony_imx347:
	$(call MAKE_SENSOR, ${@})

sony_imx385:
	$(call MAKE_SENSOR, ${@})

techpoint_tp2850:
	$(call MAKE_SENSOR, ${@})

vivo_mcs369:
	$(call MAKE_SENSOR, ${@})

vivo_mcs369q:
	$(call MAKE_SENSOR, ${@})

vivo_mm308m2:
	$(call MAKE_SENSOR, ${@})

lontium_lt6911:
	$(call MAKE_SENSOR, ${@})

all_sensor:
	@$(MAKE) -f Makefile_full || exit 1;

clean:
	@for x in `find ./ -maxdepth 2 -mindepth 2 -name "Makefile" `; \
	    do cd `dirname $$x`; if [ $$? ]; then $(MAKE) clean; cd -; fi; done
	@echo "#"
	@$(MAKE) clean -f Makefile_full || exit 1;
