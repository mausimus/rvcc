## rvcc

Bootstrapped C compiler for 32-bit RISC-V ISA

### Features

* implements a subset of C language sufficient to compile itself
* generates a Linux- and emulator-compatible RV32IM ELF binary
* written in ANSI C it can cross-compile from any platform
* includes a minimal Linux C standard library for basic I/O
* simple two-pass compilation process: source -> IL -> binary
* lexer, parser and code generator all implemented by hand

### Bootstrapping

Bootstrapped compiler is a compiler that's able to compile its own source code, which is an important stepping stone
in confirming expressiveness. Since rvcc is written in ANSI C, we can use any compiler on any platform for the initial compilation before bootstrapping with the help of a RISC-V emulator.

![diagram](bootstrap.png)

The steps to validate rvcc's bootstrap are:

1. Compiler's source code is initially compiled using gcc which generates an x86 binary.

2. Built binary is invoked with its own source code as input and generates a RISC-V binary.

3. The RISC-V binary is invoked (via an emulator) with its own source code as input and generates another RISC-V binary.

4. If outputs generated in steps 2. and 3. are identical then bootstrap has been successful - the compiler regenerated itself from its own source code in two consecutive generations.

The bootstrap can be tested by running ```make bootstrap``` (requires gcc and rv8 RISC-V emulator installed):

```sh
user@ubuntu:~/rvcc$ make bootstrap
gcc -ansi -pedantic -m32 -Wall -Wextra -g -Llib -o bin/rvcc src/rvcc.c
bin/rvcc -o bin/rvcc_1.elf -Llib src/rvcc.c
rv-jit -- bin/rvcc_1.elf -o bin/rvcc_2.elf -Llib src/rvcc.c
diff -q bin/rvcc_1.elf bin/rvcc_2.elf
Files are the same - bootstrap successful!
```

### Output

The compiler generates an RV32IM binary without going through an explicit assembly step, it directly encodes all
RISC-V instructions and packages them into an ELF file.
The generated executable includes a symbol table so by using a disassembler it's possible to
peek into the machine code for introspection. The compiler also generates a listing of its internal
IL representation for debugging purposes.

