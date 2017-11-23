# include <mdl/bci.h>
# include <sys/stat.h>
# include <string.h>
# include <fcntl.h>
# include <errno.h>
# include <stdint.h>
# include <stdlib.h>
# include <unistd.h>
# include <time.h>
# include <locale.h>
mdl_u8_t *bc = NULL;
bci_addr_t pc = 0;
mdl_u32_t rdd = 0, rps = 0, no_rd = 0, pci = 0, no_pcs = 0, no_pcg = 0, rc = 0;
# define SAMPLE_RATE 1000000.0
struct timespec last;
mdl_u8_t get_byte() {
	struct timespec s;
	clock_gettime(CLOCK_MONOTONIC, &s);
	if ((s.tv_nsec-last.tv_nsec)>SAMPLE_RATE) {
		clock_gettime(CLOCK_MONOTONIC, &last);
		rps = ((mdl_u32_t)(1000000000.0/SAMPLE_RATE))*(no_rd-rc);
		rc = no_rd;
	}
	if (rdd > 0) usleep(rdd);
	no_rd++;
	return bc[pc];
}

void pc_incr() {
	pc++;
	pci++;
}

void set_pc(bci_addr_t __pc) {
	pc = __pc;
	no_pcs++;
}

bci_addr_t get_pc() {
	no_pcg++;
	return pc;
}

//# define DEBUG_ENABLED
# ifdef DEBUG_ENABLED
mdl_u8_t code[] = {
//	_bcii_assign, 0x0, _bcit_8l, 0x0, 0x1, -100,
//	_bcii_assign, 0x0, _bcit_8l, 0x1, 0x0, 100,
//	_bcii_decr, 0x0, _bcit_8l|_bcit_msigned, 0x0, 0x0, _bcit_8l, 0x1,
//	_bcii_cmp, 0x0, _bcit_8l|_bcit_msigned, _bcit_8l|_bcit_msigned, 0x0, 0x0, 0x1, 0x0,
	_bcii_assign, 0x0, _bcit_8l, 0x0, 0x0, -2,
	_bcii_assign, 0x0, _bcit_8l, 0x1, 0x0, -4,
	_bcii_cmp, 0x0, _bcit_8l|_bcit_msigned, _bcit_8l|_bcit_msigned, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0,
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
	while(*itr != '\0') {
		if (*itr == '%') {
			itr++;
			switch(*itr) {
				case 's': {
					char *s;
					mdl_uint_t cc;
					cc = strlen((s = (char*)bci_resolv_addr(&_bci, *(bci_addr_t*)arg_p)));
					strncpy(ob_itr, s, cc);
					ob_itr+= cc;
					arg_p+= sizeof(mdl_u64_t);
					break;
				}
				case 'c': *(ob_itr++) = *(arg_p++);break;
				case 'u':
					ob_itr+= sprintf(ob_itr, "%lu", *(mdl_u64_t*)arg_p);
					arg_p+= sizeof(mdl_u64_t);
				break;
				case 'd':
					ob_itr+= sprintf(ob_itr, "%ld", *(mdl_i64_t*)arg_p);
					arg_p+= sizeof(mdl_i64_t);
				break;
				case 'x':
					ob_itr+= sprintf(ob_itr, "%lx", *(mdl_u64_t*)arg_p);
					arg_p+= sizeof(mdl_u64_t);
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
	mdl_u16_t entry_addr = 0x0000;
# ifdef DEBUG_ENABLED
	bc = code;
# else
	if (__argc < 2) {
		fprintf(stderr, "usage: ./bci -exec [src file]\n");
		return BCI_FAILURE;
	}

	mdl_u8_t show_stats = 0;
	char const *floc = NULL;
	char const **arg_itr = __argv+1;
	while(arg_itr != __argv+__argc) {
		if (!strcmp(*arg_itr, "-s")) show_stats = 1;
		else if (!strcmp(*arg_itr, "-e")) {
			if (*((mdl_u16_t*)*(arg_itr+1)) == ('0'|('x'<<8))) (*(arg_itr+1))+=2;
			sscanf(*(++arg_itr), "%04hx", &entry_addr);
		} else if (!strcmp(*arg_itr, "-exec"))
			floc = *(++arg_itr);
		else if (!strcmp(*arg_itr, "-rdd"))
			rdd = (mdl_u32_t)strtol(*(++arg_itr), NULL, 10);
		arg_itr++;
	}

	if (floc == NULL) {
		fprintf(stderr, "please provide a file.\n");
		return BCI_FAILURE;
	}

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
	clock_gettime(CLOCK_MONOTONIC, &last);

	bci_err_t any_err = BCI_SUCCESS;
	any_err = bci_init(&_bci);
	_bci.prog_size = st.st_size;
	bci_set_extern_fp(&_bci, &extern_call);
	bci_set_iei_fp(&_bci, &iei);

	struct timespec begin, end;
	clock_gettime(CLOCK_MONOTONIC, &begin);
	any_err = bci_exec(&_bci, entry_addr, 0);
	clock_gettime(CLOCK_MONOTONIC, &end);

	// ie_c = instruction execution count
	mdl_u64_t ns_taken = (end.tv_nsec-begin.tv_nsec)+((end.tv_sec-begin.tv_sec)*1000000000);
# ifndef DEBUG_ENABLED
	if (show_stats) {
# endif
		setlocale(LC_NUMERIC, "");
		fprintf(stdout, "\n statistical infomation:\n");
		fprintf(stdout, "   %'20u\t\t\t instruction executions\n", ie_c);
		fprintf(stdout, "   %'20u\t\t\t reads per second\n", rps);
		fprintf(stdout, "   %'20u\t\t\t no reads\n", no_rd);
		fprintf(stdout, "   %'20u\t\t\t prog counter incrementations\n", pci);
		fprintf(stdout, "   %'20u\t\t\t no prog counter sets\n", no_pcs);
		fprintf(stdout, "   %'20u\t\t\t no prog counter gets\n", no_pcg);
		fprintf(stdout, "   %'20lu\t\t\t file size(bytes)\n", st.st_size);
		fprintf(stdout, "	%9c0x%04hx\t\t\t entry address\n", ' ', entry_addr);
		fprintf(stdout, "\n   %'16luns or %10lfsec\t execution time\n", ns_taken, (double)ns_taken/1000000000.0);
//		fprintf(stdout, "execution time: %luns or %lfsec, ie_c: %u, rps: %u, no_rd: %u, no_pci: %u, no_pcs: %u, no_pcg: %u\n", ns_taken, (double)ns_taken/1000000000.0, ie_c, rps, no_rd, no_pci, no_pcs, no_pcg);
# ifndef DEBUG_ENABLED
	}
# endif
	bci_de_init(&_bci);
# ifndef DEBUG_ENABLED
	free(bc);
# endif
	return any_err;
}
