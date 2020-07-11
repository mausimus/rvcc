.section .text
.globl _start

_start:
	la	 a0, outfile
	addi a1, zero, 65
	addi a7, x0, 1024
	addi a2, zero, 00700
	ecall
	addi t1, a0, 0 /* outfile desc */

	la	 a0, infile
	addi a1, zero, 0
	addi a7, x0, 1024
	ecall
	addi t0, a0, 0 /* infile desc */

	/* read 5 bytes */
	addi a0, t0, 0
	la a1, buffer
	addi a2, zero, 5
	addi a7, x0, 63
	ecall

	/* write 5 bytes */
	addi a0, t1, 0
	la a1, buffer
	addi a2, zero, 5
	addi a7, x0, 64
	ecall

	/* close */
	addi a0, t1, 0
	addi a7, x0, 57
	ecall

	/* close */
	addi a0, t0, 0
	addi a7, x0, 57
	ecall

_exit:
	addi a0, x0, 0
	addi a7, x0, 93
	ecall

.data
infile: .ascii "test1.txt\0"
outfile: .ascii "test2.txt\0"
buffer: .ascii "willthiswork\0"


