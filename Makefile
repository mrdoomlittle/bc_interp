CFLAGS=-I../8xdrm/inc -I/usr/local/include
ARC=-DARC64
F_CPU=16000000UL
DEVICE=atmega328p
all:
	cd 8xdrm; make F_CPU=$(F_CPU) DEVICE=$(DEVICE) ARC=$(ARC); cd ../;
	avr-gcc -c -g $(ARC) $(CFLAGS) -D__AVR -std=c11 -DF_CPU=$(F_CPU) -Os -mmcu=$(DEVICE) -o bci.o bci.c
	ar rc lib/libbci.a bci.o
	cp bci.h inc
clean:
	sh clean.sh
