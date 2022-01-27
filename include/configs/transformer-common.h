/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2022, Svyatoslav Ryhel <clamor95@gmail.com>.
 */

#ifndef __TRANSFORMER_COMMON_H
#define __TRANSFORMER_COMMON_H

/* High-level configuration options */
#define CONFIG_TEGRA_BOARD_STRING	"ASUS Transformer"

/*
 * SOS and LNX offset is relative to
 * mmcblk0 start on both t20 and t30
 */

#define TRANSFORMER_T20_EMMC_LAYOUT \
	"bct_addr_r=0x1000000\0" \
	"bct_size=0xFF0\0" \
	"bct_block_size=0x100000\0" \
	"ebt_addr_r=0x1100000\0" \
	"ebt_block_size=0x300000\0" \
	"sos_offset_m=0x1C00\0" \
	"sos_size=0x2800\0" \
	"lnx_offset_m=0x4400\0" \
	"lnx_size=0x4000\0"

#define TRANSFORMER_T30_EMMC_LAYOUT \
	"spi_size=0x400000\0" \
	"bct_addr_r=0x81000000\0" \
	"bct_size=0x17F0\0" \
	"bct_block_size=0x100000\0" \
	"ebt_addr_r=0x81100000\0" \
	"ebt_block_size=0x300000\0" \
	"sos_offset_m=0x3C00\0" \
	"sos_size=0x4000\0" \
	"lnx_offset_m=0x7C00\0" \
	"lnx_size=0x4000\0"

#define TRANSFORMER_BOOTZ \
	"bootkernel=bootz ${kernel_addr_r} - ${fdt_addr_r}\0" \
	"bootrdkernel=bootz ${kernel_addr_r} ${ramdisk_addr_r} ${fdt_addr_r}\0"

#define TRANSFORMER_BOOT_CUSTOM_KERNEL \
	"boot_custom_kernel=echo Loading DTB;" \
		"load ${dev_type} ${mmcdev}:${mmcpart} ${fdt_addr_r} ${dtb_file};" \
		"echo Loading Kernel;" \
		"load ${dev_type} ${mmcdev}:${mmcpart} ${kernel_addr_r} ${kernel_file};" \
		"echo Loading Initramfs;" \
		"if load ${dev_type} ${mmcdev}:${mmcpart} ${ramdisk_addr_r} ${ramdisk_file};" \
		"then echo Booting Kernel;" \
			"run bootrdkernel;" \
		"else echo Booting Kernel;" \
			"run bootkernel; fi\0"

#define TRANSFORMER_BOOT_SCRIPT \
	TRANSFORMER_BOOT_CUSTOM_KERNEL \
	"boot_script=echo Loading boot script;" \
		"for i in 1 2 3 4 5 6 7 8 9 10;" \
		"do; if load mmc 1:$i ${scriptaddr} uboot-transformer.bcs;" \
		"then env import -t -r ${scriptaddr} ${filesize};" \
			"run boot_custom_kernel; fi;" \
		"done;" \
		"for i in 1 2 3 4 5 6 7 8 9 10;" \
		"do; if load mmc 0:$i ${scriptaddr} uboot-transformer.bcs;" \
		"then env import -t -r ${scriptaddr} ${filesize};" \
			"run boot_custom_kernel; fi;" \
		"done;" \
		"echo Boot Configuration NOT FOUND!;" \
		"echo Press ANY key to return to bootmenu;" \
		"continue; bootmenu\0"

#define TRANSFORMER_BOOT_SOS \
	"boot_sos=echo Reading SOS partition;" \
		"mmc dev;" \
		"if mmc read ${kernel_addr_r} ${sos_offset_m} ${sos_size};" \
		"then echo Booting Kernel;" \
			"bootm ${kernel_addr_r};" \
		"else echo Reading SOS failed;" \
		"echo Press ANY key to return to bootmenu;" \
		"continue; bootmenu; fi\0"

