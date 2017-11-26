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
# include <errno.h>
# define SLICE_SHIFT 7
# define SLICE_SIZE (1<<SLICE_SHIFT)
# define SLICING_ENABLED 0x1
# define IS_FLAG(__flag) ((flags&__flag) == __flag)
mdl_u8_t *bc = NULL, flags = 0x0;
bci_addr_t ip = 0;
mdl_u32_t rdd = 0, rps = 0, no_rd = 0, ipi = 0, no_ips = 0, no_ipg = 0, rc = 0;
# define SAMPLE_RATE 1000000.0
struct timespec last;
int fd;
struct stat st;
mdl_uint_t cur_slice_no = 0;
mdl_u32_t slice_updates = 0;
void update_slice(bci_addr_t);
mdl_u8_t get_byte(bci_addr_t __off) {
	struct timespec s;
	clock_gettime(CLOCK_MONOTONIC, &s);
	if ((s.tv_nsec-last.tv_nsec)>SAMPLE_RATE) {
		clock_gettime(CLOCK_MONOTONIC, &last);
		rps = ((mdl_u32_t)(1000000000.0/SAMPLE_RATE))*(no_rd-rc);
		rc = no_rd;
	}
	if (rdd > 0) usleep(rdd);
	no_rd++;
	if (IS_FLAG(SLICING_ENABLED)) {
		update_slice(__off);
		return bc[(ip-((ip>>SLICE_SHIFT)*SLICE_SIZE))+__off];
	} else
		return bc[ip+__off];
}

void update_slice(bci_addr_t __off) {
	mdl_uint_t slice;
	if ((slice = ((ip+__off)>>SLICE_SHIFT)) != cur_slice_no) {
		mdl_uint_t off = slice*SLICE_SIZE;
		if (lseek(fd, off, SEEK_SET) == -1) {
			printf("failed to set\n");
		}
		mdl_uint_t slice_red = 0;
		if (off+SLICE_SIZE > st.st_size)
			slice_red = (off+SLICE_SIZE)-st.st_size;
		if (read(fd, bc, SLICE_SIZE-slice_red) < 0) {
			printf("read failed.\n");
		}
		cur_slice_no = slice;
		slice_updates++;
	}
}

// pp
void ip_incr(bci_addr_t __by) {
	ip+=__by;
	if (IS_FLAG(SLICING_ENABLED))
		update_slice(0);
	ipi++;
}

bci_addr_t get_ip() {
	no_ipg++;
	return ip;
}

//# define DEBUG_ENABLED
# ifdef DEBUG_ENABLED
mdl_u8_t code[] = {
//	_bcii_assign, 0x0, _bcit_8l, 0x0, 0x1, -100,
//	_bcii_assign, 0x0, _bcit_8l, 0x1, 0x0, 100,
//	_bcii_decr, 0x0, _bcit_8l|_bcit_msigned, 0x0, 0x0, _bcit_8l, 0x1,
//	_bcii_cmp, 0x0, _bcit_8l|_bcit_msigned, _bcit_8l|_bcit_msigned, 0x0, 0x0, 0x1, 0x0,
	_bcii_assign, 0x0, _bcit_16l, 0x0, 0x0, 0x0, 0x0,
	_bcii_assign, 0x0, _bcit_16l, 0x2, 0x0, 0x0, 0x0,
	_bcii_jmp, 0x0, 0x0, 0x0,
	_bcii_nop, 0x0,
	_bcii_exit, 0x0, 0x4, 0x0
};
# endif

