// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  (C) Copyright 2022
 *  Svyatoslav Ryhel <clamor95@gmail.com>
 */

#include <common.h>
#include <env.h>
#include <fdt_support.h>
#include <linux/delay.h>
#include <asm/gpio.h>
#include <asm/io.h>

#include <asm/arch/clock.h>
#include <asm/arch/tegra.h>
#include <asm/arch/gpio.h>

#define TEGRA_FUSE_BASE		0x7000F800
#define TEGRA_CAR_BASE		0x60006000

#define FUSE_VENDOR_CODE	0x200
#define FUSE_FAB_CODE		0x204
#define FUSE_LOT_CODE_0		0x208
#define FUSE_LOT_CODE_1		0x20c
#define FUSE_WAFER_ID		0x210
#define FUSE_X_COORDINATE	0x214
#define FUSE_Y_COORDINATE	0x218

#define FUSE_VENDOR_CODE_MASK	0xf
#define FUSE_FAB_CODE_MASK	0x3f
#define FUSE_WAFER_ID_MASK	0x3f
#define FUSE_X_COORDINATE_MASK	0x1ff
#define FUSE_Y_COORDINATE_MASK	0x1ff

static u32 tegra_fuse_readl(unsigned long offset)
{
	return readl(TEGRA_FUSE_BASE + offset);
}

static void tegra_set_fuse(void)
{
	u32 reg;

	gpio_request(TEGRA_GPIO(K, 7), "fuse_en");
	gpio_direction_output(TEGRA_GPIO(K, 7), 0);
	gpio_set_value(TEGRA_GPIO(K, 7), 1);

	/*
	 * Performed by downstream and is not
	 * documented by TRM. Whithout setting
	 * this bit fuse region will not work.
	 */
	reg = readl_relaxed(TEGRA_CAR_BASE + 0x48);
	reg |= BIT(28);
	writel(reg, TEGRA_CAR_BASE + 0x48);

	clock_enable(PERIPH_ID_FUSE);
	udelay(2);
	reset_set_enable(PERIPH_ID_FUSE, 0);
}

static unsigned long long tegra_chip_uid(void)
{
	u64 uid = 0ull;
	u32 reg;
	u32 cid;
	u32 vendor;
	u32 fab;
	u32 lot;
	u32 wafer;
	u32 x;
	u32 y;
	u32 i;

	tegra_set_fuse();

	/* This used to be so much easier in prior chips. Unfortunately, there
	   is no one-stop shopping for the unique id anymore. It must be
	   constructed from various bits of information burned into the fuses
	   during the manufacturing process. The 64-bit unique id is formed
	   by concatenating several bit fields. The notation used for the
	   various fields is <fieldname:size_in_bits> with the UID composed
	   thusly:
	   <CID:4><VENDOR:4><FAB:6><LOT:26><WAFER:6><X:9><Y:9>
	   Where:
		Field    Bits  Position Data
		-------  ----  -------- ----------------------------------------
		CID        4     60     Chip id
		VENDOR     4     56     Vendor code
		FAB        6     50     FAB code
		LOT       26     24     Lot code (5-digit base-36-coded-decimal,
					re-encoded to 26 bits binary)
		WAFER      6     18     Wafer id
		X          9      9     Wafer X-coordinate
		Y          9      0     Wafer Y-coordinate
		-------  ----
		Total     64
	*/

	/* chip id is 0 for tegra 3 */
	cid = 0;

	vendor = tegra_fuse_readl(FUSE_VENDOR_CODE) & FUSE_VENDOR_CODE_MASK;
	fab = tegra_fuse_readl(FUSE_FAB_CODE) & FUSE_FAB_CODE_MASK;

	/* Lot code must be re-encoded from a 5 digit base-36 'BCD' number
	   to a binary number. */
	lot = 0;
	reg = tegra_fuse_readl(FUSE_LOT_CODE_0) << 2;

	for (i = 0; i < 5; ++i) {
		u32 digit = (reg & 0xFC000000) >> 26;
		lot *= 36;
		lot += digit;
		reg <<= 6;
	}

	wafer = tegra_fuse_readl(FUSE_WAFER_ID) & FUSE_WAFER_ID_MASK;
	x = tegra_fuse_readl(FUSE_X_COORDINATE) & FUSE_X_COORDINATE_MASK;
	y = tegra_fuse_readl(FUSE_Y_COORDINATE) & FUSE_Y_COORDINATE_MASK;

	uid = ((unsigned long long)cid  << 60ull)
	    | ((unsigned long long)vendor << 56ull)
	    | ((unsigned long long)fab << 50ull)
	    | ((unsigned long long)lot << 24ull)
	    | ((unsigned long long)wafer << 18ull)
	    | ((unsigned long long)x << 9ull)
	    | ((unsigned long long)y << 0ull);

	return uid;
}

#if defined(CONFIG_OF_LIBFDT) && defined(CONFIG_OF_BOARD_SETUP)
int ft_board_setup(void *blob, struct bd_info *bd)
{
	/* First 3 bytes refer to LG vendor */
	u8 btmacaddr[6] = { 0x00, 0x00, 0x00, 0xD0, 0xC9, 0x88 };

	/* Generate device 3 bytes based on chip sd */
	u64 bt_device = tegra_chip_uid() >> 24ull;

	btmacaddr[0] =  (bt_device >> 1 & 0x0F) |
			(bt_device >> 5 & 0xF0);
	btmacaddr[1] =  (bt_device >> 11 & 0x0F) |
			(bt_device >> 17 & 0xF0);
	btmacaddr[2] =  (bt_device >> 23 & 0x0F) |
			(bt_device >> 29 & 0xF0);

	/* Set BT MAC address */
	fdt_find_and_setprop(blob, "/serial@70006200/bluetooth",
				   "local-bd-address", btmacaddr, 6, 1);

	/* Remove TrustZone nodes */
	fdt_del_node_and_alias(blob, "/firmware");
	fdt_del_node_and_alias(blob, "/reserved-memory/trustzone@bfe00000");

	return 0;
}
#endif

void nvidia_board_late_init(void)
{
	char serialno_str[17];

	/* Set chip id as serialno */
	sprintf(serialno_str, "%016llx", tegra_chip_uid());
	env_set("serial#", serialno_str);
	env_set("platform", "Tegra 3 T30");
}