To run a RISC-V ELF file on an x86 machine you can use the excellent [rv8](https://github.com/rv8-io/rv8)
emulator, which allows running and debugging RISC-V binaries as if they were native ones,
including proxying system calls for I/O. The usage is:

`rv-jit <elf_file>`

To disassemble a binary:

`rv-bin dump -d <elf_file>`

### RISC-V

[RISC-V](https://en.wikipedia.org/wiki/RISC-V) is an open source Instruction Set Architecture,
an alternative to the likes of x86 and ARM, which can be used freely without any licenses required. It's a RISC
architecture meaning the only instructions that operate on memory are simple loads and stores, with all
other instructions operating only on registers (32 of them) and immediate values.

### Code

Supported language constructs fall within ANSI C and initial external compilation can be done using gcc or any other
C compiler. While common language constructs (structs, pointers, indirection etc.) are implemented due
to the simplicity of the compiler (no preprocessor, AST nor a formalised parser) it doesn't implement full ANSI C syntax.

Compiler sequentally translates C source code into IL and then binary meaning instruction order is preserved from C source
all the way to the binary with only jumps and glue logic being inserted. This makes some language constructs
(for example for loops or some comparisons) quite inefficient due to excessive jumps but compiler's design stays very simple.

### Example

Sample code generated for a recursive Fibonacci function. Note in RISC-V a0/a1 are aliases for x10/x11 etc.

```asm
Mnemonic   Disassembly                Compiler's IL     C Source              Comment
---------+--------------------------+-----------------+---------------------+--------------------------------------
fe010113   addi  sp, sp, -32;         fib(int n)        int fib(int n) {      reserve stack space for function
00812e23   sw    s0, 28(sp);                                                    store previous frame
00112c23   sw    ra, 24(sp);                                                    store return address
01010413   addi  s0, sp, 16;                                                    set new frame location
fea42e23   sw    a0, -4(s0);                                                    store parameter on stack
00040513   addi  a0, s0, 0;           x10 = &n            if (n == 0)         get address of variable n
ffc50513   addi  a0, a0, -4;
00052503   lw    a0, 0(a0);           x10 = *x10                              read value from address into a0
00000593   addi  a1, zero, 0;         x11 := 0                                set a1 to zero
00b50663   beq   a0, a1, pc + 12;     x10 == x11 ?                            compare a0 with a1, if equal jump +3
00000513   addi  a0, zero, 0;                                                   set a0 to zero
0080006f   jal   zero, pc + 8;                                                  skip next instruction
00100513   addi  a0, zero, 1;                                                   set a0 to one
00000013   addi  zero, zero, 0;                                              
00050863   beq   a0, zero, pc + 16;   if 0 -> +4                              if a0 is zero, jump forward
00000513   addi  a0, zero, 0;         x10 := 0              return 0;         else set return value to zero 
0880006f   jal   zero, pc + 136;      return fib                                jump to function exit
0840006f   jal   zero, pc + 132;
00040513   addi  a0, s0, 0;           x10 = &n            else if (n == 1)    get address of variable n
ffc50513   addi  a0, a0, -4;
00052503   lw    a0, 0(a0);           x10 = *x10                              read value from address into a0
00100593   addi  a1, zero, 1;         x11 := 1                                set a1 to one
00b50663   beq   a0, a1, pc + 12;     x10 == x11 ?                            compare a0 with a1, if equal jump +3
00000513   addi  a0, zero, 0;                                                   set a0 to zero
0080006f   jal   zero, pc + 8;                                                  skip next instruction
00100513   addi  a0, zero, 1;                                                   set a0 to one
00000013   addi  zero, zero, 0;      
00050863   beq   a0, zero, pc + 16;   if 0 -> +4                              if a0 is zero, jump forward
00100513   addi  a0, zero, 1;         x10 := 1              return 1;         else set return value to one
0540006f   jal   zero, pc + 84;       return fib                                jump to function exit
0500006f   jal   zero, pc + 80;                           else return
00040513   addi  a0, s0, 0;           x10 = &n              fib(n - 1)        get address of variable n
ffc50513   addi  a0, a0, -4;
00052503   lw    a0, 0(a0);           x10 = *x10                              read value from address into a0
00100593   addi  a1, zero, 1;         x11 := 1                                set a1 to one
40b50533   sub   a0, a0, a1;          x10 -= x11                              subtract a1 from a0
f71ff0ef   jal   ra, pc - 144;        x10 := fib()                            call function fib() into a0
ff010113   addi  sp, sp, -16;         push x10                                store result on stack
00a12023   sw    a0, 0(sp);
00040513   addi  a0, s0, 0;           x10 = &n              fib(n - 2)        get address of variable n
ffc50513   addi  a0, a0, -4;
00052503   lw    a0, 0(a0);           x10 = *x10                              read value from address into a0
00200593   addi  a1, zero, 2;         x11 := 2                                set a1 to two
40b50533   sub   a0, a0, a1;          x10 -= x11                              subtract a1 from a0
f51ff0ef   jal   ra, pc - 176;        x11 := fib()                            call function fib() into a1
00050593   addi  a1, a0, 0;
00012503   lw    a0, 0(sp);           pop x10                                 retrieve result from stack into a0
01010113   addi  sp, sp, 16;         
00b50533   add   a0, a0, a1;          x10 += x11          +                   add a1 to a0
0040006f   jal   zero, pc + 4;        return fib          ;                   jump to function exit
01040113   addi  sp, s0, 16;          exit fib          }                     trim stack space
ff812083   lw    ra, -8(sp);                                                  recover return address
ffc12403   lw    s0, -4(sp);                                                  recover previous frame
00008067   jalr  zero, ra, 0;                                                 return from function
```

### Thanks

Aim of the project is purely recreational/educational and thanks are due to:

* RISC-V International for the RISC-V initiative: https://riscv.org/

* K&R for giving the world C language

* rv8 team for a great emulation environment: https://github.com/rv8-io/rv8

* everyone else who supports RISC-V!
