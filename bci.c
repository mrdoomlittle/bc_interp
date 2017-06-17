# include "bci.h"
mdl_u8_t *mem_stack = NULL;

void stack_w8_put(mdl_u8_t __val, bci_addr_t __addr) {mem_stack[__addr]=__val;}
mdl_u8_t stack_w8_get(bci_addr_t __addr) {return mem_stack[__addr];}

void stack_put(mdl_u8_t *__val, mdl_uint_t __bc, bci_addr_t __addr) {
	for (mdl_u8_t *itr = __val; itr != __val+__bc; itr++)
		stack_w8_put(*itr, __addr+(itr-__val));}

void stack_get(mdl_u8_t *__val, mdl_uint_t __bc, bci_addr_t __addr) {
	for (mdl_u8_t *itr = __val; itr != __val+__bc; itr++)
		*itr = stack_w8_get(__addr+(itr-__val));}

bci_err_t bci_init(struct bci_t *__bci) {
	mem_stack = (mdl_u8_t*)malloc(__bci-> stack_size);
	return BCI_SUCCESS;
}

bci_err_t bci_de_init(struct bci_t *__bci) {
	free(mem_stack);
}

mdl_u8_t get_w8(struct bci_t *__bci) {
	mdl_u8_t val = __bci-> get_byte();
	__bci-> pc_incr();
	return val;
}

mdl_u16_t get_w16(struct bci_t *__bci) {
	mdl_u16_t val = 0x0;
	val = get_w8(__bci);
	val |= get_w8(__bci) << 8;
	return val;
}

void get(struct bci_t *__bci, mdl_u8_t *__val, mdl_uint_t __bc) {
	for (mdl_u8_t *itr = __val; itr != __val+__bc; itr++) *itr = get_w8(__bci);}

mdl_u8_t bci_sizeof(mdl_u8_t __type) {
	switch(__type) {
		case _bcit_w8: return sizeof(mdl_u8_t);
		case _bcit_w16: return sizeof(mdl_u16_t);
		case _bcit_w32: return sizeof(mdl_u32_t);
		case _bcit_w64: return sizeof(mdl_u64_t);
	}

	return 0;
}

bci_err_t bci_exec(struct bci_t *__bci, mdl_u16_t __entry_addr) {
	__bci-> set_pc(__entry_addr);

	next:
	{
		mdl_u8_t i = get_w8(__bci);

		switch(i) {
			case _bcii_print: {
				mdl_u8_t type = get_w8(__bci);
				bci_addr_t addr = get_w16(__bci);
				mdl_uint_t val = 0;
				stack_get((mdl_u8_t*)&val, bci_sizeof(type), addr);

				printf("type: %u, val: %u, addr: %u, size: %u\n", type, val, addr, bci_sizeof(type));
				break;
			}

			case _bcii_assign: {
				mdl_u8_t type = get_w8(__bci);
				bci_addr_t addr = get_w16(__bci);

				mdl_uint_t val;
				get(__bci, (mdl_u8_t*)&val, bci_sizeof(type));
				stack_put((mdl_u8_t*)&val, bci_sizeof(type), addr);
				break;
			}

			case _bcii_aop: {
				mdl_u8_t aop_type = get_w8(__bci);
				mdl_u8_t l_type = get_w8(__bci);
				mdl_u8_t r_type = get_w8(__bci);

				bci_addr_t l_addr = get_w16(__bci);
				bci_addr_t r_addr = get_w16(__bci);

				mdl_u8_t dest_type = get_w8(__bci);
				bci_addr_t dest_addr = get_w16(__bci);

				mdl_uint_t l_val = 0, r_val = 0;
				stack_get((mdl_u8_t*)&l_val, bci_sizeof(l_type), l_addr);
				stack_get((mdl_u8_t*)&r_val, bci_sizeof(r_type), r_addr);

				mdl_uint_t final;
				switch(aop_type) {
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

				stack_put((mdl_u8_t*)&final, bci_sizeof(dest_type), dest_addr);
				break;
			}

			case _bcii_mov: {
				mdl_u8_t dest_type = get_w8(__bci);
				mdl_u8_t src_type = get_w8(__bci);

				bci_addr_t dest_addr = get_w16(__bci);
				bci_addr_t src_addr = get_w16(__bci);

				mdl_uint_t src_val = 0;
				stack_get((mdl_u8_t*)&src_val, bci_sizeof(src_type), src_addr);
				stack_put((mdl_u8_t*)&src_val, bci_sizeof(dest_type), dest_addr);
				break;
			}

			case _bcii_incr: {
				mdl_u8_t type = get_w8(__bci);
				bci_addr_t addr = get_w16(__bci);
				mdl_uint_t val = 0;

				stack_get((mdl_u8_t*)&val, bci_sizeof(type), addr);
				val++;

				stack_put((mdl_u8_t*)&val, bci_sizeof(type), addr);
				break;
			}

			case _bcii_decr: {
				mdl_u8_t type = get_w8(__bci);
				bci_addr_t addr = get_w16(__bci);
				mdl_uint_t val = 0;

				stack_get((mdl_u8_t*)&val, bci_sizeof(type), addr);
				val--;

				stack_put((mdl_u8_t*)&val, bci_sizeof(type), addr);
				break;
			}

			case _bcii_cmp: {
				mdl_u8_t l_type = get_w8(__bci);
				mdl_u8_t r_type = get_w8(__bci);

				bci_addr_t l_addr = get_w16(__bci);
				bci_addr_t r_addr = get_w16(__bci);

				mdl_uint_t l_val = 0, r_val = 0;
				stack_get((mdl_u8_t*)&l_val, bci_sizeof(l_type), l_addr);
				stack_get((mdl_u8_t*)&r_val, bci_sizeof(r_type), r_addr);

				mdl_u8_t dest_addr = get_w16(__bci);
				stack_w8_put(l_val == r_val? 1:0, dest_addr);
				break;
			}

			case _bcii_jmp: {
				bci_addr_t jmp_addr = get_w16(__bci);
				__bci-> set_pc(jmp_addr);
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
