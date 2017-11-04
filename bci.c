# include "bci.h"
bci_err_t static stack_put(struct bci *__bci, mdl_u8_t *__val, mdl_uint_t __bc, bci_addr_t __addr) {
	if (__addr >= __bci->stack_size) {
		fprintf(stdout, "stack_put; address 0x%02x is not within the boundary.\n", __addr);
		return BCI_FAILURE;
	}
	mdl_u8_t *itr = __val;
	bci_addr_t addr;
	while (itr != __val+__bc) {
		addr = __addr+(itr-__val);
		if (addr >= __bci->stack_size) {
# ifdef __DEBUG_ENABLED
			fprintf(stderr, "stack_put; failed, address out of bounds.\n");
			fprintf(stderr, "stack_get; tryed to access memory at 0x%02x.\n", addr);
			return BCI_FAILURE;
# endif
		}

		*(__bci->mem_stack+addr) = *(itr++);
	}
	return BCI_SUCCESS;
}

bci_err_t static stack_get(struct bci *__bci, mdl_u8_t *__val, mdl_uint_t __bc, bci_addr_t __addr) {
	if (__addr >= __bci->stack_size) {
# ifdef __DEBUG_ENABLED
		fprintf(stderr, "stack_get; address 0x%02x is not within the boundary.\n", __addr);
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
			fprintf(stderr, "stack_get; tryed to access memory at 0x%02x.\n", addr);
# endif
			return BCI_FAILURE;;
		}

		*(itr++) = *(__bci->mem_stack+addr);
	}
	return BCI_SUCCESS;
}

mdl_u8_t static next_byte(void *__arg_p) {
	struct bci *_bci = (struct bci*)__arg_p;
	mdl_u8_t val = _bci->get_byte();
	_bci->pc_incr();
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

bci_err_t bci_init(struct bci *__bci) {
	if ((__bci->mem_stack = (mdl_u8_t*)malloc(__bci->stack_size)) == NULL) return BCI_FAILURE;
	__bci->eeb_list = NULL;
	__bci->act_indc_fp = NULL;
	_8xdrm_init(&__bci->_8xdrm_, &next_byte, NULL);
	set_get_arg(&__bci->_8xdrm_, (void*)__bci);
	memset(__bci->mem_stack, 0xFF, __bci->stack_size);
	__bci->extern_fp = NULL;
	__bci->iei_fp = NULL;
	__bci->flags = 0;
	return BCI_SUCCESS;
}

bci_err_t bci_de_init(struct bci *__bci) {
	free(__bci->mem_stack);
	if (__bci->eeb_list != NULL) free(__bci->eeb_list);
	return BCI_SUCCESS;
}

mdl_u8_t static get_lx(struct bci *__bci, mdl_u8_t __w) {
	return _8xdrm_get_lx(&__bci->_8xdrm_, __w);
}

# define get_4l(__bci) get_wx(__bci, 4)
mdl_u8_t static get_8l(struct bci *__bci) {
	return get_lx(__bci, 8);
}

mdl_u16_t static get_16l(struct bci *__bci) {
	mdl_u16_t ret_val = 0x00;
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
		case _bcii_exit:			return 0;
		case _bcii_conv: 			return 6;
		case _bcii_eeb_init:		return 1;
		case _bcii_eeb_put:			return 1+(bcit_sizeof(_bcit_addr)*2);
		default: return 0;
	}
	return 0;
}

mdl_uint_t bcii_overhead_size() {return 2;}
void bci_eeb_init(struct bci *__bci, mdl_u8_t __blk_c) {
	if (__bci->eeb_list != NULL) return;
	__bci->eeb_list = (struct bci_eeb*)malloc(__blk_c*sizeof(struct bci_eeb));
}

void bci_eeb_put(struct bci *__bci, mdl_u8_t __blk_id, bci_addr_t __b_addr, bci_addr_t __e_addr) {
	struct bci_eeb *eeb = __bci->eeb_list+__blk_id;
	eeb->b_addr = __b_addr;
	eeb->e_addr = __e_addr;
}

