// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2022 Svyatoslav Ryhel <clamor95@gmail.com>
 */

#include <common.h>
#include <dm.h>
#include <log.h>
#include <misc.h>
#include <mipi_display.h>
#include <mipi_dsi.h>
#include <backlight.h>
#include <panel.h>
#include <spi.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <asm/gpio.h>

#include "ssd2825.h"

struct ssd2825_bridge_priv {
	struct mipi_dsi_host host;
	struct mipi_dsi_device device;

	struct udevice *panel;
	struct display_timing timing;

	struct gpio_desc power_gpio;
	struct gpio_desc reset_gpio;

	u32 ref_clock_hz;
	u32 pll_freq_kbps;	/* PLL in kbps */
};

static int ssd2825_spi_write(struct udevice *dev, int reg,
				    const void *buf, int flags)
{
	struct spi_slave *slave = dev_get_parent_priv(dev);
	u8 command[2];

	if (flags & SSD2825_CMD_SEND) {
		command[0] = SSD2825_CMD_MASK;
		command[1] = reg;
		spi_xfer(slave, 9, &command,
			 NULL, SPI_XFER_ONCE);
	}

	if (flags & SSD2825_DAT_SEND) {
		u16 data = *(u16 *)buf;
		u8 cmd1, cmd2;

		/* send low byte first and then high byte */
		cmd1 = (data & 0x00FF);
		cmd2 = (data & 0xFF00) >> 8;

		command[0] = SSD2825_DAT_MASK;
		command[1] = cmd1;
		spi_xfer(slave, 9, &command,
			 NULL, SPI_XFER_ONCE);

		command[0] = SSD2825_DAT_MASK;
		command[1] = cmd2;
		spi_xfer(slave, 9, &command,
			 NULL, SPI_XFER_ONCE);
	}

	if (flags & SSD2825_DSI_SEND) {
		u16 data = *(u16 *)buf;
		data &= 0x00FF;

		debug("%s: dsi command (0x%x)\n",
		       __func__, data);

		command[0] = SSD2825_DAT_MASK;
		command[1] = data;
		spi_xfer(slave, 9, &command,
			 NULL, SPI_XFER_ONCE);
	}

	return 0;
}

static int ssd2825_spi_read(struct udevice *dev, int reg,
				   void *data, int flags)
{
	struct spi_slave *slave = dev_get_parent_priv(dev);
	u8 command[2];

	command[0] = SSD2825_CMD_MASK;
	command[1] = SSD2825_SPI_READ_REG;
	spi_xfer(slave, 9, &command,
		 NULL, SPI_XFER_ONCE);

	command[0] = SSD2825_DAT_MASK;
	command[1] = SSD2825_SPI_READ_REG_RESET;
	spi_xfer(slave, 9, &command,
		 NULL, SPI_XFER_ONCE);

	command[0] = SSD2825_DAT_MASK;
	command[1] = 0;
	spi_xfer(slave, 9, &command,
		 NULL, SPI_XFER_ONCE);

	command[0] = SSD2825_CMD_MASK;
	command[1] = reg;
	spi_xfer(slave, 9, &command,
		 NULL, SPI_XFER_ONCE);

	command[0] = SSD2825_CMD_MASK;
	command[1] = SSD2825_SPI_READ_REG_RESET;
	spi_xfer(slave, 9, &command,
		 NULL, SPI_XFER_ONCE);

	spi_xfer(slave, 16, NULL,
		 (u8 *)data, SPI_XFER_ONCE);

	return 0;
}

static void ssd2825_write_register(struct udevice *dev, u8 reg,
				   u16 command)
{
	ssd2825_spi_write(dev, reg, &command,
			  SSD2825_CMD_SEND |
			  SSD2825_DAT_SEND);
}

static void ssd2825_write_dsi(struct udevice *dev, const u8 *command,
			      int len)
{
	int i;

	ssd2825_spi_write(dev, SSD2825_PACKET_SIZE_CTRL_REG_1, &len,
			  SSD2825_CMD_SEND | SSD2825_DAT_SEND);

	ssd2825_spi_write(dev, SSD2825_PACKET_DROP_REG, NULL,
			  SSD2825_CMD_SEND);

	for (i = 0; i < len; i++)
		ssd2825_spi_write(dev, 0, &command[i], SSD2825_DSI_SEND);
}

