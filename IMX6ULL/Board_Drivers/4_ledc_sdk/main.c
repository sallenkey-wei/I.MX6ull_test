#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "MCIMX6Y2.h"

void led_init(){
	IOMUXC_SetPinMux(IOMUXC_GPIO1_IO03_GPIO1_IO03, 0);
	IOMUXC_SetPinConfig(IOMUXC_GPIO1_IO03_GPIO1_IO03, 0x10B0);
	GPIO1->GDIR = 0x08;
}

void led_on(){
    GPIO1->DR &= ~(1<<3);
}

void led_off(){
    GPIO1->DR |= (1<<3);
}

void delay_ms(unsigned int msecs){
    unsigned int i;
    while(msecs--){
        for(i = 0x7ff; i > 0; i--){
            ;
        }
    }
}

int main(void){
    led_init();
    while(1){
        led_on();
        delay_ms(1000);
        led_off();
        delay_ms(1000);
    }
    return 0;
}


