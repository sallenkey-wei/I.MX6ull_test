/**************************************************************
Copyright © zuozhongkai Co., Ltd. 1998-2019. All rights reserved.
文件名	: 	 mian.c
作者	   : 左忠凯
版本	   : V1.0
描述	   : I.MX6U开发板裸机实验16 LCD液晶屏实验
其他	   : 本实验学习如何在I.MX6U上驱动RGB LCD液晶屏幕，I.MX6U有个
 		 ELCDIF接口，通过此接口可以连接一个RGB LCD液晶屏。
论坛 	   : www.openedv.com
在线教育	: www.yuanzige.com
日志	   : 初版V1.0 2019/1/15 左忠凯创建
**************************************************************/
#include "bsp_clk.h"
#include "bsp_delay.h"
#include "bsp_led.h"
#include "bsp_beep.h"
#include "bsp_key.h"
#include "bsp_int.h"
#include "bsp_uart.h"
#include "stdio.h"
#include "bsp_lcd.h"
#include "bsp_lcdapi.h"
#include "bsp_rtc.h"
#include "bsp_ap3216c.h"


/* 背景颜色索引 */
unsigned int backcolor[10] = {
	LCD_BLUE, 		LCD_GREEN, 		LCD_RED, 	LCD_CYAN, 	LCD_YELLOW, 
	LCD_LIGHTBLUE, 	LCD_DARKBLUE, 	LCD_WHITE, 	LCD_BLACK, 	LCD_ORANGE

}; 
	

/*
 * @description	: main函数
 * @param 		: 无
 * @return 		: 无
 */
int main(void)
{
	unsigned char state = OFF;
	unsigned short ir, ps, als;

	int_init(); 				/* 初始化中断(一定要最先调用！) */
	clk_init();			/* 初始化系统时钟 			*/
	delay_init();				/* 初始化延时 			*/
	led_init();					/* 初始化led 			*/
	beep_init();				/* 初始化beep	 		*/
	uart1_init();				/* 初始化串口，波特率115200 */
	lcd_init();					/* 初始化LCD 			*/
	rtc_init();

	tftlcd_dev.forecolor = LCD_RED;	
	lcd_show_string(30, 50, 200, 16, 16, (char*)"ZERO-IMX6U IIC TEST");  
	lcd_show_string(30, 70, 200, 16, 16, (char*)"AP3216C TEST");  
	lcd_show_string(30, 90, 200, 16, 16, (char*)"ATOM@ALIENTEK");  
	lcd_show_string(30, 110, 200, 16, 16, (char*)"2019/3/26");  
	
	while(ap3216c_init())		/* 检测不到AP3216C */
	{
		lcd_show_string(30, 130, 200, 16, 16, (char*)"AP3216C Check Failed!");
		delay_ms(500);
		lcd_show_string(30, 130, 200, 16, 16, (char*)"Please Check!        ");
		delay_ms(500);
	}	
	
	lcd_show_string(30, 130, 200, 16, 16, (char*)"AP3216C Ready!");  
    lcd_show_string(30, 160, 200, 16, 16, (char*)" IR:");	 
	lcd_show_string(30, 180, 200, 16, 16, (char*)" PS:");	
	lcd_show_string(30, 200, 200, 16, 16, (char*)"ALS:");	
	tftlcd_dev.forecolor = LCD_BLUE;	
	while(1)					
	{
		ap3216c_readdata(&ir, &ps, &als);		/* 读取数据		  	*/
		printf("ir = %d, ps = %d, als = %d\r\n", ir, ps, als);
		//lcd_shownum(30 + 32, 160, ir, 5, 16);	/* 显示IR数据 		*/
        //lcd_shownum(30 + 32, 180, ps, 5, 16);	/* 显示PS数据 		*/
        //lcd_shownum(30 + 32, 200, als, 5, 16);	/* 显示ALS数据 	*/ 
		delay_ms(500);
		state = !state;
		led_switch(state);	
	}
	return 0;
}