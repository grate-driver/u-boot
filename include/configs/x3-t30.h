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

#define X3_SD_LOAD_KERNEL \
	"echo Loading Kernel;" \
	"if load mmc ${bootdev}:${bootpart} ${kernel_addr_r} ${kernel_file};" \
	"then echo Loading DTB;" \
		"load mmc ${bootdev}:${bootpart} ${fdt_addr_r} ${fdtfile};" \
		"fdt addr ${fdt_addr_r};" \
		"fdt rm /firmware;" \
		"fdt rm /reserved-memory/trustzone@bfe00000;" \
		"echo Loading Initramfs;" \
		"if load mmc ${bootdev}:${bootpart} ${ramdisk_addr_r} ${ramdisk_file};" \
		"then echo Booting Kernel;" \
			"run bootrdkernel;" \
		"else echo Booting Kernel;" \
			"run bootkernel; fi;"

#ifdef CONFIG_DEVICE_P880
/* High-level configuration options */
#define CONFIG_TEGRA_BOARD_STRING	"LG Optimus 4X HD"

#define X3_BOOTMENU_3_4 \
	"bootmenu_3=mount microsd=usb start && ums 0 mmc 1; bootmenu\0" \
	"bootmenu_4=mount /cache=usb start && ums 0 mmc 0:4; bootmenu\0"

#define X3_BOOTCOMMAND \
	"echo Loading from uSD...;" \
	"setenv bootargs console=ttyS0,115200n8 root=/dev/mmcblk1p2 rw gpt;" \
	"setenv bootdev 1;" \
	"setenv bootpart 1;" \
	X3_SD_LOAD_KERNEL \
	"else echo Loading from uSD failed!;" \
		"echo Loading from eMMC...;" \
		"setenv bootargs console=ttyS0,115200n8 root=/dev/mmcblk0p8 rw gpt;" \
		"setenv bootdev 0;" \
		"setenv bootpart 4;" \
		X3_SD_LOAD_KERNEL \
		"else echo Loading Kernel FAILED! Turning power off;" \
				"poweroff;" \
		"fi;" \
	"fi;"
#endif

#ifdef CONFIG_DEVICE_P895
/* High-level configuration options */
#define CONFIG_TEGRA_BOARD_STRING	"LG Optimus Vu"

#define X3_BOOTMENU_3_4 \
	"bootmenu_3=mount /boot=usb start && ums 0 mmc 0:4; bootmenu\0" \
	"bootmenu_4=mount /root=usb start && ums 0 mmc 0:8; bootmenu\0"

#define X3_BOOTCOMMAND \
	"echo Loading from eMMC...;" \
	"setenv bootargs console=ttyS0,115200n8 root=/dev/mmcblk0p8 rw gpt;" \
	"setenv bootdev 0;" \
	"setenv bootpart 4;" \
	X3_SD_LOAD_KERNEL \
	"else echo Loading Kernel FAILED! Turning power off;" \
			"poweroff;" \
	"fi;"
#endif

#define X3_BOOT_CUSTOM_KERNEL \
	"boot_custom_kernel=echo Loading Kernel;" \
		"load ${dev_type} ${mmcdev}:${mmcpart} ${kernel_addr_r} ${kernel_file};" \
		"echo Loading DTB;" \
		"load ${dev_type} ${mmcdev}:${mmcpart} ${fdt_addr_r} ${dtb_file};" \
		"fdt addr ${fdt_addr_r};" \
		"fdt rm /firmware;" \
		"fdt rm /reserved-memory/trustzone@bfe00000;" \
		"echo Loading Initramfs;" \
		"if load ${dev_type} ${mmcdev}:${mmcpart} ${ramdisk_addr_r} ${ramdisk_file};" \
		"then echo Booting Kernel;" \
			"run bootrdkernel;" \
		"else echo Booting Kernel;" \
			"run bootkernel; fi\0"

#define X3_BOOT_SCRIPT \
	X3_BOOT_CUSTOM_KERNEL \
	"boot_script=echo Loading boot script;" \
		"if load mmc 0:4 ${scriptaddr} uboot-x3.bcs;" \
		"then env import -t -r ${scriptaddr} ${filesize};" \
			"run boot_custom_kernel;" \
		"else if load mmc 1:1 ${scriptaddr} uboot-x3.bcs;" \
			"then env import -t -r ${scriptaddr} ${filesize};" \
				"run boot_custom_kernel;" \
			"else echo Boot Configuration NOT FOUND!;" \
				"echo Press ANY key to return to bootmenu;" \
				"continue; bootmenu; fi;" \
		"fi\0"

