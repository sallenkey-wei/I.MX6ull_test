#ifndef __BSP_KEYFILTER_H
#define __BSP_KEYFILTER_H

void key_filter_init();
void key_filter_timer_init();
void stop_epit1();
void restart_epit1(unsigned int reloadValue);
void key_filter_timer_handler(unsigned int giccIar, void *param);
void gpio1_16_31_handler(unsigned int giccIar, void *param);

#endif