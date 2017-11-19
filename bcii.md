| instruction		| info 								| hex val |
|---    			|---    							|---
| _bcii_nop			| no operation						| 0x0
| _bcii_exit		| exit program						| 0x1
| _bcii_assign		| assign value to address			| 0x2
| _bcii_mov			| move value from a to b			| 0x3
| _bcii_aop			| div, mul, add, sub				| 0x4
| _bcii_incr		| increment value					| 0x5
| _bcii_decr		| deincrement value					| 0x6
| _bcii_cmp			| compare 2 values					| 0x7
| _bcii_jmp			| jump to address					| 0x8
| _bcii_cjmp		| conditional jump					| 0x9
| _bcii_dr			| dereference address				| 0xA
| _bcii_conv		|									| 0xB
| _bcii_extern_call | call extern function				| 0xC
| _bcii_eeb_init	|									| 0xD
| _bcii_eeb_put		|									| 0xE
| _bcii_act_indc	| activity indicator				| 0xF
| _bcii_print		| print number						| 0x10
| _bcii_lop			| or, xor, and						| 0x11
| _bcii_shr			| shift right						| 0x12
| _bcii_shl			| shift left						| 0x13
