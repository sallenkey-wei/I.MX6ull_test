.global _start @全局标号

_start:
    /*使能所有外设时钟 */
    ldr r0, =0x020c4068 @ CCGR0寄存器
    ldr r1, =0xffffffff @ 要向CCGR0写入的数据
    str r1, [r0]        @ 将0xffffffff写入CCGR0

    ldr r0, =0x020c406C @ CCGR1
    str r1, [r0]        

    ldr r0, =0x020c4070 @ CCGR2
    str r1, [r0]        

    ldr r0, =0x020c4074 @ CCGR3
    str r1, [r0]  

    ldr r0, =0x020c4078 @ CCGR4
    str r1, [r0]  
    ldr r0, =0x020c407C @ CCGR5
    str r1, [r0]  
    ldr r0, =0x020c4080 @ CCGR6
    str r1, [r0]  

	/*设置CPIO1_IO03复用为GPIO1_IO03*/
	ldr r0, =0x020E0068 /*将寄存器SW_MUX_GPIO1_IO03_BASE加载到r0中*/
	ldr r1, =0x5 /*设置寄存器SW_MUX_GPIO1_IO03_BASE的MUX_MODE为5*/
	str r1, [r0]

	/* 配置GPIO1_IO03的IO属性
	 * bit16:0 HYS关闭
	 * bit [15:14]: 00 默认下拉
	 * bit [13]: 0 keeper 功能
	 * bit[12]: 1 pull/keeper使能
	 * bit[11]: 0 关闭开路输出
	 * bit[7:6]: 10 速度100MHz
	 * bit[5:3] 110 R0/6 驱动能力
	 * bit [0]: 0 低转换率
	 */
	ldr r0, =0x020e02f4 /*寄存器SW_PAD_GPIO1_IO03_BASE*/
	ldr r1, =0x10B0
	str r1, [r0]

	/*设置GPIO1_IO03为输出*/
	ldr r0, =0x0209c004 /*寄存器GPIO1_GDIR*/
	ldr r1, =0x8
	str r1, [r0]

	/*打开LED0
	 *设置GPIO1_IO03输出低电平
	 */
	ldr r0, =0x0209c000 /*寄存器GPIO1_DR*/
	ldr r1, =0
	str r1, [r0]

	/*loop死循环*/
loop:
	b loop