void bci_eeb_call(struct bci *__bci, mdl_u8_t __blk_id) {
	struct bci_eeb *eeb = __bci->eeb_list+__blk_id;

	mdl_uint_t addr = eeb->b_addr;
	while(__bci->get_pc() != eeb->e_addr) {
		bci_exec(__bci, addr, _bcie_fsie);
		addr = __bci->get_pc();
	}
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

void bci_stop(struct bci *__bci) {_bci_stop((void*)__bci);}

bci_err_t bci_exec(struct bci *__bci, mdl_u16_t __entry_addr, bci_flag_t __flags) {
	__bci->set_pc(__entry_addr);
	__bci->state = BCI_RUNNING;

	_next:
	if (__bci->iei_fp != NULL) __bci->iei_fp(__bci->iei_arg);
	if (is_flag(__bci->flags, _bci_fstop)) goto _end;
	{
		mdl_u8_t i = get_8l(__bci);
		bci_flag_t flags;
		get(__bci, (mdl_u8_t*)&flags, sizeof(bci_flag_t));
		switch(i) {
			case _bcii_nop: break;
			case _bcii_act_indc:
				if (__bci->act_indc_fp != NULL) __bci->act_indc_fp();
			break;
			case _bcii_eeb_init:
				bci_eeb_init(__bci, get_8l(__bci));
			break;
			case _bcii_eeb_put: {
				mdl_u8_t blk_id = get_8l(__bci);
				bci_addr_t b_addr = get_16l(__bci);
				bci_addr_t e_addr = get_16l(__bci);
				bci_eeb_put(__bci, blk_id, b_addr, e_addr);
				break;
			}

			case _bcii_lop: {
				mdl_u8_t lop_kind = get_8l(__bci);
				mdl_u8_t type = get_8l(__bci)&0xF8;
				mdl_u64_t l_val = 0, r_val = 0;
				bci_addr_t dst_addr = get_16l(__bci);

				if (is_flag(flags, _bcii_lop_fl_pm))
					get(__bci, (mdl_u8_t*)&l_val, bcit_sizeof(type));
				else {
					bci_addr_t l_addr = get_16l(__bci);
					if (stack_get(__bci, (mdl_u8_t*)&l_val, bcit_sizeof(type), l_addr) != BCI_SUCCESS) goto _end;
				}

				if (is_flag(flags, _bcii_lop_fr_pm))
					get(__bci, (mdl_u8_t*)&r_val, bcit_sizeof(type));
				else {
					bci_addr_t r_addr = get_16l(__bci);
					if (stack_get(__bci, (mdl_u8_t*)&r_val, bcit_sizeof(type), r_addr) != BCI_SUCCESS) goto _end;
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
				if (stack_put(__bci, (mdl_u8_t*)&final, bcit_sizeof(type), dst_addr) != BCI_SUCCESS) goto _end;
				break;
			}

			case _bcii_shr: case _bcii_shl: {
				mdl_u8_t type = get_8l(__bci)&0xF8;
				bci_addr_t val_addr = get_16l(__bci);

				mdl_u64_t val = 0;
				if (stack_get(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), val_addr) != BCI_SUCCESS) goto _end;

				mdl_u8_t _type = get_8l(__bci)&0xF8;
				mdl_u64_t sc = 0;
				get(__bci, (mdl_u8_t*)&sc, bcit_sizeof(_type));
				if (i == _bcii_shr) val >>= sc;
				else if (i == _bcii_shl) val <<= sc;
				if (stack_put(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), val_addr) != BCI_SUCCESS) goto _end;
				break;
			}

			case _bcii_extern_call: {
				mdl_u8_t ret_type = get_8l(__bci)&0xFC;
				bci_addr_t ret_addr = get_16l(__bci);
				bci_addr_t id_addr = get_16l(__bci);
				bci_addr_t arg_addr = get_16l(__bci);

				arg_addr = *(bci_addr_t*)(__bci->mem_stack+arg_addr);
				if (__bci->extern_fp != NULL) {
					void *ret = __bci->extern_fp(*(mdl_u8_t*)(__bci->mem_stack+id_addr), (void*)(__bci->mem_stack+arg_addr));
					if (stack_put(__bci, (mdl_u8_t*)ret, bcit_sizeof(ret_type), ret_addr) != BCI_SUCCESS) goto _end;
				}
				break;
			}

			case _bcii_conv: {
				mdl_u8_t to_type = get_8l(__bci)&0xFC;
				mdl_u8_t from_type = get_8l(__bci);

				mdl_u8_t ft_signed = (from_type&_bcit_msigned) == _bcit_msigned;
				if (ft_signed) from_type ^= _bcit_msigned;

				bci_addr_t dst_addr = get_16l(__bci);
				bci_addr_t src_addr = get_16l(__bci);

				mdl_u64_t val = 0;
				if (stack_get(__bci, (mdl_u8_t*)&val, bcit_sizeof(from_type), src_addr) != BCI_SUCCESS) goto _end;
				if (ft_signed && bcit_sizeof(to_type) > bcit_sizeof(from_type) && val > (1<<(bcit_sizeof(from_type)*8))>>1)
					val |= (~(mdl_u64_t)0)<<(bcit_sizeof(from_type)*8);

				if (stack_put(__bci, (mdl_u8_t*)&val, bcit_sizeof(to_type), dst_addr) != BCI_SUCCESS) goto _end;
				break;
			}

			case _bcii_dr: {
				mdl_u8_t type = get_8l(__bci)&0xFC;
				bci_addr_t src_addr = get_16l(__bci);
				bci_addr_t dst_addr = get_16l(__bci);

				bci_addr_t addr = 0;
				if (stack_get(__bci, (mdl_u8_t*)&addr, bcit_sizeof(_bcit_addr), src_addr) != BCI_SUCCESS) goto _end;

				mdl_u64_t val = 0;
				if (stack_get(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), addr) != BCI_SUCCESS) goto _end;
				if (stack_put(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), dst_addr) != BCI_SUCCESS) goto _end;
				break;
			}

			case _bcii_print: {
				mdl_u8_t type = get_8l(__bci);
				mdl_u8_t _signed = (type&_bcit_msigned) == _bcit_msigned;
				if (_signed) type ^= _bcit_msigned;

				bci_addr_t addr = get_16l(__bci);
				mdl_u64_t val = 0;
				if (stack_get(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), addr) != BCI_SUCCESS) goto _end;

				if (_signed) {
					mdl_u64_t p = val^(val&(((mdl_i64_t)~0)<<(bcit_sizeof(type)*8)));
					mdl_i64_t n = (mdl_i64_t)(val|(((mdl_i64_t)~0)<<(bcit_sizeof(type)*8)));
					if (p > (1<<(bcit_sizeof(type)*8))>>1) fprintf(stdout, "%ld\n", (mdl_i64_t)n);
					if (p < (1<<(bcit_sizeof(type)*8))>>1) fprintf(stdout, "%lu\n", p);
				} else
					fprintf(stdout, "%lu\n", val);
				break;
			}

			case _bcii_assign: {
				mdl_u8_t type = get_8l(__bci)&0xFC;
				bci_addr_t addr = get_16l(__bci);

				if (is_flag(flags, _bcii_assign_fdr_addr))
					if (stack_get(__bci, (mdl_u8_t*)&addr, bcit_sizeof(_bcit_addr), addr) != BCI_SUCCESS) goto _end;

				mdl_u64_t val = 0;
				get(__bci, (mdl_u8_t*)&val, bcit_sizeof(type));
				if (stack_put(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), addr) != BCI_SUCCESS) goto _end;
				break;
			}

			case _bcii_aop: {
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
					if (stack_get(__bci, (mdl_u8_t*)&l_val, bcit_sizeof(type), l_addr) != BCI_SUCCESS) goto _end;
				}

				if (is_flag(flags, _bcii_aop_fr_pm))
					get(__bci, (mdl_u8_t*)&r_val, bcit_sizeof(type));
				else {
					bci_addr_t r_addr = get_16l(__bci);
					if (stack_get(__bci, (mdl_u8_t*)&r_val, bcit_sizeof(type), r_addr) != BCI_SUCCESS) goto _end;
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
				if (stack_put(__bci, (mdl_u8_t*)&final, bcit_sizeof(type), dst_addr) != BCI_SUCCESS) goto _end;
				break;
			}

			case _bcii_mov: {
				mdl_u8_t type = get_8l(__bci)&0xFC;
				bci_addr_t dst_addr = get_16l(__bci);
				if (is_flag(flags, _bcii_mov_fdr_da))
					if (stack_get(__bci, (mdl_u8_t*)&dst_addr, bcit_sizeof(_bcit_addr), dst_addr) != BCI_SUCCESS) goto _end;

				bci_addr_t src_addr = get_16l(__bci);
				if (is_flag(flags, _bcii_mov_fdr_sa))
					if (stack_get(__bci, (mdl_u8_t*)&src_addr, bcit_sizeof(_bcit_addr), src_addr) != BCI_SUCCESS) goto _end;

				mdl_u64_t src_val = 0;
				if (stack_get(__bci, (mdl_u8_t*)&src_val, bcit_sizeof(type), src_addr) != BCI_SUCCESS) goto _end;
				if (stack_put(__bci, (mdl_u8_t*)&src_val, bcit_sizeof(type), dst_addr) != BCI_SUCCESS) goto _end;
				break;
			}

			case _bcii_incr: case _bcii_decr: {
				mdl_u8_t type = get_8l(__bci);
				bci_addr_t addr = get_16l(__bci);
				mdl_u64_t val = 0;
				mdl_u8_t _signed = (type&_bcit_msigned) == _bcit_msigned;
				if (_signed) type ^= _bcit_msigned;

				if (stack_get(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), addr) != BCI_SUCCESS) goto _end;

				mdl_uint_t by = 0;
				if (is_flag(flags, _bcii_iod_fbc_addr)) {
					bci_addr_t by_addr = 0;
					get(__bci, (mdl_u8_t*)&by_addr, bcit_sizeof(_bcit_addr));
					if (stack_get(__bci, (mdl_u8_t*)&by, bcit_sizeof(type), by_addr) != BCI_SUCCESS) goto _end;;
				} else {
					mdl_u8_t _type = get_8l(__bci);
					get(__bci, (mdl_u8_t*)&by, bcit_sizeof(_type));
				}

				if (_signed) val |= (~(mdl_u64_t)0)<<(bcit_sizeof(type)*8);

				if (i == _bcii_incr) {
					val = _signed?(mdl_i64_t)val+by:val+by;
				} else if (i == _bcii_decr)
					val = _signed?(mdl_i64_t)val-by:val-by;
				if (stack_put(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), addr) != BCI_SUCCESS) goto _end;
				break;
			}

			case _bcii_cmp: {
				mdl_u8_t l_type = get_8l(__bci);
				mdl_u8_t r_type = get_8l(__bci);

				mdl_u8_t l_signed = (l_type&_bcit_msigned) == _bcit_msigned;
				mdl_u8_t r_signed = (r_type&_bcit_msigned) == _bcit_msigned;
                if (l_signed) l_type ^= _bcit_msigned;
				if (r_signed) r_type ^= _bcit_msigned;

				bci_addr_t l_addr = get_16l(__bci);
				bci_addr_t r_addr = get_16l(__bci);

				mdl_u64_t l_val = 0, r_val = 0;
				if (stack_get(__bci, (mdl_u8_t*)&l_val, bcit_sizeof(l_type), l_addr) != BCI_SUCCESS) goto _end;
				if (stack_get(__bci, (mdl_u8_t*)&r_val, bcit_sizeof(r_type), r_addr) != BCI_SUCCESS) goto _end;

				if (l_val > (1<<(bcit_sizeof(l_type)*8))>>1 && l_signed)
					l_val |= (~(mdl_u64_t)0)<<(bcit_sizeof(l_type)*8);
				if (r_val > (1<<(bcit_sizeof(r_type)*8))>>1 && r_signed)
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
				if (stack_put(__bci, &flags, 1, dst_addr) != BCI_SUCCESS) goto _end;
				break;
			}

			case _bcii_cjmp: {
				mdl_u8_t cond = get_8l(__bci);
				bci_addr_t jmpm_addr = get_16l(__bci);
				bci_addr_t flags_addr = get_16l(__bci);

				bci_addr_t jmp_addr = 0;
				if (stack_get(__bci, (mdl_u8_t*)&jmp_addr, bcit_sizeof(_bcit_addr), jmpm_addr) != BCI_SUCCESS) goto _end;
				mdl_u8_t flags = 0;
				if (stack_get(__bci, &flags, 1, flags_addr) != BCI_SUCCESS) goto _end;
				switch(cond) {
					case _bcic_eq:
						if (is_flag(flags, _bcif_eq)) goto _jmp;
					break;
					case _bcic_neq:
						if (!is_flag(flags, _bcif_eq)) goto _jmp;
					break;
					case _bcic_gt:
						if (is_flag(flags, _bcif_gt)) goto _jmp;
					break;
					case _bcic_lt:
						if (is_flag(flags, _bcif_lt)) goto _jmp;
					break;
					case _bcic_geq:
						if (is_flag(flags, _bcif_gt) || is_flag(flags, _bcif_eq))
							goto _jmp;
					break;
					case _bcic_leq:
						if (is_flag(flags, _bcif_lt) || is_flag(flags, _bcif_eq))
							goto _jmp;
					break;
				}
				break;

				_jmp:
				__bci->set_pc(jmp_addr);
				break;
			}

			case _bcii_jmp: {
				bci_addr_t jmpm_addr = get_16l(__bci);
				bci_addr_t jmp_addr = 0;
				if (stack_get(__bci, (mdl_u8_t*)&jmp_addr, bcit_sizeof(_bcit_addr), jmpm_addr) != BCI_SUCCESS) goto _end;
				__bci->set_pc(jmp_addr);
				break;
			}

			// required by design
			case _bcii_exit: goto _end;
			default:
				return BCI_FAILURE;
		}
	}

	if (is_flag(__flags, _bcie_fsie)) goto _end;
	goto _next;

	__bci->state = BCI_STOPPED;
	_end:
	return BCI_SUCCESS;
}
