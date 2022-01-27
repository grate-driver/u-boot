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

#define BOARD_EXTRA_ENV_SETTINGS \
	TRANSFORMER_T20_EMMC_LAYOUT \
	TRANSFORMER_DEFAULT_FILESET \
	TRANSFORMER_BOOTZ \
	TRANSFORMER_BUTTON_CHECK \
	TRANSFORMER_BOOTMENU

#undef CONFIG_BOOTCOMMAND
#define CONFIG_BOOTCOMMAND \
	"setenv gpio_button 148;" \
	"if run check_button;" \
	"then poweroff; fi;" \
	"setenv gpio_button 132;" \
	"if run check_button;" \
	"then bootmenu;" \
	"else echo Loading from uSD...;" \
		"setenv bootdev 1;" \
		"setenv rootpart 2;" \
		TRANSFORMER_LOAD_KERNEL \
		"else echo Loading from uSD failed!;" \
			"echo Loading from eMMC...;" \
			"setenv bootdev 0;" \
			"setenv rootpart 7;" \
			TRANSFORMER_LOAD_KERNEL \
			"else echo Loading Kernel FAILED! Turning power off;" \
				"poweroff; fi;" \
		"fi;" \
	"fi;"

/* Board-specific serial config */
#define CONFIG_TEGRA_ENABLE_UARTD
#define CONFIG_SYS_NS16550_COM1		NV_PA_APB_UARTD_BASE

/* Used for downstream */
#define CONFIG_MACH_TYPE		MACH_TYPE_VENTANA

#include "tegra-common-post.h"

#endif /* __CONFIG_H */
