# include <bci.h>
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

void set_pc(bci_addr_t __pc) {
	pc = __pc;
}

bci_addr_t get_pc() {
	return pc;
}

//# define DEBUG_ENABLED

# ifdef DEBUG_ENABLED
/*
mdl_u8_t code[] = {
	_bcii_assign, _bcit_w8, 0x0, 0x0, 0x20,
	_bcii_assign, _bcit_w8, 0x1, 0x0, 0x5,
	_bcii_aop, _bci_aop_div, _bcit_w8, _bcit_w8, 0x0, 0x0, 0x1, 0x0, _bcit_w8, 0x0, 0x0,
	_bcii_mov, _bcit_w8, _bcit_w8, 0x2, 0x0, 0x0, 0x0,
	_bcii_print, _bcit_w8, 0x2, 0x0,
	_bcii_exit
};
*/
mdl_u8_t code[] = {
	_bcii_assign, 0x0, _bcit_w8, 0x0, 0x0, 0x1,
	_bcii_conv, 0x0, _bcit_w16, _bcit_w8, 0x0, 0x0,
	_bcii_print, 0x0, _bcit_w16, 0x0, 0x0,
	_bcii_exit, 0x0
};

/*
mdl_uint_t c = 0;
mdl_u8_t _get_byte(void *__arg) {
    c++;
    return code[c-1];
}

mdl_u8_t o = 0;
void _put_byte(mdl_u8_t __byte, void *__arg) {
    code[o++] = __byte;
}
*/
# endif


struct arg_t {
	mdl_u8_t pin_mode, pin_state, pid;
};

void* test_func(mdl_u8_t __id, void *__arg) {
	mdl_u8_t static ret_val = 0;

	struct arg_t *arg = (struct arg_t*)__arg;

	switch(__id) {
		case 0: {
			printf("pin_mode: %u, pid: %u\n", arg->pin_mode, arg->pid);

			break;
		}
		case 1: {
			printf("pin_state: %u, pid: %u\n", arg->pin_state, arg->pid);
			break;
		}
		case 2: {
			printf("pid: %u\n", arg->pid);
			ret_val = ~ret_val & 0x1;
			break;
		}
		case 3:
			printf("delay called\n");
			usleep((*(mdl_u16_t*)__arg)*1000);
		break;
 	}

	return (void*)&ret_val;
}

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
/*
	struct _8xdrm_t _8xdrm;
	_8xdrm_init(&_8xdrm, &_get_byte, &_put_byte);

	_8xdrm_put_wx(&_8xdrm, _bcii_print, 8);
	_8xdrm_put_wx(&_8xdrm, 0x0, 8);

	_8xdrm_put_wx(&_8xdrm, _bcit_w16, 4);
	_8xdrm_put_wx(&_8xdrm, 0x0, 8);
	_8xdrm_put_wx(&_8xdrm, 0x0, 8);

	_8xdrm_put_wx(&_8xdrm, _bcii_exit, 8);
	_8xdrm_dump(&_8xdrm);
*/
	bc = code;
# endif
	struct bci _bci = {
		.stack_size = 120,
		.get_byte = &get_byte,
		.set_pc = &set_pc,
		.get_pc = &get_pc,
		.pc_incr = &pc_incr
	};

	bci_err_t any_err;
	any_err = bci_init(&_bci);
	bci_set_extern_func(&_bci, &test_func);
	any_err = bci_exec(&_bci, 0x0);
	bci_de_init(&_bci);

# ifndef DEBUG_ENABLED
	free(bc);
# endif
	return any_err;
}
