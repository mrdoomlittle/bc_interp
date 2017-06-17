# include "bc_interp.h"
mdl_u8_t *stack = NULL;
mdl_u8_t *heep = NULL;

mdl_u16_t sp = 0;

void stack_put_w8(mdl_u8_t __val, mdl_u16_t __addr) {
	*(stack+__addr) = __val;
}

void stack_put(mdl_u8_t *__i, mdl_uint_t __bc, mdl_u16_t __addr) {
	for (mdl_u8_t *itor = __i; itor != __i + __bc; itor++)
		stack_put_w8(*itor, __addr+(itor-__i));
}

void stack_put_w16(mdl_u16_t __val, mdl_u16_t __addr) {
	*(mdl_u16_t*)(stack+__addr) = __val;}

void stack_put_w32(mdl_u32_t __val, mdl_u16_t __addr) {
	*(mdl_u32_t*)(stack+__addr) = __val;}

void stack_put_w64(mdl_u64_t __val, mdl_u16_t __addr) {
	*(mdl_u64_t*)(stack+__addr) = __val;}

mdl_u8_t stack_get_w8(mdl_u16_t __addr) {
	return *(stack+__addr);
}

mdl_u8_t stack_get(mdl_u8_t *__i, mdl_uint_t __bc, mdl_u16_t __addr) {
	for (mdl_u8_t *itor = __i; itor != __i + __bc; itor++)
		*itor = stack_get_w8(__addr+(itor-__i));}

mdl_u16_t stack_get_w16(mdl_u16_t __addr) {
    return *(mdl_u16_t*)(stack+__addr);}

mdl_u32_t stack_get_w32(mdl_u16_t __addr) {
    return *(mdl_u32_t*)(stack+__addr);}

mdl_u64_t stack_get_w64(mdl_u16_t __addr) {
    return *(mdl_u64_t*)(stack+__addr);}

bci_err_t bci_init(struct bc_interp_t *__bc_interp) {
	stack = (mdl_u8_t*)malloc(__bc_interp-> stack_size);
}

mdl_uint_t bci_sizeof(mdl_u8_t __t) {
	switch(__t) {
		case _bci_w8: return sizeof(mdl_u8_t);
		case _bci_w16: return sizeof(mdl_u16_t);
		case _bci_w32: return sizeof(mdl_u32_t);
		case _bci_w64: return sizeof(mdl_u64_t);
	}
	return 0;
}

mdl_u8_t get_w8(struct bc_interp_t *__bc_interp) {
	mdl_u8_t val = __bc_interp-> get_byte();
	__bc_interp-> pc_incr();
	return val;
}

void get(struct bc_interp_t *__bc_interp, mdl_u8_t *__i, mdl_uint_t __bc) {
	for (mdl_u8_t *itor = __i; itor != __i + __bc; itor++) *itor = get_w8(__bc_interp);}

mdl_u16_t get_w16(struct bc_interp_t *__bc_interp) {
	mdl_u16_t val = 0x0;
	val |= get_w8(__bc_interp);
	val |= get_w8(__bc_interp) << 8;
}

bci_err_t bci_exec(struct bc_interp_t *__bc_interp, mdl_u16_t __entry_addr) {
	__bc_interp-> set_pc(__entry_addr);

	do {
		mdl_u8_t i = get_w8(__bc_interp);

		switch(i) {
			case _bci_exit: {
				return 0;
				break;
			}

			case _bci_assign: {
				mdl_u16_t addr;
				mdl_u8_t val;
				mdl_u8_t type;

				type = get_w8(__bc_interp);
				addr = get_w16(__bc_interp);

				get(__bc_interp, (mdl_u8_t*)&val, bci_sizeof(type));
				stack_put((mdl_u8_t*)&val, bci_sizeof(type), addr);
				break;
			}

			case _bci_mov: {
				mdl_u16_t dest_addr, src_addr;
				mdl_u8_t type = get_w8(__bc_interp);
				dest_addr = get_w16(__bc_interp);
				src_addr = get_w16(__bc_interp);
				mdl_uint_t val = 0;

				stack_get((mdl_u8_t*)&val, bci_sizeof(type), src_addr);
				stack_put((mdl_u8_t*)&val, bci_sizeof(type), dest_addr);
				break;
			}

			case _bci_add: {
				mdl_u16_t addr_l, addr_r;
				mdl_u16_t dest_addr;
				mdl_u8_t type = get_w8(__bc_interp);
				mdl_uint_t val_l = 0, val_r = 0;

				dest_addr = get_w16(__bc_interp);
				addr_l = get_w16(__bc_interp);
				addr_r = get_w16(__bc_interp);

				stack_get((mdl_u8_t*)&val_l, bci_sizeof(type), addr_l);
				stack_get((mdl_u8_t*)&val_r, bci_sizeof(type), addr_r);

				mdl_uint_t final = val_l+val_r;
				stack_put((mdl_u8_t*)&final, bci_sizeof(type), dest_addr);
				break;
			}

			case _bci_cmp: {
				mdl_u16_t addr_l, addr_r;
				mdl_u8_t type = get_w8(__bc_interp);
 				mdl_uint_t val_l = 0, val_r = 0;

				addr_l = get_w16(__bc_interp);
				addr_r = get_w16(__bc_interp);

				stack_get((mdl_u8_t*)&val_l, bci_sizeof(type), addr_l);
				stack_get((mdl_u8_t*)&val_r, bci_sizeof(type), addr_r);

				stack_put_w8(val_l == val_r? 1:0, get_w16(__bc_interp));
				break;
			}

			case _bci_je: {
				mdl_u16_t jmp_addr = get_w16(__bc_interp);
				if (stack_get_w8(get_w16(__bc_interp)))
					__bc_interp-> set_pc(jmp_addr);
				break;
			}

			case _bci_jne: {
				mdl_u16_t jmp_addr = get_w16(__bc_interp);
				if (!stack_get_w8(get_w16(__bc_interp)))
					__bc_interp-> set_pc(jmp_addr);
				break;
			}

			case _bci_jmp: {
				mdl_u16_t jmp_addr = get_w16(__bc_interp);
				__bc_interp-> set_pc(jmp_addr);
				break;
			}

			case _bci_exnc: {
				mdl_u8_t type = get_w8(__bc_interp);
				mdl_u16_t ptr_addr = get_w16(__bc_interp);
				mdl_u16_t arg_addr = get_w16(__bc_interp);
				mdl_u16_t ret_addr = get_w16(__bc_interp);

				void *arg = (void*)(stack+arg_addr);
				void *ret = (*((void*(**)(void*))(stack+ptr_addr)))(arg);
				stack_put((mdl_u8_t*)ret, bci_sizeof(type), ret_addr);
				break;
			}

			case _bci_incr: {
				mdl_u8_t type = get_w8(__bc_interp);
				mdl_u16_t addr = get_w16(__bc_interp);
				switch(type) {
					case _bci_w8: stack_put_w8(stack_get_w8(addr)+1, addr); break;
					case _bci_w16: stack_put_w16(stack_get_w16(addr)+1, addr); break;
					case _bci_w32: stack_put_w32(stack_get_w32(addr)+1, addr); break;
					case _bci_w64: stack_put_w64(stack_get_w64(addr)+1, addr); break;
				}
				break;
			}

			case _bci_print: {
				mdl_u8_t type = get_w8(__bc_interp);
				mdl_u16_t addr = get_w16(__bc_interp);

				mdl_uint_t val = 0;
				stack_get((mdl_u8_t*)&val, bci_sizeof(type), addr);
				printf("val: %u, addr: %u\n", val, addr);
				break;
			}
		}
	} while(1);
}
