| bcii 				| info 								| val |
|---    			|---    							|---
| _bcii_exit		| exit program						| 0x1
| _bcii_assign		| assign value to stack address		| 0x2
| _bcii_mov			| move value from a to b in stack	| 0x3
| _bcii_aop			| div, mul, add, sub				| 0x4
| _bcii_incr		| increment value					| 0x5
| _bcii_decr		| deincrement value					| 0x6
| _bcii_cmp			| compare 2 values					| 0x7
| _bcii_jmp			| jump to address					| 0x8
| _bcii_cjmp		| conditional jump					| 0x9
| _bcii_dr			| deregister address				| 0xA
| _bcii_extern_call | call extern function				| 0xB
