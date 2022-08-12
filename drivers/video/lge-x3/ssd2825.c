// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2022 Svyatoslav Ryhel <clamor95@gmail.com>
 *
 * This driver is a hw bridge setup before final
 * configuration in panel driver.
 */

#include <common.h>
#include <dm.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <log.h>
#include <panel.h>
#include <asm/gpio.h>

#include "ssd2825.h"

#define LM3533_BL_DEFAULT_BRIGHTNESS 	0x71

struct ssd2825_bridge_priv {
	struct udevice *panel;

	struct gpio_desc enable_gpio;
	struct gpio_desc reset_gpio;
};

static int ssd2825_bridge_enable_panel(struct udevice *dev)
{
	struct ssd2825_bridge_priv *priv = dev_get_priv(dev);
	int ret;

	ret = dm_gpio_set_value(&priv->enable_gpio, 1);
	if (ret) {
		printf("%s: error changing enable-gpios (%d)\n", __func__, ret);
		return ret;
	}
	mdelay(10);

	ret = dm_gpio_set_value(&priv->reset_gpio, 0);
	if (ret) {
		printf("%s: error changing reset-gpios (%d)\n", __func__, ret);
		return ret;
	}
	mdelay(10);

	ret = dm_gpio_set_value(&priv->reset_gpio, 1);
	if (ret) {
		printf("%s: error changing reset-gpios (%d)\n", __func__, ret);
		return ret;
	}
	mdelay(10);

	ret = panel_enable_backlight(priv->panel);
	if (ret)
		return ret;

	/* Set backlight brightness here since set_backlight may not be called */
	ret = panel_set_backlight(priv->panel,
				  LM3533_BL_DEFAULT_BRIGHTNESS);
	if (ret)
		return ret;

	return 0;
}

static int ssd2825_bridge_set_panel(struct udevice *dev, int percent)
{
	return 0;
}

static int ssd2825_bridge_probe(struct udevice *dev)
{
	struct ssd2825_bridge_priv *priv = dev_get_priv(dev);
	int ret;

	ret = uclass_get_device_by_phandle(UCLASS_PANEL, dev,
					   "panel", &priv->panel);
	if (ret) {
		printf("%s: Cannot get panel: ret=%d\n", __func__, ret);
		return log_ret(ret);
	}

	/* get panel gpios */
	ret = gpio_request_by_name(dev, "enable-gpios", 0,
				   &priv->enable_gpio, GPIOD_IS_OUT);
	if (ret) {
		printf("%s: Could not decode enable-gpios (%d)\n", __func__, ret);
		return ret;
	}

	ret = gpio_request_by_name(dev, "reset-gpios", 0,
				   &priv->reset_gpio, GPIOD_IS_OUT);
	if (ret) {
		printf("%s: Could not decode reset-gpios (%d)\n", __func__, ret);
		return ret;
	}

	return 0;
}

static const struct panel_ops ssd2825_bridge_ops = {
	.enable_backlight	= ssd2825_bridge_enable_panel,
	.set_backlight		= ssd2825_bridge_set_panel,
};

static const struct udevice_id ssd2825_bridge_ids[] = {
	{ .compatible = "solomon,ssd2825" },
	{ }
};

U_BOOT_DRIVER(ssd2825) = {
	.name		= "ssd2825",
	.id		= UCLASS_PANEL,
	.of_match	= ssd2825_bridge_ids,
	.ops		= &ssd2825_bridge_ops,
	.probe		= ssd2825_bridge_probe,
	.priv_auto	= sizeof(struct ssd2825_bridge_priv),
};
