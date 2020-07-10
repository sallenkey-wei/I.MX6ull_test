#ifndef __BSP_INT_H
#define __BSP_INT_H
#include "imx6ul.h"
/* 中断处理函数形式 */
typedef void (*system_irq_handler_t)(unsigned int giccIar, void *param);

/* 中断处理函数结构体 */
typedef struct _sys_irq_handle{
    system_irq_handler_t irqHandler;
    void * userParam;
} sys_irq_handle_t;

/* 函数声明 */
void int_init(void);
void system_irqtable_init(void);
void system_register_irqhandler(IRQn_Type irq , system_irq_handler_t, void * userParam);
void system_irqhandler(unsigned int giccIar);
void default_irqhandler(unsigned int giccIar, void * userParam);

#endif