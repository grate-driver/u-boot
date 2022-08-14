// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2022 Svyatoslav Ryhel <clamor95@gmail.com>
 *
 * Panel specific configuration of bridge and panel itself.
 */

#include <common.h>
#include <backlight.h>
#include <dm.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <log.h>
#include <misc.h>
#include <mipi_display.h>
#include <panel.h>
#include <power/regulator.h>
#include <asm/gpio.h>

#include "ssd2825.h"

struct renesas_r69328_priv {
	struct udevice *backlight;
	struct udevice *spi;

	struct gpio_desc enable_gpio;
	struct gpio_desc reset_gpio;
};

#define MIPI_DCS_MCAP_ON		0x03B0
#define MIPI_DCS_MCAP_OFF		0x04B0

static const u8 power_set[] = {
	0xD1, 0x14, 0x1D, 0x21, 0x67,
	0x11, 0x9A
};

static const u8 gamma_setting_a[] = {
	0xC8, 0x00, 0x1A, 0x20, 0x28,
	0x25, 0x24, 0x26, 0x15, 0x13,
	0x11, 0x18, 0x1E, 0x1C, 0x00,
	0x00, 0x1A, 0x20, 0x28, 0x25,
	0x24, 0x26, 0x15, 0x13, 0x11,
	0x18, 0x1E, 0x1C, 0x00
};

static const u8 gamma_setting_b[] = {
	0xC9, 0x00, 0x1A, 0x20, 0x28,
	0x25, 0x24, 0x26, 0x15, 0x13,
	0x11, 0x18, 0x1E, 0x1C, 0x00,
	0x00, 0x1A, 0x20, 0x28, 0x25,
	0x24, 0x26, 0x15, 0x13, 0x11,
	0x18, 0x1E, 0x1C, 0x00
};

static const u8 gamma_setting_c[] = {
	0xCA, 0x00, 0x1A, 0x20, 0x28,
	0x25, 0x24, 0x26, 0x15, 0x13,
	0x11, 0x18, 0x1E, 0x1C, 0x00,
	0x00, 0x1A, 0x20, 0x28, 0x25,
	0x24, 0x26, 0x15, 0x13, 0x11,
	0x18, 0x1E, 0x1C, 0x00
};

static void write_hw_register(struct renesas_r69328_priv *priv, u8 reg,
			      u16 command)
{
	misc_write(priv->spi, reg, &command, SSD2825_CMD_SEND | SSD2825_DAT_SEND);
}

static void write_hw_dsi(struct renesas_r69328_priv *priv, const u8 *command,
			      int len)
{
	int i;

	misc_write(priv->spi, SSD2825_PACKET_SIZE_CTRL_REG_1, &len,
			SSD2825_CMD_SEND | SSD2825_DAT_SEND);

	misc_write(priv->spi, SSD2825_PACKET_DROP_REG, NULL,
			SSD2825_CMD_SEND);

	for (i = 0; i < len; i++) {
		misc_write(priv->spi, 0, &command[i], SSD2825_DSI_SEND);
	}
}

