# include "bci.h"
bci_err_t static stack_put(struct bci *__bci, mdl_u8_t *__val, mdl_uint_t __bc, bci_addr_t __addr) {
	if (__addr >= __bci->stack_size) {
		fprintf(stdout, "stack_put; address 0x%04x is not within the boundary.\n", __addr);
		return BCI_FAILURE;
	}
	mdl_u8_t *itr = __val;
	bci_addr_t addr;
	while (itr != __val+__bc) {
		addr = __addr+(itr-__val);
		if (addr >= __bci->stack_size) {
# ifdef __DEBUG_ENABLED
			fprintf(stderr, "stack_put; failed, address out of bounds.\n");
			fprintf(stderr, "stack_put; tryed to access memory at 0x%04x.\n", addr);
			return BCI_FAILURE;
# endif
		}
		if ((*(__bci->locked_addrs+(addr>>3)))>>(addr-((addr>>3)*(1<<3)))&0x1) {
# ifdef __DEBUG_ENABLED
			fprintf(stderr, "stack_put; memory addresss at 0x%04x is locked.\n", addr);
# endif
			itr++;
		} else
			*(__bci->mem_stack+addr) = *(itr++);
	}
	__bci->m_wr+= __bc;
	return BCI_SUCCESS;
}

bci_err_t static stack_get(struct bci *__bci, mdl_u8_t *__val, mdl_uint_t __bc, bci_addr_t __addr) {
	if (__addr >= __bci->stack_size) {
# ifdef __DEBUG_ENABLED
		fprintf(stderr, "stack_get; address 0x%04x is not within the boundary.\n", __addr);
# endif
		return BCI_FAILURE;
	}
	mdl_u8_t *itr = __val;
	bci_addr_t addr;
	while (itr != __val+__bc) {
		addr = __addr+(itr-__val);
		if (addr >= __bci->stack_size) {
# ifdef __DEBUG_ENABLED
			fprintf(stderr, "stack_get; failed, address out of bounds.\n");
			fprintf(stderr, "stack_get; tryed to access memory at 0x%04x.\n", addr);
# endif
			return BCI_FAILURE;
		}
		if ((*(__bci->locked_addrs+(addr>>3)))>>(addr-((addr>>3)*(1<<3)))&0x1) {
# ifdef __DEBUG_ENABLED
			fprintf(stderr, "stack_get; memory address at 0x%04x is locked.\n", addr);
# endif
			*(itr++) = 0xFF;
		} else
			*(itr++) = *(__bci->mem_stack+addr);
	}
	__bci->m_rd+= __bc;
	return BCI_SUCCESS;
}

mdl_u8_t static next_byte(void *__arg_p) {
	struct bci *_bci = (struct bci*)__arg_p;
	if ((_bci->get_ip()+_bci->ip_off) >= _bci->prog_size) return 0;
	mdl_u8_t val = _bci->get_byte(_bci->ip_off++);
	return val;
}

void* bci_resolv_addr(struct bci *__bci, bci_addr_t __addr) {
	return (void*)(__bci->mem_stack+__addr);
}

void bci_set_extern_fp(struct bci *__bci, void*(*__extern_fp)(mdl_u8_t, void*)) {
	__bci->extern_fp = __extern_fp;
}

void bci_set_act_indc_fp(struct bci *__bci, void(*__act_indc_fp)()) {
	__bci->act_indc_fp = __act_indc_fp;
}

void bci_set_iei_fp(struct bci *__bci, void(*__iei_fp)(void*)) {
	__bci->iei_fp = __iei_fp;
}

void bci_set_iei_arg(struct bci *__bci, void *__iei_arg) {
	__bci->iei_arg = __iei_arg;
}

void bci_set_arg(struct bci *__bci, void *__p, bci_uint_t __bc, mdl_u8_t __no) {
	*(__bci->args+__no) = (struct bci_arg) {
		.p = __p,
		.bc = __bc
	};
}

struct bci_arg *bci_get_arg(struct bci *__bci, mdl_u8_t __no) {
	return __bci->args+__no;
}

