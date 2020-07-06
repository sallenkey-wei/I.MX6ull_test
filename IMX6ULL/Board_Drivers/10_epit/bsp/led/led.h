#ifndef _LED_H
#define _LED_H
#include "cc.h"

void led_init();

void led_on();

void led_off();

void led_switch(com_status_t ledState);
#endif
