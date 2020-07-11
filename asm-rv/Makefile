ASM=riscv64-linux-gnu-as
LNK=riscv64-linux-gnu-ld
DMP=rv-bin dump -a
AFLAGS=-march=rv32i
LFLAGS=-b elf32-littleriscv -m elf32lriscv
OBJ=hello.o iotest.o param.o

.PRECIOUS: %.elf

%.o: %.S
	$(ASM) -o $@ $< $(AFLAGS)

%.elf: %.o
	$(LNK) -o $@ $< $(LFLAGS)

%.lst: %.elf
	$(DMP) $< >$@

all: hello.lst param.lst iotest.lst
	

.PHONY: clean

clean:
	rm -f *.o *.elf *.lst
