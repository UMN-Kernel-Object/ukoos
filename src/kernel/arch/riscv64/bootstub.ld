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
		. = ALIGN(0x1000);
		PROVIDE(_bootstub_end_phys = .);
		ASSERT(
			_bootstub_end_phys == _bootstub_start_phys + 4096,
			".bootstub page was too large; fix it or change generate_bootstub.py"
		);
	}

	/**
	 * The page tables get appended as well.
	 */
	.pgtbls ALIGN(0x1000) :
	{
		PROVIDE(_pgtbls_start_phys = .);
		*(.pgtbls)
		PROVIDE(_pgtbls_end_phys = .);
	}

	/**
	 * The kernel gets appended as a blob.
	 */
	.kernel ALIGN(0x1000) :
	{
		PROVIDE(_kernel_start_phys = .);
		*(.kernel)
	}
	.kernel_bss :
	{
		*(.kernel_bss)
		PROVIDE(_kernel_end_phys = .);
	}

	/**
	 * An initial stack page, to use in early boot (i.e., before the allocator is up).
	 */
	.stack ALIGN(0x1000) :
	{
		PROVIDE(_stack_start_phys = .);
		. += 0x1000;
		PROVIDE(_stack_end_phys = .);
	}
}
