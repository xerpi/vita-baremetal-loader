	.cpu cortex-a9
	.align 4
	.code 32

	.text

	.global _start
_start:

PAYLOAD_SRC_PADDR: .word 0
PAYLOAD_DST_PADDR: .word 0
PAYLOAD_SIZE: .word 0

stage1_start:
	# Disable interrupts and enter System mode
	cpsid aif, #0x1F

	# DACR unrestricted
	mov r0, #0xFFFFFFFF
	mcr p15, 0, r0, c3, c0, 0

	# Load the payload args
	ldr r0, PAYLOAD_SRC_PADDR
	ldr r1, PAYLOAD_DST_PADDR
	ldr r2, PAYLOAD_SIZE
	mov lr, r1

	# Disable MMU and Dcache
	mrc p15, 0, r3, c1, c0, 0
	bic r3, #0b101
	mcr p15, 0, r3, c1, c0, 0
	isb

	# Copy the payload to its destination address
copy:
	ldr r3, [r0], #4
	str r3, [r1], #4
	sub r2, #4
	cmp r2, #0
	bne copy

	# Jump to the payload
	bx lr
