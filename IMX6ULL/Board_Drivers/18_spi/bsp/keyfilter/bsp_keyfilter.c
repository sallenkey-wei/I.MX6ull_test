#include "bsp_keyfilter.h"
#include "bsp_gpio.h"
#include "imx6ul.h"
#include "bsp_int.h"
#include "bsp_beep.h"

void key_filter_init(){
	IOMUXC_SetPinMux(IOMUXC_UART1_CTS_B_GPIO1_IO18, 0);
	IOMUXC_SetPinConfig(IOMUXC_UART1_CTS_B_GPIO1_IO18, 0xF080);	
	gpio_pin_config_t exit_config;
	exit_config.direction = KGPIO_DigitalInput;
    exit_config.interruptMode = KGPIO_IntFallingEdge;

	gpio_init(GPIO1, 18, &exit_config); /*外部中断GPIO初始化*/	 
    system_register_irqhandler(GPIO1_Combined_16_31_IRQn ,gpio1_16_31_handler, NULL);
    /* 使能GIC中断 */
    GIC_EnableIRQ(GPIO1_Combined_16_31_IRQn);
    /* 使能gpio中断 */
    gpio_enableint(GPIO1, 18);

    /* 配置EPIT定时器 */
    key_filter_timer_init();
}

void key_filter_timer_init(){
    /* 初始化EPIT1定时器 */
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
    EPIT1->CR |= (1 << 1) | (1 << 2) |(1 << 3) | (1 << 24);
    /*配置CMPR寄存器*/
    EPIT1->CMPR = 0;
    /*使能中断*/
    system_register_irqhandler(EPIT1_IRQn, key_filter_timer_handler, NULL);
    GIC_EnableIRQ(EPIT1_IRQn);
}

void stop_epit1(){
    /* 关闭EPIT寄存器 */
    EPIT1->CR &= ~(1 << 0);
}

void restart_epit1(unsigned int reloadValue){
    /* 关闭EPIT寄存器 */
    EPIT1->CR &= ~(1 << 0);
    /* 配置Load寄存器 */
    EPIT1->LR = reloadValue;
    /* 使能EPIT寄存器 */
    EPIT1->CR |= (1 << 0);
}

void key_filter_timer_handler(unsigned int giccIar, void *param){
    static int beepStatus = 0;
    stop_epit1();
    if(EPIT1->SR & (1 << 0)){
        if(gpio_pinread(GPIO1, 18) == 0){
            beepStatus = !beepStatus;
            beep_switch(beepStatus);
        }
    }
    /* 清除中断标志 */
    EPIT1->SR &= (1 << 0);
}

void gpio1_16_31_handler(unsigned int giccIar, void *param){
    if(GPIO1->ISR & (1 << 18)){
        /* 重启epit1定时器，10ms */
        restart_epit1(66000000/100);
    }
    /* 清除中断标志 */
    gpio_clearintflags(GPIO1, 18);
}