static ssize_t ssd2825_bridge_transfer(struct mipi_dsi_host *host,
				       const struct mipi_dsi_msg *msg)
{
	struct udevice *dev = (struct udevice *)host->dev;
	u8 buf = *(u8 *)msg->tx_buf;
	u16 config;
	int ret;

	ret = ssd2825_spi_read(dev, SSD2825_CONFIGURATION_REG,
				&config, 0);
	if (ret)
		return ret;

	switch (msg->type) {
	case MIPI_DSI_DCS_SHORT_WRITE:
	case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
	case MIPI_DSI_DCS_LONG_WRITE:
		config |= SSD2825_CONF_REG_DCS;
		break;
	case MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM:
	case MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM:
	case MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM:
	case MIPI_DSI_GENERIC_LONG_WRITE:
		config &= ~SSD2825_CONF_REG_DCS;
		break;
	default:
		return 0;
	}

	ssd2825_write_register(dev, SSD2825_CONFIGURATION_REG, config);
	ssd2825_write_register(dev, SSD2825_VC_CTRL_REG, 0x0000);
	ssd2825_write_dsi(dev, msg->tx_buf, msg->tx_len);

	if (buf == MIPI_DCS_SET_DISPLAY_ON) {
		ssd2825_write_register(dev, SSD2825_CONFIGURATION_REG,
				SSD2825_CONF_REG_HS | SSD2825_CONF_REG_VEN |
				SSD2825_CONF_REG_DCS | SSD2825_CONF_REG_ECD |
				SSD2825_CONF_REG_EOT);
		ssd2825_write_register(dev, SSD2825_PLL_CTRL_REG, 0x0001);
		ssd2825_write_register(dev, SSD2825_VC_CTRL_REG, 0x0000);
	}

	return 0;
}

static const struct mipi_dsi_host_ops ssd2825_bridge_host_ops = {
	.transfer	= ssd2825_bridge_transfer,
};

/*
 * PLL configuration register settings.
 *
 * See the "PLL Configuration Register Description" in the SSD2825 datasheet.
 */
static u16 construct_pll_config(struct ssd2825_bridge_priv *priv,
				u32 desired_pll_freq_kbps, u32 reference_freq_khz)
{
	u32 div_factor = 1, mul_factor, fr = 0;

	while (reference_freq_khz / (div_factor + 1) >= SSD2825_REF_MIN_CLK)
		div_factor++;
	if (div_factor > 31)
		div_factor = 31;

	mul_factor = DIV_ROUND_UP(desired_pll_freq_kbps * div_factor,
				  reference_freq_khz);

	priv->pll_freq_kbps = reference_freq_khz * mul_factor / div_factor;

	if (priv->pll_freq_kbps >= 501000)
		fr = 3;
	else if (priv->pll_freq_kbps >= 251000)
		fr = 2;
	else if (priv->pll_freq_kbps >= 126000)
		fr = 1;

	return (fr << 14) | (div_factor << 8) | mul_factor;
}

static void ssd2825_setup_pll(struct udevice *dev)
{
	struct ssd2825_bridge_priv *priv = dev_get_priv(dev);
	struct mipi_dsi_device *device = &priv->device;
	struct display_timing *dt = &priv->timing;
	u16 pll_config, lp_div;
	u32 pclk_mult, tx_freq_khz, pd_lines;

	tx_freq_khz = priv->ref_clock_hz / 1000;
	pd_lines = mipi_dsi_pixel_format_to_bpp(device->format);
	pclk_mult = pd_lines / device->lanes + 1;

	pll_config = construct_pll_config(priv, pclk_mult *
					  dt->pixelclock.typ / 1000,
					  tx_freq_khz);

	lp_div = priv->pll_freq_kbps / (SSD2825_LP_MIN_CLK * 8);

	/* Disable PLL */
	ssd2825_write_register(dev, SSD2825_PLL_CTRL_REG, 0x0000);
	ssd2825_write_register(dev, SSD2825_LINE_CTRL_REG, 0x0001);

	/* Set delays */
	ssd2825_write_register(dev, SSD2825_DELAY_ADJ_REG_1, 0x2103);

	/* Set PLL coeficients */
	ssd2825_write_register(dev, SSD2825_PLL_CONFIGURATION_REG, pll_config);

	/* Clock Control Register */
	ssd2825_write_register(dev, SSD2825_CLOCK_CTRL_REG,
				SSD2828_LP_CLOCK_DIVIDER(lp_div));

	/* Enable PLL */
	ssd2825_write_register(dev, SSD2825_PLL_CTRL_REG, 0x0001);
	ssd2825_write_register(dev, SSD2825_VC_CTRL_REG, 0x0000);
}

