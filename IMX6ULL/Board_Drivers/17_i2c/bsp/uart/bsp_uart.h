#ifndef __BSP_UART_H
#define __BSP_UART_H

#include "imx6ul.h"

void uart1_init();
void uart_disable(UART_Type * base);
void uart_enable(UART_Type * base);
void uart_gpio_init();
void putc(unsigned char c);
void puts(char * str);
unsigned char getc(void);
void uart_setbaudrate(UART_Type *base, unsigned int baudrate, unsigned int srcclock_hz);

#endif