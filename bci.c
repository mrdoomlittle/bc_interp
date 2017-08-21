# include "bci.h"
mdl_u8_t static *mem_stack = NULL;

void static stack_w8_put(mdl_u8_t __val, bci_addr_t __addr) {
	mem_stack[__addr]=__val;}

mdl_u8_t static stack_w8_get(bci_addr_t __addr) {
	return mem_stack[__addr];}

void static stack_put(mdl_u8_t *__val, mdl_uint_t __bc, bci_addr_t __addr) {
	for (mdl_u8_t *itr = __val; itr != __val+__bc; itr++) {
		stack_w8_put(*itr, __addr+(itr-__val));}}

void static stack_get(mdl_u8_t *__val, mdl_uint_t __bc, bci_addr_t __addr) {
	for (mdl_u8_t *itr = __val; itr != __val+__bc; itr++)
		*itr = stack_w8_get(__addr+(itr-__val));}

mdl_u8_t static _get_w8(void *__bci) {
	struct bci *_bci = (struct bci*)__bci;

	mdl_u8_t val = _bci->get_byte();
	_bci->pc_incr();
	return val;
}

void bci_set_extern_func(struct bci *__bci, void*(*__extern_func)(mdl_u8_t, void*)) {
	__bci->extern_func = __extern_func;
}
void bci_set_act_ind_func(struct bci *__bci, void (*__act_ind_func)()) {
	__bci->act_ind_func = __act_ind_func;
}

bci_err_t bci_init(struct bci *__bci) {
	mem_stack = (mdl_u8_t*)malloc(__bci->stack_size);
	_8xdrm_init(&__bci->_8xdrm_, &_get_w8, NULL);
	set_get_arg(&__bci->_8xdrm_, (void*)__bci);
	memset(mem_stack, 0xFF, __bci->stack_size);
	__bci->extern_func = NULL;
	__bci->act_ind_func = NULL;
	return BCI_SUCCESS;
}

bci_err_t bci_de_init(struct bci *__bci) {
	free(mem_stack);
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
		case _bcit_w8: return sizeof(mdl_u8_t);
		case _bcit_w16: return sizeof(mdl_u16_t);
		case _bcit_w32: return sizeof(mdl_u32_t);
		case _bcit_w64: return sizeof(mdl_u64_t);
		case _bcit_addr: return sizeof(bci_addr_t);
	}

	return 0;
}

mdl_u8_t is_flag(bcii_flag_t __flags, bcii_flag_t __flag) {
	return (__flags & __flag) == __flag? 1:0;}

mdl_uint_t bcii_sizeof(mdl_u8_t *__p, bcii_flag_t __flags) {
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
		case _bcii_mov: 	return 6;
		case _bcii_incr: case _bcii_decr:	return 4+bcit_sizeof(*(__p+5));
		case _bcii_cmp:		return 6;
		case _bcii_cjmp:	return 5;
		case _bcii_jmp:		return 2;
		case _bcii_exit:	return 1;
		case _bcii_conv: 	return 4;
	}
	return 0;
}

mdl_uint_t bcii_overhead_size() {
	return 2;
}

