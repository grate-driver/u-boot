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

#include <common.h>
#include <circbuf.h>
#include <asm/arch-tegra/tegra_nvec_keyboard.h>
#include <asm/arch-tegra/tegra_nvec_keytable.h>
#include <asm/arch-tegra/tegra_nvec.h>


#define TRACE() debug("nvec keyboard: %s\n", __func__)
#define error(fmt, args...) printf(pr_fmt(fmt), ##args)

circbuf_t key_buf = { 0, 0, NULL, NULL, NULL, NULL };

/* nvec commands */
static char enable_kbd[] = { NVEC_KBD, ENABLE_KBD };
static char reset_kbd[] = { NVEC_PS2, MOUSE_SEND_CMD, MOUSE_RESET, 3 };
static char clear_leds[] = { NVEC_KBD, SET_LEDS, 0 };


void nvec_push_key(unsigned short code, unsigned short state)
{
	int code_state;

	assert(key_buf.totalsize > 0);

	if (key_buf.size == key_buf.totalsize)
		return;

	code_state = ((state << 16) | code);
	buf_push(&key_buf, (const char *)&code_state, sizeof(code_state));
}


int nvec_have_keys(void)
{
	return key_buf.size > 0;
}


int nvec_pop_key(void)
{
	int code_state;
	int len = buf_pop(&key_buf, (char *)&code_state, sizeof(code_state));

	if (len < sizeof(code_state))
		return -1;

	return code_state;
}


void nvec_process_keyboard_msg(const unsigned char *msg)
{
	int code, state;
	int event_type;
	int _size;

	event_type = nvec_msg_event_type(msg);
	if (event_type != NVEC_KEYBOARD)
		return;

	_size = (msg[0] & (3 << 5)) >> 5;

	if (_size == NVEC_VAR_SIZE)
		return;

	if (_size == NVEC_3BYTES)
		msg++;

	code = msg[1] & 0x7f;
	state = msg[1] & 0x80;

	nvec_push_key(code_tabs[_size][code], state);
}


void nvec_enable_kbd_events(void)
{
	TRACE();
	buf_init(&key_buf, NVEC_KEYS_QUEUE_SIZE * sizeof(int));

	if (nvec_do_request(reset_kbd, 4))
		error("NVEC: failed to reset keyboard\n");
	if (nvec_do_request(clear_leds, 3))
		error("NVEC: failed to clear leds\n");
	if (nvec_do_request(enable_kbd, 2))
		error("NVEC: failed to enable keyboard\n");

	debug("NVEC: keyboard initialization finished\n");
}
