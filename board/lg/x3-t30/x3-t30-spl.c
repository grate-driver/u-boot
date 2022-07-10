// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  T30 LGE X3 SPL stage configuration
 *
 *  (C) Copyright 2010-2013
 *  NVIDIA Corporation <www.nvidia.com>
 *
 *  (C) Copyright 2022
 *  Svyatoslav Ryhel <clamor95@gmail.com>
 */

#include <common.h>
#include <asm/arch-tegra/tegra_spl.h>
#include <linux/delay.h>

/* I2C addr is in 8 bit */
#define MAX77663_I2C_ADDR		0x38

#define MAX77663_REG_SD0		0x16
#define MAX77663_REG_SD0_DATA		(0x2100 | MAX77663_REG_SD0)
#define MAX77663_REG_SD1		0x17
#define MAX77663_REG_SD1_DATA		(0x3000 | MAX77663_REG_SD1)
#define MAX77663_REG_LDO4		0x2B
#define MAX77663_REG_LDO4_DATA		(0xE000 | MAX77663_REG_LDO4)

#define MAX77620_REG_GPIO1		0x37
#define MAX77620_REG_GPIO1_DATA		(0x0800 | MAX77620_REG_GPIO1)
#define MAX77620_REG_GPIO4		0x3A
#define MAX77620_REG_GPIO4_DATA		(0x0100 | MAX77620_REG_GPIO4)

void pmic_enable_cpu_vdd(void)
{
	/* Set VDD_CORE to 1.200V. */
	tegra_i2c_ll_write_addr(MAX77663_I2C_ADDR, 2);
	tegra_i2c_ll_write_data(MAX77663_REG_SD1_DATA, I2C_SEND_2_BYTES);

	udelay(1000);

	/* Bring up VDD_CPU to 1.0125V. */
	tegra_i2c_ll_write_addr(MAX77663_I2C_ADDR, 2);
	tegra_i2c_ll_write_data(MAX77663_REG_SD0_DATA, I2C_SEND_2_BYTES);
	udelay(1000);

	/* Bring up VDD_RTC to 1.200V. */
	tegra_i2c_ll_write_addr(MAX77663_I2C_ADDR, 2);
	tegra_i2c_ll_write_data(MAX77663_REG_LDO4_DATA, I2C_SEND_2_BYTES);
	udelay(10 * 1000);

	/* Set GPIO4 and GPIO1 states */
	tegra_i2c_ll_write_addr(MAX77663_I2C_ADDR, 2);
	tegra_i2c_ll_write_data(MAX77620_REG_GPIO4_DATA, I2C_SEND_2_BYTES);
	tegra_i2c_ll_write_data(MAX77620_REG_GPIO1_DATA, I2C_SEND_2_BYTES);
}
