CFLAGS=-Ibitct/inc -I/usr/local/include
ARC=-DARC64
F_CPU=16000000UL
DEVICE=atmega328p
all:
	cd 8xdrm; make F_CPU=$(F_CPU) DEVICE=$(DEVICE) ARC=$(ARC); cd ../;
	avr-gcc -c -g -Wall $(ARC) $(CFLAGS) -D__AVR -std=c11 -DF_CPU=$(F_CPU) -Os -mmcu=$(DEVICE) -o bci.o bci.c
	ar rc lib/libmdl-bci.a bci.o
	cp bci.h inc/mdl
clean:
	sh clean.sh
