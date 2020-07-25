#ifndef __BSP_LED_H
#define _BSP_LED_H
#include "cc.h"

void led_init();

void led_on();

void led_off();

void led_switch(unsigned char ledState);
#endif
