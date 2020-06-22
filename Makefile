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

tests/%.elf: tests/%.c
	./$(BIN)/$(EXECUTABLE) -L$(LIBDIRS) -o $@ $^ >$(basename $^).log
	rv-bin dump -a $@ >$(basename $@).lst

tests: all $(TESTBINS)


bootstrap: all
	./$(BIN)/$(EXECUTABLE) -o $(BIN)/rvcc_1.elf -L$(LIBDIRS) $(SRC)/rvcc.c >$(BIN)/rvcc_1.log
	rv-jit -- $(BIN)/rvcc_1.elf -o $(BIN)/rvcc_2.elf -L$(LIBDIRS) $(SRC)/rvcc.c >$(BIN)/rvcc_2.log 2>&1
	diff -q $(BIN)/rvcc_1.elf $(BIN)/rvcc_2.elf
	@if diff -q $(BIN)/rvcc_1.elf $(BIN)/rvcc_2.elf; then \
	echo "Files are the same - bootstrap successful!"; \
	else \
	echo "Files are NOT the same - bootstrap unsuccessful."; \
	fi

$(BIN)/$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(CLIBS) $^ -o $@ $(LIBRARIES)
