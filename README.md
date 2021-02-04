## rvcc

Bootstrapped C compiler for 32-bit RISC-V and ARM ISAs

### Features

* implements a subset of C language sufficient to compile itself
* generates executable Linux ELF binaries for RV32IM and ARMv7
* includes an embedded minimal Linux C standard library for basic I/O
* single binary - a microcompiler for environments without GCC toolchain
* written in ANSI C it can cross-compile from any platform
* simple two-pass compilation process: source -> IL -> binary
* lexer, parser and code generator all implemented by hand

### Bootstrapping

Bootstrapped compiler is a compiler that's able to compile its own source code which is a key verification milestone. Since rvcc is written in ANSI C, we can use any compiler on any platform for the initial compilation before bootstrapping with the help of RISC-V and/or ARM emulators.

![diagram](bootstrap.png)

The steps to validate rvcc's bootstrap on RISC-V are:

1. Compiler's source code is initially compiled using gcc which generates an x86 binary.

2. Built binary is invoked with its own source code as input and generates a RISC-V binary.

3. The RISC-V binary is invoked (via an emulator) with its own source code as input and generates another RISC-V binary.

4. If outputs generated in steps 2. and 3. are identical then bootstrap has been successful - the compiler regenerated itself from its own source code in two consecutive generations.

The bootstrap can be tested by running ```make bootstrap-riscv``` (requires gcc and rv8 RISC-V emulator installed):

```sh
user@ubuntu:~/rvcc$ make bootstrap-riscv
gcc -ansi -pedantic -m32 -Wall -Wextra -g -Llib -o bin/rvcc src/rvcc.c
bin/rvcc -o bin/rvcc_1.elf -march=riscv -Llib src/rvcc.c
rv-jit -- bin/rvcc_1.elf -o bin/rvcc_2.elf -march=riscv -Llib src/rvcc.c
diff -q bin/rvcc_1.elf bin/rvcc_2.elf
Files are the same - bootstrap successful!
```

Bootstrapping on ARM follows the same process and the test can be run via ```make bootstrap-arm``` (requires qemu-arm-static installed). To run both bootstraps use ```make bootstrap```.

### Usage

`rvcc [-o outfile] [-noclib] [-march=riscv|arm] <infile.c>`

- -o - output file name (default: out.elf)
- -noclib - exclude embedded C library (default: include)
- -march=riscv|arm - output architecture (default: riscv)

### Output

The compiler generates an executable binary file without going through explicit linking and assembly steps,
it directly encodes all RISC-V/ARM opcode instructions and packages them in an ELF file.
The generated executable includes a symbol table so by using a disassembler it's possible to
peek into the machine code for introspection. The compiler also generates a listing of its internal
IL representation for debugging purposes.

