#ifndef __BSP_DELAY_H
#define __BSP_DELAY_H

void delay_init();
void delay_us(unsigned int timeValue);
void delay_ms(unsigned int timeValue);
void delay_short(volatile unsigned int i);
//void delay_ms(volatile unsigned int msecs);

#endif