# include "bci.h"
bci_err_t _any_err;

void static stack_w8_put(struct bci *__bci, mdl_u8_t __val, bci_addr_t __addr, bci_err_t *__any_err) {
	if (__addr < __bci->stack_size)
		__bci->mem_stack[__addr]=__val;
	else {
# ifdef __DEBUG_ENABLED
		fprintf(stderr, "stack_put: address out of bounds.\n");
# endif
		*__any_err = BCI_FAILURE;
	}
	*__any_err = BCI_SUCCESS;
}

mdl_u8_t static stack_w8_get(struct bci *__bci, bci_addr_t __addr, bci_err_t *__any_err) {
	mdl_u8_t ret_val;
	if (__addr < __bci->stack_size)
		ret_val = __bci->mem_stack[__addr];
	else {
# ifdef __DEBUG_ENABLED
		fprintf(stderr, "stack_get: address out of bounds.\n");
# endif
		*__any_err = BCI_FAILURE;
		return 0;
	}
	*__any_err = BCI_SUCCESS;
	return ret_val;
}

void static stack_put(struct bci *__bci, mdl_u8_t *__val, mdl_uint_t __bc, bci_addr_t __addr) {
	bci_err_t any_err;
	for (mdl_u8_t *itr = __val; itr != __val+__bc; itr++) {
		stack_w8_put(__bci, *itr, __addr+(itr-__val), &any_err);
		if (any_err != BCI_SUCCESS) {
# ifdef __DEBUG_ENABLED
			fprintf(stderr,  "stack_put: failed.\n");
# endif
			return;
		}
	}
}

void static stack_get(struct bci *__bci, mdl_u8_t *__val, mdl_uint_t __bc, bci_addr_t __addr) {
	bci_err_t any_err;
	for (mdl_u8_t *itr = __val; itr != __val+__bc; itr++) {
		*itr = stack_w8_get(__bci, __addr+(itr-__val), &any_err);
		if (any_err != BCI_SUCCESS) {
# ifdef __DEBUG_ENABLED
			fprintf(stderr, "stack_get: failed.\n");
# endif
			return;
		}
	}
}

mdl_u8_t static _get_w8(void *__bci) {
	struct bci *_bci = (struct bci*)__bci;

	mdl_u8_t val = _bci->get_byte();
	_bci->pc_incr();
	return val;
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
	__bci->mem_stack = (mdl_u8_t*)malloc(__bci->stack_size);
	__bci->eeb_list = NULL;
	__bci->act_indc_fp = NULL;
	_8xdrm_init(&__bci->_8xdrm_, &_get_w8, NULL);
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
}

mdl_u8_t static get_wx(struct bci *__bci, mdl_u8_t __w) {
	return _8xdrm_get_wx(&__bci->_8xdrm_, __w);
}

# define get_w4(__bci) get_wx(__bci, 4)

mdl_u8_t static get_w8(struct bci *__bci) {
	return get_wx(__bci, 8);
}

mdl_u16_t static get_w16(struct bci *__bci) {
	mdl_u16_t val = 0x0;
	val = get_w8(__bci);
	val |= get_w8(__bci) << 8;
	return val;
}

void static get(struct bci *__bci, mdl_u8_t *__val, mdl_uint_t __bc) {
	for (mdl_u8_t *itr = __val; itr != __val+__bc; itr++) *itr = get_w8(__bci);
}

mdl_u8_t bcit_sizeof(mdl_u8_t __type) {
	switch(__type) {
		case _bcit_void: return 0;
		case _bcit_w8: return sizeof(mdl_u8_t);
		case _bcit_w16: return sizeof(mdl_u16_t);
		case _bcit_w32: return sizeof(mdl_u32_t);
		case _bcit_w64: return sizeof(mdl_u64_t);
		case _bcit_addr: return sizeof(bci_addr_t);
	}

	return 0;
}

mdl_u8_t is_flag(bci_flag_t __flags, bci_flag_t __flag) {
	return (__flags & __flag) == __flag? 1:0;}

mdl_uint_t bcii_sizeof(mdl_u8_t *__p, bci_flag_t __flags) {
	switch(*__p) {
		case _bcii_extern_call: 	return 5;
		case _bcii_dr:		return 6;
		case _bcii_print:	return 3;
		case _bcii_assign:	return 3+bcit_sizeof(*(__p+1));
		case _bcii_aop: {
			mdl_uint_t size = 4;
			if (is_flag(__flags, _bcii_aop_fl_pm)) size += 2;
			if (is_flag(__flags, _bcii_aop_fr_pm)) size += 2;
			return size;
		}
		case _bcii_mov: 	return 5;
		case _bcii_incr: case _bcii_decr:	return 4+bcit_sizeof(*(__p+5));
		case _bcii_cmp:		return 6;
		case _bcii_cjmp:	return 5;
		case _bcii_jmp:		return 2;
		case _bcii_exit:	return 1;
		case _bcii_conv: 	return 6;
	}
	return 0;
}

mdl_uint_t bcii_overhead_size() {
	return 2;
}

