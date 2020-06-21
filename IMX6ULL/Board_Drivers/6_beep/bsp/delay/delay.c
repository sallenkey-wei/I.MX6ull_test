#include "delay.h"

void delay_short(volatile unsigned int i){
	while(i--)
		continue;

}

void delay_ms(volatile unsigned int msecs){  

    while(msecs--){
		delay_short(0x7ff);
    }
}