#define X3_BOOT_SOS \
	"boot_sos=echo Reading SOS partition;" \
		"mmc dev;" \
		"if mmc read ${kernel_addr_r} ${sos_offset_m} ${sos_size};" \
		"then echo Booting Kernel;" \
			"bootm ${kernel_addr_r};" \
		"else echo Reading SOS failed;" \
		"echo Press ANY key to return to bootmenu;" \
		"continue; bootmenu; fi\0"

#define X3_BOOT_LNX \
	"boot_lnx=echo Reading LNX partition;" \
		"mmc dev;" \
		"if mmc read ${kernel_addr_r} ${lnx_offset_m} ${lnx_size};" \
		"then echo Booting Kernel;" \
			"bootm ${kernel_addr_r};" \
		"else echo Reading LNX failed;" \
		"echo Press ANY key to return to bootmenu;" \
		"continue; bootmenu; fi\0"

#define X3_FLASH_UBOOT \
	"flash_uboot=echo Preparing RAM;" \
		"mw ${bct_addr_r} 0 ${bct_block_size_r};" \
		"mw ${ebt_addr_r} 0 ${ebt_block_size_r};" \
		"echo Reading BCT;" \
		"mmc dev 0 1;" \
		"mmc read ${bct_addr_r} 0 ${bct_block_size};" \
		"echo Reading bootloader;" \
		"if load mmc 0:4 ${ebt_addr_r} ${bootloader_file};" \
		"then echo Calculating bootloader size;" \
			"size mmc 0:4 ${bootloader_file};" \
			"ebtupdate ${bct_addr_r} ${ebt_addr_r} ${filesize};" \
			"echo Writing bootloader to eMMC;" \
			"mmc dev 0 1;" \
			"mmc write ${bct_addr_r} 0 ${bct_block_size};" \
			"mmc dev 0 2;" \
			"mmc write ${bct_addr_r} 0 ${bct_block_size};" \
			"mmc dev;" \
			"mmc write ${ebt_addr_r} ${ebt_block_shift} ${ebt_block_size};" \
			"echo Bootloader written successfully;" \
			"echo Press ANY key to reboot device; continue; reset;" \
		"else echo Reading bootloader failed;" \
			"echo Press ANY key to return to bootmenu; continue; bootmenu; fi\0"

#define X3_BOOTMENU \
	X3_BOOT_SCRIPT \
	X3_BOOT_SOS \
	X3_BOOT_LNX \
	X3_FLASH_UBOOT \
	"bootmenu_0=boot with script=run boot_script\0" \
	"bootmenu_1=boot LNX=run boot_lnx\0" \
	"bootmenu_2=boot SOS=run boot_sos\0" \
	X3_BOOTMENU_3_4 \
	"bootmenu_5=fastboot=echo Starting Fastboot protocol ...; fastboot usb 0; bootmenu\0" \
	"bootmenu_6=update bootloader=run flash_uboot\0" \
	"bootmenu_7=reboot RCM=enterrcm\0" \
	"bootmenu_8=reboot=reset\0" \
	"bootmenu_9=power off=poweroff\0" \
	"bootmenu_delay=-1\0"

#define X3_EMMC_LAYOUT \
	"bct_addr_r=0x81000000\0" \
	"bct_block_size_r=0x200000\0" \
	"bct_block_size=0x1000\0" \
	"ebt_addr_r=0x81200000\0" \
	"ebt_block_size_r=0x600000\0" \
	"ebt_block_size=0x3000\0" \
	"ebt_block_shift=0x1000\0" \
	"sos_offset_m=0x7000\0" \
	"sos_size=0x5000\0" \
	"lnx_offset_m=0xC000\0" \
	"lnx_size=0x5000\0"

#define BOARD_EXTRA_ENV_SETTINGS \
	"kernel_file=vmlinuz\0" \
	"ramdisk_file=uInitrd\0" \
	"bootloader_file=u-boot-dtb-tegra.bin\0" \
	"bootkernel=bootz ${kernel_addr_r} - ${fdt_addr_r}\0" \
	"bootrdkernel=bootz ${kernel_addr_r} ${ramdisk_addr_r} ${fdt_addr_r}\0" \
	"check_button=gpio input ${gpio_button}; test $? -eq 0\0" \
	X3_EMMC_LAYOUT \
	X3_BOOTMENU

#undef CONFIG_BOOTCOMMAND
#define CONFIG_BOOTCOMMAND \
	"setenv gpio_button 116;" \
	"if run check_button;" \
	"then bootmenu; else " \
	X3_BOOTCOMMAND \
	"fi;"

/* Board-specific serial config */
#define CONFIG_TEGRA_ENABLE_UARTD
#define CONFIG_SYS_NS16550_COM1		NV_PA_APB_UARTD_BASE

#include "tegra-common-post.h"

#endif /* __CONFIG_H */
