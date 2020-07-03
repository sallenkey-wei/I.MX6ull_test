#include "gpio.h"
#include "fsl_common.h"
#include "fsl_iomuxc.h"

void gpio_init(GPIO_Type * base, int pin, gpio_pin_config_t * config){
	if(config->direction == KGPIO_DigitalInput){ /* 输入 */
		base->GDIR &= ~(1 << pin);
	}
	else{
		base->GDIR |= 1 << pin;
		gpio_pinwrite(base, pin, config->outputLogic);
	}
}

int gpio_pinread(GPIO_Type * base, int pin){
	return (base->DR >> pin) & 0x01;
}

void gpio_pinwrite(GPIO_Type * base, int pin, int value){
	switch (value){
	case 0U:
		base->DR &= ~(1U << pin);
		break;
	default:
		base->DR |= (1u << pin);
		break;
	} 
}