void bci_eeb_init(struct bci *__bci, mdl_u8_t __blk_c) {
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

void bci_stop(struct bci *__bci) {
	_bci_stop((void*)__bci);
}

bci_err_t bci_exec(struct bci *__bci, mdl_u16_t __entry_addr, bci_flag_t __flags) {
	__bci->set_pc(__entry_addr);
	__bci->state = BCI_RUNNING;

	_again:
	if (__bci->iei_fp != NULL) __bci->iei_fp(__bci->iei_arg);
	if (is_flag(__bci->flags, _bci_fstop)) goto _end;
	{
		mdl_u8_t i = get_w8(__bci);
		bci_flag_t flags;
		get(__bci, (mdl_u8_t*)&flags, sizeof(bci_flag_t));
		switch(i) {
			case _bcii_nop: break;
			case _bcii_act_indc:
				if (__bci->act_indc_fp != NULL) __bci->act_indc_fp();
			break;
			case _bcii_eeb_init:
				bci_eeb_init(__bci, get_w8(__bci));
			break;
			case _bcii_eeb_put: {
				mdl_u8_t blk_id = get_w8(__bci);
				bci_addr_t b_addr = get_w16(__bci);
				bci_addr_t e_addr = get_w16(__bci);
				bci_eeb_put(__bci, blk_id, b_addr, e_addr);
				break;
			}

			case _bcii_extern_call: {
				mdl_u8_t ret_type = get_w8(__bci);
				bci_addr_t ret_addr = get_w16(__bci);
				bci_addr_t id_addr = get_w16(__bci);
				bci_addr_t arg_addr = get_w16(__bci);

				arg_addr = *(bci_addr_t*)(__bci->mem_stack+arg_addr);

				if (__bci->extern_fp != NULL) {
					void *ret = __bci->extern_fp(*(mdl_u8_t*)(__bci->mem_stack+id_addr), (void*)(__bci->mem_stack+arg_addr));
					stack_put(__bci, (mdl_u8_t*)ret, bcit_sizeof(ret_type), ret_addr);
				}

				break;
			}

			case _bcii_conv: {
				mdl_u8_t to_type = get_w8(__bci);
				mdl_u8_t from_type = get_w8(__bci);
				bci_addr_t dst_addr = get_w16(__bci);
				bci_addr_t src_addr = get_w16(__bci);

				mdl_uint_t val = 0;
				stack_get(__bci, (mdl_u8_t*)&val, bcit_sizeof(from_type), src_addr);

				stack_put(__bci, (mdl_u8_t*)&val, bcit_sizeof(to_type), dst_addr);
				break;
			}

			case _bcii_dr: {
				mdl_u8_t src_type = get_w8(__bci);
				mdl_u8_t dst_type = get_w8(__bci);
				bci_addr_t src_addr = get_w16(__bci);
				bci_addr_t dst_addr = get_w16(__bci);


				bci_addr_t addr = 0;
				stack_get(__bci, (mdl_u8_t*)&addr, bcit_sizeof(_bcit_addr), src_addr);

				mdl_uint_t val = 0;
				stack_get(__bci, (mdl_u8_t*)&val, bcit_sizeof(src_type), addr);
				stack_put(__bci, (mdl_u8_t*)&val, bcit_sizeof(dst_type), dst_addr);
				break;
			}

			case _bcii_print: {
				mdl_u8_t type = get_w8(__bci);
				mdl_u8_t _signed = (type&_bcit_msigned) == _bcit_msigned;
				if (_signed) type = type^_bcit_msigned;

				bci_addr_t addr = get_w16(__bci);
				mdl_uint_t val = 0;
				stack_get(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), addr);

				if (_signed)
					printf("out: %d\n", (mdl_int_t)(val|(((mdl_int_t)~0)<<(bcit_sizeof(type)*8))));
				else
					printf("out: %u\n", val);
				break;
			}

			case _bcii_assign: {
				mdl_u8_t type = get_w8(__bci);
				bci_addr_t addr = get_w16(__bci);

				if (is_flag(flags, _bcii_assign_fdr_addr))
					stack_get(__bci, (mdl_u8_t*)&addr, bcit_sizeof(_bcit_addr), addr);

				mdl_uint_t val = 0;
				get(__bci, (mdl_u8_t*)&val, bcit_sizeof(type));
				stack_put(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), addr);
				break;
			}

			case _bcii_aop: {
				mdl_u8_t aop_kind = get_w8(__bci);
				mdl_u8_t type = get_w8(__bci);
				mdl_u8_t _signed = (type&_bcit_msigned) == _bcit_msigned;
				if (_signed) type = type^_bcit_msigned;

				bci_addr_t dst_addr = get_w16(__bci);

				mdl_uint_t l_val = 0, r_val = 0;
				if (is_flag(flags, _bcii_aop_fl_pm))
					get(__bci, (mdl_u8_t*)&l_val, bcit_sizeof(type));
				else {
					bci_addr_t l_addr = get_w16(__bci);
					stack_get(__bci, (mdl_u8_t*)&l_val, bcit_sizeof(type), l_addr);
				}

				if (is_flag(flags, _bcii_aop_fr_pm))
					get(__bci, (mdl_u8_t*)&r_val, bcit_sizeof(type));
				else {
					bci_addr_t r_addr = get_w16(__bci);
					stack_get(__bci, (mdl_u8_t*)&r_val, bcit_sizeof(type), r_addr);
				}

				mdl_uint_t final;
				switch(aop_kind) {
					case _bci_aop_add:
						final = _signed? (mdl_int_t)l_val+(mdl_int_t)r_val:l_val+r_val;
					break;

					case _bci_aop_mul:
						final = _signed? (mdl_int_t)l_val*(mdl_int_t)r_val:l_val*r_val;
					break;

					case _bci_aop_sub:
						final = _signed? (mdl_int_t)l_val-(mdl_int_t)r_val:l_val-r_val;
					break;

					case _bci_aop_div:
						final = _signed? (mdl_int_t)l_val/(mdl_int_t)r_val:l_val/r_val;
					break;
				}

//				printf("final sum: %u, l_val: %u, r_val: %u\n", final, l_val, r_val);
				stack_put(__bci, (mdl_u8_t*)&final, bcit_sizeof(type), dst_addr);
				break;
			}

			case _bcii_mov: {
				mdl_u8_t type = get_w8(__bci);

				bci_addr_t dst_addr = get_w16(__bci);
				if (is_flag(flags, _bcii_mov_fdr_da))
					stack_get(__bci, (mdl_u8_t*)&dst_addr, bcit_sizeof(_bcit_addr), dst_addr);

				bci_addr_t src_addr = get_w16(__bci);
				if (is_flag(flags, _bcii_mov_fdr_sa))
					stack_get(__bci, (mdl_u8_t*)&src_addr, bcit_sizeof(_bcit_addr), src_addr);

				mdl_uint_t src_val = 0;
				stack_get(__bci, (mdl_u8_t*)&src_val, bcit_sizeof(type), src_addr);
				stack_put(__bci, (mdl_u8_t*)&src_val, bcit_sizeof(type), dst_addr);
				break;
			}

			case _bcii_incr: case _bcii_decr: {
				mdl_u8_t type = get_w8(__bci);
				bci_addr_t addr = get_w16(__bci);
				mdl_uint_t val = 0;

				mdl_u8_t _signed = (type&_bcit_msigned) == _bcit_msigned;
				if (_signed) type = type^_bcit_msigned;

				stack_get(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), addr);

				mdl_u8_t _type = get_w8(__bci);
				mdl_uint_t bc = 0;

				get(__bci, (mdl_u8_t*)&bc, bcit_sizeof(_type));

				if (i == _bcii_incr)
					val = _signed?(mdl_int_t)val+(mdl_int_t)bc:val+bc;
				else if (i == _bcii_decr)
					val = _signed?(mdl_int_t)val-(mdl_int_t)bc:val-bc;

			//	printf("%s addr: %u, bytes: %u, val: %u\n", i == _bcii_incr? "incr":"decr", addr, bc, val);
				stack_put(__bci, (mdl_u8_t*)&val, bcit_sizeof(type), addr);
				break;
			}

			case _bcii_cmp: {
				mdl_u8_t l_type = get_w8(__bci);
				mdl_u8_t r_type = get_w8(__bci);

				bci_addr_t l_addr = get_w16(__bci);
				bci_addr_t r_addr = get_w16(__bci);

				mdl_uint_t l_val = 0, r_val = 0;
				stack_get(__bci, (mdl_u8_t*)&l_val, bcit_sizeof(l_type), l_addr);
				stack_get(__bci, (mdl_u8_t*)&r_val, bcit_sizeof(r_type), r_addr);

				mdl_u8_t dst_addr = get_w16(__bci);
				stack_w8_put(__bci, l_val == r_val? 1:0, dst_addr, &_any_err);
				break;
			}

			case _bcii_cjmp: {
				mdl_u8_t cond = get_w8(__bci);
				bci_addr_t jmpm_addr = get_w16(__bci);
				bci_addr_t cond_addr = get_w16(__bci);

				bci_addr_t jmp_addr = 0;
				stack_get(__bci, (mdl_u8_t*)&jmp_addr, bcit_sizeof(_bcit_addr), jmpm_addr);

				switch(cond) {
					case _bcic_eq:
						if (stack_w8_get(__bci, cond_addr, &_any_err)) __bci->set_pc(jmp_addr);
					break;
					case _bcic_neq:
						if (!stack_w8_get(__bci, cond_addr, &_any_err)) __bci->set_pc(jmp_addr);
					break;
				}
				break;
			}

			case _bcii_jmp: {
				bci_addr_t jmpm_addr = get_w16(__bci);
				bci_addr_t jmp_addr = 0;
				stack_get(__bci, (mdl_u8_t*)&jmp_addr, bcit_sizeof(_bcit_addr), jmpm_addr);

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
	goto _again;

	__bci->state = BCI_STOPPED;

	_end:
	return BCI_SUCCESS;
}
