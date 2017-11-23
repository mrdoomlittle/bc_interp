C_IFLAGS="-Ibitct/inc -Imdlint/inc"
C_LFLAGS="-Lbitct/lib"
DEFINES=""
#"-D__DEBUG_ENABLED"
cd bitct;
sh compile.sh -I../mdlint/inc
cd ../;

rm -f bci.o bin/bci
gcc -O0 -Wall -c $C_IFLAGS $DEFINES -std=c11 -o bci.o bci.c

ar rc lib/libmdl-bci.a bci.o
cp bci.h inc/mdl

gcc -Wall -Iinc -Llib $C_IFLAGS $C_LFLAGS $DEFINES -std=gnu11 -o bin/bci main.c -lmdl-bci -lmdl-bitct