void set_ip(bci_addr_t __ip) {
# ifndef DEBUG_ENABLED
	if (__ip >= st.st_size) {
# else
	if (__ip >= sizeof(code)) {
# endif
		fprintf(stderr, "tryed to set ip to %u\n", __ip);
		return;
	}
	ip = __ip;
	if (IS_FLAG(SLICING_ENABLED))
		update_slice(0);
	no_ips++;
}

struct bci _bci = {
	.stack_size = 120,
	.get_byte = &get_byte,
	.set_ip = &set_ip,
	.get_ip = &get_ip,
	.ip_incr = &ip_incr
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

void prusage() {
	fprintf(stdout, "usage: bci [options] -exec [.rbc]\n\
   -s		show statistical info\n\
   -rdd		read delay ns, example -rdd 200\n\
   -e		entry address, format: {0000 or 0x0000}hex\n");
}

int main(int __argc, char const *__argv[]) {
	mdl_u16_t entry_addr = 0x0000;
# ifdef DEBUG_ENABLED
	bc = code;
# else
	if (__argc < 2) {
		prusage();
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
		else if (!strcmp(*arg_itr, "-slice"))
			flags = SLICING_ENABLED;
		arg_itr++;
	}

	if (floc == NULL) {
		fprintf(stderr, "please provide a file.\n");
		return BCI_FAILURE;
	}

	if ((fd = open(floc, O_RDONLY)) < 0) {
		fprintf(stderr, "bci: failed to open file provided.\n");
		return BCI_FAILURE;
	}

	if (stat(floc, &st) < 0) {
		fprintf(stderr, "bci: failed to stat file.\n");
		close(fd);
		return BCI_FAILURE;
	}

	if (IS_FLAG(SLICING_ENABLED) && !(st.st_size < SLICE_SIZE)) {
		bc = (mdl_u8_t*)malloc(SLICE_SIZE);
		read(fd, bc, SLICE_SIZE);
	} else {
		bc = (mdl_u8_t*)malloc(st.st_size);
		read(fd, bc, st.st_size);
		if(!IS_FLAG(SLICING_ENABLED))
			close(fd);
	}
# endif
	clock_gettime(CLOCK_MONOTONIC, &last);

	bci_err_t any_err = BCI_SUCCESS;
	any_err = bci_init(&_bci);
# ifndef DEBUG_ENABLED
	*(mdl_uint_t*)&_bci.prog_size = st.st_size;
# else
	*(mdl_uint_t*)&_bci.prog_size = sizeof(code);
# endif
	bci_set_extern_fp(&_bci, &extern_call);
	bci_set_iei_fp(&_bci, &iei);

	mdl_u16_t exit_addr;
	bci_err_t exit_status;
	struct timespec begin, end;
	clock_gettime(CLOCK_MONOTONIC, &begin);
	any_err = bci_exec(&_bci, entry_addr, &exit_addr, &exit_status, 0);
	clock_gettime(CLOCK_MONOTONIC, &end);
	// ie_c = instruction execution count
	mdl_u64_t ns_taken = (end.tv_nsec-begin.tv_nsec)+((end.tv_sec-begin.tv_sec)*1000000000);
# ifndef DEBUG_ENABLED
	if (show_stats) {
# endif
		setlocale(LC_NUMERIC, "");
		fprintf(stdout, "\nstatistical infomation:\n\n");
		fprintf(stdout, "   %'10u\t\t instruction executions\n", ie_c);
		fprintf(stdout, "   %'10u\t\t reads per second\n", rps);
		fprintf(stdout, "   %'10u\t\t no reads\n", no_rd);
		fprintf(stdout, "   %'10u\t\t prog counter incrementations\n", ipi);
		fprintf(stdout, "   %'10u\t\t no instruction pointer sets\n", no_ips);
		fprintf(stdout, "   %'10u\t\t no instruction pointer gets\n", no_ipg);
		fprintf(stdout, "   %'10lu\t\t file size(bytes)\n", st.st_size);
		fprintf(stdout, "       0x%04hx\t\t entry address\n", entry_addr);
		fprintf(stdout, "       0x%04hx\t\t exit address\n", exit_addr);
		if (IS_FLAG(SLICING_ENABLED)) {
			fprintf(stdout, "   %'10lu\t\t total no slices\n", (st.st_size>>SLICE_SHIFT)+((st.st_size-((st.st_size>>SLICE_SHIFT)*SLICE_SIZE))>0));
			fprintf(stdout, "   %'10u\t\t slice updates\n", slice_updates);
		}
		fprintf(stdout, "   %'10u\t\t memory written(bytes)\n", _bci.m_wr);
		fprintf(stdout, "   %'10u\t\t memory read(bytes)\n", _bci.m_rd);
		fprintf(stdout, "   %'10d\t\t exit status - %s \n", exit_status, exit_status == _bcie_success?"success":(exit_status == _bcie_failure?"failure":"unknown"));
		fprintf(stdout, "\n   %'10luns   or %10lfsec\t execution time\n", ns_taken, (double)ns_taken/1000000000.0);
//		fprintf(stdout, "execution time: %luns or %lfsec, ie_c: %u, rps: %u, no_rd: %u, no_pci: %u, no_pcs: %u, no_pcg: %u\n", ns_taken, (double)ns_taken/1000000000.0, ie_c, rps, no_rd, no_pci, no_pcs, no_pcg);
# ifndef DEBUG_ENABLED
	}
# endif
	bci_de_init(&_bci);
# ifndef DEBUG_ENABLED
	free(bc);
	if (IS_FLAG(SLICING_ENABLED))
		close(fd);
# endif
	return any_err;
}
