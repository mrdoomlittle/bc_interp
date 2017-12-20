# include "bci.h"
# define _err(__r) ((__r) != BCI_SUCCESS)
# define _ok(__r) ((__r) == BCI_SUCCESS)
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
	if ((_bci->get_ip()+_bci->ip_off) >= _bci->prog_size) {
		_bci->err = BCI_FAILURE;
		return 0x0;
	}
	_bci->err = BCI_SUCCESS;
	mdl_u8_t val = _bci->get_byte(_bci->ip_off++);
	return val;
}

void* bci_resolv_addr(struct bci *__bci, bci_addr_t __addr) {
	if (__addr >= __bci->stack_size) return NULL;
	return (void*)(__bci->mem_stack+__addr);
}

void bci_set_exc(struct bci *__bci, void*(*__exc)(mdl_u8_t, void*)) {__bci->exc = __exc;}
void bci_set_act_indc(struct bci *__bci, void(*__act_indc)()) {__bci->act_indc = __act_indc;}
void bci_set_iei(struct bci *__bci, void(*__iei)(void*)) {__bci->iei = __iei;}
void bci_set_iei_arg(struct bci *__bci, void *__iei_arg) {__bci->iei_arg = __iei_arg;}

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
	if ((__bci->mem_stack = (mdl_u8_t*)malloc(__bci->stack_size)) == NULL)
		return BCI_FAILURE;
	if (__arg_c > 0) {
		if ((__bci->args = (struct bci_arg*)malloc(__arg_c*sizeof(struct bci_arg))) == NULL)
			return BCI_FAILURE;
	}
	struct bci_arg *at = __bci->args;
	while(at != __bci->args+__arg_c)
		*(at++) = (struct bci_arg) {.p = NULL, .bc = 0};

	mdl_u16_t bytes;
	if ((__bci->locked_addrs = (mdl_u8_t*)malloc((bytes = ((__bci->stack_size>>3)+((__bci->stack_size-((__bci->stack_size>>3)*(1<<3)))>0))))) == NULL)
		return BCI_FAILURE;
	memset(__bci->locked_addrs, 0x0, bytes);
	__bci->eeb_list = NULL;
	__bci->act_indc = NULL;
	bitct_init(&__bci->_bitct, &next_byte, NULL);
	set_get_arg(&__bci->_bitct, (void*)__bci);
	memset(__bci->mem_stack, 0xFF, __bci->stack_size);
	__bci->exc = NULL;
	__bci->iei = NULL;
	__bci->flags = 0;
	__bci->m_wr = 0;
	__bci->m_rd = 0;
	__bci->ula_guard = NULL;
	__bci->ulag_arg = NULL;
	__bci->err = BCI_SUCCESS;
	return BCI_SUCCESS;
}

bci_err_t bci_de_init(struct bci *__bci) {
	free(__bci->mem_stack);
	free(__bci->locked_addrs);
	if (__bci->eeb_list != NULL) free(__bci->eeb_list);
	return BCI_SUCCESS;
}

mdl_u8_t static get_lx(struct bci *__bci, mdl_u8_t __l) {
	return bitct_get_lx(&__bci->_bitct, __l);
}

# define get_4l(__bci) get_lx(__bci, 4)
mdl_u8_t static get_8l(struct bci *__bci, bci_err_t *__err) {
	mdl_u8_t ret = get_lx(__bci, 8);
	*__err = __bci->err;
	return ret;
}

mdl_u16_t static get_16l(struct bci *__bci, bci_err_t *__err) {
	return ((mdl_u16_t)get_8l(__bci, __err))|((mdl_u16_t)get_8l(__bci, __err))<<8;
}

mdl_u32_t static get_32l(struct bci *__bci, bci_err_t *__err) {
	return ((mdl_u32_t)get_16l(__bci, __err))|((mdl_u32_t)get_16l(__bci, __err))<<16;
}

