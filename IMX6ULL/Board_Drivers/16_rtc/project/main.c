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
	unsigned char key = 0;
	char buf[160];
	struct rtc_datetime rtcdate;
	unsigned char state = OFF;
	int i = 3, t = 0;

	int_init(); 				/* 初始化中断(一定要最先调用！) */
	clk_init();			/* 初始化系统时钟 			*/
	delay_init();				/* 初始化延时 			*/
	led_init();					/* 初始化led 			*/
	beep_init();				/* 初始化beep	 		*/
	uart1_init();				/* 初始化串口，波特率115200 */
	lcd_init();					/* 初始化LCD 			*/
	rtc_init();

	tftlcd_dev.forecolor = LCD_RED;	  
	tftlcd_dev.backcolor = LCD_WHITE;
	lcd_show_string(10,10,400,32,32,(char*)"ZERO-IMX6UL ELCD TEST");  /* 显示字符串 */

	memset(buf, 0, sizeof(buf));

	while(1)
	{
		if(t==100)	//1s时间到了
		{
			t=0;
			printf("will be running %d s......\r", i);
			
			lcd_fill(50, 90, 370, 110, tftlcd_dev.backcolor); /* 清屏 */
			sprintf(buf, "will be running %ds......", i);
			lcd_show_string(50, 90, 300, 16, 16, buf); 
			i--;
			if(i < 0)
				break;
		}

		key = key_getvalue();
		if(key == KEY0)
		{
			rtcdate.year = 2018;
   			rtcdate.month = 1;
    		rtcdate.day = 15;
    		rtcdate.hour = 16;
    		rtcdate.minute = 23;
    		rtcdate.second = 0;
			rtc_setdatetime(&rtcdate); /* 初始化时间和日期 */
			printf("\r\n RTC Init finish\r\n");
			break;
		}
			
		delay_ms(10);
		t++;
	}
	tftlcd_dev.forecolor = LCD_RED;
	lcd_fill(50, 90, 370, 110, tftlcd_dev.backcolor); /* 清屏 */
	lcd_show_string(50, 90, 200, 16, 16, (char*)"Current Time:");  			/* 显示字符串 */
	tftlcd_dev.forecolor = LCD_BLUE;

	while(1)					
	{	
		rtc_getdatetime(&rtcdate);
		sprintf(buf,"%d/%d/%d %d:%d:%d",rtcdate.year, rtcdate.month, rtcdate.day, rtcdate.hour, rtcdate.minute, rtcdate.second);
		lcd_fill(50,110, 300,130, tftlcd_dev.backcolor);
		lcd_show_string(50, 110, 250, 16, 16,(char*)buf);  /* 显示字符串 */
		
		state = !state;
		led_switch(state);
		delay_ms(1000);	/* 延时一秒 */
	}
	return 0;

}