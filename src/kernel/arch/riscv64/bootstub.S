.section .text

# The entrypoint to the bootstub.
#
# Expects the following arguments:
#
# a0: The hart ID.
# a1: The DeviceTree pointer.
.p2align 2
.global _start
.type _start, @function
_start:
	# Set up the initial stack.
	la sp, 0xffffffffffffd000

	# Get the (physical) address of the root page table.
	la t0, boot_pgtbl_2_0000000000000000

	# Transform the address of the root page table to a value we can use
	# with the SATP register to enable Sv39 paging.
	srl t0, t0, 12
	li t1, -1
	sll t2, t1, 63
	or t0, t0, t2

	# Get the (virtual) address of the higher-half entrypoint.
	ld t1, (entrypoint)

	# Load the symbol table and string tables' addresses and lengths.
	ld a2, (symtab_va)
	ld a3, (symtab_len)
	ld a4, (strtab_va)
	ld a5, (strtab_len)

	# Prepare to fault to the entrypoint.
	csrw stvec, t1

	# Load the new page tables, which unmap the current code.
	sfence.vma
	csrw satp, t0
	sfence.vma

	# Jump to the entrypoint, in case we didn't fault.
	jr t1
