# ifndef __bci__h
# define __bci__h
# include <stdio.h>
# define BCI_SUCCESS 0
# define BCI_FAILURE -1
# include <eint_t.h>
# include <malloc.h>
# define _bcii_print 0x0
# define _bcii_exit 0x1
# define _bcii_assign 0x2
# define _bcii_mov 0x3
# define _bcii_aop 0x4
# define _bcii_incr 0x5
# define _bcii_decr 0x6
# define _bcii_cmp 0x7
# define _bcii_jmp 0x8
// types
# define _bcit_w8 0x0
# define _bcit_w16 0x1
# define _bcit_w32 0x2
# define _bcit_w64 0x3

// aop types
# define _bci_aop_add 0x0
# define _bci_aop_mul 0x1
# define _bci_aop_sub 0x2
# define _bci_aop_div 0x3
struct bci_t {
	mdl_u16_t const stack_size;
	mdl_u8_t (*get_byte)();
	void (*set_pc)(mdl_u16_t);
	mdl_u16_t (*get_pc)();
	void (*pc_incr)();
};

typedef mdl_u16_t bci_addr_t;
typedef mdl_i8_t bci_err_t;

mdl_u8_t bci_sizeof(mdl_u8_t);
bci_err_t bci_init(struct bci_t*);
bci_err_t bci_exec(struct bci_t*, mdl_u16_t);
# endif /*__bci__h*/
