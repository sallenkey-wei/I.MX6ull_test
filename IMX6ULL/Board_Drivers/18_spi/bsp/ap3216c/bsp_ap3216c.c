#include "bsp_ap3216c.h"
#include "bsp_i2c.h"
#include "bsp_delay.h"
#include "imx6ul.h"
#include "stdio.h"

unsigned char ap3216c_init(void){
    unsigned char data = 0;

	/* 1、IO初始化，配置I2C IO属性	
     * I2C1_SCL -> UART4_TXD
     * I2C1_SDA -> UART4_RXD
     */
	IOMUXC_SetPinMux(IOMUXC_UART4_TX_DATA_I2C1_SCL, 1);
	IOMUXC_SetPinMux(IOMUXC_UART4_RX_DATA_I2C1_SDA, 1);
    /* 
	 *bit 16:0 HYS关闭
	 *bit [15:14]: 1 默认47K上拉
	 *bit [13]: 1 pull功能
	 *bit [12]: 1 pull/keeper使能 
	 *bit [11]: 0 关闭开路输出
	 *bit [7:6]: 10 速度100Mhz
	 *bit [5:3]: 110 驱动能力为R0/6
	 *bit [0]: 1 高转换率
	 */
	IOMUXC_SetPinConfig(IOMUXC_UART4_TX_DATA_I2C1_SCL, 0x70B0);
	IOMUXC_SetPinConfig(IOMUXC_UART4_RX_DATA_I2C1_SDA, 0X70B0);
    i2c_init(I2C1);/* 初始化I2C1 */
    /* 初始化AP3216c */
    ap3216c_writeonebyte(AP3216C_ADDR, AP3216C_SYSTEMCONG, 0x04); /* 复位AP3216c */
    delay_ms(10);
    ap3216c_writeonebyte(AP3216C_ADDR, AP3216C_SYSTEMCONG, 0x03); /* 开启ALS, PS+IR */
    data = ap3216c_readonebyte(AP3216C_ADDR, AP3216C_SYSTEMCONG);
    if(data == 0x03)
        return 0; /* 成功 */
    else
        return 1; /* 失败 */
}

unsigned char ap3216c_readonebyte(unsigned char addr,unsigned char reg){
    unsigned char value;
    struct i2c_transfer xfer;
    xfer.slaveAddress = addr;
    xfer.data = &value;
    xfer.dataSize = 1;
    xfer.direction = KI2C_Read;
    xfer.subaddress = reg;
    xfer.subaddressSize = 1;
    i2c_master_transfer(I2C1,  &xfer);
    return value;
    
}

unsigned char ap3216c_writeonebyte(unsigned char addr,unsigned char reg, unsigned char data){
    int ret;
    struct i2c_transfer xfer;
    xfer.slaveAddress = addr;
    xfer.data = &data;
    xfer.dataSize = 1;
    xfer.direction = KI2C_Write;
    xfer.subaddress = reg;
    xfer.subaddressSize = 1;
    ret = i2c_master_transfer(I2C1,  &xfer);
    return ret;
}

unsigned char ap3216c_readNumbyte(unsigned char addr,unsigned char regStartAddr, unsigned char * buf, unsigned char size){
    unsigned char ret = 0;
    struct i2c_transfer xfer;
    xfer.slaveAddress = addr;
    xfer.data = buf;
    xfer.dataSize = size;
    xfer.direction = KI2C_Read;
    xfer.subaddress = regStartAddr;
    xfer.subaddressSize = 1;
    ret = i2c_master_transfer(I2C1,  &xfer);
    return ret;
}

void ap3216c_readdata(unsigned short *ir, unsigned short *ps, unsigned short *als){
#if 0
/* 连续读取的数值有问题 */
    unsigned char buf[6];
    memset(buf, 0, sizeof(buf));
    ap3216c_readNumbyte(AP3216C_ADDR, AP3216C_IRDATALOW, buf, 6);
    if(buf[0] & 0X80) 	/* IR_OF位为1,则数据无效 */
		*ir = 0;					
	else 				/* 读取IR传感器的数据   		*/
		*ir = ((unsigned short)buf[1] << 2) | (buf[0] & 0X03); 			
	
	*als = ((unsigned short)buf[3] << 8) | buf[2];	/* 读取ALS传感器的数据 			 */  
	
    if(buf[4] & 0x40)	/* IR_OF位为1,则数据无效 			*/
		*ps = 0;    													
	else 				/* 读取PS传感器的数据    */
		*ps = ((unsigned short)(buf[5] & 0X3F) << 4) | (buf[4] & 0X0F);
    *als = (buf[3] << 8) | buf[2];
#else
    unsigned char buf[6];
    unsigned char i;

	/* 循环读取所有传感器数据 */
    for(i = 0; i < 6; i++)	
    {
        buf[i] = ap3216c_readonebyte(AP3216C_ADDR, AP3216C_IRDATALOW + i);	
    }
	
    if(buf[0] & 0X80) 	/* IR_OF位为1,则数据无效 */
		*ir = 0;					
	else 				/* 读取IR传感器的数据   		*/
		*ir = ((unsigned short)buf[1] << 2) | (buf[0] & 0X03); 			
	
	*als = ((unsigned short)buf[3] << 8) | buf[2];	/* 读取ALS传感器的数据 			 */  
	
    if(buf[4] & 0x40)	/* IR_OF位为1,则数据无效 			*/
		*ps = 0;    													
	else 				/* 读取PS传感器的数据    */
		*ps = ((unsigned short)(buf[5] & 0X3F) << 4) | (buf[4] & 0X0F); 
    *als = (buf[3] << 8) | buf[2];
#endif
}