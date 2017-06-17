# include "bci.h"
# include <sys/stat.h>
# include <string.h>
# include <fcntl.h>
# include <errno.h>
# include <stdint.h>
# include <stdlib.h>
# include <unistd.h>

mdl_u8_t *bc = NULL;
bci_addr_t pc = 0;

mdl_u8_t get_byte() {
	return bc[pc];
}

void pc_incr() {
	pc++;
}

void set_pc(mdl_u16_t __pc) {
	pc = __pc;
}

bci_addr_t get_pc() {
	return pc;
}

//# define DEBUG_ENABLED

# ifdef DEBUG_ENABLED
mdl_u8_t code[] = {
	_bcii_assign, _bcit_w8, 0x0, 0x0, 0x20,
	_bcii_assign, _bcit_w8, 0x1, 0x0, 0x5,
	_bcii_aop, _bci_aop_div, _bcit_w8, _bcit_w8, 0x0, 0x0, 0x1, 0x0, _bcit_w8, 0x0, 0x0,
	_bcii_mov, _bcit_w8, _bcit_w8, 0x2, 0x0, 0x0, 0x0,
	_bcii_print, _bcit_w8, 0x2, 0x0,
	_bcii_exit
};
# endif

int main(int __argc, char const *__argv[]) {
# ifndef DEBUG_ENABLED
	if (__argc < 2) {
		fprintf(stderr, "usage: ./bci [src file]\n");
		return BCI_FAILURE;
	}

	int fd;
	if ((fd = open(__argv[1], O_RDONLY)) < 0) {
		fprintf(stderr, "bci: failed to open file provided.\n");
		return BCI_FAILURE;
	}

	struct stat st;
	if (stat(__argv[1], &st) < 0) {
		fprintf(stderr, "bci: failed to stat file.\n");
		close(fd);
		return BCI_FAILURE;
	}

	bc = (mdl_u8_t*)malloc(st.st_size);
	read(fd, bc, st.st_size);
	close(fd);
# else
	bc = code;
# endif
	struct bci_t bci = {
		.stack_size = 120,
		.get_byte = &get_byte,
		.set_pc = &set_pc,
		.get_pc = &get_pc,
		.pc_incr = &pc_incr
	};

	bci_err_t any_err;
	any_err = bci_init(&bci);
	any_err = bci_exec(&bci, 0x0);

# ifndef DEBUG_ENABLED
	free(bc);
# endif
	return any_err;
}
