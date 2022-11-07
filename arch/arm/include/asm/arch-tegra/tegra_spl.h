/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2022 Svyatoslav Ryhel <clamor95@gmail.com>
 * (C) Copyright 2010 - 2011 NVIDIA Corporation <www.nvidia.com>
 */

#ifndef _TEGRA_SPL_H_
#define _TEGRA_SPL_H_

#include <asm/io.h>
#include <asm/arch-tegra/tegra_i2c.h>

#define I2C_SEND_2_BYTES		0x0A02

/* Pre-dm way to work with i2c */
static inline void tegra_i2c_ll_write_addr(uint addr, uint config)
{
	struct i2c_ctlr *reg = (struct i2c_ctlr *)TEGRA_DVC_BASE;

	writel(addr, &reg->cmd_addr0);
	writel(config, &reg->cnfg);
}

static inline void tegra_i2c_ll_write_data(uint data, uint config)
{
	struct i2c_ctlr *reg = (struct i2c_ctlr *)TEGRA_DVC_BASE;

	writel(data, &reg->cmd_data1);
	writel(config, &reg->cnfg);
}

void pmic_enable_cpu_vdd(void);

#endif /* _TEGRA_SPL_H_ */
