#ifndef RVLIB_H
#define RVLIB_H
#include <stdint.h>
#define uart_tx ((volatile char*)0x1000000)
#define uart_rx ((volatile char*)0x1000004)
void putchar(char c);
void wait_instr(uint16_t in);
void print_hex(uint32_t value);
#endif
