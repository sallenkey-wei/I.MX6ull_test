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
	unsigned char index = 0;
	unsigned char state = OFF;

	int_init(); 				/* 初始化中断(一定要最先调用！) */
	clk_init();			/* 初始化系统时钟 			*/
	delay_init();				/* 初始化延时 			*/
	led_init();					/* 初始化led 			*/
	beep_init();				/* 初始化beep	 		*/
	uart1_init();				/* 初始化串口，波特率115200 */
	lcd_init();					/* 初始化LCD 			*/

	tftlcd_dev.forecolor = LCD_RED;	  
	tftlcd_dev.backcolor = LCD_WHITE;
	lcd_show_string(10,10,400,32,32,(char*)"ZERO-IMX6UL ELCD TEST");  /* 显示字符串 */

	while(1)				
	{	
		index++;
		if(index == 10)
			index = 0;
		lcd_clear(backcolor[index]);
		lcd_show_string(10,10,400,32,32,(char*)"ZERO-IMX6UL ELCD TEST");  /* 显示字符串 */
		state = !state;
		led_switch(state);
		delay_ms(1000);	/* 延时一秒	*/
	}
	return 0;
}