bci_err_t bci_init(struct bci *__bci, mdl_u8_t __arg_c) {
	if ((__bci->mem_stack = (mdl_u8_t*)malloc(__bci->stack_size)) == NULL) return BCI_FAILURE;
	__bci->args = (struct bci_arg*)malloc(__arg_c*sizeof(struct bci_arg));
	struct bci_arg *at = __bci->args;
	while(at != __bci->args+__arg_c)
		*(at++) = (struct bci_arg) {.p = NULL, .bc = 0};

	mdl_u16_t bytes;
	__bci->locked_addrs = (mdl_u8_t*)malloc((bytes = ((__bci->stack_size>>3)+((__bci->stack_size-((__bci->stack_size>>3)*(1<<3)))>0))));
	memset(__bci->locked_addrs, 0x0, bytes);
	__bci->eeb_list = NULL;
	__bci->act_indc_fp = NULL;
	bitct_init(&__bci->_bitct, &next_byte, NULL);
	set_get_arg(&__bci->_bitct, (void*)__bci);
	memset(__bci->mem_stack, 0xFF, __bci->stack_size);
	__bci->extern_fp = NULL;
	__bci->iei_fp = NULL;
	__bci->flags = 0;
	__bci->m_wr = 0;
	__bci->m_rd = 0;
	__bci->ula_guard = NULL;
	__bci->ulag_arg = NULL;
	return BCI_SUCCESS;
}

bci_err_t bci_de_init(struct bci *__bci) {
	free(__bci->mem_stack);
	free(__bci->locked_addrs);
	if (__bci->eeb_list != NULL) free(__bci->eeb_list);
	return BCI_SUCCESS;
}

mdl_u8_t static get_lx(struct bci *__bci, mdl_u8_t __w) {
	return bitct_get_lx(&__bci->_bitct, __w);
}

# define get_4l(__bci) get_wx(__bci, 4)
mdl_u8_t static get_8l(struct bci *__bci) {
	return get_lx(__bci, 8);
}

mdl_u16_t static get_16l(struct bci *__bci) {
	mdl_u16_t ret_val = 0x0000;
	ret_val = get_8l(__bci);
	ret_val |= get_8l(__bci)<<8;
	return ret_val;
}

void static get(struct bci *__bci, mdl_u8_t *__val, mdl_uint_t __bc) {
	mdl_u8_t *itr = __val;
	for (; itr != __val+__bc; itr++) *itr = get_8l(__bci);
}

mdl_u8_t bcit_sizeof(mdl_u8_t __type) {
	switch(__type) {
		case _bcit_void: return 0;
		case _bcit_8l: return sizeof(mdl_u8_t);
		case _bcit_16l: return sizeof(mdl_u16_t);
		case _bcit_32l: return sizeof(mdl_u32_t);
		case _bcit_64l: return sizeof(mdl_u64_t);
		case _bcit_addr: return sizeof(bci_addr_t);
	}
	return 0;
}

mdl_u8_t is_flag(bci_flag_t __flags, bci_flag_t __flag) {
	return (__flags&__flag) == __flag;}

mdl_uint_t bcii_sizeof(mdl_u8_t *__p, bci_flag_t __flags) {
	switch(*__p) {
		case _bcii_nop: 			return 0;
		case _bcii_exit:            return 0;
		case _bcii_extern_call: 	return 7;
		case _bcii_dr:				return 5;
		case _bcii_print:			return 3;
		case _bcii_assign:			return 3+bcit_sizeof(*(__p+2));
		case _bcii_aop: {
			mdl_uint_t size = 4;
			if (is_flag(__flags, _bcii_aop_fl_pm)) size += 2;
			if (is_flag(__flags, _bcii_aop_fr_pm)) size += 2;
			return size;
		}
		case _bcii_lop: {
			mdl_uint_t size = 4;
			if (is_flag(__flags, _bcii_lop_fl_pm)) size += 2;
			if (is_flag(__flags, _bcii_lop_fr_pm)) size += 2;
			return size;
		}
		case _bcii_mov: 			return 5;
		case _bcii_incr: case _bcii_decr:
			if (is_flag(__flags, _bcii_iod_fbc_addr)) return 5;
			else
				return 4+bcit_sizeof(*(__p+5));
		case _bcii_cmp:				return 6;
		case _bcii_cjmp:			return 5;
		case _bcii_jmp:				return 2;
		case _bcii_eeb_init:		return 1;
		case _bcii_eeb_put:			return 1+(bcit_sizeof(_bcit_addr)*2);
		case _bcii_conv: return 2+(bcit_sizeof(_bcit_addr)*2);
		case _bcii_shr: case _bcii_shl: return 2+bcit_sizeof(_bcit_addr)+bcit_sizeof(*(__p+4));
		default: return 0;
	}
	return 0;
}

