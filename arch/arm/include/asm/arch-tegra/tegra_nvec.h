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

#ifndef _TEGRA_NVEC_H_
#define _TEGRA_NVEC_H_

#define I2C_CNFG			0x00
#define I2C_CNFG_PACKET_MODE_EN		(1<<10)
#define I2C_CNFG_NEW_MASTER_SFM		(1<<11)
#define I2C_CNFG_DEBOUNCE_CNT_SHIFT	12

#define I2C_SL_CNFG		0x20
#define I2C_SL_NEWSL		(1<<2)
#define I2C_SL_NACK		(1<<1)
#define I2C_SL_RESP		(1<<0)
#define I2C_SL_IRQ		(1<<3)
#define END_TRANS		(1<<4)
#define RCVD			(1<<2)
#define RNW			(1<<1)

#define I2C_SL_RCVD		0x24
#define I2C_SL_STATUS		0x28
#define I2C_SL_ADDR1		0x2c
#define I2C_SL_ADDR2		0x30
#define I2C_SL_DELAY_COUNT	0x3c


enum nvec_msg_type {
	NVEC_KEYBOARD = 0,
	NVEC_SYS = 1,
	NVEC_BAT,
	NVEC_GPIO,
	NVEC_SLEEP,
	NVEC_KBD,
	NVEC_PS2,
	NVEC_CNTL,
	NVEC_OEM0 = 0x0d,
	NVEC_KB_EVT = 0x80,
	NVEC_PS2_EVT,
};

enum nvec_event_size {
	NVEC_2BYTES,
	NVEC_3BYTES,
	NVEC_VAR_SIZE,
};

enum sys_subcmds {
	SYS_GET_STATUS,
	SYS_CNFG_EVENT_REPORTING,
	SYS_ACK_STATUS,
	SYS_CNFG_WAKE = 0xfd,
};

enum kbd_subcmds {
	CNFG_WAKE = 3,
	CNFG_WAKE_KEY_REPORTING,
	SET_LEDS = 0xed,
	ENABLE_KBD = 0xf4,
	DISABLE_KBD,
};

enum cntl_subcmds {
	CNTL_RESET_EC = 0x00,
	CNTL_SELF_TEST = 0x01,
	CNTL_NOOP = 0x02,
	CNTL_GET_EC_SPEC_VER = 0x10,
	CNTL_GET_FIRMWARE_VERSION = 0x15,
};

enum nvec_sleep_subcmds {
	GLOBAL_EVENTS,
	AP_PWR_DOWN,
	AP_SUSPEND,
};

#define MOUSE_SEND_CMD 0x01
#define MOUSE_RESET 0xff


int board_nvec_init(void);

int nvec_msg_is_event(const unsigned char *msg);
int nvec_msg_event_type(const unsigned char *msg);

/**
 * Send request and read response. If write or read failed
 * operation will be repeated NVEC_ATTEMPTS_MAX times.
 *
 * @param buf		request data
 * @param size		request data size
 * @return 0 if ok, -1 on error
 */
int nvec_do_request(char *buf, int size);


#endif /* _TEGRA_NVEC_H_ */
