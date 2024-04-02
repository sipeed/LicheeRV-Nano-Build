#include <mmio.h>
#include <reg_soc.h>
#include <ddr_sys.h>

void cvx16_dram_cap_check(uint8_t size)
{
	uartlog("%s\n", __func__);
	// ddr_debug_wr32(0x5A);
	// ddr_debug_num_write();
	KC_MSG("-5A- %s\n", __func__);

#ifdef ETRON_DDR2_512
	KC_MSG("ETRON_DDR2_512\n");

	if (size == 6) {
		KC_MSG("dram_cap_check = 512Mb (64MB)\n");
	} else {
		KC_MSG("dram_cap_check ERROR !!! size = %x\n", size);
	}
#elif ESMT_N25_DDR3_1G
	KC_MSG("ESMT_N25_DDR3_1G\n");

	if (size == 7) {
		KC_MSG("dram_cap_check = 1Gb (128MB)\n");
	} else {
		KC_MSG("dram_cap_check ERROR !!! size = %x\n", size);
	}
#elif ESMT_DDR3_2G
	KC_MSG("ESMT_DDR3_2G\n");

	if (size == 8) {
		KC_MSG("dram_cap_check = 2Gb (256MB)\n");
	} else {
		KC_MSG("dram_cap_check ERROR !!! size = %x\n", size);
	}
#elif ETRON_DDR3_1G
	KC_MSG("ETRON_DDR3_1G\n");

	if (size == 7) {
		KC_MSG("dram_cap_check = 1Gb (128MB)\n");
	} else {
		KC_MSG("dram_cap_check ERROR !!! size = %x\n", size);
	}
#elif DDR3_1G
	KC_MSG("DDR3_1G\n");

	if (size == 7) {
		KC_MSG("dram_cap_check = 1Gb (128MB)\n");
	} else {
		KC_MSG("dram_cap_check ERROR !!! size = %x\n", size);
	}
#elif DDR3_2G
	KC_MSG("DDR3_2G\n");

	if (size == 8) {
		KC_MSG("dram_cap_check = 2Gb (256MB)\n");
	} else {
		KC_MSG("dram_cap_check ERROR !!! size = %x\n", size);
	}
#elif DDR3_4G
	KC_MSG("DDR3_4G\n");

	if (size == 9) {
		KC_MSG("dram_cap_check = 4Gb (512MB)\n");
	} else {
		KC_MSG("dram_cap_check ERROR !!! size = %x\n", size);
	}
#elif DDR3_DBG
	KC_MSG("DDR3_DBG\n");

	if (size == 9) {
		KC_MSG("dram_cap_check = 4Gb (512MB)\n");
	} else {
		KC_MSG("dram_cap_check ERROR !!! size = %x\n", size);
	}
#elif DDR3_PINMUX
	KC_MSG("DDR3_6mil\n");

	if (size == 9) {
		KC_MSG("dram_cap_check = 4Gb (512MB)\n");
	} else {
		KC_MSG("dram_cap_check ERROR !!! size = %x\n", size);
	}
#elif DDR2_512
	KC_MSG("DDR2_512\n");

	if (size == 6) {
		KC_MSG("dram_cap_check = 512Mb (64MB)\n");
	} else {
		KC_MSG("dram_cap_check ERROR !!! size = %x\n", size);
	}
#elif N25_DDR2_512
	KC_MSG("N25_DDR2_512\n");

	if (size == 6) {
		KC_MSG("dram_cap_check = 512Mb (64MB)\n");
	} else {
		KC_MSG("dram_cap_check ERROR !!! size = %x\n", size);
	}
#elif DDR2_PINMUX
	KC_MSG("DDR2\n");

	if (size == 6) {
		KC_MSG("dram_cap_check = 512Mb (64MB)\n");
	} else {
		KC_MSG("dram_cap_check ERROR !!! size = %x\n", size);
	}
#else
	KC_MSG("no pinmux\n");
#endif
}