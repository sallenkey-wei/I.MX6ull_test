#include "imx6ul.h"
#include "bsp_i2c.h"

void i2c_init(I2C_Type * base){
    /* 关闭I2C */
    base->I2CR &= ~(1 << 7);

    /* 设置波特率66000000 / 640 = 103.2 Kb/s */
    base->IFDR = 0x15;

    /* 开启I2C */
    base->I2CR |= (1 << 7);
}
/* 
 * @description            : 产生开始信号，并发送要读或者写的从机的地址
 * @return                 : 返回1表示失败 
 * */
unsigned char i2c_master_start(I2C_Type *base, 
                               unsigned char address, 
                               enum i2c_direction direction){
    /* 总线忙则返回1 表示失败 */
    if(base->I2SR & (1 << 5))
        return 1;
    /* 设置为master发送模式 */
    base->I2CR |= (1 << 4) | (1 << 5); /* bit5 从0到1,切换到master模式，并产生start信号,详见手册 */
    base->I2DR = (address << 1) | (direction == KI2C_Read ? 1 : 0);
    return 0;
}
unsigned char i2c_master_repeated_start(I2C_Type *base, 
                                        unsigned char address, 
                                        enum i2c_direction direction){
    /* 如果总线被占用，但是不是被我们占用(即工作在从机模式)，则返回1 表示失败 */
    if((base->I2SR & (1 << 5)) && !(base->I2CR & (1 << 5)))
        return 1;

    /* 产生restart信号 */
    base->I2CR |= (1 << 2) | (1 << 4);
    base->I2DR = address;
    
}
unsigned char i2c_check_and_clear_error(I2C_Type *base, 
                                        unsigned int status);
unsigned char i2c_master_stop(I2C_Type *base);
void i2c_master_write(I2C_Type *base, const unsigned char *buf,
                      unsigned size);
void i2c_master_read(I2C_Type *base, unsigned char *buf, 
                     unsigned int size);
unsigned char i2c_master_transfer(I2C_Type *base, 
                                  struct i2c_transfer *xfer);