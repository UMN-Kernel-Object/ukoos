ENTRY(_start)
OUTPUT_FORMAT(elf64-littleriscv)

SECTIONS
{
	/**
	 * We store the kernel in the top 1GiB of virtual memory. This lets us
	 * be relatively agnostic to what the lowest address of himem is, which
	 * would depend on whether we're using Sv39, Sv48, Sv57, etc.
	 */
	. = 0xffffffffc0000000;

	PROVIDE(_kernel_start = .);

	.text ALIGN(0x1000) :
	{
		PROVIDE(_kernel_start_text = .);
		*(.text .text.*)
		PROVIDE(_kernel_end_text = .);
	}

	/**
	 * Unlike some linker scripts, we separate .text from .rodata. This is
	 * only a very weak mitigation against code reuse attacks, but it seems
	 * better to not have the possibility of reusing constants as code.
	 */
	.rodata ALIGN(0x1000) :
	{
		PROVIDE(_kernel_start_rodata = .);
		*(.rodata .rodata.*)
		PROVIDE(_kernel_end_rodata = .);
	}

	.data ALIGN(0x1000) :
	{
		PROVIDE(_kernel_start_data = .);
		*(.data .data.*)
		PROVIDE(_kernel_end_data = .);
	}
	.bss :
	{
		PROVIDE(_kernel_start_bss = .);
		*(.bss .bss.*)
		PROVIDE(_kernel_end_bss = .);
	}

	PROVIDE(_kernel_end = .);

	PROVIDE(kernel_pgtbl2 = 0xffffffffffffd000);
}