#define TRANSFORMER_BOOT_LNX \
	"boot_lnx=echo Reading LNX partition;" \
		"mmc dev;" \
		"if mmc read ${kernel_addr_r} ${lnx_offset_m} ${lnx_size};" \
		"then echo Booting Kernel;" \
			"bootm ${kernel_addr_r};" \
		"else echo Reading LNX failed;" \
		"echo Press ANY key to return to bootmenu;" \
		"continue; bootmenu; fi\0"

#define TRANSFORMER_BRICKSAFE_HOOK \
	"bricksafe_hook=echo Loading bricksafe.img;" \
		"if load mmc 1:1 0x81000000 bricksafe.img;" \
		"then echo Restoring bricksafe.img;" \
			"mmc dev 0 1;" \
			"mmc write 0x81000000 0 0x1000;" \
			"mmc dev 0 2;" \
			"mmc write 0x81200000 0 0x1000;" \
			"mmc dev;" \
			"mmc write 0x81400000 0 0x3C00;" \
			"echo Restoration of bricksafe.img completed;" \
			"echo Rebooting...;" \
			"sleep 3;" \
			"reset;" \
		"else echo Reading bricksafe.img;" \
			"mmc dev 0 1;" \
			"mmc read 0x81000000 0 0x1000;" \
			"mmc dev 0 2;" \
			"mmc read 0x81200000 0 0x1000;" \
			"mmc dev;" \
			"mmc read 0x81400000 0 0x3C00;" \
			"if fatwrite mmc 1:1 0x81000000 bricksafe.img 0xB80000;" \
			"then echo bricksafe.img dumped successfully;" \
				"echo Press ANY key to turn off device; continue; poweroff;" \
			"else bricksafe.img dump FAILED! ABORTING...;" \
				"echo Press ANY key to return to bootmenu; continue; bootmenu; fi; fi\0"

#define TRANSFORMER_FLASH_UBOOT \
	"flash_uboot=echo Preparing RAM;" \
		"mw ${bct_addr_r} 0 0x100000;" \
		"echo Reading BCT;" \
		"mmc dev 0 1;" \
		"mmc read ${bct_addr_r} 0 0x10;" \
		"echo Reading bootloader;" \
		"if load mmc 1:1 ${ebt_addr_r} ${bootloader_file};" \
		"then echo Calculating bootloader size;" \
			"size mmc 1:1 ${bootloader_file};" \
			"ebtupdate ${bct_addr_r} ${ebt_addr_r} ${filesize};" \
			"echo Writing bootloader to eMMC;" \
			"mmc dev 0 1;" \
			"mmc write ${bct_addr_r} 0 0x10;" \
			"mmc dev 0 2;" \
			"mmc write ${ebt_addr_r} 0xC00 0x400;" \
			"mmc dev;" \
			"mmc write 0x81180000 0 0x1400;" \
			"echo Bootloader written successfully;" \
			"echo Press ANY key to reboot device; continue; reset;" \
		"else echo Reading bootloader failed;" \
			"echo Press ANY key to return to bootmenu; continue; bootmenu; fi\0"

#define TRANSFORMER_FLASH_SPI \
	"update_spi=sf probe 0:1;" \
		"echo Dumping current SPI flash content ...;" \
		"sf read ${bct_addr_r} 0x0 ${spi_size};" \
		"if fatwrite mmc 1:1 ${bct_addr_r} spi-flash-backup.bin ${spi_size};" \
		"then echo SPI flash content was successfully written into spi-flash-backup.bin;" \
			"echo Reading SPI flash binary;" \
			"if load mmc 1:1 ${bct_addr_r} bootloader-update.bin;" \
			"then echo Writing bootloader into SPI flash;" \
				"sf probe 0:1;" \
				"sf update ${bct_addr_r} 0x0 ${spi_size};" \
				"poweroff;" \
			"else echo Preparing RAM;" \
				"mw ${bct_addr_r} 0 0x100000;" \
				"echo Reading BCT;" \
				"sf read ${bct_addr_r} 0x0 ${bct_size};" \
				"echo Reading bootloader;" \
				"if load mmc 1:1 ${ebt_addr_r} ${bootloader_file};" \
				"then echo Calculating bootloader size;" \
					"size mmc 1:1 ${bootloader_file};" \
					"ebtupdate ${bct_addr_r} ${ebt_addr_r} ${filesize};" \
					"echo Writing bootloader into SPI flash;" \
					"sf probe 0:1;" \
					"sf update ${bct_addr_r} 0x0 ${spi_size};" \
					"echo Bootloader written successfully; reset;" \
				"else echo Reading bootloader failed;" \
					"poweroff; fi;" \
			"fi;" \
		"else echo SPI flash backup FAILED! Aborting ...;" \
			"poweroff; fi\0"

