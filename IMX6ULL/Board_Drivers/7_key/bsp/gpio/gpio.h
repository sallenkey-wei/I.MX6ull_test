#ifndef __GPIO_H
#define __GPIO_H
#include "cc.h"
#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "MCIMX6Y2.h"

typedef enum{
	KGPIO_DigitalInput = 0U,	/* 输入 */
	KGPIO_DigitalOutput = 1U,	/* 输出 */
}gpio_pin_direction_t;

typedef struct _gpio_pin_config{
	gpio_pin_direction_t direction;
	uint8_t outputLogic;
}gpio_pin_config_t;

void gpio_init(GPIO_Type * base, int pin, gpio_pin_config_t * config);
int gpio_pinread(GPIO_Type * base, int pin);
void gpio_pinwrite(GPIO_Type * base, int pin, int value);

#endif
