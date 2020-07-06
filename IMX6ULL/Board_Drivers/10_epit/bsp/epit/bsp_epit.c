#include "bsp_epit.h"
#include "imx6ul.h"
#include "bsp_int.h"
#include "led.h"

void epit_handler(unsigned int giccIar, void *param);
/*
 * @description            :初始化epit1定时器，并开启比较中断，比较值为0,即定时器计数器递减
 *                          至0的时候，触发一次中断
 * @param  -  frac         : 外设时钟分频参数 0-4095 对应1-4096分频
 * @param  -  reloadValue  : 定时器计数器每次重新加载时加载的值
 * @return                 : 无
 */
void epit_init(unsigned int frac, unsigned int reloadValue){
    if(frac > 4095)
        frac = 4095;
    /* 配置EPIT1 */
    EPIT1->CR = 0;
    /* 
     * CR寄存器:
     *  bit 25:24  :01 时钟选择 Perpheral clock = 66MHz
     *  bit15:4    :frac 分频值
     *  bit3       :1 当计数器到0的话从LR重新加载数值
     *  bit2       :1 比较使能中断
     *  bit1       :1 初始化计数值来源于LR寄存器值
     *  bit0       :0 先关闭EPIT1
     * */
    EPIT1->CR |= (1 << 1) | (1 << 2) |(1 << 3) | (frac << 4) | (1 << 24);

    /*配置CMPR寄存器*/
    EPIT1->CMPR = 0;

    /* 配置Load寄存器 */
    EPIT1->LR = reloadValue;

    /*使能中断*/
    system_register_irqhandler(EPIT1_IRQn, epit_handler, NULL);
    GIC_EnableIRQ(EPIT1_IRQn);

    /* 使能EPIT寄存器 */
    EPIT1->CR |= (1 << 0);
}

void epit_handler(unsigned int giccIar, void *param){
    static int state = 0;
    if(EPIT1->SR & (1 << 0)){
        state = !state;
        led_switch(state);

        /* 清除中断标志 */
        EPIT1->SR &= (1 << 0);
    }
}