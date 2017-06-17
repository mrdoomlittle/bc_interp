rm -f bci.o bin/bci
gcc -c -std=c11 -o bci.o bci.c

ar rc lib/libbci.a bci.o
cp bci.h inc

gcc -Iinc -Llib -std=c11 -o bin/bci main.c -lbci
