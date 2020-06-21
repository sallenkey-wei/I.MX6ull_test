#include "bsp_key.h"
#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "delay.h"

void key_init(){
	IOMUXC_SetPinMux(IOMUXC_UART1_CTS_B_GPIO1_IO18, 0);
	IOMUXC_SetPinConfig(IOMUXC_UART1_CTS_B_GPIO1_IO18, 0xF080);	
	GPIO1->GDIR &= ~(1<<18); /*设置为输入*/
}

int read_key(){
	int ret; 
	ret = (GPIO1->DR >> 18) & 0x01;
	return ret;
}

key_status key_getvalue(){
	if(read_key() == 0){
		delay_ms(10);
		if(read_key() == 0)
			return KEY0;
	}
	return KEY_NONE;
}

