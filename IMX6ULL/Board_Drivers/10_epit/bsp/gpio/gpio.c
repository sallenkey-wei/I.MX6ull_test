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
	gpio_intconfig(base, pin, config->interruptMode);
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

/*
 * @description        :  配置GPIO中断触发模式（高点平 低电平 上升沿 下降沿 上升下降沿都触发）
 * @base			   :  GPIO组
 * @pin 			   :  GPIO引脚号
 * @pin_int_mode       :  中断触发模式
 * @return      	   :  无
 * */
void gpio_intconfig(GPIO_Type * base, unsigned int pin, gpio_interrupt_mode_t pin_int_mode){
	volatile uint32_t * icr;
	int shiftNum = pin;
	if(pin > 15){
		shiftNum = pin - 16;
		icr = &base->ICR2;
	}
	else{
		icr = &base->ICR1;
	}
	base->EDGE_SEL &= ~(0x1 << pin);
	switch(pin_int_mode){
		case KGPIO_IntLowLevel:
			*icr &= ~(0x3 << (shiftNum * 2));
			break;
		case KGPIO_IntHightLevel:
			*icr &= ~(0x3 << (shiftNum * 2));
			*icr |= (0x1 << (shiftNum * 2));
			break;
		case KGPIO_IntRisingEdge:
			*icr &= ~(0x3 << (shiftNum * 2));
			*icr |= (0x2 << (shiftNum * 2));
			break;
		case KGPIO_IntFallingEdge:
			*icr &= ~(0x3 << (shiftNum * 2));
			*icr |= (0x3 << (shiftNum * 2));
			break; 
		case KGPIO_IntRisingOrFallingEdge:
			base->EDGE_SEL &= ~(0x1 << pin);
			break;
		default:
			break;
	}
}

/*
 * @description              :   使能gpio引脚的中断功能
 * @base					 :   GPIO组
 * @pin						 :   要使能中断的引脚号
 * @return					 :   无
 * */
void gpio_enableint(GPIO_Type * base, unsigned int pin){
	base->IMR |= (1U << pin);
}

/*
 * @description              :   禁止gpio引脚的中断功能
 * @base					 :   GPIO组
 * @pin						 :   要使能中断的引脚号
 * @return					 :   无
 * */
void gpio_disableint(GPIO_Type * base, unsigned int pin){
	base->IMR &= ~(1U << pin);
}

/*
 * @description               :   清除中断标志位（写1清除）
 * @base					  :   GPIO组
 * @pin 					  :   引脚号
 * @return					  :   无
 * */
void gpio_clearintflags(GPIO_Type * base, unsigned int pin){
	base->ISR |= (1U << pin);
}