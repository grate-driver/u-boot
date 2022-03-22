// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _BCT_H_
#define _BCT_H_

/*
 * Defines the BCT parametres for T30
 */
#define BCT_LENGTH		0x17E0
#define BCT_HASH		0x10
#define EBT_ALIGNMENT		0x10

/*
 * Defines the CMAC-AES-128 hash length in 32 bit words. (128 bits = 4 words)
 */
#define NVBOOT_CMAC_AES_HASH_LENGTH		4

/*
 * Defines the maximum number of bootloader descriptions in the BCT.
 */
#define NVBOOT_MAX_BOOTLOADERS			4

typedef struct nv_bootloader_info_rec {
	u32 version;
	u32 start_blk;
	u32 start_page;
	u32 length;
	u32 load_addr;
	u32 entry_point;
	u32 attribute;
	u32 crypto_hash[NVBOOT_CMAC_AES_HASH_LENGTH];
} nv_bootloader_info;

typedef struct nvboot_config_table_rec {
	u32 unused0[4];
	u32 boot_data_version;
	u32 unused1[972];
	nv_bootloader_info bootloader[NVBOOT_MAX_BOOTLOADERS];
	u32 unused2[508];
} nvboot_config_table;

#endif /* _BCT_H_ */
