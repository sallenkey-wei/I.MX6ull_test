#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "MCIMX6Y2.h"
#include "bsp_delay.h"
#include "bsp_beep.h"
#include "bsp_clk.h"
#include "bsp_led.h"
#include "bsp_int.h"
#include "bsp_keyfilter.h"
#include "bsp_uart.h"
#include "stdio.h"

int main(void){
	int_init();
	led_init();
	delay_init();
	beep_init();
	clk_init();
	key_filter_init();
	key_filter_timer_init();
	uart1_init();
	unsigned char ch;
	unsigned char status = 0;
	int a, b;
	
	while(1){		
		printf("输入两个整数, 使用空格隔开:");
		scanf("%d %d", &a, &b);
		printf("\r\n数据%d+%d=%d\r\n", a, b, a+b);
		status = !status;
		led_switch(status);
	}
    return 0;
}
