	.cpu cortex-a9
	.align 4
	.code 32

	.text

	.global _start
_start:
	# Disable interrupts and enter System mode
	cpsid aif, #0x1F

	# DACR unrestricted
	mov r0, #0xFFFFFFFF
	mcr p15, 0, r0, c3, c0, 0

	# Disable MMU and Dcache
	mrc p15, 0, r0, c1, c0, 0
	bic r0, #0b101
	mcr p15, 0, r0, c1, c0, 0
	isb

	# Read CPU ID
	mrc p15, 0, r0, c0, c0, 5
	and r0, r0, #7

	# Setup the SP at the end of the scratchpad
	mov sp, #0x00008000
	sub sp, r0, lsl #10

	# Jump to the C code
	b main

# Variables
cpu_sync: .word 0

	.ltorg
