!/bin/bash

make clean
make
rm -f bin/blank_a
bin/rvcc -o bin/blank_a -march=arm tests/blank.c >blank_a.lst
arm-linux-gnueabi-objdump -d bin/blank_a >blank_a_dasm.lst
chmod a+x bin/blank_a
bin/blank_a
