.section .text
.globl _start

_start:
	/* sp has num params + 1, first param is exe */
	/* then there's array of pointers to strings */

	lw a0, 12(sp)

	call _printf

/*	lw	a0, 0(sp)
	call _dump*/
	
	jal _exit

_dump:
	la	 a1, helloworld
	call _itoa
	la	 a0, helloworld
	call _printf
	ret

/* calc string length */
_printf:
	/*la	 a0, helloworld*/
	addi t2, a0, 0 /* store param */

	addi s0, ra, 0 /* store return address */	
	call _strlen
	addi ra, s0, 0
	addi a2, a0, 0

	addi a0, x0, 0

	addi a1, t2, 0
	/*la	 a1, helloworld*/

	addi a7, x0, 64
	ecall
	ret

_exit:
	addi a0, x0, 0
	addi a7, x0, 93
	ecall

_itoa:
	/* export to hex */
	/* a0 = value */
	/* a1 = mem address of buffer */
	/* no loop, fixed size? */
	/*'0x12345678'*/
	addi t0, zero, '0'
	sb t0, 0(a1)
	addi t0, zero, 'x'
	sb t0, 1(a1)
	addi t0, zero, 0 /* string term */
	sb t0, 10(a1)

	addi t1, a1, 1 /* stop writing spot */
	addi a1, a1, 9 /* t1 := last digit  */
	addi t2, zero, 10
_itoa_loop:
	/* take lowest 4 bits and shift */
	/* 0x100dc */
	andi t0, a0, 15
	/* digit or char? */
	blt t0, t2, _itoa_base /* digit */
	addi t0, t0, 'a' - '0' - 10

_itoa_base:
	addi t0, t0, '0'

	sb t0, 0(a1)

	srli a0, a0, 4 /* shift 4 bits */
	addi a1, a1, -1 /* move char pointer back */
	bne t1, a1, _itoa_loop
	addi a0, a1, -9 /* restore original ptr */
	ret

_strlen:
	addi sp, sp, -16 /* extend stack */
	sw 	 s0, 16(sp) /* store fp */
	sw	 ra, 12(sp) /* store ra */
	addi s0, sp, 8 /* fp is start of local env? */

	addi t0, a0, 0 
	addi a0, x0, 0 

_cntloop:
	lb t1, 0(t0) 
	beq t1, zero, _afterloop
	addi t0, t0, 1
	addi a0, a0, 1
	jal zero, _cntloop
_afterloop:

	lw ra, 12(sp)
	lw s0, 16(sp)	
	addi sp, sp, 16 /* trim stack */

	ret

.data
helloworld: .ascii "Hello World!\n"
