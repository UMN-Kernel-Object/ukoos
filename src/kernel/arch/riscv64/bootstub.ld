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
		_bootstub_start_phys = .;
		*(.text)
		*(.rodata)
		*(.data)
		*(.bss)
		. = ALIGN(0x1000);
		_bootstub_end_phys = .;
		ASSERT(
			_bootstub_end_phys == _bootstub_start_phys + 4096,
			".bootstub page was too large; fix it or change generate_bootstub.py"
		);
	}

	/**
	 * This gets set after the .bootstub page, since we never return back
	 * to it or use it in any way.
	 */
	_kernel_start_phys = .;

	/**
	 * The page tables get appended as well.
	 */
	.pgtbls ALIGN(0x1000) :
	{
		*(.pgtbls)
	}

	/**
	 * The kernel's string and symbol table sections get appended.
	 */
	.kernel_tables ALIGN(0x1000) :
	{
		*(.kernel_tables)
	}

	/**
	 * The kernel gets appended as a blob.
	 */
	.kernel ALIGN(0x1000) :
	{
		*(.kernel)
	}
	.kernel_bss :
	{
		*(.kernel_bss)
	}

	/**
	 * An initial stack page, to use in early boot (i.e., before the allocator is up).
	 */
	.stack ALIGN(0x1000) :
	{
		. += 0x1000;
	}

	_kernel_end_phys = .;
}
