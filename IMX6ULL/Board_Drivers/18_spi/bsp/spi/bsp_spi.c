#include "bsp_spi.h"

void spi_init(ECSPI_Type *base){
    /*
     * bit0: 使能spi
     * bit3: SMC 设置每次往TXFIFO写数据立即触发burst
     * bit[7:4]: 0001, channel0为master模式
     * bit[19:18]: 00 硬件片选选中通道0, 这里我们使用软件片选
     * bit31:20 : spi数据长度
     */
    base->CONREG = 0;
    base->CONREG |= (1 << 0) | (1 << 3) | (1 << 4) | (7 << 20);

    /*
     * ECSPI通道0设置,即设置CONFIGREG寄存器
     * bit0:	0 通道0 PHA为0
     * bit4:	0 通道0 SCLK高电平有效
     * bit8: 	0 通道0片选信号 当SMC为1的时候此位无效
     * bit12：	0 通道0 POL为0
     * bit16：	0 通道0 数据线空闲时高电平
     * bit20:	0 通道0 时钟线空闲时低电平
	 */
    base->CONFIGREG = 0;

    /*  
     * ECSPI通道0设置，设置采样周期
     * bit[14:0] :	0X2000  采样等待周期，比如当SPI时钟为10MHz的时候
     *  		    0X2000就等于1/10000 * 0X2000 = 0.8192ms，也就是连续
     *          	读取数据的时候每次之间间隔0.8ms
     * bit15	 :  0  采样时钟源为SPI CLK
     * bit[21:16]:  0  片选延时，可设置为0~63
	 */
	base->PERIODREG = 0X2000;		/* 设置采样周期寄存器 */

	/*
     * ECSPI的SPI时钟配置，SPI的时钟源来源于pll3_sw_clk/8=480/8=60MHz
     * 通过设置CONREG寄存器的PER_DIVIDER(bit[11:8])和POST_DIVEDER(bit[15:12])来
     * 对SPI时钟源分频，获取到我们想要的SPI时钟：
     * SPI CLK = (SourceCLK / PER_DIVIDER) / (2^POST_DIVEDER)
     * 比如我们现在要设置SPI时钟为6MHz，那么PER_DIVEIDER和POST_DEIVIDER设置如下：
     * PER_DIVIDER = 0X9。
     * POST_DIVIDER = 0X0。
     * SPI CLK = 60000000/(0X9 + 1) = 60000000=6MHz
	 */
    base->CONREG &= ~((0xf << 12) | (0xf << 8));
    base->CONREG |= (9 << 12);


    


    
}
unsigned char spich0_readwrite_byte(ECSPI_Type *base, unsigned char txdata){
    unsigned int sendData = txdata;
    unsigned int recvData = 0;

    /* 选择通道0 */
    base->CONREG &= ~(3 << 18);
    base->CONREG |= (0 << 18);

    /* 等待TXFIFO为空 */
    while((base->STATREG & (1 << 0)) == 0)
    {}
    base->TXDATA = sendData;
    /* 等待RXFIFO有数据 */
    while((base->STATREG & (1 << 3)) == 0)
    {}
    recvData = base->RXDATA;
    return recvData;
}