mdl_u64_t static get_64l(struct bci *__bci, bci_err_t *__err) {
	return ((mdl_u64_t)get_32l(__bci, __err))|((mdl_u64_t)get_32l(__bci, __err))<<32;
}

bci_addr_t static get_addr(struct bci *__bci, bci_err_t *__err) {
	switch(_bci_addr_size) {
		case 1: return get_8l(__bci, __err);
		case 2:	return get_16l(__bci, __err);
		case 4:	return get_32l(__bci, __err);
		case 8:	return get_64l(__bci, __err);
	}
	*__err = BCI_FAILURE;
	return 0;
}

void static get(struct bci *__bci, mdl_u8_t *__val, mdl_uint_t __bc, bci_err_t *__err) {
	mdl_u8_t *itr = __val;
	for (;itr != __val+__bc;itr++) {
		*itr = get_8l(__bci, __err);
		if (_err(*__err)) break;
	}
}

mdl_u8_t bcit_sizeof(mdl_u8_t __type) {
	switch(__type^(__type&0x3)) {
		case _bcit_void: return 0;
		case _bcit_8l: return sizeof(mdl_u8_t);
		case _bcit_16l: return sizeof(mdl_u16_t);
		case _bcit_32l: return sizeof(mdl_u32_t);
		case _bcit_64l: return sizeof(mdl_u64_t);
		case _bcit_addr: return _bci_addr_size;
	}
	return 0;
}

mdl_u8_t is_flag(bci_flag_t __flags, bci_flag_t __flag) {
	return (__flags&__flag) == __flag;}

mdl_uint_t bcii_sizeof(mdl_u8_t *__p, bci_flag_t __flags) {
	switch(*__p) {
		case _bcii_nop:					return 0;
		case _bcii_exit:				return 0;
		case _bcii_as:                  return 1+_bci_addr_size+bcit_sizeof(*(__p+2));
		case _bcii_mov:                 return 1+(_bci_addr_size*2);
		case _bcii_aop: {
			mdl_uint_t size = 2+_bci_addr_size;
			if (is_flag(__flags, _bcii_aop_fl_pm)) size+= _bci_addr_size;
			if (is_flag(__flags, _bcii_aop_fr_pm)) size+= _bci_addr_size;
			return size;
		}
		case _bcii_incr: case _bcii_decr:
			if (is_flag(__flags, _bcii_iod_fbc_addr)) return 1+(_bci_addr_size*2);
			else
				return 2+_bci_addr_size+bcit_sizeof(*(__p+5));
		case _bcii_cmp:                 return 2+(_bci_addr_size*2);
		case _bcii_jmp:                 return _bci_addr_size;
		case _bcii_cjmp:                return 1+(_bci_addr_size*2);
		case _bcii_dr:                  return 1+(_bci_addr_size*2);
		case _bcii_conv:                return 2+(_bci_addr_size*2);
		case _bcii_exc:				return 1+(_bci_addr_size*3);
		case _bcii_eeb_init:            return 1;
		case _bcii_eeb_put:             return 1+(_bci_addr_size*2);
		case _bcii_act_indc:					return 0;
		case _bcii_print:				return 1+_bci_addr_size;
		case _bcii_lop: {
			mdl_uint_t size = 2+_bci_addr_size;
			if (is_flag(__flags, _bcii_lop_fl_pm)) size+= _bci_addr_size;
			if (is_flag(__flags, _bcii_lop_fr_pm)) size+= _bci_addr_size;
			return size;
		}
		case _bcii_shr: case _bcii_shl: return 1+_bci_addr_size+bcit_sizeof(*(__p+4));
		case _bcii_la:					return _bci_addr_size;
		case _bcii_getarg:				return _bci_addr_size*3;
		case _bcii_ula:					return _bci_addr_size;
	}
	return 0;
}

mdl_uint_t bcii_overhead_size() {return 2;}
void bci_eeb_init(struct bci *__bci, mdl_u8_t __blk_c) {
	if (__bci->eeb_list != NULL) return;
	__bci->eeb_list = (struct bci_eeb*)malloc(__blk_c*sizeof(struct bci_eeb));
}

