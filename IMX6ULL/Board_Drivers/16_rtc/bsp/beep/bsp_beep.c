#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "MCIMX6Y2.h"
#include "bsp_beep.h"
#include "bsp_gpio.h"



void beep_init(){
	IOMUXC_SetPinMux(IOMUXC_SNVS_SNVS_TAMPER1_GPIO5_IO01, 0);
	IOMUXC_SetPinConfig(IOMUXC_SNVS_SNVS_TAMPER1_GPIO5_IO01, 0x10b0);
	gpio_pin_config_t beep_config;
	beep_config.direction = KGPIO_DigitalOutput;
	beep_config.outputLogic = 1;
	gpio_init(GPIO5, 1, &beep_config); /*设置为输出*/
}

void beep_switch(com_status_t status){
	switch(status){
	case ON:
		gpio_pinwrite(GPIO5, 1, 0U);
		break;
	case OFF:
		gpio_pinwrite(GPIO5, 1, 1U);
		break;
	}
}