static int renesas_r69328_enable_backlight(struct udevice *dev)
{
	struct renesas_r69328_priv *priv = dev_get_priv(dev);
	int ret;

	ret = dm_gpio_set_value(&priv->enable_gpio, 1);
	if (ret) {
		printf("%s: error changing enable-gpios (%d)\n", __func__, ret);
		return ret;
	}
	mdelay(5);

	ret = dm_gpio_set_value(&priv->reset_gpio, 0);
	if (ret) {
		printf("%s: error changing reset-gpios (%d)\n", __func__, ret);
		return ret;
	}
	mdelay(5);

	ret = dm_gpio_set_value(&priv->reset_gpio, 1);
	if (ret) {
		printf("%s: error changing reset-gpios (%d)\n", __func__, ret);
		return ret;
	}

	mdelay(5);

	/* Bridge configuration */
	write_hw_register(priv, SSD2825_RGB_INTERFACE_CTRL_REG_1, 0x0104);
	write_hw_register(priv, SSD2825_RGB_INTERFACE_CTRL_REG_2, 0x0442);
	write_hw_register(priv, SSD2825_RGB_INTERFACE_CTRL_REG_3, 0x065C);
	write_hw_register(priv, SSD2825_RGB_INTERFACE_CTRL_REG_4, 0x02D0);
	write_hw_register(priv, SSD2825_RGB_INTERFACE_CTRL_REG_5, 0x0500);
	write_hw_register(priv, SSD2825_RGB_INTERFACE_CTRL_REG_6, 0xE007);
	write_hw_register(priv, SSD2825_LANE_CONFIGURATION_REG, 0x0003);
	write_hw_register(priv, SSD2825_TEST_REG, 0x0004);

	write_hw_register(priv, SSD2825_PLL_CTRL_REG, 0x0000);
	write_hw_register(priv, SSD2825_LINE_CTRL_REG, 0x0001);
	write_hw_register(priv, SSD2825_DELAY_ADJ_REG_1, 0x2103);
	write_hw_register(priv, SSD2825_PLL_CONFIGURATION_REG, 0xC8AB);
	write_hw_register(priv, SSD2825_CLOCK_CTRL_REG, 0x0009);
	write_hw_register(priv, SSD2825_PLL_CTRL_REG, 0x0001);
	write_hw_register(priv, SSD2825_VC_CTRL_REG, 0x0000);

	/* Panel configuration */
	write_hw_register(priv, SSD2825_CONFIGURATION_REG,
			SSD2825_CONF_REG_CKE | SSD2825_CONF_REG_ECD |
			SSD2825_CONF_REG_EOT);
	write_hw_register(priv, SSD2825_VC_CTRL_REG, 0x0000);

	write_hw_register(priv, SSD2825_PACKET_SIZE_CTRL_REG_1, 0x0002);
	write_hw_register(priv, SSD2825_PACKET_DROP_REG,
			  0x00FF & MIPI_DCS_SET_ADDRESS_MODE);

	write_hw_register(priv, SSD2825_PACKET_SIZE_CTRL_REG_1, 0x0002);
	write_hw_register(priv, SSD2825_PACKET_DROP_REG,
			  (MIPI_DCS_PIXEL_FMT_24BIT << 12) &
			   MIPI_DCS_SET_PIXEL_FORMAT);

	write_hw_register(priv, SSD2825_CONFIGURATION_REG,
			SSD2825_CONF_REG_CKE | SSD2825_CONF_REG_DCS |
			SSD2825_CONF_REG_ECD | SSD2825_CONF_REG_EOT);
	write_hw_register(priv, SSD2825_VC_CTRL_REG, 0x0000);

	write_hw_register(priv, SSD2825_PACKET_SIZE_CTRL_REG_1, 0x0002);
	write_hw_register(priv, SSD2825_PACKET_DROP_REG,
			  0x00FF & MIPI_DCS_EXIT_SLEEP_MODE);
	mdelay(80);

	write_hw_register(priv, SSD2825_CONFIGURATION_REG,
			SSD2825_CONF_REG_CKE | SSD2825_CONF_REG_ECD |
			SSD2825_CONF_REG_EOT);
	write_hw_register(priv, SSD2825_VC_CTRL_REG, 0x0000);

	write_hw_register(priv, SSD2825_PACKET_SIZE_CTRL_REG_1, 0x0002);
	write_hw_register(priv, SSD2825_PACKET_DROP_REG, MIPI_DCS_MCAP_OFF);

	write_hw_dsi(priv, power_set, sizeof(power_set));
	write_hw_dsi(priv, gamma_setting_a, sizeof(gamma_setting_a));
	write_hw_dsi(priv, gamma_setting_b, sizeof(gamma_setting_b));
	write_hw_dsi(priv, gamma_setting_c, sizeof(gamma_setting_c));

	write_hw_register(priv, SSD2825_PACKET_SIZE_CTRL_REG_1, 0x0002);
	write_hw_register(priv, SSD2825_PACKET_DROP_REG, MIPI_DCS_MCAP_ON);

	write_hw_register(priv, SSD2825_CONFIGURATION_REG,
			SSD2825_CONF_REG_CKE | SSD2825_CONF_REG_DCS |
			SSD2825_CONF_REG_ECD | SSD2825_CONF_REG_EOT);
	write_hw_register(priv, SSD2825_VC_CTRL_REG, 0x0000);

	write_hw_register(priv, SSD2825_PACKET_SIZE_CTRL_REG_1, 0x0002);
	write_hw_register(priv, SSD2825_PACKET_DROP_REG,
			  0x00FF & MIPI_DCS_SET_DISPLAY_ON);
	mdelay(50);

	write_hw_register(priv, SSD2825_PLL_CONFIGURATION_REG, 0xC8AB);
	write_hw_register(priv, SSD2825_CLOCK_CTRL_REG, 0x0009);
	write_hw_register(priv, SSD2825_PLL_CTRL_REG, 0x0001);
	write_hw_register(priv, SSD2825_VC_CTRL_REG, 0x0000);

	write_hw_register(priv, SSD2825_CONFIGURATION_REG,
			SSD2825_CONF_REG_HS | SSD2825_CONF_REG_VEN |
			SSD2825_CONF_REG_ECD | SSD2825_CONF_REG_EOT);

	ret = backlight_enable(priv->backlight);
	if (ret)
		return ret;

	return 0;
}

