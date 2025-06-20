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
	# Get the ELF header.
	la t0, _kernel_start_phys

	# Get the entrypoint address out of it.
	ld t1, 24(t0)
	add t1, t0, t1

	# Load the address of the root page table.
	la t0, boot_pgtbl_2_0000000000000000

	# Transform the address of the root page table to a value we can use
	# with the SATP register to enable Sv39 paging.
	srl t0, t0, 12
	li t2, -1
	sll t3, t2, 63
	or t0, t0, t3

	# Prepare to fault to the entrypoint.
	sll t2, t2, 30
	csrw stvec, t2

	# Load the new page tables, which unmap the current code.
	sfence.vma
	csrw satp, t0
	sfence.vma

	# Jump to the entrypoint, in case we didn't fault.
	jr t1
