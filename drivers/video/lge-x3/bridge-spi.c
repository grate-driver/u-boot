// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2022 Svyatoslav Ryhel <clamor95@gmail.com>
 *
 * SPI interface for ssd2825 bridge on t30
 */

#include <common.h>
#include <dm.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <log.h>
#include <misc.h>
#include <spi.h>
#include <asm/gpio.h>

#include "ssd2825.h"

static int bridge_spi_write(struct udevice *dev, int reg,
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

static int bridge_spi_read(struct udevice *dev, int reg,
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

static int bridge_spi_probe(struct udevice *dev)
{
	struct spi_slave *slave = dev_get_parent_priv(dev);
	int ret;

	ret = spi_claim_bus(slave);
	if (ret) {
		printf("%s: SPI bus allocation failed (%d)\n", __func__, ret);
		return ret;
	}

	return 0;
}

static const struct misc_ops bridge_spi_ops = {
	.write	= bridge_spi_write,
	.read	= bridge_spi_read,
};

static const struct udevice_id bridge_spi_ids[] = {
	{ .compatible = "lge,bridge-spi" },
	{ }
};

U_BOOT_DRIVER(bridge_spi) = {
	.name		= "bridge_spi",
	.id		= UCLASS_MISC,
	.of_match	= bridge_spi_ids,
	.ops		= &bridge_spi_ops,
	.probe		= bridge_spi_probe,
};