bci_err_t bci_exec(struct bci *__bci, mdl_u16_t __entry_addr) {
	__bci->set_pc(__entry_addr);

	next:
	if (__bci->act_ind_func != NULL)
		__bci->act_ind_func();

	{
//		usleep(10000);
		mdl_u8_t i = get_w8(__bci);
		bcii_flag_t flags;
		get(__bci, (mdl_u8_t*)&flags, sizeof(bcii_flag_t));

		switch(i) {
			case _bcii_extern_call: {
				mdl_u8_t ret_type = get_w8(__bci);
				bci_addr_t ret_addr = get_w16(__bci);
				bci_addr_t id_addr = get_w16(__bci);
				bci_addr_t arg_addr = get_w16(__bci);

				arg_addr = *(bci_addr_t*)(mem_stack+arg_addr);

				if (__bci->extern_func != NULL) {
					void *ret = __bci->extern_func(*(mdl_u8_t*)(mem_stack+id_addr), (void*)(mem_stack+arg_addr));
					stack_put((mdl_u8_t*)ret, bcit_sizeof(ret_type), ret_addr);
				}

				break;
			}

			case _bcii_conv: {
				mdl_u8_t to_type = get_w8(__bci);
				mdl_u8_t from_type = get_w8(__bci);
				bci_addr_t addr = get_w16(__bci);

				mdl_uint_t val = 0;
				stack_get((mdl_u8_t*)&val, bcit_sizeof(from_type), addr);

				stack_put((mdl_u8_t*)&val, bcit_sizeof(to_type), addr);
				break;
			}

			case _bcii_dr: {
				mdl_u8_t src_type = get_w8(__bci);
				mdl_u8_t dest_type = get_w8(__bci);
				bci_addr_t src_addr = get_w16(__bci);
				bci_addr_t dest_addr = get_w16(__bci);


				bci_addr_t addr = 0;
				stack_get((mdl_u8_t*)&addr, bcit_sizeof(_bcit_addr), src_addr);

				mdl_uint_t val = 0;
				stack_get((mdl_u8_t*)&val, bcit_sizeof(src_type), addr);
				stack_put((mdl_u8_t*)&val, bcit_sizeof(dest_type), dest_addr);
				break;
			}

			case _bcii_print: {
				mdl_u8_t type = get_w8(__bci);
				bci_addr_t addr = get_w16(__bci);
				mdl_uint_t val = 0;
				stack_get((mdl_u8_t*)&val, bcit_sizeof(type), addr);
				printf("out: %u\n", val);
				break;
			}

			case _bcii_assign: {
				mdl_u8_t type = get_w8(__bci);
				bci_addr_t addr = get_w16(__bci);

				if (is_flag(flags, _bcii_assign_fdr_addr))
					stack_get((mdl_u8_t*)&addr, bcit_sizeof(_bcit_addr), addr);

				mdl_uint_t val = 0;
				get(__bci, (mdl_u8_t*)&val, bcit_sizeof(type));
				stack_put((mdl_u8_t*)&val, bcit_sizeof(type), addr);
				break;
			}

			case _bcii_aop: {
				mdl_u8_t aop_kind = get_w8(__bci);
				mdl_u8_t type = get_w8(__bci);

				bci_addr_t dest_addr = get_w16(__bci);

				mdl_uint_t l_val = 0, r_val = 0;
				if (is_flag(flags, _bcii_aop_fl_pm))
					get(__bci, (mdl_u8_t*)&l_val, bcit_sizeof(type));
				else {
					bci_addr_t l_addr = get_w16(__bci);
					stack_get((mdl_u8_t*)&l_val, bcit_sizeof(type), l_addr);
				}

				if (is_flag(flags, _bcii_aop_fr_pm))
					get(__bci, (mdl_u8_t*)&r_val, bcit_sizeof(type));
				else {
					bci_addr_t r_addr = get_w16(__bci);
					stack_get((mdl_u8_t*)&r_val, bcit_sizeof(type), r_addr);
				}

				mdl_uint_t final;
				switch(aop_kind) {
					case _bci_aop_add:
						final = l_val+r_val;
					break;

					case _bci_aop_mul:
						final = l_val*r_val;
					break;

					case _bci_aop_sub:
						final = l_val-r_val;
					break;

					case _bci_aop_div:
						final = l_val/r_val;
					break;
				}

//				printf("final sum: %u, l_val: %u, r_val: %u\n", final, l_val, r_val);
				stack_put((mdl_u8_t*)&final, bcit_sizeof(type), dest_addr);
				break;
			}

			case _bcii_mov: {
				mdl_u8_t dest_type = get_w8(__bci);
				mdl_u8_t src_type = get_w8(__bci);

				bci_addr_t dest_addr = get_w16(__bci);
				if (is_flag(flags, _bcii_mov_fdr_da))
					stack_get((mdl_u8_t*)&dest_addr, bcit_sizeof(_bcit_addr), dest_addr);

				bci_addr_t src_addr = get_w16(__bci);
				if (is_flag(flags, _bcii_mov_fdr_sa))
					stack_get((mdl_u8_t*)&src_addr, bcit_sizeof(_bcit_addr), src_addr);

				mdl_uint_t src_val = 0;
				stack_get((mdl_u8_t*)&src_val, bcit_sizeof(src_type), src_addr);
				stack_put((mdl_u8_t*)&src_val, bcit_sizeof(dest_type), dest_addr);
				break;
			}

			case _bcii_incr: case _bcii_decr: {
				mdl_u8_t type = get_w8(__bci);
				bci_addr_t addr = get_w16(__bci);
				mdl_uint_t val = 0;

				stack_get((mdl_u8_t*)&val, bcit_sizeof(type), addr);

				mdl_u8_t _type = get_w8(__bci);
				mdl_uint_t bc = 0;

				get(__bci, (mdl_u8_t*)&bc, bcit_sizeof(_type));

				if (i == _bcii_incr)
					val+= bc;
				else if (i == _bcii_decr)
					val-= bc;

			//	printf("%s addr: %u, bytes: %u, val: %u\n", i == _bcii_incr? "incr":"decr", addr, bc, val);
				stack_put((mdl_u8_t*)&val, bcit_sizeof(type), addr);
				break;
			}

			case _bcii_cmp: {
				mdl_u8_t l_type = get_w8(__bci);
				mdl_u8_t r_type = get_w8(__bci);

				bci_addr_t l_addr = get_w16(__bci);
				bci_addr_t r_addr = get_w16(__bci);

				mdl_uint_t l_val = 0, r_val = 0;
				stack_get((mdl_u8_t*)&l_val, bcit_sizeof(l_type), l_addr);
				stack_get((mdl_u8_t*)&r_val, bcit_sizeof(r_type), r_addr);

				mdl_u8_t dest_addr = get_w16(__bci);
				stack_w8_put(l_val == r_val? 1:0, dest_addr);
				break;
			}

			case _bcii_cjmp: {
				mdl_u8_t cond = get_w8(__bci);
				bci_addr_t jmpm_addr = get_w16(__bci);
				bci_addr_t cond_addr = get_w16(__bci);

				bci_addr_t jmp_addr = 0;
				stack_get((mdl_u8_t*)&jmp_addr, bcit_sizeof(_bcit_addr), jmpm_addr);

				switch(cond) {
					case _bcic_eq:
						if (stack_w8_get(cond_addr)) __bci->set_pc(jmp_addr);
					break;
					case _bcic_neq:
						if (!stack_w8_get(cond_addr)) __bci->set_pc(jmp_addr);
					break;
				}
				break;
			}

			case _bcii_jmp: {
				bci_addr_t jmpm_addr = get_w16(__bci);
				bci_addr_t jmp_addr = 0;
				stack_get((mdl_u8_t*)&jmp_addr, bcit_sizeof(_bcit_addr), jmpm_addr);

				__bci->set_pc(jmp_addr);
				break;
			}

			// required by design
			case _bcii_exit:
				return BCI_SUCCESS;
			default:
				return BCI_FAILURE;
		}
	}

	goto next;
	return BCI_SUCCESS;
}
