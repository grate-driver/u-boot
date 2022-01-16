// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  (C) Copyright 2011
 *      NVIDIA Corporation <www.nvidia.com>
 *  (C) Copyright 2022
 *      Jonas Schwöbel <jonasschwoebel@yahoo.de>
 */

#include <dm.h>
#include <dm/devres.h>
#include <input.h>
#include <keyboard.h>
#include <log.h>
#include <linux/input.h>
#include <asm/gpio.h>

struct gpio_kbd_priv {
	struct input_config *input;
	struct input_key *keys;

	unsigned char num_gpios;
	unsigned int num_keys;
};

struct input_key {
	struct gpio_desc gpio;
	unsigned int key_code;
};

/**
 * Check the gpio keys, and send any keys that are pressed.
 *
 * This is called by input_tstc() and input_getc() when they need more
 * characters
 *
 * @param input		Input configuration
 * @return 1, to indicate that we have something to look at
 */
static int gpio_read_keys(struct input_config *input)
{
	struct gpio_kbd_priv *priv = dev_get_priv(input->dev);
	int i;

	for (i = 0; i < priv->num_keys; i++) {
		if (dm_gpio_get_value(&priv->keys[i].gpio))
			input_send_keycodes(input, &priv->keys[i].key_code, 1);
	}

	return 1;
}

static int gpio_kbd_start(struct udevice *dev)
{
	return 0;
}

/**
 * Set up the gpio keys. This is called by the stdio device handler
 *
 * @return 0 if ok, -ve on error
 */

#define KBC_REPEAT_DELAY_MS		250
#define KBC_REPEAT_RATE_MS		250

static int gpio_kbd_probe(struct udevice *dev)
{
	struct gpio_kbd_priv *priv = dev_get_priv(dev);
	struct keyboard_priv *uc_priv = dev_get_uclass_priv(dev);
	struct stdio_dev *sdev = &uc_priv->sdev;
	struct input_config *input = &uc_priv->input;
	ofnode node = dev_ofnode(dev);
	int count = ofnode_get_child_count(node);
	int ret;

	if (count < 1) {
		printf("No (gpio) subnodes found\n");
		return -EINVAL;
	}

	priv->keys = devm_kzalloc(dev, sizeof(struct input_key) * count, 0);
	priv->num_gpios = count;

	input_set_delays(input, KBC_REPEAT_DELAY_MS, KBC_REPEAT_RATE_MS);
	input_allow_repeats(input, false);
	input_add_tables(input, false);

	priv->num_keys = 0;

	dev_for_each_subnode(node, dev) {
		const char *label = ofnode_read_string(node, "label");

		if (!label) {
			printf("%s: node %s has no label\n", __func__,
			       ofnode_get_name(node));
			return -EINVAL;
		}

		ofnode_read_u32(node, "linux,code", &priv->keys[priv->num_keys].key_code);

		ret = gpio_request_by_name_nodev(node, "gpios", 0,
						 &priv->keys[priv->num_keys].gpio, GPIOD_IS_IN);
		if (ret)
			printf("GPIO request error: %d\n", ret);

		priv->num_keys++;
	}

	/* Register the device */
	priv->input = input;
	input->dev = dev;
	input->read_keys = gpio_read_keys;
	strcpy(sdev->name, "gpio-kbd");

	ret = input_stdio_register(sdev);
	if (ret) {
		debug("%s: input_stdio_register() failed\n", __func__);
		return ret;
	}

	return 0;
}

static const struct keyboard_ops gpio_kbd_ops = {
	.start	= gpio_kbd_start,
};

static const struct udevice_id gpio_kbd_ids[] = {
	{ .compatible = "gpio-kbd" },
	{ }
};

U_BOOT_DRIVER(gpio_kbd_input) = {
	.name		= "gpio_kbd_input",
	.id		= UCLASS_KEYBOARD,
	.of_match	= gpio_kbd_ids,
	.probe		= gpio_kbd_probe,
	.ops		= &gpio_kbd_ops,
	.priv_auto	= sizeof(struct gpio_kbd_priv),
};
