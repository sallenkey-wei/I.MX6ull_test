
.global _start

_start:
    ldr pc, =Reset_Handler      /* 复位中断 */
    ldr pc, =Undefined_Handler  /* 未定义指令中断 */
    ldr pc, =SVC_Handler        /* SVC(Supervisor)中断 */
    ldr pc, =PrefAbort_Handler  /* 预取终止中断 */
    ldr pc, =DataAbort_Handler  /* 数据终止中断 */
    ldr pc, =NotUsed_Handler    /* 未使用中断 */
    ldr pc, =IRQ_Handler        /* IRQ中断 */
    ldr pc, =FIQ_Handler        /* FIQ(快速中断) */

.globl _bss_start
_bss_start:
        .word __bss_start

.globl _bss_end
_bss_end:
        .word __bss_end

/* 复位中断 */
Reset_Handler:

    cpsid i             /* 关闭全局中断 */

    /* 关闭I,DCache 和 MMU
     * 采用读-改-写的方式
     */
    mrc p15, 0, r0, c1, c0, 0   /* 读cp15的c1寄存器到r0中 */
    bic r0, r0, #(0x01 << 12)   /* 关闭I cache */
    bic r0, r0, #(0x01 << 2)    /* 关闭D cache */
    bic r0, r0, #(0x01 << 0)    /* 关闭mmu */
    bic r0, r0, #(0x01 << 1)    /* 关闭对齐 */
    bic r0, r0, #(0x01 << 11)   /* 关闭分支预测 */
    mcr p15, 0, r0, c1, c0, 0

#if 0
    /* 汇编设置中断向量偏移 */
    ldr r0, =0x87800000
    dsb
    isb
    mcr p15, 0, r0, c12, c0, 0
    dsb
    isb
#endif

    /* 设置各个模式下的栈指针 */
    /* 设置IRQ模式下sp指针 */
    mrs r0, cpsr
    bic r0, r0, #0x1f
    orr r0, r0, #0x12
    msr cpsr, r0
    ldr sp, =0x80600000
    /* 设置SYS模式下sp指针 */
    mrs r0, cpsr
    bic r0, r0, #0x1f
    orr r0, r0, #0x1f
    msr cpsr, r0
    ldr sp, =0x80400000

    /*设置处理器到SVC模式，并设置sp指针*/
    mrs r0, cpsr
    bic r0, r0, #0x1f
    orr r0, r0, #0x13
    msr cpsr, r0
    ldr sp, =0x80200000


	/*清除BSS段*/
	ldr r0, _bss_start
	ldr r1, _bss_end
	mov r2, #0
bss_loop:
	stmia r0!, {r2}
	cmp r0, r1
	ble bss_loop
    
    cpsie i   /* 使能全局中断 */

    b main

/* 未定义指令中断 */
Undefined_Handler:
    ldr r0, =Undefined_Handler
    bx r0

/* SVC(Supervisor)中断 */
SVC_Handler:
    ldr r0, =SVC_Handler
    bx r0

/* 预取终止中断 */
PrefAbort_Handler:
    ldr r0, =PrefAbort_Handler
    bx r0

/* 数据终止中断 */
DataAbort_Handler:
    ldr r0, =DataAbort_Handler
    bx r0

/* 未使用中断 */
NotUsed_Handler:
    ldr r0, =NotUsed_Handler
    bx r0

/* IRQ中断 */
IRQ_Handler:
    /* 保存现场, lr_irq 和spsr_irq寄存器的之值*/
    push {lr}
    push {r0-r3, r12}
    mrs r0, spsr
    push {r0}

    /* 获取中断id */
    MRC p15, 4, r1, c15, c0, 0   /* 获取GIC寄存器组首地址 */
    add r1, r1, #0x2000          /* 获取CPU Interface 寄存器首地址*/
    ldr r0, [r1, #0xc]           /* 获取GIC_IAR的值 */


    /* 设置进入svc模式, 允许其他IRQ中断进入*/
    #if 0
    MRS r2, cpsr
    bic r2, r2, 0x1f
    orr r2, r2, 0x13
    MSR cpsr, r2
    #endif
    cps #0x13
    
    push {r0, r1}   /* 调用C函数之前保存可能被破坏并且我们用到的寄存器 */
    push {lr}         /* 保存SVC模式的lr寄存器 */
    ldr r2, =system_irqhandler
    blx r2          /* 运行C语言中断处理函数 */
    pop {lr}
    pop {r0, r1}

    cps #0x12       /* 进入IRQ模式 */
    str r0, [r1, #0x10] /* 中断执行完成， 写GIC寄存器组的EOIR寄存器 */

    pop {r0}
    msr spsr_cxsf, r0  /* 恢复spsr */

    pop {r0-r3, r12}
    pop {lr}
    subs pc, lr, #4   /* 这里硬件应该会自动把spsr_irq 恢复到cpsr */ /* 减4是因为流水线 */


/* FIQ(快速中断) */
FIQ_Handler:
    ldr r0, =FIQ_Handler
    bx r0