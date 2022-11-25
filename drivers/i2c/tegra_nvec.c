/*
 * (C) Copyright 2013
 * Andrey Danin <andreydanin@mail.ru>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#define DEBUG 0

#include <common.h>
#include <linux/delay.h>
#include <errno.h>
#include <dm.h>
#include <i2c.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/clock.h>
#include <asm/arch/funcmux.h>
#include <asm/arch-tegra/tegra_nvec.h>
#include <asm/arch-tegra/tegra_nvec_keyboard.h>

#ifndef CONFIG_TEGRA_NVEC
#error "You should enable CONFIG_TEGRA_NVEC"
#endif

DECLARE_GLOBAL_DATA_PTR;
#define TRACE() debug("nvec: %s\n", __func__)
#define error(fmt, args...) printf(pr_fmt(fmt), ##args)

#define GPIO_ACTIVE 1
#define GPIO_INACTIVE 0

/* Nvec perfroms io interval is beteween 20 and 500 ms,
no response in 600 ms means error */
enum {
	NVEC_TIMEOUT_MIN = 20,
	NVEC_TIMEOUT_MAX = 600,
};
enum {
	NVEC_WAIT_FOR_EC = 1,
	NVEC_DONT_WAIT_FOR_EC = 0,
	NVEC_ATTEMPTS_MAX = 10,
};

enum {
	nvec_io_error = -1,
	nvec_io_timeout,
	nvec_io_read_ok,
	nvec_io_write_ok,
	nvec_io_not_ready,
	nvec_io_retry,
};

enum {
	NVST_BEGIN = 0,
	NVST_CMD = 1,
	NVST_SUBCMD = 2,
	NVST_READ = 3,
	NVST_WRITE_SIZE = 4,
	NVST_WRITE = 5,
};

struct nvec_t {
	struct gpio_desc request_gpio;
	int i2c_addr;
	int i2c_clk;
	void __iomem *base;
	int state;
	char rx_buf[34];
	int rx_pos;
	char *tx_buf;
	int tx_pos;
	int tx_size;
} *nvec_data;

#if 0
struct fdt_nvec_config {
	int gpio;
	int i2c_addr;
	int i2c_clk;
	fdt_addr_t base_addr;
	struct gpio_desc request_gpio;
};
#endif


/* nvec commands */
char noop[] = { NVEC_CNTL, CNTL_NOOP };


int nvec_msg_is_event(const unsigned char *msg)
{
	return msg[0] >> 7;
}


int nvec_msg_event_type(const unsigned char *msg)
{
	return msg[0] & 0x0f;
}


/**
 * Process incoming io message.
 * If message is keyboard event then key code will
 * be added to keys buffer.
 *
 * See: nvec_push_key, nvec_pop_key, nvec_have_key
 *
 * @param nvec	nvec state struct
 */
void nvec_process_msg(struct nvec_t *nvec)
{
	const unsigned char *msg = (const unsigned char *)nvec->rx_buf;
	int event_type;

	if (!nvec_msg_is_event(msg))
		return;

	event_type = nvec_msg_event_type(msg);
	if (event_type == NVEC_KEYBOARD)
		nvec_process_keyboard_msg(msg);
}


static inline int is_read(unsigned long status)
{
	return (status & RNW) == 0;
}


static inline int is_ready(unsigned long status)
{
	return status & I2C_SL_IRQ;
}


/**
 * Perform complete io operation (read or write).
 * NOTE: function will wait NVEC_TIMEOUT_MIN (20ms)
 * before status check to avoid nvec hang.
 *
 * @param nvec		nvec state struct
 * @param wait_for_ec	if 1(NVEC_WAIT_FOR_EC) operation
 *			timeout is NVEC_TIMEOUT_MAX (600ms),
 *			otherwise function will return if io
 *			is not ready.
 *
 * @return nvec_io_* code
 */
