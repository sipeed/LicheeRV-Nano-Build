ROOT:=../../sdk
CONTRIB:=../../3rd

__ENABLE_X264__ ?= 0
__ENABLE_X262__ ?= 0
__ENABLE_OPENH264__ ?= 0
__ENABLE_FAAC__ ?= 0
__ENABLE_OPUS__ ?= 0
__ENABLE_MP2__ ?= 0
__ENABLE_FREETYPE__ ?= 0
__ENABLE_RESAMPLER__ ?= 0

#--------------------------------Output------------------------------
# OUTTYPE: 0-exe, 1-dll, 2-static
#--------------------------------------------------------------------
OUTTYPE = 2
OUTFILE = libavcodec.a

#-------------------------------Include------------------------------
#
# INCLUDES = $(addprefix -I,$(INCLUDES)) # add -I prefix
#--------------------------------------------------------------------
INCLUDES = . \
					./include \
					$(ROOT)/include \
					$(CONTRIB) \
					$(CONTRIB)/opus \
					$(CONTRIB)/faac \
					$(CONTRIB)/x264 \
					$(CONTRIB)/openh264 \
					$(CONTRIB)/freetype/include \
					$(CONTRIB)/twolame \
					$(CONTRIB)/soxr

#-------------------------------Source-------------------------------
#
#--------------------------------------------------------------------
SOURCE_PATHS = source
SOURCE_FILES = $(foreach dir,$(SOURCE_PATHS),$(wildcard $(dir)/*.cpp))
SOURCE_FILES += $(foreach dir,$(SOURCE_PATHS),$(wildcard $(dir)/*.c))

_SOURCE_FILES = 

#-----------------------------Library--------------------------------
#
# LIBPATHS = $(addprefix -L,$(LIBPATHS)) # add -L prefix
#--------------------------------------------------------------------
LIBPATHS = $(CONTRIB)/openssl/$(PLATFORM) $(CONTRIB)/soxr/$(PLATFORM)
ifdef RELEASE
# relase library path
LIBPATHS += 
else
LIBPATHS +=
endif

LIBS =
STATIC_LIBS =

#-----------------------------DEFINES--------------------------------
#
# DEFINES := $(addprefix -D,$(DEFINES)) # add -L prefix
#--------------------------------------------------------------------
DEFINES = NO_CRYPTO

#-----------------------------CUSTOM--------------------------------
#
#--------------------------------------------------------------------
ifeq ($(__ENABLE_X264__),1)
STATIC_LIBS += $(CONTRIB)/x264/$(PLATFORM)/libx264.a
DEFINES += _AVCODEC_X264_
else
_SOURCE_FILES += source/x264enc.c
endif

ifeq ($(__ENABLE_X262__),1)
STATIC_LIBS += $(CONTRIB)/x264/$(PLATFORM)/libx264.a
DEFINES += _AVCODEC_X262_
else
_SOURCE_FILES += source/x262enc.c
endif

ifeq ($(__ENABLE_OPENH264__),1)
STATIC_LIBS += $(CONTRIB)/openh264/$(PLATFORM)/libopenh264.a
DEFINES += _AVCODEC_OPENH264_
else
_SOURCE_FILES += source/openh264enc.cpp
endif

ifeq ($(__ENABLE_FAAC__),1)
STATIC_LIBS += $(CONTRIB)/faac/$(PLATFORM)/libfaac.a
STATIC_LIBS += $(CONTRIB)/faac/$(PLATFORM)/libfaad2.a
DEFINES += _AVCODEC_FAAC_
else
_SOURCE_FILES += source/faac-decoder.c
_SOURCE_FILES += source/faac-encoder.c
endif

ifeq ($(__ENABLE_OPUS__),1)
STATIC_LIBS += $(CONTRIB)/opus/$(PLATFORM)/libopus.a
DEFINES += _AVCODEC_OPUS_
else
_SOURCE_FILES += source/opus-decoder.c
_SOURCE_FILES += source/opus-encoder.c
endif

ifeq ($(__ENABLE_MP2__),1)
STATIC_LIBS += $(CONTRIB)/twolame/$(PLATFORM)/libtwolame.a
DEFINES += _AVCODEC_TWOLAME_
else
_SOURCE_FILES += source/mp2lame-encoder.c
endif

ifeq ($(__ENABLE_FREETYPE__),1)
LIBS += freetype
DEFINES += _AVCODEC_FREETYPE_
else
_SOURCE_FILES += source/text-render-freetype.c
endif

ifeq ($(__ENABLE_RESAMPLER__),1)
LIBS += soxr
DEFINES += _AVCODEC_SOXR_
else
_SOURCE_FILES += source/audio-resampler.c
endif

#-----------------------------COMMON--------------------------------
#
#--------------------------------------------------------------------
SOURCE_FILES := $(filter-out $(_SOURCE_FILES),$(SOURCE_FILES))

include $(ROOT)/gcc.mk

GCC_VER_GTE44 := $(shell echo `gcc -dumpversion | cut -f1-2 -d.` \< 4.4 | bc )
ifeq ($(GCC_VER_GTE44),1)
CFLAGS += -march=i586
endif
