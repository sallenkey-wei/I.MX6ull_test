#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "MCIMX6Y2.h"
#include "delay.h"
#include "led.h"


int main(void){
    led_init();
    while(1){
        led_on();
        delay_ms(100);
        led_off();
        delay_ms(900);
    }
    return 0;
}