static int renesas_r69328_set_backlight(struct udevice *dev, int percent)
{
	struct renesas_r69328_priv *priv = dev_get_priv(dev);
	int ret;

	ret = backlight_set_brightness(priv->backlight, percent);
	if (ret)
		return ret;

	return 0;
}

static int renesas_r69328_probe(struct udevice *dev)
{
	struct renesas_r69328_priv *priv = dev_get_priv(dev);
	int ret;

	ret = uclass_get_device_by_phandle(UCLASS_PANEL_BACKLIGHT, dev,
					   "backlight", &priv->backlight);
	if (ret) {
		printf("%s: Cannot get backlight: ret = %d\n", __func__, ret);
		return ret;
	}

	ret = uclass_get_device_by_phandle(UCLASS_MISC, dev,
					   "bridge-spi", &priv->spi);
	if (ret) {
		debug("%s: Cannot get bridge-spi: ret = %d\n", __func__, ret);
		return ret;
	}

	ret = gpio_request_by_name(dev, "enable-gpios", 0,
				   &priv->enable_gpio, GPIOD_IS_OUT);
	if (ret) {
		printf("%s: Could not decode enable-gpios (%d)\n", __func__, ret);
		return ret;
	}

	ret = gpio_request_by_name(dev, "reset-gpios", 0,
				   &priv->reset_gpio, GPIOD_IS_OUT);
	if (ret) {
		printf("%s: Could not decode reser-gpios (%d)\n", __func__, ret);
		return ret;
	}

	return 0;
}

static const struct panel_ops renesas_r69328_ops = {
	.enable_backlight	= renesas_r69328_enable_backlight,
	.set_backlight		= renesas_r69328_set_backlight,
};

static const struct udevice_id renesas_r69328_ids[] = {
	{ .compatible = "jdi,dx12d100vm0eaa" },
	{ }
};

U_BOOT_DRIVER(renesas_r69328) = {
	.name		= "renesas_r69328",
	.id		= UCLASS_PANEL,
	.of_match	= renesas_r69328_ids,
	.ops		= &renesas_r69328_ops,
	.probe		= renesas_r69328_probe,
	.priv_auto	= sizeof(struct renesas_r69328_priv),
};
