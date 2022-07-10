// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  (C) Copyright 2010-2013
 *  NVIDIA Corporation <www.nvidia.com>
 *
 *  (C) Copyright 2022
 *  Svyatoslav Ryhel <clamor95@gmail.com>
 */

#include <common.h>
#include <dm.h>
#include <i2c.h>
#include <log.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/clock.h>
#include <asm/arch/gpio.h>
#include <asm/arch/tegra.h>
#include <asm/arch-tegra/clk_rst.h>
#include <asm/arch-tegra/pmc.h>
#include <asm/gpio.h>
#include <linux/delay.h>
#include "pinmux-config-x3.h"

#ifdef CONFIG_TEGRA_CRYPTO
#include <asm/arch-tegra/crypto.h>
#include "uboot_aes.h"
#endif

#define PMC_CLK_OUT_CNTRL_0		0x01A8

#define MAX77663_I2C_ADDR		0x1C	/* 7 bit */
#define MAX77663_REG_SD2		0x18
#define MAX77663_REG_LDO2		0x27
#define MAX77663_REG_LDO3		0x29
#define MAX77663_REG_LDO5		0x2D
#define MAX77663_REG_ONOFF_CFG1		0x41
#define MAX77663_ONOFF_PWR_OFF		0x02

#ifdef CONFIG_CMD_POWEROFF
int do_poweroff(struct cmd_tbl *cmdtp, int flag,
		 int argc, char *const argv[])
{
	struct udevice *dev;
	uchar data_buffer[1];
	int ret;

	ret = i2c_get_chip_for_busnum(0, MAX77663_I2C_ADDR, 1, &dev);
	if (ret) {
		printf("%s: Cannot find PMIC I2C chip\n", __func__);
		return 0;
	}

	ret = dm_i2c_read(dev, MAX77663_REG_ONOFF_CFG1, data_buffer, 1);
	if (ret)
		return ret;

	data_buffer[0] |= MAX77663_ONOFF_PWR_OFF;

	ret = dm_i2c_write(dev, MAX77663_REG_ONOFF_CFG1, data_buffer, 1);
	if (ret)
		return ret;

	/* wait some time and then print error */
	mdelay(5000);

	printf("Failed to power off!!!\n");
	return 1;
}
#endif

#ifdef CONFIG_TEGRA_CRYPTO
/*
 * Pass sbk for boards where it is common
 */
void get_secure_key(u8 *key)
{
	/* SBK is device/board specific */
#ifdef CONFIG_DEVICE_P880
	u8 sbk[AES128_KEY_LENGTH] = {
		0xc1, 0x82, 0x69, 0xee, 0x09, 0x00, 0xce, 0x58,
		0x48, 0x2a, 0xbe, 0x34, 0xbf, 0xf1, 0xbc, 0x01
	};
#endif
#ifdef CONFIG_DEVICE_P895
	u8 sbk[AES128_KEY_LENGTH] = {
		0x95, 0x08, 0x21, 0xad, 0x09, 0x64, 0xce, 0x58,
		0xfe, 0x98, 0xbe, 0x34, 0xbf, 0xf1, 0xac, 0x02
	};
#endif

	memcpy(key, sbk, AES128_KEY_LENGTH);
}
#endif

/*
 * Routine: pinmux_init
 * Description: Do individual peripheral pinmux configs
 */
void pinmux_init(void)
{
	pinmux_config_pingrp_table(tegra3_x3_pinmux_common,
		ARRAY_SIZE(tegra3_x3_pinmux_common));

#ifdef CONFIG_DEVICE_P880
	pinmux_config_pingrp_table(tegra3_p880_pinmux,
		ARRAY_SIZE(tegra3_p880_pinmux));
#endif

#ifdef CONFIG_DEVICE_P895
	pinmux_config_pingrp_table(tegra3_p895_pinmux,
		ARRAY_SIZE(tegra3_p895_pinmux));
#endif
}

#ifdef CONFIG_MMC_SDHCI_TEGRA
/*
 * Do I2C/PMU writes to bring up SD card and eMMC bus power
 */
void board_sdmmc_voltage_init(void)
{
	struct udevice *dev;
	uchar val;
	int ret;

	ret = i2c_get_chip_for_busnum(0, MAX77663_I2C_ADDR, 1, &dev);
	if (ret) {
		debug("%s: Cannot find PMIC I2C chip\n", __func__);
		return;
	}

	/* 0x60 for 1.8v, bit7:0 = voltage */
	val = 0x60;
	ret = dm_i2c_write(dev, MAX77663_REG_SD2, &val, 1);
	if (ret)
		printf("vdd_1v8_vio set failed: %d\n", ret);

	/* 0xF2 for 3.30v, enabled: bit7:6 = 11 = enable, bit5:0 = voltage */
	val = 0xF2;
	ret = dm_i2c_write(dev, MAX77663_REG_LDO2, &val, 1);
	if (ret)
		printf("avdd_usb set failed: %d\n", ret);

	/* 0xEC for 3.00v, enabled: bit7:6 = 11 = enable, bit5:0 = voltage */
	val = 0xEC;
	ret = dm_i2c_write(dev, MAX77663_REG_LDO3, &val, 1);
	if (ret)
		printf("vdd_usd set failed: %d\n", ret);

	/* 0xE9 for 2.85v, enabled: bit7:6 = 11 = enable, bit5:0 = voltage */
	val = 0xE9;
	ret = dm_i2c_write(dev, MAX77663_REG_LDO5, &val, 1);
	if (ret)
		printf("vcore_emmc set failed: %d\n", ret);
}

/*
 * Routine: pin_mux_mmc
 * Description: setup the MMC muxes, power rails, etc.
 */
void pin_mux_mmc(void)
{
	/*
	 * NOTE: We don't do mmc-specific pin muxes here.
	 * They were done globally in pinmux_init().
	 */

	/* Bring up uSD and eMMC power */
	board_sdmmc_voltage_init();
}
#endif	/* MMC */

int nvidia_board_init(void)
{
	u32 value;

	clock_start_periph_pll(PERIPH_ID_EXTPERIPH3, CLOCK_ID_PERIPH,
			       24 * 1000000);
	clock_enable(PERIPH_ID_EXTPERIPH3);
	udelay(5);
	reset_set_enable(PERIPH_ID_EXTPERIPH3, 0);

	/* Configure clk_out_3 */
	value = tegra_pmc_readl(PMC_CLK_OUT_CNTRL_0);
	tegra_pmc_writel(value | BIT(18) | BIT(22) | BIT(23),
			 PMC_CLK_OUT_CNTRL_0);

	return 0;
}
