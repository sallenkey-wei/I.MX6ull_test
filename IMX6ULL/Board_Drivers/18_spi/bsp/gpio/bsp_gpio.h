#ifndef __BSP_GPIO_H
#define __BSP_GPIO_H
#include "cc.h"
#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "MCIMX6Y2.h"

typedef enum{
	KGPIO_NoIntmode = 0U,				/* 无中断功能 */
	KGPIO_IntLowLevel = 1U,				/* 低电平触发 */
	KGPIO_IntHightLevel = 2U,			/* 高点平触发 */
	KGPIO_IntRisingEdge = 3U,			/* 上升沿触发 */
	KGPIO_IntFallingEdge = 4U,			/* 下降沿触发 */
	KGPIO_IntRisingOrFallingEdge = 5u,  /* 上升沿和下降沿都触发 */
} gpio_interrupt_mode_t;

typedef enum{
	KGPIO_DigitalInput = 0U,	/* 输入 */
	KGPIO_DigitalOutput = 1U,	/* 输出 */
}gpio_pin_direction_t;

typedef struct _gpio_pin_config{
	gpio_pin_direction_t direction;
	uint8_t outputLogic;
	gpio_interrupt_mode_t interruptMode;  /* 中断方式 */
}gpio_pin_config_t;

void gpio_init(GPIO_Type * base, int pin, gpio_pin_config_t * config);
int gpio_pinread(GPIO_Type * base, int pin);
void gpio_pinwrite(GPIO_Type * base, int pin, int value);
void gpio_intconfig(GPIO_Type * base, unsigned int pin, gpio_interrupt_mode_t pin_int_mode);
void gpio_enableint(GPIO_Type * base, unsigned int pin);
void gpio_disableint(GPIO_Type * base, unsigned int pin);
void gpio_clearintflags(GPIO_Type * base, unsigned int pin);

#endif
