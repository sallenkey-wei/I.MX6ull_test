#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "MCIMX6Y2.h"
#include "delay.h"
#include "led.h"
#include "beep.h"


int main(void){
    led_init();
	beep_init();
    while(1){
        led_on();
		beep_switch(ON);
        delay_ms(1000);
        led_off();
		beep_switch(OFF);
        delay_ms(1000);
    }
    return 0;
}