#define TRANSFORMER_REFRESH_USB \
	"refresh_usb=usb start; usb reset; usb tree; usb info;" \
		"echo Press ANY key to return to bootmenu;" \
		"continue; bootmenu\0"

#define TRANSFORMER_FASTBOOT_ALIAS \
	"fastboot_raw_partition_boot=${lnx_offset_m} ${lnx_size} mmcpart 0\0" \
	"fastboot_raw_partition_recovery=${sos_offset_m} ${sos_size} mmcpart 0\0" \
	"fastboot_partition_alias_system=APP\0" \
	"fastboot_partition_alias_cache=CAC\0" \
	"fastboot_partition_alias_misc=MSC\0" \
	"fastboot_partition_alias_staging=USP\0" \
	"fastboot_partition_alias_vendor=VDR\0" \
	"fastboot_partition_alias_userdata=UDA\0"

#define TRANSFORMER_BOOTMENU \
	TRANSFORMER_BOOT_SCRIPT \
	TRANSFORMER_BOOT_SOS \
	TRANSFORMER_BOOT_LNX \
	TRANSFORMER_BRICKSAFE_HOOK \
	TRANSFORMER_FLASH_UBOOT \
	TRANSFORMER_FLASH_SPI \
	TRANSFORMER_REFRESH_USB \
	TRANSFORMER_FASTBOOT_ALIAS \
	"bootmenu_0=boot with script=run boot_script\0" \
	"bootmenu_1=boot LNX=run boot_lnx\0" \
	"bootmenu_2=boot SOS=run boot_sos\0" \
	"bootmenu_3=fastboot=echo Starting Fastboot protocol ...; fastboot usb 0; bootmenu\0" \
	"bootmenu_4=bricksafe=run bricksafe_hook\0" \
	"bootmenu_5=update bootloader=run flash_uboot\0" \
	"bootmenu_6=refresh USB=run refresh_usb\0" \
	"bootmenu_7=reboot RCM=enterrcm\0" \
	"bootmenu_8=reboot=reset\0" \
	"bootmenu_9=power off=poweroff\0" \
	"bootmenu_delay=-1\0"

#define TRANSFORMER_BUTTON_CHECK \
	"check_button=gpio input ${gpio_button}; test $? -eq 0;\0"

#define TRANSFORMER_DEFAULT_FILESET \
	"kernel_file=vmlinuz\0" \
	"ramdisk_file=uInitrd\0" \
	"bootloader_file=u-boot-dtb-tegra.bin\0"

#define TRANSFORMER_LOAD_KERNEL \
	"echo Loading Kernel;" \
	"if load mmc ${bootdev}:1 ${kernel_addr_r} ${kernel_file};" \
	"then echo Loading DTB;" \
		"load mmc ${bootdev}:1 ${fdt_addr_r} ${fdtfile};" \
		"fdt addr ${fdt_addr_r};" \
		"fdt rm /firmware;" \
		"fdt rm /reserved-memory/trustzone@bfe00000;" \
		"setenv bootargs console=ttyS0,115200n8 root=/dev/mmcblk${bootdev}p${rootpart} rw gpt;" \
		"echo Loading Initramfs;" \
		"if load mmc ${bootdev}:1 ${ramdisk_addr_r} ${ramdisk_file};" \
		"then echo Booting Kernel;" \
			"run bootrdkernel;" \
		"else echo Booting Kernel;" \
			"run bootkernel; fi;"

#endif /* __CONFIG_H */