mdl_uint_t bcii_overhead_size() {return 2;}
void bci_eeb_init(struct bci *__bci, mdl_u8_t __blk_c) {
	if (__bci->eeb_list != NULL) return;
	__bci->eeb_list = (struct bci_eeb*)malloc(__blk_c*sizeof(struct bci_eeb));
}

void bci_eeb_put(struct bci *__bci, mdl_u8_t __blk_no, bci_addr_t __b_addr, bci_addr_t __e_addr) {
	struct bci_eeb *eeb = __bci->eeb_list+__blk_no;
	eeb->b_addr = __b_addr;
	eeb->e_addr = __e_addr;
}

void bci_eeb_call(struct bci *__bci, mdl_u8_t __blk_no) {
	struct bci_eeb *eeb = __bci->eeb_list+__blk_no;

	mdl_uint_t addr = eeb->b_addr;
	while(__bci->get_ip() != eeb->e_addr) {
		bci_exec(__bci, addr, NULL, NULL, _bcie_fsie);
		addr = __bci->get_ip();
	}
}

void bci_unlock_addr(struct bci *__bci, bci_addr_t __addr) {
	*(__bci->locked_addrs+(__addr>>3)) ^= (1<<(__addr-((__addr>>3)*(1<<3))))&(*(__bci->locked_addrs+(__addr>>3)));
}

void _bci_stop(void *__arg) {
	struct bci *_bci = (struct bci*)__arg;
	static void(*old_iei_fp)(void*) = NULL;
	static void *old_iei_arg = NULL;

	if (!is_flag(_bci->flags, _bci_fstop)) {
		_bci->flags |= _bci_fstop;
		old_iei_fp = _bci->iei_fp;
		old_iei_arg = _bci->iei_arg;

		_bci->iei_fp = &_bci_stop;
		_bci->iei_arg = __arg;
	} else {
		if (_bci->state == BCI_STOPPED) {
			_bci->iei_fp = old_iei_fp;
			_bci->iei_arg = old_iei_arg;

			_bci->flags ^= _bci_fstop;
		}
	}
}

void bci_set_ula_guard(struct bci *__bci, mdl_u8_t(*__guard)(void*, void*), void *__arg_p) {
	__bci->ula_guard = __guard;
	__bci->ulag_arg = __arg_p;
}

void bci_stop(struct bci *__bci) {_bci_stop((void*)__bci);}
void _nop();
void _exit();
void _assign();
void _mov();
void _aop();
void _incr_or_decr();
void _cmp();
void _jmp();
void _cjmp();
void _dr();
void _conv();
void _extern_call();
void _eeb_init();
void _eeb_put();
void _act_indc();
void _print();
void _lop();
void _shr_or_shl();
void _la();
void _getarg();
void _ula();

static void(*i[])() = {
	&_nop,
	&_exit,
	&_assign,
	&_mov,
	&_aop,
	&_incr_or_decr,
	&_incr_or_decr,
	&_cmp,
	&_jmp,
	&_cjmp,
	&_dr,
	&_conv,
	&_extern_call,
	&_eeb_init,
	&_eeb_put,
	&_act_indc,
	&_print,
	&_lop,
	&_shr_or_shl,
	&_shr_or_shl,
	&_la,
	&_getarg,
	&_ula
};

void mem_cpy(void *__dst, void *__src, mdl_uint_t __bc) {
	mdl_u8_t *itr = (mdl_u8_t*)__src;
	while(itr != (mdl_u8_t*)__src+__bc) {
		*((mdl_u8_t*)__dst+(itr-(mdl_u8_t*)__src)) = *itr;
		itr++;
	}
}

