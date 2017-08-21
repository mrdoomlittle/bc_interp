# ifndef __bci__h
# define __bci__h
# include <stdio.h>
# define BCI_SUCCESS 0
# define BCI_FAILURE -1
# include <eint_t.h>
# include <string.h>
# ifndef __AVR
#	include <malloc.h>
# else
#	include <stdlib.h>
# endif
# include <8xdrm.h>
# define _bcii_print 0x0
# define _bcii_exit 0x1
# define _bcii_assign 0x2
# define _bcii_mov 0x3
# define _bcii_aop 0x4
# define _bcii_incr 0x5
# define _bcii_decr 0x6
# define _bcii_cmp 0x7
# define _bcii_jmp 0x8
# define _bcii_cjmp 0x9
# define _bcii_dr 0xA
# define _bcii_conv 0xB // debugging
# define _bcii_extern_call 0xC
// deref addr
# define _bcii_assign_fdr_addr 0b10000000

// deref left address
# define _bcii_mov_fdr_sa 0b10000000
// deref right address
# define _bcii_mov_fdr_da 0b01000000

# define _bcii_aop_fl_pm 0b10000000
# define _bcii_aop_fr_pm 0b01000000
// types
# define _bcit_w8 0b10000000
# define _bcit_w16 0b01000000
# define _bcit_w32 0b00100000
# define _bcit_w64 0b00010000
# define _bcit_addr 0b00001000

# define _bcic_eq 0x0
# define _bcic_neq 0x1

// aop types
# define _bci_aop_add 0x0
# define _bci_aop_mul 0x1
# define _bci_aop_sub 0x2
# define _bci_aop_div 0x3
struct bci {
	struct _8xdrm_t _8xdrm;
	mdl_u16_t const stack_size;
	mdl_u8_t (*get_byte)();
	void (*set_pc)(mdl_u16_t);
	mdl_u16_t (*get_pc)();
	void (*pc_incr)();
	void*(*extern_func)(mdl_u8_t, void*);
	void (*act_ind_func)();
};

typedef mdl_u8_t bcii_flag_t;
typedef mdl_u16_t bci_addr_t;
typedef mdl_i8_t bci_err_t;


mdl_u8_t is_flag(bcii_flag_t, bcii_flag_t);
mdl_uint_t bcii_overhead_size();
mdl_uint_t bcii_sizeof(mdl_u8_t*, bcii_flag_t);
mdl_u8_t bcit_sizeof(mdl_u8_t);
bci_err_t bci_init(struct bci*);
bci_err_t bci_de_init(struct bci*);
bci_err_t bci_exec(struct bci*, mdl_u16_t);
void bci_set_extern_func(struct bci*, void*(*)(mdl_u8_t, void*));
void bci_set_act_ind_func(struct bci*, void(*)());
# endif /*__bci__h*/
