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
	# Stash s0 into SSCRATCH for now. We don't print SSCRATCH or depend on
	# its value, so it's fine to just clobber it.
	csrw sscratch, s0

	# Stash all the registers into unrecoverable_exception_registers,
	# except s0 (since we clobbered it).
	la s0, unrecoverable_exception_registers
	sd ra, 0(s0)
	sd sp, 8(s0)
	sd gp, 16(s0)
	sd tp, 24(s0)
	sd t0, 32(s0)
	sd t1, 40(s0)
	sd t2, 48(s0)
	# Skip s0 for now.
	sd s1, 64(s0)
	sd a0, 72(s0)
	sd a1, 80(s0)
	sd a2, 88(s0)
	sd a3, 96(s0)
	sd a4, 104(s0)
	sd a5, 112(s0)
	sd a6, 120(s0)
	sd a7, 128(s0)
	sd s2, 136(s0)
	sd s3, 144(s0)
	sd s4, 152(s0)
	sd s5, 160(s0)
	sd s6, 168(s0)
	sd s7, 176(s0)
	sd s8, 184(s0)
	sd s9, 192(s0)
	sd s10, 200(s0)
	sd s11, 208(s0)
	sd t3, 216(s0)
	sd t4, 224(s0)
	sd t5, 232(s0)
	sd t6, 240(s0)

	# Get the old value of s0 back, then store it.
	csrr a0, sscratch
	sd a0, 56(s0)

	# Print the "BLUF" registers: SEPC (printed as PC), RA, SCAUSE, STVAL.
	la a0, unrecoverable_exception_msg0
	call unrecoverable_exception_print_cstr
	csrr a0, sepc
	call unrecoverable_exception_print_hex

	la a0, unrecoverable_exception_msg1
	call unrecoverable_exception_print_cstr
	ld a0, (s0)
	call unrecoverable_exception_print_hex

	la a0, unrecoverable_exception_msg2
	call unrecoverable_exception_print_cstr
	csrr a0, scause
	call unrecoverable_exception_print_hex

	la a0, unrecoverable_exception_msg3
	call unrecoverable_exception_print_cstr
	csrr a0, stval
	call unrecoverable_exception_print_hex

	# Print all the registers, in a denser format.
	la s1, unrecoverable_exception_gpr_msgs
	mv s2, zero
1:
	add a0, s1, s2
	ld a0, (a0)
	call unrecoverable_exception_print_cstr
	add a0, s0, s2
	ld a0, (a0)
	call unrecoverable_exception_print_hex

	addi s2, s2, 8
	addi a0, s2, -248
	bnez a0, 1b

	# Print that we're halting, then halt.
	la a0, unrecoverable_exception_msg4
	call unrecoverable_exception_print_cstr

	# Halt.
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

.section .data

unrecoverable_exception_registers: .zero 248

.section .rodata

unrecoverable_exception_digits:
	.ascii "0123456789abcdef"
unrecoverable_exception_msg0:
	.byte 13, 10
	.byte 13, 10
	.ascii "ukoOS: Got an unrecoverable exception!"
	.byte 13, 10, 13, 10
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
	.byte 13, 10, 13, 10
	.ascii "Halting..."
	.byte 13, 10
	.byte 0

unrecoverable_exception_gpr_msgs:
	.quad unrecoverable_exception_ra
	.quad unrecoverable_exception_sp, unrecoverable_exception_gp
	.quad unrecoverable_exception_tp, unrecoverable_exception_t0
	.quad unrecoverable_exception_t1, unrecoverable_exception_t2
	.quad unrecoverable_exception_s0, unrecoverable_exception_s1
	.quad unrecoverable_exception_a0, unrecoverable_exception_a1
	.quad unrecoverable_exception_a2, unrecoverable_exception_a3
	.quad unrecoverable_exception_a4, unrecoverable_exception_a5
	.quad unrecoverable_exception_a6, unrecoverable_exception_a7
	.quad unrecoverable_exception_s2, unrecoverable_exception_s3
	.quad unrecoverable_exception_s4, unrecoverable_exception_s5
	.quad unrecoverable_exception_s6, unrecoverable_exception_s7
	.quad unrecoverable_exception_s8, unrecoverable_exception_s9
	.quad unrecoverable_exception_s10, unrecoverable_exception_s11
	.quad unrecoverable_exception_t3, unrecoverable_exception_t4
	.quad unrecoverable_exception_t5, unrecoverable_exception_t6

unrecoverable_exception_ra:
	.byte 13, 10, 13, 10
	.ascii "ZERO: 0x0000000000000000   RA: 0x"
	.byte 0
unrecoverable_exception_sp:
	.byte 13, 10
	.ascii "  SP: 0x"
	.byte 0
unrecoverable_exception_gp:
	.ascii "   GP: 0x"
	.byte 0
unrecoverable_exception_tp:
	.byte 13, 10
	.ascii "  TP: 0x"
	.byte 0
unrecoverable_exception_t0:
	.ascii "   T0: 0x"
	.byte 0
unrecoverable_exception_t1:
	.byte 13, 10
	.ascii "  T1: 0x"
	.byte 0
unrecoverable_exception_t2:
	.ascii "   T2: 0x"
	.byte 0
unrecoverable_exception_s0:
	.byte 13, 10
	.ascii "  S0: 0x"
	.byte 0
unrecoverable_exception_s1:
	.ascii "   S1: 0x"
	.byte 0
unrecoverable_exception_a0:
	.byte 13, 10
	.ascii "  A0: 0x"
	.byte 0
unrecoverable_exception_a1:
	.ascii "   A1: 0x"
	.byte 0
unrecoverable_exception_a2:
	.byte 13, 10
	.ascii "  A2: 0x"
	.byte 0
unrecoverable_exception_a3:
	.ascii "   A3: 0x"
	.byte 0
unrecoverable_exception_a4:
	.byte 13, 10
	.ascii "  A4: 0x"
	.byte 0
unrecoverable_exception_a5:
	.ascii "   A5: 0x"
	.byte 0
unrecoverable_exception_a6:
	.byte 13, 10
	.ascii "  A6: 0x"
	.byte 0
unrecoverable_exception_a7:
	.ascii "   A7: 0x"
	.byte 0
unrecoverable_exception_s2:
	.byte 13, 10
	.ascii "  S2: 0x"
	.byte 0
unrecoverable_exception_s3:
	.ascii "   S3: 0x"
	.byte 0
unrecoverable_exception_s4:
	.byte 13, 10
	.ascii "  S4: 0x"
	.byte 0
unrecoverable_exception_s5:
	.ascii "   S5: 0x"
	.byte 0
unrecoverable_exception_s6:
	.byte 13, 10
	.ascii "  S6: 0x"
	.byte 0
unrecoverable_exception_s7:
	.ascii "   S7: 0x"
	.byte 0
unrecoverable_exception_s8:
	.byte 13, 10
	.ascii "  S8: 0x"
	.byte 0
unrecoverable_exception_s9:
	.ascii "   S9: 0x"
	.byte 0
unrecoverable_exception_s10:
	.byte 13, 10
	.ascii " S10: 0x"
	.byte 0
unrecoverable_exception_s11:
	.ascii "  S11: 0x"
	.byte 0
unrecoverable_exception_t3:
	.byte 13, 10
	.ascii "  T3: 0x"
	.byte 0
unrecoverable_exception_t4:
	.ascii "   T4: 0x"
	.byte 0
unrecoverable_exception_t5:
	.byte 13, 10
	.ascii "  T5: 0x"
	.byte 0
unrecoverable_exception_t6:
	.ascii "   T6: 0x"
	.byte 0
