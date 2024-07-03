#include "regdef.h"

#define UART_REF_CLK 100000000	//100MHz for 3A5000
#define UART_DIV_HI	(((UART_REF_CLK + (115200*8)) / (115200*16)) >> 8)
#define UART_DIV_LO	(((UART_REF_CLK + (115200*8)) / (115200*16)) & 0xff)
#define CONSOLE_BASE_ADDR 0x1FE001E0

.globl init_serial
init_serial:
    or     a4, ra, zero

	li.d	a0, CONSOLE_BASE_ADDR
	li.w	a1, 0x80
	st.b	a1, a0, 3

	li.w	a1, UART_DIV_HI
	st.b	a1, a0, 1
	li.w	a1, UART_DIV_LO
	st.b	a1, a0, 0
	li.w	a1, 3
	st.b	a1, a0, 3

	li.w	a1, 71
	st.b	a1, a0, 2

	or      ra, a4, zero
	jirl	zero, ra, 0
    
.globl print_char
print_char:
	li.d	a1, CONSOLE_BASE_ADDR
1:
	ld.bu	a2, a1, 0x5
	andi	a2, a2, 0x20
	beqz	a2, 1b

	st.b	a0, a1, 0

	jirl	zero, ra, 0