#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "MCIMX6Y2.h"
#include "delay.h"
#include "led.h"
#include "beep.h"
#include "clk.h"
#include "bsp_int.h"
#include "bsp_exit.h"
#include "bsp_epit.h"

int main(void){
	int_init();
	led_init();
	beep_init();
	clk_init();
	epit_init(0, 66000000 / 2);
	
	while(1){		
		delay_ms(1000);
	}
    return 0;
}
