#include "bsp_delay.h"
#include "imx6ul.h"

void delay_init()
{
	GPT1->CR = 0;
	/* 软件复位GPT1定时器 */
	GPT1->CR |= (1 << 15);
	while (GPT1->CR & (1 << 15))
		;

	/*
   * 设置GPT1使用ipg时钟66Mhz
	 * 设置GPT1 使用restartmode即和计数器和计较寄存器值相等就重置为0
	 * 设置GPT1 disable后不保存计数器的值 
	 */
	GPT1->CR |= (1 << 1) | (1 << 6);
	/* 设置66分频 即 最终的时钟为66000000/66 = 1Mhz*/
	GPT1->PR = 65;
	/* 设置通道1比较寄存器的值 */
	GPT1->OCR[0] = 0xffffffff;
	/* 使能GPT1定时器 */
	GPT1->CR |= (1 << 0);
}

void delay_us(unsigned int timeValue)
{
	unsigned int oldCnt, newCnt, pastTime;
	oldCnt = GPT1->CNT;
	while(1)
	{
		newCnt = GPT1->CNT;
		if (newCnt != oldCnt) {
			if(newCnt > oldCnt){
				pastTime = newCnt - oldCnt;
			}
			else{
				pastTime = 0xffffffff - oldCnt + newCnt + 1;
			}
			if(pastTime >= timeValue)
				break;
		}
	}
}

void delay_ms(unsigned int timeValue){
	unsigned int i;
	for(i = 0; i < timeValue; i++){
		delay_us(1000);
	}

}

void delay_short(volatile unsigned int i)
{
	while (i--)
		;
}

#if 0
void delay_ms(volatile unsigned int msecs)
{

	while (msecs--)
	{
		delay_short(0x7ff);
	}
}
#endif