bci_err_t bci_eeb_put(struct bci *__bci, mdl_u8_t __blk_no, bci_addr_t __b_addr, bci_addr_t __e_addr) {
	struct bci_eeb *blk = __bci->eeb_list+__blk_no;
	if (__b_addr >= __bci->prog_size || __e_addr >= __bci->prog_size) {
		return BCI_FAILURE;
	}
	blk->b_addr = __b_addr;
	blk->e_addr = __e_addr;
	return BCI_SUCCESS;
}

void bci_eeb_call(struct bci *__bci, mdl_u8_t __blk_no) {
	struct bci_eeb *blk = __bci->eeb_list+__blk_no;
	mdl_uint_t addr = blk->b_addr;
	while(__bci->get_ip() < blk->e_addr) {
		if (_err(bci_exec(__bci, addr, NULL, NULL, _bcie_fsie))) {
			break;
		}
		addr = __bci->get_ip();
	}
}

void bci_unlock_addr(struct bci *__bci, bci_addr_t __addr) {
	if (__addr >= __bci->stack_size) return;
	*(__bci->locked_addrs+(__addr>>3)) ^= (1<<(__addr-((__addr>>3)*(1<<3))))&(*(__bci->locked_addrs+(__addr>>3)));
}

void _bci_stop(void *__arg) {
	struct bci *_bci = (struct bci*)__arg;
	static void(*old_iei)(void*) = NULL;
	static void *old_iei_arg = NULL;

	if (!is_flag(_bci->flags, _bci_fstop)) {
		_bci->flags |= _bci_fstop;
		old_iei = _bci->iei;
		old_iei_arg = _bci->iei_arg;

		_bci->iei = &_bci_stop;
		_bci->iei_arg = __arg;
	} else {
		if (_bci->state == BCI_STOPPED) {
			_bci->iei = old_iei;
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
void _as();
void _mov();
void _aop();
void _incr_or_decr();
void _cmp();
void _jmp();
void _cjmp();
void _dr();
void _conv();
void _exc();
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
	&_as,
	&_mov,
	&_aop,
	&_incr_or_decr,
	&_incr_or_decr,
	&_cmp,
	&_jmp,
	&_cjmp,
	&_dr,
	&_conv,
	&_exc,
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

void static mem_cpy(void *__dst, void *__src, mdl_uint_t __bc) {
	mdl_u8_t *itr = (mdl_u8_t*)__src;
	while(itr != (mdl_u8_t*)__src+__bc) {
		*((mdl_u8_t*)__dst+(itr-(mdl_u8_t*)__src)) = *itr;
		itr++;
	}
}

# define jmpdone __asm__("jmp _done")
# ifdef __AVR
#	define jmpto(__p) __asm__("ijmp" : : "z"(__p))
# else
#	define jmpto(__p) __asm__("jmp *%0" : : "r"(__p))
# endif
# define jmpnext __asm__("jmp _next")
# define jmpend __asm__("jmp _end")
# define errjmp if (_err(err)) jmpend
bci_err_t bci_exec(struct bci *__bci, bci_addr_t __entry_addr, bci_addr_t *__exit_addr, bci_err_t *__exit_status, bci_flag_t __flags) {
	bci_err_t err = BCI_SUCCESS;
	__bci->set_ip(__entry_addr);
	__bci->state = BCI_RUNNING;
	__asm__("_next:");
	if (__bci->iei != NULL) __bci->iei(__bci->iei_arg);
	if (is_flag(__bci->flags, _bci_fstop)) jmpend;
	{
		__bci->ip_off = 0;
		mdl_u8_t ino = get_8l(__bci, &err);
		if (_err(err)) jmpend;

		bci_flag_t flags = 0x0;
		get(__bci, (mdl_u8_t*)&flags, sizeof(bci_flag_t), &err);
		if (_err(err)) jmpend;

		jmpto(i[ino]);
		jmpend;
		__asm__("_ula:");
		{
			bci_addr_t maddr = get_addr(__bci, &err);
			errjmp;
			bci_addr_t addr;
			if (_err(err = stack_get(__bci, (mdl_u8_t*)&addr, _bci_addr_size, maddr))) jmpend;

			bci_addr_t arg_maddr = get_addr(__bci, &err);
			errjmp;

			bci_addr_t arg_addr;
			if (_err(err = stack_get(__bci, (mdl_u8_t*)&arg_addr, _bci_addr_size, arg_maddr))) jmpend;
			if (arg_addr >= __bci->stack_size) {
				err = BCI_FAILURE;
				jmpend;
			}
			if (__bci->ula_guard != NULL) {
				if (!__bci->ula_guard(__bci->ulag_arg, bci_resolv_addr(__bci, arg_addr)))
					bci_unlock_addr(__bci, addr);
			}
			jmpdone;
		}
		__asm__("_getarg:");
		{

			bci_addr_t buf_maddr = get_addr(__bci, &err);
			errjmp;
			bci_addr_t bc_maddr = get_addr(__bci, &err);
			errjmp;

			bci_addr_t buf_addr;
			if (_err(err = stack_get(__bci, (mdl_u8_t*)&buf_addr, _bci_addr_size, buf_maddr))) jmpend;

			bci_addr_t bc_addr;
			if (_err(err = stack_get(__bci, (mdl_u8_t*)&bc_addr, _bci_addr_size, bc_maddr))) jmpend;
			if (buf_addr >= __bci->stack_size) {
				err = BCI_FAILURE;
				jmpend;
			}


			mdl_u8_t *buf = (mdl_u8_t*)bci_resolv_addr(__bci, buf_addr);
			mdl_u8_t no;
			if (_err(err = stack_get(__bci, &no, 1, get_addr(__bci, &err)))) jmpend;
			struct bci_arg *arg = bci_get_arg(__bci, no);
			if (arg->p != NULL) {
				mem_cpy(buf, arg->p, arg->bc);
				if (_err(err = stack_put(__bci, (mdl_u8_t*)&arg->bc, bcit_sizeof(_bcit_16l), bc_addr))) jmpend;
			}

			jmpdone;

		}
		__asm__("_la:");
		{
			bci_addr_t maddr = get_addr(__bci, &err);
			errjmp;
			bci_addr_t addr;
			if (_err(err = stack_get(__bci, (mdl_u8_t*)&addr, _bci_addr_size, maddr))) jmpend;
			*(__bci->locked_addrs+(addr>>3)) |= 1<<(addr-((addr>>3)*(1<<3)));
			printf("locked addr: %u , %u\n", addr, maddr);
			jmpdone;
		}
		__asm__("_nop:");
		// nothing
		jmpdone;
		__asm__("_act_indc:");
			if (__bci->act_indc != NULL) __bci->act_indc();
		jmpdone;
		__asm__("_eeb_init:");
			bci_eeb_init(__bci, get_8l(__bci, &err));
		jmpdone;
		__asm__("_eeb_put:");
		{
			mdl_u8_t blk_no = get_8l(__bci, &err);
			errjmp;
			bci_addr_t b_addr = get_16l(__bci, &err);
			errjmp;
			bci_addr_t e_addr = get_16l(__bci, &err);
			errjmp;
			if (_err(err = bci_eeb_put(__bci, blk_no, b_addr, e_addr))) jmpend;
			jmpdone;
		}
		__asm__("_lop:");
		{
			mdl_u8_t lop_kind = get_8l(__bci, &err);
			errjmp;
			mdl_u8_t type = get_8l(__bci, &err)&0xFC;
			errjmp;
			mdl_u64_t l_val = 0, r_val = 0;
			bci_addr_t dst_addr = get_addr(__bci, &err);
			errjmp;

			if (is_flag(flags, _bcii_lop_fl_pm)) {
				get(__bci, (mdl_u8_t*)&l_val, bcit_sizeof(type), &err);
				errjmp;
			} else {
				bci_addr_t l_addr = get_addr(__bci, &err);
				errjmp;
				if (_err(err = stack_get(__bci, (mdl_u8_t*)&l_val, bcit_sizeof(type), l_addr))) jmpend;
			}

			if (is_flag(flags, _bcii_lop_fr_pm)) {
				get(__bci, (mdl_u8_t*)&r_val, bcit_sizeof(type), &err);
				errjmp;
			} else {
				bci_addr_t r_addr = get_addr(__bci, &err);
				errjmp;
				if (_err(err = stack_get(__bci, (mdl_u8_t*)&r_val, bcit_sizeof(type), r_addr))) jmpend;
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
			if (_err(err = stack_put(__bci, (mdl_u8_t*)&final, bcit_sizeof(type), dst_addr))) jmpend;
			jmpdone;
		}
		__asm__("_shr_or_shl:");
		{
			mdl_u8_t type = get_8l(__bci, &err)&0xFC;
			errjmp;
			bci_addr_t val_addr = get_addr(__bci, &err);
			errjmp;
			mdl_u64_t val = 0;
			if (_err(err = stack_get(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), val_addr))) jmpend;

			mdl_u8_t _type = get_8l(__bci, &err)&0xFC;
			errjmp;
			mdl_u64_t sc = 0;
			get(__bci, (mdl_u8_t*)&sc, bcit_sizeof(_type), &err);
			errjmp;
			if (ino == _bcii_shr) val >>= sc;
			else if (ino == _bcii_shl) val <<= sc;
			if (_err(err = stack_put(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), val_addr))) jmpend;
			jmpdone;
		}
		__asm__("_exc:");
		{
			mdl_u8_t ret_type = get_8l(__bci, &err)&0xFC;
			errjmp;
			bci_addr_t ret_addr = get_addr(__bci, &err);
			errjmp;
			bci_addr_t id_addr = get_addr(__bci, &err);
			errjmp;
			bci_addr_t arg_maddr = get_addr(__bci, &err);
			errjmp;
			bci_addr_t arg_addr;
			if (_err(err = stack_get(__bci, (mdl_u8_t*)&arg_addr, _bci_addr_size, arg_maddr))) jmpend;
			if (arg_addr >= __bci->stack_size || id_addr >= __bci->stack_size) {
				err = BCI_FAILURE;
				jmpend;
			}
			if (__bci->exc != NULL) {
				void *ret = __bci->exc(*(mdl_u8_t*)(__bci->mem_stack+id_addr), (void*)(__bci->mem_stack+arg_addr));
				if (_err(err = stack_put(__bci, (mdl_u8_t*)ret, bcit_sizeof(ret_type), ret_addr))) jmpend;
			}
			jmpdone;
		}
		__asm__("_conv:");
		{
			mdl_u8_t to_type = get_8l(__bci, &err)&0xFC;
			errjmp;
			mdl_u8_t from_type = get_8l(__bci, &err);
			errjmp;
			mdl_u8_t ft_signed = (from_type&_bcit_msigned) == _bcit_msigned;
			from_type ^= from_type&0x3;

			bci_addr_t dst_addr = get_addr(__bci, &err);
			errjmp;
			bci_addr_t src_addr = get_addr(__bci, &err);
			errjmp;
			mdl_u64_t val = 0;
			if (_err(err = stack_get(__bci, (mdl_u8_t*)&val, bcit_sizeof(from_type), src_addr))) jmpend;
			if (ft_signed && bcit_sizeof(to_type) > bcit_sizeof(from_type) && val > ((mdl_u64_t)1<<(bcit_sizeof(from_type)*8))>>1)
				val |= (~(mdl_u64_t)0)<<(bcit_sizeof(from_type)*8);

			if (_err(err = stack_put(__bci, (mdl_u8_t*)&val, bcit_sizeof(to_type), dst_addr))) jmpend;
			jmpdone;
		}
		__asm__("_dr:");
		{
			mdl_u8_t type = get_8l(__bci, &err)&0xFC;
			errjmp;
			bci_addr_t src_addr = get_addr(__bci, &err);
			errjmp;
			bci_addr_t dst_addr = get_addr(__bci, &err);
			errjmp;
			bci_addr_t addr;
			if (_err(err = stack_get(__bci, (mdl_u8_t*)&addr, _bci_addr_size, src_addr))) jmpend;

			mdl_u64_t val = 0;
			if (_err(err = stack_get(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), addr))) jmpend;
			if (_err(err = stack_put(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), dst_addr))) jmpend;
			jmpdone;
		}
		__asm__("_print:");
		{
			mdl_u8_t type = get_8l(__bci, &err);
			errjmp;
			mdl_u8_t _signed = (type&_bcit_msigned) == _bcit_msigned;
			type ^= type&0x3;

			bci_addr_t addr = get_addr(__bci, &err);
			errjmp;
			mdl_u64_t val = 0;
			if (_err(err = stack_get(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), addr))) jmpend;

			if (_signed) {
				mdl_u64_t p = val^(val&(((mdl_u64_t)~0)<<(bcit_sizeof(type)*8)));
				mdl_i64_t n = (mdl_i64_t)(val|(((mdl_u64_t)~0)<<(bcit_sizeof(type)*8)));
				if (p > ((mdl_u64_t)1<<(bcit_sizeof(type)*8))>>1) fprintf(stdout, "%ld\n", (mdl_i64_t)n);
				if (p < ((mdl_u64_t)1<<(bcit_sizeof(type)*8))>>1) fprintf(stdout, "%lu\n", p);
			} else
				fprintf(stdout, "%lu\n", val);
			jmpdone;
		}
		__asm__("_as:");
		{
			mdl_u8_t type = get_8l(__bci, &err)&0xFC;
			errjmp;
			bci_addr_t addr = get_addr(__bci, &err);
			errjmp;
			if (is_flag(flags, _bcii_as_fdr_addr))
				if (_err(err = stack_get(__bci, (mdl_u8_t*)&addr, _bci_addr_size, addr))) jmpend;

			mdl_u64_t val = 0;
			get(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), &err);
			errjmp;
			if (_err(err = stack_put(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), addr))) jmpend;
			jmpdone;
		}
		__asm__("_aop:");
		{
			mdl_u8_t aop_kind = get_8l(__bci, &err);
			errjmp;
			mdl_u8_t type = get_8l(__bci, &err);
			errjmp;
			mdl_u8_t _signed = (type&_bcit_msigned) == _bcit_msigned;
			type ^= type&0x3;

			bci_addr_t dst_addr = get_addr(__bci, &err);
			errjmp;
			mdl_u64_t l_val = 0, r_val = 0;
			if (is_flag(flags, _bcii_aop_fl_pm)) {
				get(__bci, (mdl_u8_t*)&l_val, bcit_sizeof(type), &err);
				errjmp;
			} else {
				bci_addr_t l_addr = get_addr(__bci, &err);
				errjmp;
				if (_err(err = stack_get(__bci, (mdl_u8_t*)&l_val, bcit_sizeof(type), l_addr))) jmpend;
			}

			if (is_flag(flags, _bcii_aop_fr_pm)) {
				get(__bci, (mdl_u8_t*)&r_val, bcit_sizeof(type), &err);
				errjmp;
			} else {
				bci_addr_t r_addr = get_addr(__bci, &err);
				errjmp;
				if (_err(err = stack_get(__bci, (mdl_u8_t*)&r_val, bcit_sizeof(type), r_addr))) jmpend;
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
			if (_err(err = stack_put(__bci, (mdl_u8_t*)&final, bcit_sizeof(type), dst_addr))) jmpend;
			jmpdone;
		}
		__asm__("_mov:");
		{
			mdl_u8_t type = get_8l(__bci, &err)&0xFC;
			errjmp;
			bci_addr_t dst_addr = get_addr(__bci, &err);
			errjmp;
			if (is_flag(flags, _bcii_mov_fdr_da))
				if (_err(err = stack_get(__bci, (mdl_u8_t*)&dst_addr, _bci_addr_size, dst_addr))) jmpend;

			bci_addr_t src_addr = get_16l(__bci, &err);
			errjmp;
			if (is_flag(flags, _bcii_mov_fdr_sa))
				if (_err(err = stack_get(__bci, (mdl_u8_t*)&src_addr, _bci_addr_size, src_addr))) jmpend;

			mdl_u64_t src_val = 0;
			if (_err(err = stack_get(__bci, (mdl_u8_t*)&src_val, bcit_sizeof(type), src_addr))) jmpend;
			if (_err(err = stack_put(__bci, (mdl_u8_t*)&src_val, bcit_sizeof(type), dst_addr))) jmpend;
			jmpdone;
		}
		__asm__("_incr_or_decr:");
		{
			mdl_u8_t type = get_8l(__bci, &err);
			errjmp;
			bci_addr_t addr = get_addr(__bci, &err);
			errjmp;
			mdl_u64_t val = 0;
			mdl_u8_t _signed = (type&_bcit_msigned) == _bcit_msigned;
			type ^= type&0x3;

			if (_err(err = stack_get(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), addr))) jmpend;

			mdl_uint_t by = 0;
			if (is_flag(flags, _bcii_iod_fbc_addr)) {
				bci_addr_t by_addr;
				get(__bci, (mdl_u8_t*)&by_addr, _bci_addr_size, &err);
				errjmp;
				if (_err(err = stack_get(__bci, (mdl_u8_t*)&by, bcit_sizeof(type), by_addr))) jmpend;
			} else {
				mdl_u8_t _type = get_8l(__bci, &err);
				get(__bci, (mdl_u8_t*)&by, bcit_sizeof(_type), &err);
				errjmp;
			}

			if (_signed) val |= (~(mdl_u64_t)0)<<(bcit_sizeof(type)*8);
			if (ino == _bcii_incr)
				val = _signed?(mdl_i64_t)val+by:val+by;
			else if (ino == _bcii_decr)
				val = _signed?(mdl_i64_t)val-by:val-by;
			if (_err(err = stack_put(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), addr))) jmpend;
			jmpdone;
		}
		__asm__("_cmp:");
		{
			mdl_u8_t l_type = get_8l(__bci, &err);
			errjmp;
			mdl_u8_t r_type = get_8l(__bci, &err);
			errjmp;
			mdl_u8_t l_signed = (l_type&_bcit_msigned) == _bcit_msigned;
			mdl_u8_t r_signed = (r_type&_bcit_msigned) == _bcit_msigned;
			l_type ^= l_type&0x3;
			r_type ^= r_type&0x3;

			bci_addr_t l_addr = get_addr(__bci, &err);
			errjmp;
			bci_addr_t r_addr = get_addr(__bci, &err);
			errjmp;
			mdl_u64_t l_val = 0, r_val = 0;
			if (_err(err = stack_get(__bci, (mdl_u8_t*)&l_val, bcit_sizeof(l_type), l_addr))) jmpend;
			if (_err(err = stack_get(__bci, (mdl_u8_t*)&r_val, bcit_sizeof(r_type), r_addr))) jmpend;

			if (l_val > ((mdl_u64_t)1<<(bcit_sizeof(l_type)*8))>>1 && l_signed)
				l_val |= (~(mdl_u64_t)0)<<(bcit_sizeof(l_type)*8);
			if (r_val > ((mdl_u64_t)1<<(bcit_sizeof(r_type)*8))>>1 && r_signed)
				r_val |= (~(mdl_u64_t)0)<<(bcit_sizeof(r_type)*8);
# ifdef __DEBUG_ENABLED
			fprintf(stdout, "l_val: %ld, r_val: %ld\n", (mdl_i64_t)l_val, (mdl_i64_t)r_val);
# endif
			mdl_u8_t dst_addr = get_addr(__bci, &err);
			errjmp;
			mdl_u8_t flags = 0;
			if (l_signed?(r_signed?((mdl_i64_t)l_val == (mdl_i64_t)r_val):((mdl_i64_t)l_val == r_val)):(r_signed?(l_val == (mdl_i64_t)r_val):(l_val == r_val))) flags |= _bcif_eq;
			if (l_signed?(r_signed?((mdl_i64_t)l_val > (mdl_i64_t)r_val):((mdl_i64_t)l_val > r_val)):(r_signed?(l_val > (mdl_i64_t)r_val):(l_val > r_val))) flags |= _bcif_gt;
			if (l_signed?(r_signed?((mdl_i64_t)l_val < (mdl_i64_t)r_val):((mdl_i64_t)l_val < r_val)):(r_signed?(l_val < (mdl_i64_t)r_val):(l_val < r_val))) flags |= _bcif_lt;
# ifdef __DEBUG_ENABLED
			fprintf(stdout, "l_signed: %u, r_signed: %u\n", l_signed, r_signed);
			fprintf(stdout, "eq: %s, gt: %s, lt: %s\n", is_flag(flags, _bcif_eq)?"yes":"no", is_flag(flags, _bcif_gt)?"yes":"no", is_flag(flags, _bcif_lt)?"yes":"no");
# endif
			if (_err(err = stack_put(__bci, &flags, 1, dst_addr))) jmpend;
			jmpdone;
		}
		__asm__("_cjmp:");
		{
			mdl_u8_t cond = get_8l(__bci, &err);
			errjmp;
			bci_addr_t jmpm_addr = get_addr(__bci, &err);
			errjmp;
			bci_addr_t flags_addr = get_addr(__bci, &err);
			errjmp;
			bci_addr_t jmp_addr;
			if (_err(err = stack_get(__bci, (mdl_u8_t*)&jmp_addr, _bci_addr_size, jmpm_addr))) jmpend;
			mdl_u8_t flags = 0;
			if (_err(err = stack_get(__bci, &flags, 1, flags_addr))) jmpend;
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
			if (jmp_addr >= __bci->prog_size) {
				err = BCI_FAILURE;
				jmpend;
			}
			__bci->set_ip(jmp_addr);
			jmpnext;
		}
		__asm__("_jmp:");
		{
			bci_addr_t jmpm_addr = get_addr(__bci, &err);
			errjmp;
			bci_addr_t jmp_addr;
			if (_err(err = stack_get(__bci, (mdl_u8_t*)&jmp_addr, _bci_addr_size, jmpm_addr))) jmpend;
			if (jmp_addr >= __bci->prog_size) {
				err = BCI_FAILURE;
				jmpend;
			}
			__bci->set_ip(jmp_addr);
			jmpnext;
		}
		// required
		__asm__("_exit:");
		{
			if (__exit_addr != NULL)
				*__exit_addr = __bci->get_ip();
			bci_addr_t addr = get_addr(__bci, &err);
			errjmp;
			bci_err_t exit_status = 0x0;
			if (__exit_status != NULL) {
				if (_err(err = stack_get(__bci, (mdl_u8_t*)&exit_status, sizeof(bci_err_t), addr))) jmpend;
				*__exit_status = exit_status;
			}
			jmpend;
		}
	}
	__asm__("_done:");
	__bci->ip_incr(__bci->ip_off);
	if (is_flag(__flags, _bcie_fsie)) jmpend;
	jmpnext;
	__bci->state = BCI_STOPPED;
	__asm__("_end:");
	if (_err(__bci->err = err))
		return err;
	return BCI_SUCCESS;
}
