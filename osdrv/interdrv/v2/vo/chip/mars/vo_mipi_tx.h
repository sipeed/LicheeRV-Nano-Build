#ifndef _VO_MIPI_TX_H_
#define _VO_MIPI_TX_H_

#include <linux/clk.h>
#include "linux/vo_mipi_tx.h"

extern bool __clk_is_enabled(struct clk *clk);
int mipi_tx_get_combo_dev_cfg(struct combo_dev_cfg_s *dev_cfg);

#endif // _VO_MIPI_TX_H_
