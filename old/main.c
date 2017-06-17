# include <stdio.h>
# include "bc_interp.h"
# include <sys/stat.h>
# include <string.h>
# include <fcntl.h>
# include <errno.h>
# include <stdint.h>
# include <stdlib.h>
# include <unistd.h>
/*
mdl_u8_t code[] = {
	_bci_assign, _bci_w8, 0x7, 0x0, 0x1,			// counter
	_bci_assign, _bci_w8, 0x8, 0x0, 0x4,			// exit value
	_bci_assign, _bci_w8, 0x9, 0x0, 0x1,			// number one <- ignore
	_bci_cmp, _bci_w8, 0x7, 0x0, 0x8, 0x0,			// compare
	_bci_print, _bci_w8, 0x7, 0x0,					// print
	_bci_add, _bci_w8, 0x7, 0x0, 0x9, 0x0, 0x7, 0x0,// add
	_bci_jne, 0xF, 0x0,								// jump back to address 15
	_bci_exit // exit
};*/
/*
mdl_u8_t code[] = {
	_bci_exnc, _bci_w8, 0x0, 0x0, 0x0, 0x0, BCI_DR_W8, 0x0,
	_bci_print, _bci_w8, BCI_DR_W8, 0x0,
	_bci_exit
};*/


mdl_u8_t *bc = NULL;
mdl_u16_t pc = 0;

mdl_u8_t get_byte() {
	return bc[pc];
}

void pc_incr() {
	pc++;
}

void set_pc(mdl_u16_t __pc) {
	pc = __pc;
}

mdl_u16_t get_pc() {
	return pc;
}

int main(int argc, char const *argv[]) {
	if (argc == 2) {

	int fd;
	if ((fd = open(argv[1], O_RDONLY)) < 0) {
		fprintf(stderr, "bci_exec: error\n");
		return -1;
	}

	struct stat st;
	if (stat(argv[1], &st) < 0) {
		close(fd);
		return -1;
	}

	bc = (mdl_u8_t*)malloc(st.st_size);
	read(fd, bc, st.st_size);
	close(fd);

	}
	printf("using bytecode interp by mrdoomlittle.\n");

	struct bc_interp_t bc_interp = {
		.stack_size = 120,
		.heep_size = 21,
		.get_byte = &get_byte,
		.set_pc = &set_pc,
		.get_pc = &get_pc,
		.pc_incr = &pc_incr
	};

	bci_init(&bc_interp);
	bci_exec(&bc_interp, 0x0);
}
