#ifndef RVLIB_H
#define RVLIB_H
#include <stdint.h>
#define uart ((volatile char*)0x1000000)
void putchar(char c);
#endif
