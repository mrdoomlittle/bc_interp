# include <stdio.h>
# include <eint_t.h>
# define _bci_assign 0x0
# define _bci_mov 0x1
# define _bci_add 0x2
# define _bci_sub 0x3
# define _bci_mul 0x4
# define _bci_div 0x5
# define _bci_sw8 0x6
# define _bci_pop 0x7

mdl_u8_t buff[] = {
	_bci_pop, _bci_assign, 0x20, _bci_mov, 0x1, 0x0};

mdl_u8_t stack[12];

mdl_u16_t pc = 0, sp = 0;

mdl_u8_t get_byte() {
	return buff[pc];
}

void incrment_sp() {
	sp++;
}

void incrment_pc() {
	pc++;
}

void set_pc(mdl_u16_t __pc) {
	pc = __pc;
}

mdl_u16_t get_pc() {
	return pc;
}

void set_val(mdl_u8_t __b) {
	stack[sp] = __b;
}

mdl_u8_t get_val() {
	return stack[sp];
}

void bci_init(void) {

}

void bci_exec(void) {
	mdl_u8_t i = get_byte();
	printf("instruct: %d\n", i);

	switch(i) {
		case _bci_pop: {
				incrment_sp();
			break;
		}

		case _bci_assign: {
			incrment_pc();
			mdl_u8_t val = get_byte();
			set_val(val);
			break;
		}
		case _bci_mov: {
			incrment_pc();
			mdl_u8_t addr_src = get_byte();
			incrment_pc();
			mdl_u8_t addr_dest = get_byte();

			stack[addr_dest] = stack[addr_src];

			break;
		}

		case _bci_add: {

			break;
		}

		case _bci_sub: {


			break;
		}

		case _bci_mul: {


			break;
		}

		case _bci_div: {


			break;
		}

		case _bci_sw8: {

			break;
		}
	}

	incrment_pc();
}

int main() {
	printf("working.\n");
	for (mdl_u8_t ic = pc; pc != sizeof(buff); ic ++) {
		bci_exec();
	}

	for (mdl_u8_t ic = 0; ic != sizeof(stack); ic++) {
		printf("%u, ", stack[ic]);
	}
}
