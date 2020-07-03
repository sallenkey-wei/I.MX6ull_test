#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "MCIMX6Y2.h"
#include "delay.h"
#include "led.h"
#include "beep.h"
#include "bsp_key.h"
#include "clk.h"

int main(void){
    led_init();
	beep_init();
	key_init();
	clk_init();
	
	int i = 0;
	int led_status = 0;
    while(1){		
		i++;
		if(i >= 50){
			i = 0;
			led_status = !led_status;
			switch(led_status){
			case 0:
				led_on();
				break;
			case 1:
				led_off();
				break;
			}
		}

		switch(key_getvalue()){
		case KEY0:
			beep_switch(ON);
			break;
		case KEY_NONE:
			beep_switch(OFF);
			break;
		}

        delay_ms(10);

    }
    return 0;
}