int nvec_do_io(struct nvec_t *nvec, int wait_for_ec)
{
	unsigned int poll_start_ms = 0;
	unsigned long status;
	unsigned int received = 0;
	unsigned int to_send = 0;
	unsigned int timeout_ms = NVEC_TIMEOUT_MAX;
	int is_first_iteration = 1;

	if (!nvec_data)
		return nvec_io_not_ready;

	poll_start_ms = get_timer(0);
	mdelay(NVEC_TIMEOUT_MIN);

	while (1) {
		status = readl(nvec->base + I2C_SL_STATUS);
		if (!is_ready(status)) {
			if (is_first_iteration && !wait_for_ec)
				return nvec_io_not_ready;

			if (get_timer(poll_start_ms) > timeout_ms)
				return nvec_io_timeout;

			is_first_iteration = 0;
			udelay(100);
			continue;
		}
		is_first_iteration = 0;

		if (is_read(status))
			received = readl(nvec->base + I2C_SL_RCVD);

		if (status == (I2C_SL_IRQ | RCVD)) {
			nvec->state = NVST_BEGIN;
			nvec->rx_pos = 0;
			nvec->tx_pos = 0;
		}

		switch (nvec->state) {
		case NVST_BEGIN:
			nvec->rx_pos = 0;
			nvec->tx_pos = 0;
			if (received != nvec->i2c_addr) {
				error("NVEC io: unknown addr 0x%x", received);
				return nvec_io_error;
			}
			nvec->state = NVST_CMD;
			break;

		case NVST_CMD:
			nvec->rx_buf[nvec->rx_pos++] = (char)received;
			nvec->state = NVST_SUBCMD;
			break;

		case NVST_SUBCMD:
			if (status == (I2C_SL_IRQ | RNW | RCVD)) {
				if (nvec->rx_buf[0] != 0x01) {
					error("NVEC io: wrong read");
					nvec->state = NVST_BEGIN;
					return nvec_io_error;
				}
				nvec->state = NVST_WRITE;
				if (nvec->tx_buf == 0) {
					debug("NVEC io: error, tx buffer is 0\n");
					nvec->tx_buf = noop;
					nvec->tx_size = 2;
					nvec->tx_pos = 0;
				}
				to_send = nvec->tx_size;
				writel(to_send, nvec->base + I2C_SL_RCVD);
				if (dm_gpio_set_value(&nvec_data->request_gpio, GPIO_INACTIVE))
					error("NVEC io: failed to set gpio value to INACTIVE");
				nvec->state = NVST_WRITE;
			} else {
				nvec->state = NVST_READ;
				nvec->rx_buf[nvec->rx_pos] = (char)received;
				++nvec->rx_pos;
			}
			break;

		case NVST_READ:
			if (nvec->rx_pos >= 34) {
				error("NVEC io: read buffer is full");
				break;
			}
			nvec->rx_buf[nvec->rx_pos++] = (char)received;
			if (status & END_TRANS) {
				nvec_process_msg(nvec);
				nvec->rx_pos = 0;
				return nvec_io_read_ok;
			}
			break;

		case NVST_WRITE_SIZE:
			to_send = nvec->tx_size;
			writel(to_send, nvec->base + I2C_SL_RCVD);
			nvec->state = NVST_WRITE;
			break;

		case NVST_WRITE:
			if (nvec->tx_pos >= nvec->tx_size) {
				if (status & END_TRANS)
					return nvec_io_write_ok;

				error("NVEC io: no data to write");
				return nvec_io_error;
			}
			to_send = nvec->tx_buf[nvec->tx_pos++];
			writel(to_send, nvec->base + I2C_SL_RCVD);
			if (status & END_TRANS) {
				nvec->tx_pos = 0;
				nvec->tx_buf = 0;
				return nvec_io_write_ok;
			}

			break;

		default:
			error("NVEC io: unknown state");
			break;
		}
		if (status & END_TRANS)
			return nvec_io_retry;
	}
}


/**
 * Send request and read response. If write or read failed
 * operation will be repeated NVEC_ATTEMPTS_MAX times.
 *
 * @param buf	request data
 * @param size	request data size
 * @return 0 if ok, -1 on error
 */
int nvec_do_request(char *buf, int size)
{
	int res = 0;
	int i = 0;
	int send_failed = 0;
	int recv_failed = 0;
	int gpio_failed = 0;

	nvec_data->tx_buf = buf;
	nvec_data->tx_size = size;

	while (i++ < NVEC_ATTEMPTS_MAX) {
		nvec_data->tx_pos = 0;

		/* request */
		if (dm_gpio_set_value(&nvec_data->request_gpio, GPIO_ACTIVE)) {
			++gpio_failed;
			error("NVEC io: failed to set gpio value to ACTIVE");
		}

		res = nvec_do_io(nvec_data, NVEC_WAIT_FOR_EC);
		if (res != nvec_io_write_ok) {
			++send_failed;
			debug("warning: nvec failed to send request\n");
			continue;
		}

		/* response */
		res = nvec_do_io(nvec_data, NVEC_WAIT_FOR_EC);
		if (res != nvec_io_read_ok) {
			++recv_failed;
			debug("warning: nvec failed to read response\n");
			continue;
		}

		nvec_data->tx_buf = 0;
		nvec_data->tx_size = 0;
		nvec_data->tx_pos = 0;

		return 0;
	}

	error("nvec failed to perform request (send: %d, recv: %d, gpio: %d)",
			send_failed, recv_failed, gpio_failed);
	return -1;
}


/**
 * Init i2c controller to operate in slave mode.
 *
 * @param nvec	nvec state struct
 */
