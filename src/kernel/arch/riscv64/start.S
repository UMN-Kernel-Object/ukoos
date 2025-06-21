.section .text

/**
 * The ELF entrypoint for the kernel. The bootstub uses this as a trap handler,
 * so it needs to be aligned to four bytes due to the layout of stvec.
 */
.p2align 2
.global _start
.type _start, @function
_start:
	mv ra, zero
	mv s0, zero
	tail main
