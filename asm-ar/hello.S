.text
.global _start

_start:
	mov r0, #1
	add r1, pc, #16
	mov r2, #12
	mov r7, #4
	swi #0

_exit:
	mov r7, #1
	swi #0

_string:
.asciz "Hello World\n"

