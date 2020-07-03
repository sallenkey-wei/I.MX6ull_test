#include "clk.h"
#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "MCIMX6Y2.h"
/*可以超频到696MHz，推荐频率528MHz*/
#define OVERCLOCK 0


void clk_init(){
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
}

