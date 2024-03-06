.PHONY: clean all
.SILENT: clean all

include audio.mk

all:
	cp lib/$(SDK_VER)/lib* $(MW_LIB)

clean:
	rm $(MW_LIB)/libaac*
	rm $(MW_LIB)/libcvi_audio*
	rm $(MW_LIB)/libcvi_RES1*
	rm $(MW_LIB)/libcvi_ssp*
	rm $(MW_LIB)/libcvi_VoiceEngine*
	rm $(MW_LIB)/libcvi_vqe*
	rm $(MW_LIB)/libdnvqe*
	rm $(MW_LIB)/libtinyalsa*
