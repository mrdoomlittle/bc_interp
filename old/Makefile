CXXFLAGS=-I/usr/local/include
ARC=-DARC64
F_CPU=16000000UL
DEVICE=atmega328p
all: clean
	gcc -c -std=c11 -o bc_interp.o bc_interp.c
	gcc -std=c11 -o main main.c bc_interp.o
avr:
	avr-gcc -c -g $(ARC) $(CXXFLAGS) -std=c11 -DF_CPU=$(F_CPU) -Os -mmcu=$(DEVICE) -o bc_interp.o bc_interp.c
	ar rc lib/libbc_interp.a bc_interp.o
	cp bc_interp.h inc

clean:
	rm -f main
