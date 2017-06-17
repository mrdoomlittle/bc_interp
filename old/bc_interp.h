# ifndef __bc__interp__h
# define __bc__interp__h
# include <eint_t.h>
# include <stdio.h>
# include <stdlib.h>
# include "instruct_set.h"
struct bc_interp_t {
	mdl_u16_t const stack_size;
	mdl_u16_t const heep_size;
	mdl_u8_t (*get_byte)();
	void (*set_pc)(mdl_u16_t);
	mdl_u16_t (*get_pc)();
	void (*pc_incr)();
};

typedef mdl_u8_t bci_err_t;
mdl_uint_t bci_sizeof(mdl_u8_t);

bci_err_t bci_init(struct bc_interp_t*);
bci_err_t bci_exec(struct bc_interp_t*, mdl_u16_t);
# endif /*__bc__interp__h*/
