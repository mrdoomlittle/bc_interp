all: clean
	gcc -c -std=c11 -o bc_interp.o bc_interp.c
	gcc -std=c11 -o main main.c bc_interp.o
clean:
	rm -f main
