/*
 *  (C) Copyright 2011,2013
 *  Andrey Danin <danindrey@mail.ru>
 *  NVIDIA Corporation <www.nvidia.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#define DEBUG 0

#include <common.h>
#include <dm.h>
#include <input.h>
#include <keyboard.h>
#include <stdio_dev.h>
#include <asm/arch-tegra/tegra_nvec_events.h>
#include <asm/arch-tegra/tegra_nvec_keyboard.h>
#include <linux/input.h>

#define TRACE() debug("%s\n", __func__)

enum {
	KBC_MAX_KPENT = 8,
};

enum {
	KBC_REPEAT_RATE_MS	= 30,
	KBC_REPEAT_DELAY_MS	= 240,
};

/**
 * Check the tegra nvec keyboard, and send any keys that are pressed.
 *
 * This is called by input_tstc() and input_getc() when they need more
 * characters
 *
 * @param input		Input configuration
 * @return 1, to indicate that we have something to look at
 */
int tegra_nvec_kbc_check(struct input_config *input)
{
	int res = 0;
	int fifo[KBC_MAX_KPENT];
	int cnt = 0;

	if (!nvec_have_keys())
		nvec_read_events();

	while (nvec_have_keys() && cnt < KBC_MAX_KPENT) {
		res = 1;
		fifo[cnt++] = nvec_pop_key();
		if (cnt == KBC_MAX_KPENT) {
			input_send_keycodes(input, fifo, cnt);
			cnt = 0;
		}
	}

	if (cnt > 0)
		input_send_keycodes(input, fifo, cnt);

	return res;
}


static int tegra_nvec_kbd_start(struct udevice *dev)
{
	(void)dev;

	debug("%s: Tegra nvec keyboard ready\n", __func__);

	return 0;
}

/**
 * Set up the tegra keyboard. This is called by the stdio device handler
 *
 * We want to do this init when the keyboard is actually used rather than
 * at start-up, since keyboard input may not currently be selected.
 *
 * Once the keyboard starts there will be a period during which we must
 * wait for the keyboard to init. We do this only when a key is first
 * read - see kbd_wait_for_fifo_init().
 *
 * @return 0 if ok, -ve on error
 */
static int tegra_nvec_kbd_probe(struct udevice *dev)
{
	(void)dev;
	struct keyboard_priv *uc_priv = dev_get_uclass_priv(dev);
	struct stdio_dev *sdev = &uc_priv->sdev;
	struct input_config *input = &uc_priv->input;
	int ret;

	TRACE();
	input_set_delays(input, KBC_REPEAT_DELAY_MS, KBC_REPEAT_RATE_MS);

	input->dev = dev;
	input->read_keys = tegra_nvec_kbc_check;
	input_add_tables(input, false);
	strcpy(sdev->name, "tegra-nvec-kbc");
	ret = input_stdio_register(sdev);
	if (ret) {
		debug("%s: input_stdio_register() failed\n", __func__);
		return ret;
	}

	debug("tegra kbc is initialized\n");

	return 0;
}

static const struct keyboard_ops tegra_nvec_kbd_ops = {
	.start	= tegra_nvec_kbd_start,
};

static const struct udevice_id tegra_nvec_kbd_ids[] = {
	{ .compatible = "nvidia,tegra20-nvec-kbc" },
	{ }
};

U_BOOT_DRIVER(tegra_kbd) = {
	.name	= "tegra_nvec_kbd",
	.id	= UCLASS_KEYBOARD,
	.of_match = tegra_nvec_kbd_ids,
	.probe = tegra_nvec_kbd_probe,
	.ops	= &tegra_nvec_kbd_ops,
};
