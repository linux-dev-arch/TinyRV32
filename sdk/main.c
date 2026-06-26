#include "rvlibs.h"
volatile unsigned char *led = (volatile unsigned char *)0x1000100;
int i=0;
int main(void)
{
	char str[]="Hello world!\n";
	for(int i=0;i<sizeof(str)-1;i++){
		putchar(str[i]);
	}
	while (1){
		for(uint8_t i=0;i<=255;i++){
			*led=i;
		}
	}
}
