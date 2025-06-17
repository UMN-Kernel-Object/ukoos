ENTRY(_start)
OUTPUT_FORMAT(elf64-littleriscv)

SECTIONS
{
	. = 0x80080000;

	/**
	 * We mix up all the sections of the bootstub, since it won't run with
	 * page-level protection anyways.
	 */
	.bootstub :
	{
		PROVIDE(_bootstub_start_phys = .);
		*(.text)
		*(.rodata)
		*(.data)
		*(.bss)
		PROVIDE(_bootstub_end_phys = .);
	}

	/**
	 * The kernel then gets appended as a blob.
	 */
	.kernel ALIGN(0x1000) :
	{
		PROVIDE(_kernel_start_phys = .);
		*(.kernel)
		PROVIDE(_kernel_end_phys = .);
	}
}
