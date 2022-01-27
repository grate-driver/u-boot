/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 *  (C) Copyright 2010,2012
 *  NVIDIA Corporation <www.nvidia.com>
 *
 *  (C) Copyright 2022
 *  Svyatoslav Ryhel <clamor95@gmail.com>
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>

#include "tegra30-common.h"
#include "transformer-common.h"

#ifdef CONFIG_TRANSFORMER_SPI_BOOT
#define TRANSFORMER_VOLDOWN_ACTION \
	"setenv gpio_button 222;" \
	"if run check_button;" \
	"then poweroff; fi;" \
	"setenv gpio_button 132;" \
	"if run check_button;" \
	"then echo Starting SPI flash update ...;" \
		"run update_spi;"
/* SPI */
#define CONFIG_TEGRA_SLINK_CTRLS       6
#define CONFIG_SPI_FLASH_SIZE          (4 << 20)
#else
#define TRANSFORMER_VOLDOWN_ACTION \
	"setenv gpio_button 150;" \
	"if run check_button;" \
	"then poweroff; fi;" \
	"setenv gpio_button 131;" \
	"if run check_button;" \
	"then bootmenu;"
#endif

#define BOARD_EXTRA_ENV_SETTINGS \
	TRANSFORMER_T30_EMMC_LAYOUT \
	TRANSFORMER_DEFAULT_FILESET \
	TRANSFORMER_BOOTZ \
	TRANSFORMER_BUTTON_CHECK \
	TRANSFORMER_BOOTMENU

#undef CONFIG_BOOTCOMMAND
#define CONFIG_BOOTCOMMAND \
	TRANSFORMER_VOLDOWN_ACTION \
	"else echo Loading from uSD...;" \
		"setenv bootdev 1;" \
		"setenv rootpart 2;" \
		TRANSFORMER_LOAD_KERNEL \
		"else echo Loading from uSD failed!;" \
			"echo Loading from eMMC...;" \
			"setenv bootdev 0;" \
			"setenv rootpart 8;" \
			TRANSFORMER_LOAD_KERNEL \
			"else echo Loading Kernel FAILED! Turning power off;" \
				"poweroff; fi;" \
		"fi;" \
	"fi;"

/* Board-specific serial config */
#define CONFIG_TEGRA_ENABLE_UARTA
#define CONFIG_SYS_NS16550_COM1		NV_PA_APB_UARTA_BASE

/* Used for downstream */
#define CONFIG_MACH_TYPE		MACH_TYPE_CARDHU

#include "tegra-common-post.h"

#endif /* __CONFIG_H */
