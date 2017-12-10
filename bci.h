# ifndef __bci__h
# define __bci__h
# include <stdio.h>
# define BCI_SUCCESS 0
# define BCI_FAILURE -1
# define BCI_RUNNING 0
# define BCI_STOPPED 1
//# define __DEBUG_ENABLED
# include <mdlint.h>
# include <string.h>
# ifndef __AVR
#	include <malloc.h>
# else
#	include <stdlib.h>
# endif
# include <mdl/bitct.h>
# define _bci_addr_size sizeof(bci_addr_t)
# define _bcie_success 0
# define _bcie_failure 1

# define _bcii_nop 0x0
# define _bcii_exit 0x1
# define _bcii_as 0x2
# define _bcii_mov 0x3
# define _bcii_aop 0x4
# define _bcii_incr 0x5
# define _bcii_decr 0x6
# define _bcii_cmp 0x7
# define _bcii_jmp 0x8
# define _bcii_cjmp 0x9
# define _bcii_dr 0xA
# define _bcii_conv 0xB
# define _bcii_exc 0xC
# define _bcii_eeb_init 0xD
# define _bcii_eeb_put 0xE
# define _bcii_act_indc 0xF
# define _bcii_print 0x10
# define _bcii_lop 0x11
# define _bcii_shr 0x12
# define _bcii_shl 0x13
# define _bcii_la 0x14
# define _bcii_getarg 0x15
# define _bcii_ula 0x16
// deref addr
# define _bcii_as_fdr_addr 0b10000000

// deref left address
# define _bcii_mov_fdr_sa 0b10000000
// deref right address
# define _bcii_mov_fdr_da 0b01000000

# define _bcii_aop_fl_pm 0b10000000
# define _bcii_aop_fr_pm 0b01000000
# define _bcii_iod_fbc_addr 0b10000000

# define _bcii_lop_fl_pm 0b10000000
# define _bcii_lop_fr_pm 0b01000000

// types
# define _bcit_8l 0x80
# define _bcit_16l 0x40
# define _bcit_32l 0x20
# define _bcit_64l 0x10
# define _bcit_addr 0x8
// ignore
# define _bcit_void 0x4

// signed mask
# define _bcit_msigned 0b00000010

# define _bcif_eq 0x1
# define _bcif_gt 0x2
# define _bcif_lt 0x4

# define _bcic_eq 0x1
# define _bcic_neq 0x2
# define _bcic_gt 0x3
# define _bcic_lt 0x4
# define _bcic_leq 0x5
# define _bcic_geq 0x6
// aop kinds
# define _bci_aop_add 0x0
# define _bci_aop_mul 0x1
# define _bci_aop_sub 0x2
# define _bci_aop_div 0x3

# define _bci_lop_xor 0x0
# define _bci_lop_or 0x1
# define _bci_lop_and 0x2

# define _bcie_fsie 0b10000000
# define _bci_fstop 0b10000000

# define _bcii_flg_dra

typedef mdl_u8_t bci_flag_t;
typedef mdl_u16_t bci_addr_t;
typedef mdl_i8_t bci_err_t;
typedef mdl_u16_t bci_off_t;
typedef mdl_u16_t bci_uint_t;
struct bci_eeb {
	bci_addr_t b_addr, e_addr;
};

struct bci_arg {
	void *p;
	bci_uint_t bc;
};

struct bci {
	struct bitct _bitct;
	bci_off_t ip_off;
	bci_uint_t const stack_size;
	mdl_u8_t(*get_byte)(bci_off_t);
	void(*set_ip)(bci_addr_t);
	bci_addr_t(*get_ip)();
	void(*ip_incr)(bci_uint_t);
	void*(*extern_fp)(mdl_u8_t, void*);
	mdl_u8_t *mem_stack;
	bci_uint_t const prog_size;
	struct bci_eeb *eeb_list;
	void(*act_indc_fp)();
	void(*iei_fp)(void*);
	void *iei_arg;

	mdl_u8_t(*ula_guard)(void*, void*);
	void *ulag_arg;
	mdl_u8_t *locked_addrs;
	mdl_uint_t m_rd, m_wr;
	mdl_u8_t state;
	bci_flag_t flags;
	struct bci_arg *args;
};

void bci_set_ula_guard(struct bci*, mdl_u8_t(*)(void*, void*), void*);
void bci_unlock_addr(struct bci*, bci_addr_t);
void bci_set_arg(struct bci*, void*, bci_uint_t, mdl_u8_t);
struct bci_arg *bci_get_arg(struct bci*, mdl_u8_t);
void* bci_resolv_addr(struct bci*, bci_addr_t);
void bci_eeb_call(struct bci*, mdl_u8_t);
mdl_u8_t is_flag(bci_flag_t, bci_flag_t);
mdl_uint_t bcii_overhead_size();
mdl_uint_t bcii_sizeof(mdl_u8_t*, bci_flag_t);
mdl_u8_t bcit_sizeof(mdl_u8_t);
bci_err_t bci_init(struct bci*, mdl_u8_t);
bci_err_t bci_de_init(struct bci*);
bci_err_t bci_exec(struct bci*, bci_addr_t, bci_addr_t*, bci_err_t*, bci_flag_t);
void bci_stop(struct bci*);
void bci_set_extern_fp(struct bci*, void*(*)(mdl_u8_t, void*));
void bci_set_act_indc_fp(struct bci*, void(*)());
void bci_set_iei_fp(struct bci*, void(*)(void*));
void bci_set_iei_arg(struct bci*, void*);
# endif /*__bci__h*/
