CC		:= gcc
CFLAGS		:= -ansi -pedantic -m32 -Wall -Wextra -g
BIN		:= bin
SRC		:= src
LIB		:= lib
LIBRARIES	:=
EXECUTABLE	:= rvcc
SOURCEDIRS	:= $(shell find $(SRC) -type d)
LIBDIRS		:= $(shell find $(LIB) -type d)
CLIBS		:= $(patsubst %,-L%, $(LIBDIRS:%/=%))
SOURCES		:= $(wildcard $(patsubst %,%/rvcc.c, $(SOURCEDIRS)))
OBJECTS		:= $(SOURCES:.c=.o)
TESTS		:= $(wildcard tests/*.c)
TESTBINS	:= $(TESTS:.c=.elf)

all: $(BIN)/$(EXECUTABLE)

.PHONY: clean

clean:
	-$(RM) $(BIN)/$(EXECUTABLE)
	-$(RM) $(OBJECTS)
	-$(RM) $(TESTBINS) tests/*.log tests/*.lst
	-$(RM) $(BIN)/rvcc*.elf $(BIN)/rvcc*.log
	-$(RM) $(BIN)/embed

tests/%.elf: tests/%.c
	./$(BIN)/$(EXECUTABLE) -L$(LIBDIRS) -o $@ $^ >$(basename $^).log
	rv-bin dump -a $@ >$(basename $@).lst

tests: all $(TESTBINS)

clib: 
	$(CC) $(CFLAGS) $(CLIBS) lib/embed.c -o $(BIN)/embed $(LIBRARIES)
	$(BIN)/embed $(LIB)/rvclib.c $(SRC)/rvclib.inc

bootstrap-riscv: all
	./$(BIN)/$(EXECUTABLE) -o $(BIN)/rvcc_riscv_1.elf -march=riscv -L$(LIBDIRS) $(SRC)/rvcc.c >$(BIN)/rvcc_riscv_1.log
	chmod a+x $(BIN)/rvcc_riscv_1.elf
	rv-jit -- $(BIN)/rvcc_riscv_1.elf -o $(BIN)/rvcc_riscv_2.elf -march=riscv -L$(LIBDIRS) $(SRC)/rvcc.c >$(BIN)/rvcc_riscv_2.log 2>&1
	diff -q $(BIN)/rvcc_riscv_1.elf $(BIN)/rvcc_riscv_2.elf
	@if diff -q $(BIN)/rvcc_riscv_1.elf $(BIN)/rvcc_riscv_2.elf; then \
	echo "Files are the same - RISC-V bootstrap successful!"; \
	else \
	echo "Files are NOT the same - RISC-V bootstrap unsuccessful."; \
	fi

bootstrap-arm: all
	./$(BIN)/$(EXECUTABLE) -o $(BIN)/rvcc_arm_1.elf -march=arm -L$(LIBDIRS) $(SRC)/rvcc.c >$(BIN)/rvcc_arm_1.log
	chmod a+x $(BIN)/rvcc_arm_1.elf
	qemu-arm-static $(BIN)/rvcc_arm_1.elf -o $(BIN)/rvcc_arm_2.elf -march=arm -L$(LIBDIRS) $(SRC)/rvcc.c >$(BIN)/rvcc_arm_2.log 2>&1
	diff -q $(BIN)/rvcc_arm_1.elf $(BIN)/rvcc_arm_2.elf
	@if diff -q $(BIN)/rvcc_arm_1.elf $(BIN)/rvcc_arm_2.elf; then \
	echo "Files are the same - ARM bootstrap successful!"; \
	else \
	echo "Files are NOT the same - ARM bootstrap unsuccessful."; \
	fi

bootstrap: bootstrap-riscv bootstrap-arm
	@if diff -q $(BIN)/rvcc_riscv_1.elf $(BIN)/rvcc_riscv_2.elf; then \
	echo "RISC-V bootstrap successful!"; \
	else \
	echo "RISC-V bootstrap unsuccessful."; \
	fi
	@if diff -q $(BIN)/rvcc_arm_1.elf $(BIN)/rvcc_arm_2.elf; then \
	echo "ARM bootstrap successful!"; \
	else \
	echo "ARM bootstrap unsuccessful."; \
	fi	

$(BIN)/$(EXECUTABLE): $(OBJECTS)
	mkdir -p $(BIN)
	$(CC) $(CFLAGS) $(CLIBS) $^ -o $@ $(LIBRARIES)
