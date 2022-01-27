/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 *  (C) Copyright 2010,2011
 *  NVIDIA Corporation <www.nvidia.com>
 *
 *  (C) Copyright 2022
 *  Svyatoslav Ryhel <clamor95@gmail.com>
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>

#include "tegra20-common.h"
#include "transformer-common.h"

/* Board-specific serial config */
#define CFG_SYS_NS16550_COM1		NV_PA_APB_UARTD_BASE

#include "tegra-common-post.h"

#undef CONFIG_BOOTCOMMAND
#define CONFIG_BOOTCOMMAND \
	"setenv gpio_button 148;" \
	"if run check_button;" \
	"then poweroff; fi;" \
	"setenv gpio_button 132;" \
	"if run check_button;" \
	"then bootmenu; fi;" \
	"run bootcmd_mmc1;" \
	"run bootcmd_mmc0;" \
	"poweroff;"

#endif /* __CONFIG_H */
