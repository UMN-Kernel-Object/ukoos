.section .text

/**
 * The ELF entrypoint for the kernel. The bootstub uses this as a trap handler,
 * so it needs to be aligned to four bytes due to the layout of stvec.
 */
.p2align 2
.global _start
.type _start, @function
_start:
	# Disable interrupts.
	csrr t0, sstatus
	and t0, t0, ~(1 << 1)
	csrw sstatus, t0

	# Set up the early-exception trap handler.
	la t0, unrecoverable_exception
	csrw stvec, t0

	# Clear the return address and frame pointer.
	mv ra, zero
	mv fp, zero

	# Call into main.
	tail main

.section .text

/**
 * A handler for an unrecoverable exception.
 */
.p2align 2
.global unrecoverable_exception
.type unrecoverable_exception, @function
unrecoverable_exception:
	mv s1, ra
	la a0, unrecoverable_exception_msg0
	call unrecoverable_exception_print_cstr
	csrr a0, sepc
	call unrecoverable_exception_print_hex
	la a0, unrecoverable_exception_msg1
	call unrecoverable_exception_print_cstr
	mv a0, s1
	call unrecoverable_exception_print_hex
	la a0, unrecoverable_exception_msg2
	call unrecoverable_exception_print_cstr
	csrr a0, scause
	call unrecoverable_exception_print_hex
	la a0, unrecoverable_exception_msg3
	call unrecoverable_exception_print_cstr
	csrr a0, stval
	call unrecoverable_exception_print_hex
	la a0, unrecoverable_exception_msg4
	call unrecoverable_exception_print_cstr
	tail _panic_halt

.type unrecoverable_exception_print_cstr, @function
unrecoverable_exception_print_cstr:
	mv t0, a0
	li a6, 0
	li a7, 1
1:
	lb a0, (t0)
	beqz a0, 2f
	ecall
	addi t0, t0, 1
	j 1b
2:
	ret

.type unrecoverable_exception_print_hex, @function
unrecoverable_exception_print_hex:
	mv t0, a0
	li t1, 16
	la t2, unrecoverable_exception_digits
	li a6, 0
	li a7, 1
1:
	beqz t1, 2f
	srli t3, t0, 60
	slli t0, t0, 4
	add t3, t2, t3
	lb a0, (t3)
	ecall
	addi t1, t1, -1
	j 1b
2:
	ret

.section .rodata

unrecoverable_exception_digits:
	.ascii "0123456789abcdef"
unrecoverable_exception_msg0:
	.ascii "ukoOS: Got an unrecoverable exception!"
	.byte 13, 10
	.ascii "    PC: 0x"
	.byte 0
unrecoverable_exception_msg1:
	.byte 13, 10
	.ascii "    RA: 0x"
	.byte 0
unrecoverable_exception_msg2:
	.byte 13, 10
	.ascii "SCAUSE: 0x"
	.byte 0
unrecoverable_exception_msg3:
	.byte 13, 10
	.ascii " STVAL: 0x"
	.byte 0
unrecoverable_exception_msg4:
	.byte 13, 10, 0
