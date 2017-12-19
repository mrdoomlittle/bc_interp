ifndef inc_flags
 inc_flags=-Ibitct/inc -I/usr/local/include
endif
ifndef arc
 arc=arc64
endif
ifndef f_cpu
 f_cpu=16000000UL
endif
ifndef device
 device=atmega328p
endif
all:
	cd bitct; make f_cpu=$(f_cpu) device=$(device) arc=$(arc); cd ../;
	avr-gcc -c -g -Wall -D__$(arc) $(inc_flags) -std=c11 -DF_CPU=$(f_cpu) -Os -mmcu=$(device) -o bci.o bci.c
	ar rc lib/libmdl-bci.a bci.o
	cp bci.h inc/mdl
clean:
	sh clean.sh
