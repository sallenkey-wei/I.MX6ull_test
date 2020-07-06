#include "clk.h"
#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "MCIMX6Y2.h"
/*可以超频到696MHz，推荐频率528MHz*/
#define OVERCLOCK 0

/* 使能所有外设时钟 */
void clk_enable(){  /*由于boot rom已经使能所有外设时钟，所以这里就不实现了*/
}


void clk_init(){
	/* 1.配置系统时钟 */
	if(((CCM->CCSR >> 2) & 0x1) == 0){ /* 如果系统时钟源为PLL1则切换为step_clk */
		CCM->CCSR &= ~(1 << 8);/* 设置osc_clk 也就是24Mhz作为step_clk时钟源 */
		CCM->CCSR |= (1 << 2); /* 设置step_clk作为pll1的时钟源 */
	}
	CCM_ANALOG->PLL_ARM &= ~(0x7F << 0);/* CCM_ANALOG_PLL_ARM[DIV_SELECT]清零 */
#if OVERCLOCK
	CCM_ANALOG->PLL_ARM |= (58 << 0);/* CCM_ANALOG_PLL_ARM[DIV_SELECT]设置为58，从而 pll1_main_clk 为696MHz*/
#else
	CCM_ANALOG->PLL_ARM |= (88 << 0);/* CCM_ANALOG_PLL_ARM[DIV_SELECT]设置为88，从而 pll1_main_clk 为1056MHz*/
#endif
	CCM_ANALOG->PLL_ARM |= (1 << 13);/* 使能pll_main_clk输出 */	
#if OVERCLOCK
	CCM->CACRR = 0; /* 设置pll_main_clk一分频作为PLL1 */
#else
	CCM->CACRR = 1; /* 设置pll_main_clk二分频作为PLL1 */
#endif

	CCM->CCSR &= ~(1 << 2);/* 设置pll_main_clk作为pll1的时钟源 */


	/* 2.设置PLL2各个PFD 一下操作没有使能各个PFD，可能是bootrom已经使能了吧*/
	CCM_ANALOG->PFD_528 &= ~(0x3f3f3f3f);
	CCM_ANALOG->PFD_528 |= (27 << 0) | (16 << 8) | (24 << 16) | (32 << 24);


	/* 3.设置PLL3各个PFD */
	CCM_ANALOG->PFD_480 &= ~(0x3f3f3f3f);
	CCM_ANALOG->PFD_480 |= (12 << 0) | (16 << 8) | (17 << 16) | (19 << 24);

	/* 4.设置AHB_CLK_ROOT   132MHz */
	CCM->CBCMR &= ~(3 << 18);/* 清零 */
	CCM->CBCMR |= (1 << 18); /* 设置pre_periph_clk 为PLL2_PFD2 = 396MHz */
	CCM->CBCDR &= ~(1 << 25); /* 设置periph_clk 为 pre_periph_clk = 396MHz */
	while((CCM->CDHIPR >> 5) &0x1)/* 等待稳定 */
		;

#if 0
	/* 这里设置之前要先关闭AHB_ROOT_CLK输出，否者会卡在等待稳定那里，但是没找到如何关闭的方法 */
	/* 另外bootrom已经将分频器设置为3分频了，所以这里可以不用设置 */
	CCM->CBCDR &= ~(7 << 10);/* 清零 */
	CCM->CBCDR |= (2 << 10); /* 设置3分频 AHB_ROOT = periph_clk/3 */
	while((CCM->CDHIPR >> 1) &0x1)/* 等待稳定 */
		;
#endif
	/* 5.设置IPG_CLK_ROOT 66MHz   */
	CCM->CBCDR &= ~(3 << 8);/* 清零 */
	CCM->CBCDR |= (1 << 8); /* 设置 IPG_CLK_ROOT = AHB_CLK_ROOT/2 */

	/* 6.设置PRECLK_CLK 66MHz*/
	CCM->CSCMR1 &= ~(1 << 6);/* 设置 设置PRECLK_CLK = IPG_CLK_ROOT =66MHz */
	CCM->CSCMR1 &= ~(0x3F << 0);/* 设置PRECLK_CLK 一分频 */
	
	
}

