#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "MCIMX6Y2.h"
#include "bsp_delay.h"
#include "bsp_beep.h"
#include "bsp_clk.h"
#include "bsp_led.h"
#include "bsp_int.h"
#include "bsp_keyfilter.h"

int main(void){
	int_init();
	led_init();
	delay_init();
	beep_init();
	clk_init();
	key_filter_init();
	key_filter_timer_init();
	unsigned int ledState = 0;
	
	while(1){		
		ledState = !ledState;
		led_switch(ledState);
		delay_ms(500);
	}
    return 0;
}
