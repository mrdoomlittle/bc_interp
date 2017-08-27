# include <bci.h>
# include <sys/stat.h>
# include <string.h>
# include <fcntl.h>
# include <errno.h>
# include <stdint.h>
# include <stdlib.h>
# include <unistd.h>
# include <time.h>
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
mdl_u8_t code[] = {
	_bcii_nop, 0x0,
	_bcii_nop, 0x0,
	_bcii_nop, 0x0,
	_bcii_nop, 0x0,
	_bcii_nop, 0x0,
	_bcii_nop, 0x0,
	_bcii_nop, 0x0,
	_bcii_nop, 0x0,
	_bcii_exit, 0x0
};
# endif

struct m_arg {
	mdl_u8_t pin_mode, pin_state, pid;
} __attribute__((packed));

void* test_func(mdl_u8_t __id, void *__arg) {
	mdl_u8_t static ret_val = 0;

	struct m_arg *_m_arg = (struct arg*)__arg;

	switch(__id) {
		case 0: {
			printf("pin_mode: %u, pid: %u\n", _m_arg->pin_mode, _m_arg->pid);

			break;
		}
		case 1: {
			printf("pin_state: %u, pid: %u\n", _m_arg->pin_state, _m_arg->pid);
			break;
		}
		case 2: {
			printf("pid: %u\n", _m_arg->pid);
			ret_val = ~ret_val & 0x1;
			break;
		}
		case 3:
			printf("delay called\n");
			usleep((*(mdl_u16_t*)__arg)*1000);
		break;
		case 4:
//			printf("%u\n", *(mdl_u8_t*)__arg);
			printf("%s", (char*)__arg);
		break;
	}

	return (void*)&ret_val;
}

void iei(void *__arg) {
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
	bci_set_extern_fp(&_bci, &test_func);
//	bci_set_iei_fp(&_bci, &iei);
//	bci_set_iei_arg(&_bci, &_bci);
	struct timespec begin, end;
	clock_gettime(CLOCK_MONOTONIC, &begin);
	any_err = bci_exec(&_bci, 0x0, 0);
	clock_gettime(CLOCK_MONOTONIC, &end);

	printf("execution time: %uns\n", end.tv_nsec-begin.tv_nsec);
	bci_de_init(&_bci);

# ifndef DEBUG_ENABLED
	free(bc);
# endif
	return any_err;
}
