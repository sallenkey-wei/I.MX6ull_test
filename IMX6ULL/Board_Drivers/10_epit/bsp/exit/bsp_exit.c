#include "imx6ul.h"
#include "bsp_exit.h"
#include "delay.h"
#include "gpio.h"
#include "beep.h"
#include "bsp_int.h"

void exit_handler(unsigned int giccIar, void *param);

void exit_init(){
	IOMUXC_SetPinMux(IOMUXC_UART1_CTS_B_GPIO1_IO18, 0);
	IOMUXC_SetPinConfig(IOMUXC_UART1_CTS_B_GPIO1_IO18, 0xF080);	
	gpio_pin_config_t exit_config;
	exit_config.direction = KGPIO_DigitalInput;
    exit_config.interruptMode = KGPIO_IntFallingEdge;

	gpio_init(GPIO1, 18, &exit_config); /*外部中断GPIO初始化*/	 
    system_register_irqhandler(GPIO1_Combined_16_31_IRQn , exit_handler, NULL);
    /* 使能GIC中断 */
    GIC_EnableIRQ(GPIO1_Combined_16_31_IRQn);
    /* 使能gpio中断 */
    gpio_enableint(GPIO1, 18);
}

void exit_handler(unsigned int giccIar, void *param){
    static int beepStatus = 0;
    delay_ms(10);
    if(gpio_pinread(GPIO1, 18) == 0){
        beepStatus = !beepStatus;
        beep_switch(beepStatus);
    }
    gpio_clearintflags(GPIO1, 18);
}