To run a RISC-V ELF file on an x86 machine you can use the excellent [rv8](https://github.com/rv8-io/rv8)
emulator, which allows running and debugging RISC-V binaries as if they were native ones,
including proxying system calls for I/O. The usage is:

`rv-jit <elf_file>`

To disassemble a RISC-V binary:

`rv-bin dump -d <elf_file>`

To run ARM on x86 you can use [qemu user emulation](https://wiki.debian.org/QemuUserEmulation) through qemu-arm-static.

`qemu-arm-static <elf_file>`

To disassemble an ARM binary you can use the GNU toolchain:

`arm-linux-gnueabi-objdump -d <elf_file>`

### Code

Supported language constructs fall within ANSI C and initial external compilation can be done using gcc or any other
C compiler. While common language constructs (structs, pointers, indirection etc.) are implemented due
to the simplicity of the compiler (no preprocessor, AST nor a formalised parser) it doesn't implement full ANSI C syntax.

Compiler sequentially translates C source code into IL and then binary meaning instruction order is preserved from C source
all the way to the binary with only jumps and glue logic being inserted. This makes some language constructs
(for example for loops or some comparisons) quite inefficient due to excessive jumps but compiler's design stays very simple.

#### RISC-V

[RISC-V](https://en.wikipedia.org/wiki/RISC-V) is an open source Instruction Set Architecture,
an alternative to the likes of x86 and ARM, which can be used freely without any licenses required. It's a RISC
architecture meaning the only instructions that operate on memory are simple loads and stores, with all
other instructions operating only on registers (32 of them) and immediate values.

#### ARM

ARM output generates code compatible with ARMv7 ISA which is owned by [Arm Holdings](https://www.arm.com/). Similarly to RISC-V,
ARM is a RISC ISA but it offers a wider variety of operations by adding conditional execution codes, automatic index increments, 
value shifts etc. to many instructions resulting in potentially denser code (fewer instructions required), at the expense of reducing
the number of bits available for immediate values.

The compiler generates Linux ARMv7 EABI-compliant binaries which can be run directly on a Raspberry Pi (verified on Pi 4 Model B running armv7l).

### Example RISC-V output

Sample RISC-V binary code generated for a recursive Fibonacci sequence function.

```asm
 C Source            Internal IL     Binary     Disassembly               Comment
-------------------+---------------+----------+-------------------------+--------------------------------------
 int fib(int n) {    fib(int n)      fe010113   addi  sp, sp, -32;        reserve stack space for function
                                     00812e23   sw    s0, 28(sp);           store previous frame
                                     00112c23   sw    ra, 24(sp);           store return address
                                     01010413   addi  s0, sp, 16;           set new frame location
                                     fea42e23   sw    a0, -4(s0);           store parameter on stack
   if (n == 0)       x10 := &n       00040513   addi  a0, s0, 0;          get address of variable n
                                     ffc50513   addi  a0, a0, -4;                    
                     x10 := *x10     00052503   lw    a0, 0(a0);          read value from address into a0
                     x11 := 0        00000593   addi  a1, zero, 0;        set a1 to zero
                     x10 ?= x11      00b50663   beq   a0, a1, pc + 12;    compare a0 with a1, if equal jump +3
                                     00000513   addi  a0, zero, 0;          set a0 to zero
                                     0080006f   jal   zero, pc + 8;         skip next instruction
                                     00100513   addi  a0, zero, 1;          set a0 to one
                                     00000013   addi  zero, zero, 0;                
                     if 0 -> +4      00050863   beq   a0, zero, pc + 16;  if a0 is zero, jump forward
     return 0;       x10 := 0        00000513   addi  a0, zero, 0;        else set return value to zero 
                     return fib      0880006f   jal   zero, pc + 136;       jump to function exit
                                     0840006f   jal   zero, pc + 132;            
   else if (n == 1)  x10 := &n       00040513   addi  a0, s0, 0;          get address of variable n
                                     ffc50513   addi  a0, a0, -4;                   
                     x10 := *x10     00052503   lw    a0, 0(a0);          read value from address into a0
                     x11 := 1        00100593   addi  a1, zero, 1;        set a1 to one
                     x10 ?= x11      00b50663   beq   a0, a1, pc + 12;    compare a0 with a1, if equal jump +3
                                     00000513   addi  a0, zero, 0;          set a0 to zero
                                     0080006f   jal   zero, pc + 8;         skip next instruction
                                     00100513   addi  a0, zero, 1;          set a0 to one
                                     00000013   addi  zero, zero, 0;                       
                     if 0 -> +4      00050863   beq   a0, zero, pc + 16;  if a0 is zero, jump forward
     return 1;       x10 := 1        00100513   addi  a0, zero, 1;        else set return value to one
                     return fib      0540006f   jal   zero, pc + 84;        jump to function exit
   else return                       0500006f   jal   zero, pc + 80;                  
     fib(n - 1)      x10 := &n       00040513   addi  a0, s0, 0;          get address of variable n
                                     ffc50513   addi  a0, a0, -4;                       
                     x10 := *x10     00052503   lw    a0, 0(a0);          read value from address into a0
                     x11 := 1        00100593   addi  a1, zero, 1;        set a1 to one
                     x10 -= x11      40b50533   sub   a0, a0, a1;         subtract a1 from a0
                     x10 := fib()    f71ff0ef   jal   ra, pc - 144;       call function fib() into a0
     +               push x10        ff010113   addi  sp, sp, -16;        store result on stack
                                     00a12023   sw    a0, 0(sp);                     
     fib(n - 2)      x10 := &n       00040513   addi  a0, s0, 0;          get address of variable n
                                     ffc50513   addi  a0, a0, -4;                 
                     x10 := *x10     00052503   lw    a0, 0(a0);          read value from address into a0
                     x11 := 2        00200593   addi  a1, zero, 2;        set a1 to two
                     x10 -= x11      40b50533   sub   a0, a0, a1;         subtract a1 from a0
                     x11 := fib()    f51ff0ef   jal   ra, pc - 176;       call function fib() into a1
                                     00050593   addi  a1, a0, 0;                              
                     pop x10         00012503   lw    a0, 0(sp);          retrieve result off stack into a0
                                     01010113   addi  sp, sp, 16;                       
                     x10 += x11      00b50533   add   a0, a0, a1;         add a1 to a0
     ;               return fib      0040006f   jal   zero, pc + 4;       jump to function exit
 }                   exit fib        01040113   addi  sp, s0, 16;         trim stack space
                                     ff812083   lw    ra, -8(sp);         recover return address
                                     ffc12403   lw    s0, -4(sp);         recover previous frame
                                     00008067   jalr  zero, ra, 0;        return from function
```

### Thanks

Aim of the project is purely recreational/educational and thanks are due to:

* [RISC-V International](https://riscv.org/) for the RISC-V initiative

* K&R for giving the world C language (here we are 40 years later...)

* [rv8](https://github.com/rv8-io/rv8) and [qemu](https://www.qemu.org/) teams for a great emulation environments

* everyone else who supports RISC architectures!
