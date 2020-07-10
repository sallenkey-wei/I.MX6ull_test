#include "bsp_key.h"
#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "delay.h"
#include "bsp_gpio.h"

void key_init(){
	IOMUXC_SetPinMux(IOMUXC_UART1_CTS_B_GPIO1_IO18, 0);
	IOMUXC_SetPinConfig(IOMUXC_UART1_CTS_B_GPIO1_IO18, 0xF080);	
	gpio_pin_config_t key_config;
	key_config.direction = KGPIO_DigitalInput;
	gpio_init(GPIO1, 18, &key_config); /*设置为输入*/	 
}

int read_key(){
	int ret; 
	ret = gpio_pinread(GPIO1,  18);
	return ret;
}

key_status key_getvalue(){
	if(gpio_pinread(GPIO1,  18) == 0){
		delay_ms(10);
		if(gpio_pinread(GPIO1,  18) == 0)
			return KEY0;
	}
	return KEY_NONE;
}

