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
	_bcii_assign, 0x0, _bcit_8l, 0x0, 0x1, -100,
	_bcii_assign, 0x0, _bcit_8l, 0x1, 0x0, 100,
	_bcii_decr, 0x0, _bcit_8l|_bcit_msigned, 0x0, 0x0, _bcit_8l, 0x1,
	_bcii_cmp, 0x0, _bcit_8l|_bcit_msigned, _bcit_8l|_bcit_msigned, 0x0, 0x0, 0x1, 0x0,
	_bcii_exit, 0x0
};
# endif

struct bci _bci = {
	.stack_size = 120,
	.get_byte = &get_byte,
	.set_pc = &set_pc,
	.get_pc = &get_pc,
	.pc_incr = &pc_incr
};

struct m_arg {
	mdl_u8_t pin_mode, pin_state, pid;
} __attribute__((packed));

struct pair {
	bci_addr_t p1, p2;
} __attribute__((packed));

void bci_printf(struct pair *__pair) {
	char obuf[200];
	mdl_u8_t *arg_p = (mdl_u8_t*)bci_resolv_addr(&_bci, __pair->p2);
	char *s = (char*)bci_resolv_addr(&_bci, __pair->p1);
	char *itr = s, *ob_itr = obuf;

	mdl_uint_t cc;
	while(*itr != '\0') {
		if (*itr == '%') {
			itr++;
			switch(*itr) {
				case 's': {
					char *s;
					cc = strlen((s = (char*)bci_resolv_addr(&_bci, *(bci_addr_t*)arg_p)));
					strncpy(ob_itr, s, cc);
					ob_itr+= cc;
					arg_p+= sizeof(mdl_u64_t);
					break;
				}
				case 'c': *(ob_itr++) = *(arg_p++);break;
				case 'u':
					cc = sprintf(ob_itr, "%lu", *(mdl_u64_t*)arg_p);
					ob_itr+= cc;
					arg_p+= sizeof(mdl_u64_t);
				break;
				case 'd':
					cc = sprintf(ob_itr, "%ld", *(mdl_i64_t*)arg_p);
					ob_itr+= cc;
					arg_p+= sizeof(mdl_i64_t);
				break;
			}
			itr++;
		} else
			*(ob_itr++) = *(itr++);
	}

	*ob_itr = '\0';
	fprintf(stdout, "%s", obuf);
}

void* extern_call(mdl_u8_t __id, void *__arg) {
	mdl_u8_t static ret_val = 0;

	struct m_arg *_m_arg = (struct m_arg*)__arg;

	switch(__id) {
		case 0x0: {
			printf("pin_mode: %u, pid: %u\n", _m_arg->pin_mode, _m_arg->pid);

			break;
		}
		case 0x1: {
			printf("pin_state: %u, pid: %u\n", _m_arg->pin_state, _m_arg->pid);
			break;
		}
		case 0x2: {
			printf("pid: %u\n", _m_arg->pid);
			ret_val = ~ret_val & 0x1;
			break;
		}
		case 0x3:
			usleep(*(mdl_u16_t*)__arg*1000000);
		break;
		case 0x4:
			bci_printf((struct pair*)__arg);
		break;
	}

	return (void*)&ret_val;
}

mdl_uint_t ie_c = 0;
void iei(void *__arg_p) {
	ie_c++;
}

int main(int __argc, char const *__argv[]) {
	mdl_u16_t entry_point = 0x0;
# ifdef DEBUG_ENABLED
	bc = code;
# else
	if (__argc < 2) {
		fprintf(stderr, "usage: ./bci [src file]\n");
		return BCI_FAILURE;
	}

	mdl_u8_t show_stats = 0;
	char const **arg_itr = __argv+1;
	while(arg_itr != __argv+__argc) {
		if (!strcmp(*arg_itr, "-s")) show_stats = 1;
		else if (!strcmp(*arg_itr, "-e"))
			sscanf(*(++arg_itr), "%4hx", &entry_point);
		arg_itr++;
	}

	char const *floc = __argv[__argc-1];
	int fd;
	if ((fd = open(floc, O_RDONLY)) < 0) {
		fprintf(stderr, "bci: failed to open file provided.\n");
		return BCI_FAILURE;
	}

	struct stat st;
	if (stat(floc, &st) < 0) {
		fprintf(stderr, "bci: failed to stat file.\n");
		close(fd);
		return BCI_FAILURE;
	}

	bc = (mdl_u8_t*)malloc(st.st_size);
	read(fd, bc, st.st_size);
	close(fd);
# endif
	bci_err_t any_err = BCI_SUCCESS;
	any_err = bci_init(&_bci);
	bci_set_extern_fp(&_bci, &extern_call);
	bci_set_iei_fp(&_bci, &iei);

	struct timespec begin, end;
	clock_gettime(CLOCK_MONOTONIC, &begin);
	any_err = bci_exec(&_bci, entry_point, 0);
	clock_gettime(CLOCK_MONOTONIC, &end);

	// ie_c = instruction execution count
# ifndef DEBUG_ENABLED
	if (show_stats)
# endif
		fprintf(stdout, "execution time: %luns, ie_c: %u\n", (end.tv_nsec-begin.tv_nsec)+((end.tv_sec-begin.tv_sec)*1000000000), ie_c);
	bci_de_init(&_bci);
# ifndef DEBUG_ENABLED
	free(bc);
# endif
	return any_err;
}
