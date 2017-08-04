cd ../8xdrm;
sh compile.sh

cd ../bci;

rm -f bci.o bin/bci
gcc -c -I../8xdrm/inc -std=c11 -o bci.o bci.c

ar rc lib/libbci.a bci.o
cp bci.h inc

gcc -Iinc -Llib -I../8xdrm/inc -L../8xdrm/lib -std=c11 -o bin/bci main.c -lbci -l8xdrm
