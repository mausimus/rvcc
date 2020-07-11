ASM=arm-linux-gnueabi-as
LNK=arm-linux-gnueabi-ld
DMP=arm-linux-gnueabi-objdump -d
AFLAGS=
LFLAGS=
OBJ=hello.o

.PRECIOUS: %.elf

%.o: %.S
	$(ASM) -o $@ $< $(AFLAGS)

%.elf: %.o
	$(LNK) -o $@ $< $(LFLAGS)

%.lst: %.elf
	$(DMP) $< >$@

all: hello.lst
	

.PHONY: clean

clean:
	rm -f *.o *.elf *.lst
