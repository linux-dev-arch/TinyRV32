#include "rvlibs.h"
volatile unsigned char *led = (volatile unsigned char *)0x1000100;
int main(void)
{
	while(1){
		print_hex(*uart_rx);
		putchar('\n');
		wait_instr(2000);
	}
}
