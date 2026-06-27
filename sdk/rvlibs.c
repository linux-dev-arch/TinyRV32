#include "rvlibs.h"
void putchar(char c){
	*uart_tx= c;
}
void wait_instr(uint16_t in){
	uint16_t n=0;
	for(uint16_t i=0;i<=in;i++){
		n++;
	}
}
void print_hex(uint32_t value){
	const char hex[]="0123456789ABCDEF";
	for(int i =28;i>=0;i-=4){
		putchar(hex[(value >> i) & 0xF]);
	}
}
