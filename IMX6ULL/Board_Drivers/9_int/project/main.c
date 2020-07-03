#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "MCIMX6Y2.h"
#include "delay.h"
#include "led.h"
#include "beep.h"
#include "clk.h"
#include "bsp_int.h"
#include "bsp_exit.h"

int main(void){
	int_init();
    led_init();
	beep_init();
	clk_init();
	exit_init();
	
	int led_status = 0;
    while(1){		
		led_status = !led_status;
		switch(led_status){
		case 0:
			led_on();
			break;
		case 1:
			led_off();
			break;
		}
		delay_ms(1000);
    }
    return 0;
}