# define jmpdone __asm__("jmp _done\nnop")
# define jmpto(__p) __asm__("jmp *%0\nnop" : : "r"(__p))
# define jmpnext __asm__("jmp _next\nnop")
# define jmpend __asm__("jmp _end\nnop")
bci_err_t bci_exec(struct bci *__bci, bci_addr_t __entry_addr, bci_addr_t *__exit_addr, bci_err_t *__exit_status, bci_flag_t __flags) {
	__bci->set_ip(__entry_addr);
	__bci->state = BCI_RUNNING;

	__asm__("_next:\nnop");
	if (__bci->iei_fp != NULL) __bci->iei_fp(__bci->iei_arg);
	if (is_flag(__bci->flags, _bci_fstop)) jmpend;
	{
		__bci->ip_off = 0;
		mdl_u8_t ino = get_8l(__bci);
		bci_flag_t flags = 0x0;
		get(__bci, (mdl_u8_t*)&flags, sizeof(bci_flag_t));
		jmpto(i[ino]);
		jmpend;
		__asm__("_ula:\nnop"); {
			bci_addr_t maddr = get_16l(__bci);
			bci_addr_t addr = 0x0000;
			stack_get(__bci, (mdl_u8_t*)&addr, bcit_sizeof(_bcit_addr), maddr);
			bci_addr_t arg_maddr = get_16l(__bci);
			bci_addr_t arg_addr = 0x0000;
			stack_get(__bci, (mdl_u8_t*)&arg_addr, bcit_sizeof(_bcit_addr), arg_maddr);
			if (__bci->ula_guard != NULL) {
				if (!__bci->ula_guard(__bci->ulag_arg, bci_resolv_addr(__bci, arg_addr)))
					bci_unlock_addr(__bci, addr);
			}
			jmpdone;
		}
		__asm__("_getarg:\nnop"); {
			bci_addr_t buf_maddr = get_16l(__bci);
			bci_addr_t bc_maddr = get_16l(__bci);

			bci_addr_t buf_addr = 0x0000;
			stack_get(__bci, (mdl_u8_t*)&buf_addr, bcit_sizeof(_bcit_addr), buf_maddr);

			bci_addr_t bc_addr = 0x0000;
			stack_get(__bci, (mdl_u8_t*)&bc_addr, bcit_sizeof(_bcit_addr), bc_maddr);

			mdl_u8_t *buf = (mdl_u8_t*)bci_resolv_addr(__bci, buf_addr);

			mdl_u8_t no = 0x0;
			stack_get(__bci, (mdl_u8_t*)&no, bcit_sizeof(_bcit_8l), get_16l(__bci));
			struct bci_arg *arg = bci_get_arg(__bci, no);
			if (arg->p != NULL) {
			mem_cpy(buf, arg->p, arg->bc);
			stack_put(__bci, (mdl_u8_t*)&arg->bc, bcit_sizeof(_bcit_16l), bc_addr);
			}
			jmpdone;
		}

		__asm__("_la:\nnop"); {
			bci_addr_t maddr = get_16l(__bci);
			bci_addr_t addr = 0x0000;
			if (stack_get(__bci, (mdl_u8_t*)&addr, bcit_sizeof(_bcit_addr), maddr) != BCI_SUCCESS) jmpend;
			*(__bci->locked_addrs+(addr>>3)) |= 1<<(addr-((addr>>3)*(1<<3)));
			printf("locked addr: %u , %u\n", addr, maddr);
			jmpdone;
		}
		__asm__("_nop:\nnop");
		// nothing
		jmpdone;
		__asm__("_act_indc:\nnop");
			if (__bci->act_indc_fp != NULL) __bci->act_indc_fp();
		jmpdone;
		__asm__("_eeb_init:\nnop");
			bci_eeb_init(__bci, get_8l(__bci));
		jmpdone;
		__asm__("_eeb_put:\nnop"); {
			mdl_u8_t blk_no = get_8l(__bci);
			bci_addr_t b_addr = get_16l(__bci);
			bci_addr_t e_addr = get_16l(__bci);
			bci_eeb_put(__bci, blk_no, b_addr, e_addr);
			jmpdone;
		}
		__asm__("_lop:\nnop"); {
			mdl_u8_t lop_kind = get_8l(__bci);
			mdl_u8_t type = get_8l(__bci)&0xF8;
			mdl_u64_t l_val = 0, r_val = 0;
			bci_addr_t dst_addr = get_16l(__bci);

			if (is_flag(flags, _bcii_lop_fl_pm))
				get(__bci, (mdl_u8_t*)&l_val, bcit_sizeof(type));
			else {
				bci_addr_t l_addr = get_16l(__bci);
				if (stack_get(__bci, (mdl_u8_t*)&l_val, bcit_sizeof(type), l_addr) != BCI_SUCCESS) jmpend;
			}

			if (is_flag(flags, _bcii_lop_fr_pm))
				get(__bci, (mdl_u8_t*)&r_val, bcit_sizeof(type));
			else {
				bci_addr_t r_addr = get_16l(__bci);
				if (stack_get(__bci, (mdl_u8_t*)&r_val, bcit_sizeof(type), r_addr) != BCI_SUCCESS) jmpend;
			}

			mdl_u64_t final = 0;
			switch(lop_kind) {
				case _bci_lop_xor:
					final = l_val^r_val;
				break;
				case _bci_lop_or:
					final = l_val|r_val;
				break;
				case _bci_lop_and:
					final = l_val&r_val;
				break;
			}
			if (stack_put(__bci, (mdl_u8_t*)&final, bcit_sizeof(type), dst_addr) != BCI_SUCCESS) jmpend;
			jmpdone;
		}
		__asm__("_shr_or_shl:\nnop"); {
			mdl_u8_t type = get_8l(__bci)&0xF8;
			bci_addr_t val_addr = get_16l(__bci);

			mdl_u64_t val = 0;
			if (stack_get(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), val_addr) != BCI_SUCCESS) jmpend;

			mdl_u8_t _type = get_8l(__bci)&0xF8;
			mdl_u64_t sc = 0;
			get(__bci, (mdl_u8_t*)&sc, bcit_sizeof(_type));
			if (ino == _bcii_shr) val >>= sc;
			else if (ino == _bcii_shl) val <<= sc;
			if (stack_put(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), val_addr) != BCI_SUCCESS) jmpend;
			jmpdone;
		}
		__asm__("_extern_call:\nnop"); {
			mdl_u8_t ret_type = get_8l(__bci)&0xFC;
			bci_addr_t ret_addr = get_16l(__bci);
			bci_addr_t id_addr = get_16l(__bci);
			bci_addr_t arg_addr = get_16l(__bci);

			arg_addr = *(bci_addr_t*)(__bci->mem_stack+arg_addr);
			if (__bci->extern_fp != NULL) {
				void *ret = __bci->extern_fp(*(mdl_u8_t*)(__bci->mem_stack+id_addr), (void*)(__bci->mem_stack+arg_addr));
				if (stack_put(__bci, (mdl_u8_t*)ret, bcit_sizeof(ret_type), ret_addr) != BCI_SUCCESS) jmpend;
			}
			jmpdone;
		}
		__asm__("_conv:\nnop"); {
			mdl_u8_t to_type = get_8l(__bci)&0xFC;
			mdl_u8_t from_type = get_8l(__bci);

			mdl_u8_t ft_signed = (from_type&_bcit_msigned) == _bcit_msigned;
			if (ft_signed) from_type ^= _bcit_msigned;

			bci_addr_t dst_addr = get_16l(__bci);
			bci_addr_t src_addr = get_16l(__bci);

			mdl_u64_t val = 0;
			if (stack_get(__bci, (mdl_u8_t*)&val, bcit_sizeof(from_type), src_addr) != BCI_SUCCESS) jmpend;
			if (ft_signed && bcit_sizeof(to_type) > bcit_sizeof(from_type) && val > ((mdl_u64_t)1<<(bcit_sizeof(from_type)*8))>>1)
				val |= (~(mdl_u64_t)0)<<(bcit_sizeof(from_type)*8);

			if (stack_put(__bci, (mdl_u8_t*)&val, bcit_sizeof(to_type), dst_addr) != BCI_SUCCESS) jmpend;
			jmpdone;
		}
		__asm__("_dr:\nnop"); {
			mdl_u8_t type = get_8l(__bci)&0xFC;
			bci_addr_t src_addr = get_16l(__bci);
			bci_addr_t dst_addr = get_16l(__bci);

			bci_addr_t addr = 0;
			if (stack_get(__bci, (mdl_u8_t*)&addr, bcit_sizeof(_bcit_addr), src_addr) != BCI_SUCCESS) jmpend;

			mdl_u64_t val = 0;
			if (stack_get(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), addr) != BCI_SUCCESS) jmpend;
			if (stack_put(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), dst_addr) != BCI_SUCCESS) jmpend;
			jmpdone;
		}
		__asm__("_print:\nnop"); {
			mdl_u8_t type = get_8l(__bci);
			mdl_u8_t _signed = (type&_bcit_msigned) == _bcit_msigned;
			if (_signed) type ^= _bcit_msigned;

			bci_addr_t addr = get_16l(__bci);
			mdl_u64_t val = 0;
			if (stack_get(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), addr) != BCI_SUCCESS) jmpend;

			if (_signed) {
				mdl_u64_t p = val^(val&(((mdl_u64_t)~0)<<(bcit_sizeof(type)*8)));
				mdl_i64_t n = (mdl_i64_t)(val|(((mdl_u64_t)~0)<<(bcit_sizeof(type)*8)));
				if (p > ((mdl_u64_t)1<<(bcit_sizeof(type)*8))>>1) fprintf(stdout, "%ld\n", (mdl_i64_t)n);
				if (p < ((mdl_u64_t)1<<(bcit_sizeof(type)*8))>>1) fprintf(stdout, "%lu\n", p);
			} else
				fprintf(stdout, "%lu\n", val);
			jmpdone;
		}
		__asm__("_assign:\nnop"); {
			mdl_u8_t type = get_8l(__bci)&0xFC;
			bci_addr_t addr = get_16l(__bci);

			if (is_flag(flags, _bcii_assign_fdr_addr))
				if (stack_get(__bci, (mdl_u8_t*)&addr, bcit_sizeof(_bcit_addr), addr) != BCI_SUCCESS) jmpend;

			mdl_u64_t val = 0;
			get(__bci, (mdl_u8_t*)&val, bcit_sizeof(type));
			if (stack_put(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), addr) != BCI_SUCCESS) jmpend;
			jmpdone;
		}
		__asm__("_aop:\nnop"); {
			mdl_u8_t aop_kind = get_8l(__bci);
			mdl_u8_t type = get_8l(__bci);
			mdl_u8_t _signed = (type&_bcit_msigned) == _bcit_msigned;
			if (_signed) type ^= _bcit_msigned;

			bci_addr_t dst_addr = get_16l(__bci);

			mdl_u64_t l_val = 0, r_val = 0;
			if (is_flag(flags, _bcii_aop_fl_pm))
				get(__bci, (mdl_u8_t*)&l_val, bcit_sizeof(type));
			else {
				bci_addr_t l_addr = get_16l(__bci);
				if (stack_get(__bci, (mdl_u8_t*)&l_val, bcit_sizeof(type), l_addr) != BCI_SUCCESS) jmpend;
			}

			if (is_flag(flags, _bcii_aop_fr_pm))
				get(__bci, (mdl_u8_t*)&r_val, bcit_sizeof(type));
			else {
				bci_addr_t r_addr = get_16l(__bci);
				if (stack_get(__bci, (mdl_u8_t*)&r_val, bcit_sizeof(type), r_addr) != BCI_SUCCESS) jmpend;
			}

			mdl_u64_t final;
			switch(aop_kind) {
				case _bci_aop_add:
					final = _signed? (mdl_i64_t)l_val+(mdl_i64_t)r_val:l_val+r_val;
				break;
				case _bci_aop_mul:
					final = _signed? (mdl_i64_t)l_val*(mdl_i64_t)r_val:l_val*r_val;
				break;
				case _bci_aop_sub:
					final = _signed? (mdl_i64_t)l_val-(mdl_i64_t)r_val:l_val-r_val;
				break;
				case _bci_aop_div:
					final = _signed? (mdl_i64_t)l_val/(mdl_i64_t)r_val:l_val/r_val;
				break;
			}
			if (stack_put(__bci, (mdl_u8_t*)&final, bcit_sizeof(type), dst_addr) != BCI_SUCCESS) jmpend;
			jmpdone;
		}
		__asm__("_mov:\nnop"); {
			mdl_u8_t type = get_8l(__bci)&0xFC;
			bci_addr_t dst_addr = get_16l(__bci);
			if (is_flag(flags, _bcii_mov_fdr_da))
				if (stack_get(__bci, (mdl_u8_t*)&dst_addr, bcit_sizeof(_bcit_addr), dst_addr) != BCI_SUCCESS) jmpend;

			bci_addr_t src_addr = get_16l(__bci);
			if (is_flag(flags, _bcii_mov_fdr_sa))
				if (stack_get(__bci, (mdl_u8_t*)&src_addr, bcit_sizeof(_bcit_addr), src_addr) != BCI_SUCCESS) jmpend;

			mdl_u64_t src_val = 0;
			if (stack_get(__bci, (mdl_u8_t*)&src_val, bcit_sizeof(type), src_addr) != BCI_SUCCESS) jmpend;
			if (stack_put(__bci, (mdl_u8_t*)&src_val, bcit_sizeof(type), dst_addr) != BCI_SUCCESS) jmpend;
			jmpdone;
		}
		__asm__("_incr_or_decr:\nnop"); {
			mdl_u8_t type = get_8l(__bci);
			bci_addr_t addr = get_16l(__bci);
			mdl_u64_t val = 0;
			mdl_u8_t _signed = (type&_bcit_msigned) == _bcit_msigned;
			if (_signed) type ^= _bcit_msigned;

			if (stack_get(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), addr) != BCI_SUCCESS) jmpend;

			mdl_uint_t by = 0;
			if (is_flag(flags, _bcii_iod_fbc_addr)) {
				bci_addr_t by_addr = 0;
				get(__bci, (mdl_u8_t*)&by_addr, bcit_sizeof(_bcit_addr));
				if (stack_get(__bci, (mdl_u8_t*)&by, bcit_sizeof(type), by_addr) != BCI_SUCCESS) jmpend;
			} else {
				mdl_u8_t _type = get_8l(__bci);
				get(__bci, (mdl_u8_t*)&by, bcit_sizeof(_type));
			}

			if (_signed) val |= (~(mdl_u64_t)0)<<(bcit_sizeof(type)*8);

			if (ino == _bcii_incr)
				val = _signed?(mdl_i64_t)val+by:val+by;
			else if (ino == _bcii_decr)
				val = _signed?(mdl_i64_t)val-by:val-by;
			if (stack_put(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), addr) != BCI_SUCCESS) jmpend;
			jmpdone;
		}
		__asm__("_cmp:\nnop"); {
			mdl_u8_t l_type = get_8l(__bci);
			mdl_u8_t r_type = get_8l(__bci);

			mdl_u8_t l_signed = (l_type&_bcit_msigned) == _bcit_msigned;
			mdl_u8_t r_signed = (r_type&_bcit_msigned) == _bcit_msigned;
			if (l_signed) l_type ^= _bcit_msigned;
			if (r_signed) r_type ^= _bcit_msigned;

			bci_addr_t l_addr = get_16l(__bci);
			bci_addr_t r_addr = get_16l(__bci);

			mdl_u64_t l_val = 0, r_val = 0;
			if (stack_get(__bci, (mdl_u8_t*)&l_val, bcit_sizeof(l_type), l_addr) != BCI_SUCCESS) jmpend;
			if (stack_get(__bci, (mdl_u8_t*)&r_val, bcit_sizeof(r_type), r_addr) != BCI_SUCCESS) jmpend;

			if (l_val > ((mdl_u64_t)1<<(bcit_sizeof(l_type)*8))>>1 && l_signed)
				l_val |= (~(mdl_u64_t)0)<<(bcit_sizeof(l_type)*8);
			if (r_val > ((mdl_u64_t)1<<(bcit_sizeof(r_type)*8))>>1 && r_signed)
				r_val |= (~(mdl_u64_t)0)<<(bcit_sizeof(r_type)*8);
# ifdef __DEBUG_ENABLED
			fprintf(stdout, "l_val: %ld, r_val: %ld\n", (mdl_i64_t)l_val, (mdl_i64_t)r_val);
# endif
			mdl_u8_t dst_addr = get_16l(__bci);
			mdl_u8_t flags = 0;
			if (l_signed?(r_signed?((mdl_i64_t)l_val == (mdl_i64_t)r_val):((mdl_i64_t)l_val == r_val)):(r_signed?(l_val == (mdl_i64_t)r_val):(l_val == r_val))) flags |= _bcif_eq;
			if (l_signed?(r_signed?((mdl_i64_t)l_val > (mdl_i64_t)r_val):((mdl_i64_t)l_val > r_val)):(r_signed?(l_val > (mdl_i64_t)r_val):(l_val > r_val))) flags |= _bcif_gt;
			if (l_signed?(r_signed?((mdl_i64_t)l_val < (mdl_i64_t)r_val):((mdl_i64_t)l_val < r_val)):(r_signed?(l_val < (mdl_i64_t)r_val):(l_val < r_val))) flags |= _bcif_lt;
# ifdef __DEBUG_ENABLED
			fprintf(stdout, "l_signed: %u, r_signed: %u\n", l_signed, r_signed);
			fprintf(stdout, "eq: %s, gt: %s, lt: %s\n", is_flag(flags, _bcif_eq)?"yes":"no", is_flag(flags, _bcif_gt)?"yes":"no", is_flag(flags, _bcif_lt)?"yes":"no");
# endif
			if (stack_put(__bci, &flags, 1, dst_addr) != BCI_SUCCESS) jmpend;
			jmpdone;
		}
		__asm__("_cjmp:\nnop"); {
			mdl_u8_t cond = get_8l(__bci);
			bci_addr_t jmpm_addr = get_16l(__bci);
			bci_addr_t flags_addr = get_16l(__bci);

			bci_addr_t jmp_addr = 0x0000;
			if (stack_get(__bci, (mdl_u8_t*)&jmp_addr, bcit_sizeof(_bcit_addr), jmpm_addr) != BCI_SUCCESS) jmpend;
			mdl_u8_t flags = 0;
			if (stack_get(__bci, &flags, 1, flags_addr) != BCI_SUCCESS) jmpend;
			switch(cond) {
				case _bcic_eq: if (is_flag(flags, _bcif_eq)) goto _jmp; break;
				case _bcic_neq: if (!is_flag(flags, _bcif_eq)) goto _jmp; break;
				case _bcic_gt: if (is_flag(flags, _bcif_gt)) goto _jmp; break;
				case _bcic_lt: if (is_flag(flags, _bcif_lt)) goto _jmp; break;
				case _bcic_geq:
					if (is_flag(flags, _bcif_gt) || is_flag(flags, _bcif_eq))
						goto _jmp;
				break;
				case _bcic_leq:
					if (is_flag(flags, _bcif_lt) || is_flag(flags, _bcif_eq))
						goto _jmp;
				break;
			}
			jmpdone;
			_jmp:
			__bci->set_ip(jmp_addr);
			jmpnext;
		}
		__asm__("_jmp:\nnop"); {
			bci_addr_t jmpm_addr = get_16l(__bci);
			bci_addr_t jmp_addr = 0x0000;
			if (stack_get(__bci, (mdl_u8_t*)&jmp_addr, bcit_sizeof(_bcit_addr), jmpm_addr) != BCI_SUCCESS) jmpend;
			__bci->set_ip(jmp_addr);
			jmpnext;
		}
		// required by design
		__asm__("_exit:\nnop"); {
			if (__exit_addr != NULL)
				*__exit_addr = __bci->get_ip();
			bci_addr_t addr = get_16l(__bci);
			bci_err_t exit_status = 0x0;
			if (__exit_status != NULL) {
				if (stack_get(__bci, (mdl_u8_t*)&exit_status, sizeof(bci_err_t), addr) == BCI_SUCCESS)
					*__exit_status = exit_status;
			}
			jmpend;
		}
	}
	__asm__("_done:\nnop");
	__bci->ip_incr(__bci->ip_off);
	if (is_flag(__flags, _bcie_fsie)) jmpend;
	jmpnext;
	__bci->state = BCI_STOPPED;
	__asm__("_end:\nnop");
	return BCI_SUCCESS;
}
