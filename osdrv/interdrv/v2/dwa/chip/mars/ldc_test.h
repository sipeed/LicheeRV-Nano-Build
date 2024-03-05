#ifndef _LDC_TEST_H_
#define _LDC_TEST_H_

extern uint8_t ldc_test_enabled;

void ldc_dump_register(void);
int32_t ldc_test_proc_init(void);
int32_t ldc_test_proc_deinit(void);

void ldc_test_irq_handler(uint32_t intr_raw_status);

#endif /* _LDC_TEST_H_ */
