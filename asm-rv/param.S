.section .text
.globl _start

_start:
	addi a7, a0, 0
	addi a0, a1, 0
	addi a1, a2, 0
	addi a2, a3, 0
	addi a3, a4, 0
	addi a4, a5, 0
	addi a5, a6, 0
	ecall

_exit:
	addi a0, x0, 0
	addi a7, x0, 93
	ecall

.data

