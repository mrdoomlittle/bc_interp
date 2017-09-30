# ifndef __bci__h
# define __bci__h
# include <stdio.h>
# define BCI_SUCCESS 0
# define BCI_FAILURE -1
# define BCI_RUNNING 0
# define BCI_STOPPED 1
# include <mdlint.h>
# include <string.h>
# ifndef __AVR
#	include <malloc.h>
# else
#	include <stdlib.h>
# endif
# include <8xdrm.h>
# define _bcii_nop 0x0
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
# define _bcii_eeb_init 0xD
# define _bcii_eeb_put 0xE
# define _bcii_act_indc 0xF
# define _bcii_print 0x10
// deref addr
# define _bcii_assign_fdr_addr 0b10000000

// deref left address
# define _bcii_mov_fdr_sa 0b10000000
// deref right address
# define _bcii_mov_fdr_da 0b01000000

# define _bcii_aop_fl_pm 0b10000000
# define _bcii_aop_fr_pm 0b01000000
# define _bcii_iod_fbc_addr 0b10000000

// types
# define _bcit_w8 0b10000000
# define _bcit_w16 0b01000000
# define _bcit_w32 0b00100000
# define _bcit_w64 0b00010000
# define _bcit_addr 0b00001000
# define _bcit_void 0b00000100
// signed mask
# define _bcit_msigned 0b00000010

# define _bcif_eq 0b10000000
# define _bcif_gt 0b01000000
# define _bcif_lt 0b00100000

# define _bcic_eq 0x1
# define _bcic_neq 0x2
# define _bcic_gt 0x3
# define _bcic_lt 0x4
# define _bcic_leq 0x5
# define _bcic_geq 0x6
// aop types
# define _bci_aop_add 0x0
# define _bci_aop_mul 0x1
# define _bci_aop_sub 0x2
# define _bci_aop_div 0x3

# define _bcie_fsie 0b10000000
# define _bci_fstop 0b10000000

typedef mdl_u8_t bci_flag_t;
typedef mdl_u16_t bci_addr_t;
typedef mdl_i8_t bci_err_t;

struct bci_eeb {
	bci_addr_t b_addr, e_addr;
};

struct bci {
	struct _8xdrm _8xdrm_;
	bci_addr_t const stack_size;
	mdl_u8_t(*get_byte)();
	void(*set_pc)(mdl_u16_t);
	mdl_u16_t (*get_pc)();
	void(*pc_incr)();
	void*(*extern_fp)(mdl_u8_t, void*);
	mdl_u8_t *mem_stack;
	struct bci_eeb *eeb_list;
	void(*act_indc_fp)();
	void(*iei_fp)(void*);
	void *iei_arg;
	mdl_u8_t state;
	bci_flag_t flags;
};

void* bci_resolv_addr(struct bci*, bci_addr_t);
void bci_eeb_call(struct bci*, mdl_u8_t);
mdl_u8_t is_flag(bci_flag_t, bci_flag_t);
mdl_uint_t bcii_overhead_size();
mdl_uint_t bcii_sizeof(mdl_u8_t*, bci_flag_t);
mdl_u8_t bcit_sizeof(mdl_u8_t);
bci_err_t bci_init(struct bci*);
bci_err_t bci_de_init(struct bci*);
bci_err_t bci_exec(struct bci*, mdl_u16_t, bci_flag_t);
void bci_stop(struct bci*);
void bci_set_extern_fp(struct bci*, void*(*)(mdl_u8_t, void*));
void bci_set_act_indc_fp(struct bci*, void(*)());
void bci_set_iei_fp(struct bci*, void(*)(void*));
void bci_set_iei_arg(struct bci*, void*);
# endif /*__bci__h*/
