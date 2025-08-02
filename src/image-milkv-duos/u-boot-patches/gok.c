// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <command.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

static int do_gok(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	ulong	addr, fdt_addr, rc;

	if (argc != 2 && argc != 3)
		return CMD_RET_USAGE;

	addr = hextoul(argv[1], NULL);
	ulong (*entry)(ulong hartid, ulong fdt) = (void*)addr;
	if(argc == 3)
		fdt_addr = hextoul(argv[2], NULL);
	else
		fdt_addr = env_get_hex("fdt_addr", 0);

	printf("## Starting kernel at 0x%08lX ...\n", addr);

	cleanup_before_linux();
	rc = entry(gd->arch.boot_hart, fdt_addr);

	printf ("## Kernel terminated, rc = 0x%lX\n", rc);
	return 1;
}

/* -------------------------------------------------------------------- */

U_BOOT_CMD(
	gok, 3, 1,	do_gok,
	"start kernel at address 'addr'",
	"addr [fdt-addr]\n    - start kernel at address 'addr'"
);
