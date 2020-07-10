#include "bsp_int.h"

/* 中断服务函数表 */
static sys_irq_handle_t irqTable[NUMBER_OF_INT_VECTORS];
static unsigned int irqNesting;

/*
 * @description : 中断初始化函数
 * @param       : 无 
 * @return      : 无 
 * */
void int_init(void){
    GIC_Init();             /* 初始化GIC */
    system_irqtable_init(); /* 初始化中断表 */
    __set_VBAR(0x87800000); /* 设置中断向量偏移 */
    
}

/*
 * @description : 初始化中断服务函数表
 * @param       : 无
 * @return      : 无 
 * */
void system_irqtable_init(void){
    unsigned int i = 0;
    irqNesting = 0;
    for(i = 0; i < NUMBER_OF_INT_VECTORS; i++){
        irqTable[i].irqHandler = default_irqhandler;
        irqTable[i].userParam = NULL;
    }
}

/*
 * @description        : 给指定的中断号注册中断服务函数
 * @param - irq        : 要注册的中断号
 * @param - handler    : 要注册的中断处理函数 
 * @param - userParam  : 中断服务处理函数参数
 * @return             : 无
 *  */
void system_register_irqhandler(IRQn_Type irq , system_irq_handler_t handle, void * userParam){
    irqTable[irq].irqHandler = handle;
    irqTable[irq].userParam = userParam;
}

/* 
 * @description           :   C语言中断处理函数，irq汇编中断处理函数会调用此函数
 * @param                 :   中断号 
 * @return                :   无
 * */
void system_irqhandler(unsigned int giccIar){
    int intNum = giccIar & 0x3FFUL;
    if(intNum >= NUMBER_OF_INT_VECTORS){
        return;
    }
    irqNesting++;
    irqTable[intNum].irqHandler(giccIar, irqTable[intNum].userParam);
    irqNesting--;
    
}
void default_irqhandler(unsigned int giccIar, void * userParam){
    while(1);
}