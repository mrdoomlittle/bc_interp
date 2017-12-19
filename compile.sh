inc_flags="-Ibitct/inc -Imdlint/inc"
lib_flags="-Lbitct/lib"
defines=""
cd bitct;
sh compile.sh -I../mdlint/inc
cd ../;

rm -f bci.o bin/bci
gcc -O0 -Wall -c $inc_flags $defines -std=c11 -o bci.o bci.c

ar rc lib/libmdl-bci.a bci.o
cp bci.h inc/mdl

gcc -Wall -Iinc -Llib $inc_flags $lib_flags $defines -std=gnu11 -o bin/bci main.c -lmdl-bci -lmdl-bitct
