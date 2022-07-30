// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#include <common.h>
#include <command.h>

static int do_continue(struct cmd_tbl *cmdtp, int flag, int argc,
		   char *const argv[])
{
	while (true) {
		if (tstc())
			break;
	}

	return 0;
}

static int do_exit(struct cmd_tbl *cmdtp, int flag, int argc,
		   char *const argv[])
{
	if (argc > 1)
		return dectoul(argv[1], NULL);

	return 0;
}

U_BOOT_CMD(
	continue,	1,	0,	do_continue,
	"contimue script on key pressed",
	""
);

U_BOOT_CMD(
	exit,	2,	1,	do_exit,
	"exit script",
	""
);
