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

#ifndef _TEGRA_NVEC_KEYBOARD_H_
#define _TEGRA_NVEC_KEYBOARD_H_


#define NVEC_KEYS_QUEUE_SIZE		256

void nvec_enable_kbd_events(void);
void nvec_process_keyboard_msg(const unsigned char *msg);
int nvec_pop_key(void);
int nvec_have_keys(void);


#endif /* _TEGRA_NVEC_KEYBOARD_H_ */
