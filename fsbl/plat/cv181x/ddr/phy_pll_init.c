#include <platform.h>
#include <ddr_sys.h>
#ifdef DDR2_3
#include <ddr3_1866_init.h>
#include <ddr2_1333_init.h>
#else
#include <ddr_init.h>
#endif

uint32_t  freq_in;
uint32_t  tar_freq;
uint32_t  mod_freq;
uint32_t  dev_freq;
uint64_t  reg_set;
uint64_t  reg_span;
uint64_t  reg_step;

void pll_init(void)
{
	freq_in = 752;
	mod_freq = 100;
	dev_freq = 15;
	NOTICE("Data rate=%d.\n", ddr_data_rate);
#ifdef SSC_EN
	tar_freq = (ddr_data_rate >> 4) * 0.985;
#else
	tar_freq = (ddr_data_rate >> 4);
#endif
	reg_set = (uint64_t)freq_in * 67108864 / tar_freq;
	reg_span = ((tar_freq * 250) / mod_freq);
	reg_step = reg_set * dev_freq / (reg_span * 1000);
	uartlog("ddr_data_rate = %d, freq_in = %d reg_set = %lx tar_freq = %x reg_span = %lx reg_step = %lx\n",
		ddr_data_rate, freq_in, reg_set, tar_freq, reg_span, reg_step);
	// uartlog("reg_set = %lx\n", reg_set);
	// uartlog("tar_freq = %x\n", tar_freq);
	// uartlog("reg_span = %lx\n", reg_span);
	// uartlog("reg_step = %lx\n", reg_step);

	cvx16_pll_init();
}