static int ssd2825_bridge_enable_panel(struct udevice *dev)
{
	struct ssd2825_bridge_priv *priv = dev_get_priv(dev);
	struct mipi_dsi_device *device = &priv->device;
	struct display_timing *dt = &priv->timing;
	int ret;

	ret = dm_gpio_set_value(&priv->power_gpio, 1);
	if (ret) {
		printf("%s: error changing power-gpios (%d)\n", __func__, ret);
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

	/* Perform panel HW setup */
	ret = panel_enable_backlight(priv->panel);
	if (ret)
		return ret;

	/* Perform SW reset */
	ssd2825_write_register(dev, SSD2825_OPERATION_CTRL_REG, 0x0100);

	/* Set panel timings */
	ssd2825_write_register(dev, SSD2825_RGB_INTERFACE_CTRL_REG_1,
				dt->vsync_len.typ << 8 | dt->hsync_len.typ);
	ssd2825_write_register(dev, SSD2825_RGB_INTERFACE_CTRL_REG_2,
				(dt->vsync_len.typ + dt->vback_porch.typ) << 8 |
				(dt->hsync_len.typ + dt->hback_porch.typ));
	ssd2825_write_register(dev, SSD2825_RGB_INTERFACE_CTRL_REG_3,
				dt->vfront_porch.typ << 8 | dt->hfront_porch.typ);
	ssd2825_write_register(dev, SSD2825_RGB_INTERFACE_CTRL_REG_4,
				dt->hactive.typ);
	ssd2825_write_register(dev, SSD2825_RGB_INTERFACE_CTRL_REG_5,
				dt->vactive.typ);
	ssd2825_write_register(dev, SSD2825_RGB_INTERFACE_CTRL_REG_6,
				SSD2825_HSYNC_HIGH | SSD2825_VSYNC_HIGH |
				SSD2825_PCKL_HIGH | SSD2825_NON_BURST |
				(3 - device->format));
	ssd2825_write_register(dev, SSD2825_LANE_CONFIGURATION_REG,
				device->lanes - 1);
	ssd2825_write_register(dev, SSD2825_TEST_REG, 0x0004);

	/* Call PLL configuration */
	ssd2825_setup_pll(dev);

	mdelay(10);

	/* Initial DSI configuration register set */
	ssd2825_write_register(dev, SSD2825_CONFIGURATION_REG,
				SSD2825_CONF_REG_CKE | SSD2825_CONF_REG_DCS |
				SSD2825_CONF_REG_ECD | SSD2825_CONF_REG_EOT);
	ssd2825_write_register(dev, SSD2825_VC_CTRL_REG, 0x0000);

	/* Set up SW panel configuration */
	ret = panel_set_backlight(priv->panel, BACKLIGHT_MAX / 2);
	if (ret)
		return ret;

	return 0;
}

static int ssd2825_bridge_set_panel(struct udevice *dev, int percent)
{
	return 0;
}

static int ssd2825_bridge_panel_timings(struct udevice *dev,
					struct display_timing *timing)
{
	struct ssd2825_bridge_priv *priv = dev_get_priv(dev);

	memcpy(timing, &priv->timing, sizeof(*timing));

	return 0;
}

static int ssd2825_bridge_probe(struct udevice *dev)
{
	struct ssd2825_bridge_priv *priv = dev_get_priv(dev);
	struct spi_slave *slave = dev_get_parent_priv(dev);
	struct mipi_dsi_device *device = &priv->device;
	struct mipi_dsi_panel_plat *mipi_plat;
	int ret;

	ret = spi_claim_bus(slave);
	if (ret) {
		printf("%s: SPI bus allocation failed (%d)\n", __func__, ret);
		return ret;
	}

	ret = uclass_get_device_by_phandle(UCLASS_PANEL, dev,
					   "panel", &priv->panel);
	if (ret) {
		printf("%s: Cannot get panel: ret=%d\n", __func__, ret);
		return ret;
	}

	panel_get_display_timing(priv->panel, &priv->timing);

	mipi_plat = dev_get_plat(priv->panel);
	mipi_plat->device = device;

	priv->host.dev = (struct device *)dev;
	priv->host.ops = &ssd2825_bridge_host_ops;

	device->host = &priv->host;
	device->lanes = mipi_plat->lanes;
	device->format = mipi_plat->format;
	device->mode_flags = mipi_plat->mode_flags;

	/* get panel gpios */
	ret = gpio_request_by_name(dev, "power-gpios", 0,
				   &priv->power_gpio, GPIOD_IS_OUT);
	if (ret) {
		printf("%s: Could not decode power-gpios (%d)\n", __func__, ret);
		return ret;
	}

	ret = gpio_request_by_name(dev, "reset-gpios", 0,
				   &priv->reset_gpio, GPIOD_IS_OUT);
	if (ret) {
		printf("%s: Could not decode reset-gpios (%d)\n", __func__, ret);
		return ret;
	}

	/* Since u-boot has no unified API for clocks, just get clock rate */
	priv->ref_clock_hz = dev_read_u32_default(dev, "clock-rates",
						  SSD2825_DEFAULT_REF_CLOCK);

	return 0;
}

static const struct panel_ops ssd2825_bridge_ops = {
	.enable_backlight	= ssd2825_bridge_enable_panel,
	.set_backlight		= ssd2825_bridge_set_panel,
	.get_display_timing	= ssd2825_bridge_panel_timings,
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