static void nvec_init_i2c_slave(struct nvec_t *nvec)
{
	unsigned long val;

	val = I2C_CNFG_NEW_MASTER_SFM | I2C_CNFG_PACKET_MODE_EN |
	    (0x2 << I2C_CNFG_DEBOUNCE_CNT_SHIFT);
	writel(val, nvec->base + I2C_CNFG);

	/* i2c3 -> 67 */
	clock_start_periph_pll(67, CLOCK_ID_PERIPH,
			       nvec->i2c_clk * 8);

	reset_periph(67, 1);

	writel(I2C_SL_NEWSL, nvec->base + I2C_SL_CNFG);
	writel(0x1E, nvec->base + I2C_SL_DELAY_COUNT);

	writel(nvec->i2c_addr>>1, nvec->base + I2C_SL_ADDR1);
	writel(0, nvec->base + I2C_SL_ADDR2);

	funcmux_select(67, FUNCMUX_DEFAULT);
}


/**
 * Decode the nvec information from the fdt.
 *
 * @param dev          nvec device
 * @param config       structure to store fdt config into
 * @return 0 if ok, -ve on error
 */
static int nvec_decode_fdt(struct udevice *dev, struct nvec_t *nvec)
{
//	int ret;

//	ret = gpio_request_by_name_nodev(blob, node, "request-gpios", 0,
//				   &nvec->request_gpio, GPIOD_IS_OUT);
//	if (ret < 0) {
//		error("Failed to get request-gpios (%d)", ret);
//		return -ENOENT;
//	}

	nvec->base = dev_read_addr_ptr(dev);
	if (!nvec->base)
		return -ENOMEM;

	nvec->i2c_addr = dev_read_u32_default(dev, "slave-addr", -1);
	nvec->i2c_clk = dev_read_u32_default(dev, "clock-frequency", -1);

	return 0;
}

static int nvec_probe(struct udevice *dev)
{
	struct nvec_t *nvec = dev_get_priv(dev);
	int res;

	TRACE();
	nvec_data = nvec;

	nvec_data->rx_pos = 0;
	nvec_data->tx_buf = 0;
	nvec_data->tx_pos = 0;
	nvec_data->tx_size = 0;
	nvec_data->state = NVST_BEGIN;

	debug("NVEC initialization...\n");

	if (nvec_decode_fdt(dev, nvec_data)) {
		error("Failed to decode fdt");
		return -EBUSY;
	}

	res = gpio_request_by_name(dev, "request-gpios", 0, &nvec_data->request_gpio, GPIOD_IS_OUT);
	if (res != 0)
		error("NVEC: err, gpio_request (%d)", res);
	debug("request-gpios flags: 0x%lx\n", nvec_data->request_gpio.flags);
	res = dm_gpio_set_value(&nvec_data->request_gpio, GPIO_INACTIVE);
	if (res != 0)
		error("NVEC: err, dm_gpio_set_value to INACTIVE error (%d)", res);
	udelay(100);

	nvec_init_i2c_slave(nvec_data);

	nvec_enable_kbd_events();

	// TODO: free gpios and memory

	return 0;
}


int nvec_read_events(void)
{
	int res;
	int cnt = 0;

	while (++cnt <= 8) {
		res = nvec_do_io(nvec_data, NVEC_DONT_WAIT_FOR_EC);
		switch (res) {
		case nvec_io_not_ready:
			return 0;

		case nvec_io_read_ok:
		case nvec_io_retry:
			break;

		case nvec_io_error:
		case nvec_io_timeout:
			debug("NVEC events: io failed %d\n", res);
			return 0;

		case nvec_io_write_ok:
		default:
			debug("NVEC events: unexpected io result %d\n", res);
			return 0;
		}
	}

	return 0;
}

static int nvec_set_bus_speed(struct udevice *dev, unsigned int speed)
{
	TRACE();
	return 0;
}

static int nvec_xfer(struct udevice *dev, struct i2c_msg *msg,
			    int nmsgs)
{
	TRACE();
	return 0;
}

/* Probe to see if a chip is present. */
static int nvec_probe_chip(struct udevice *bus, uint chip_addr,
				uint chip_flags)
{
	/* Shift 7-bit address over for lower-level i2c functions */
	/*rc = tegra_i2c_write_data(i2c_bus, chip_addr << 1, &reg, sizeof(reg),
				  false);*/
	TRACE();

	return 0;
}

static const struct dm_i2c_ops i2c_nvec_ops = {
	.xfer		= nvec_xfer,
	.probe_chip	= nvec_probe_chip,
	.set_bus_speed	= nvec_set_bus_speed,
};

static const struct udevice_id i2c_nvec_ids[] = {
	{ .compatible = "nvidia,tegra20-nvec" },
	{ }
};

U_BOOT_DRIVER(i2c_nvec) = {
	.name	= "i2c-nvec",
	.id	= UCLASS_I2C,
	.of_match = i2c_nvec_ids,
	.probe	= nvec_probe,
	.priv_auto = sizeof(struct nvec_t),
	.ops	= &i2c_nvec_ops,
};
