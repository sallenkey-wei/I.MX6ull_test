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
/* restart信号只在读取I2C时用，其他时候不用 */
unsigned char i2c_master_repeated_start(I2C_Type *base, 
                                        unsigned char address, 
                                        enum i2c_direction direction){
    /* 如果总线被占用，但是不是被我们占用(即工作在从机模式)，则返回1 表示失败 */
    if((base->I2SR & (1 << 5)) && !(base->I2CR & (1 << 5)))
        return 1;

    /* 产生restart信号 */
    base->I2CR |= (1 << 2) | (1 << 4); /* 触发restart信号，并设置为发送模式 */
    base->I2DR = (address << 1) | (direction == KI2C_Read ? 1 : 0);
    return 0;
}

unsigned char i2c_check_and_clear_error(I2C_Type *base, 
                                        unsigned int status){
    /* 检查是否总线仲裁丢失 */
    if(status & (1 << 4)) {
        base->I2SR &= ~(1 << 4); /* 清楚仲裁丢失错误标志位 */
        base->I2CR &= ~(1 << 7); /* 关闭I2C */
        base->I2CR |= (1 << 7); /* 打开I2C */
        return I2C_STATUS_ARBITRATIONLOST;
    }
    /* 检查是否ack没有回复错误 */
    if(status & (1 << 0)){
        return I2C_STATUS_NAK;
    }
    return I2C_STATUS_OK;
}

unsigned char i2c_master_stop(I2C_Type *base){
    unsigned short timeout = 0xffff;
    base->I2CR &= ~((1 << 5) | (1 << 4) | (1 << 3));
    while(base->I2SR & (1 << 5)){
        timeout--;
        if(timeout == 0){
            return I2C_STATUS_TIMEOUT;
        }
    }
    return I2C_STATUS_OK;
}
void i2c_master_write(I2C_Type *base, const unsigned char *buf,
                      unsigned size){
    /* 等待传输完成 */
    while(!(base->I2SR & (1 << 7)))
    {}
    base->I2SR &= ~(1 << 1); /* 清除中断标志位 */
    base->I2CR |= 1 << 4; /* 设置发送数据 */
    while(size--){
        base->I2DR = *buf++; /* 将buf中数据写入到I2DR寄存器 */
        /* 等待发送完成 */
        while(!(base->I2SR & (1 << 1)))
        {}
        base->I2SR &= ~(1 << 1); /* 清除标志位 */
        /* 检查ack */
        if(i2c_check_and_clear_error(base, base->I2SR))
            break;
    }
    base->I2SR &= ~(1 << 1);
    i2c_master_stop(base);
}

void i2c_master_read(I2C_Type *base, unsigned char *buf, 
                     unsigned int size){
    volatile unsigned char dummy = 0;

    dummy++; /* 防止编译出错 */

    /* 等待传输完成 */
    while(!(base->I2SR & (1 << 7)))
    {}
    base->I2SR &= ~(1 << 1); /* 清除中断标志位 */
    base->I2CR &= ~((1 << 4) | (1 << 3)); /* 设置为接收数据 */
    /* 如果接收只接收一个数据的话， 发送NACK信号*/
    if(size == 1)
        base->I2CR |= (1 << 3);
    dummy = base->I2DR; /* 假读 */
    while(size--){
        while(!(base->I2SR & (1 << 1))) /* 等待传输完成 */
        {}
        base->I2SR &= ~(1 << 1); /* 清除中断标志位 */
        if(size == 0)
            i2c_master_stop(base);
        if(size == 1)
            base->I2CR |= (1 << 3);
        *buf++ = base->I2DR;
    }
}
unsigned char i2c_master_transfer(I2C_Type *base, 
                                  struct i2c_transfer *xfer){
    unsigned char ret = 0;
    enum i2c_direction direction = xfer->direction;
    base->I2SR &= ~((1 << 1) | (1 << 4)); /* 清除中断标志位，并设置为接收 */
    /* 等待传输完成 */
    while(!(base->I2SR & (1 << 7))){}
    /* 如果时读的话，先发送寄存器地址，所以先改为写 */
    if((xfer->subaddressSize > 0) && (xfer->direction == KI2C_Read)){
        direction = KI2C_Write;
    }
    ret = i2c_master_start(base, xfer->slaveAddress, direction); /* 发送开始信号 */
    if(ret)
        return ret;
    while(!(base->I2SR &(1 << 1))){}; /* 等待传输完成 */
    ret = i2c_check_and_clear_error(base, base->I2SR);
    if(ret){
        i2c_master_stop(base);
        return ret;
    }
    /* 发送寄存器地址 */
    if(xfer->subaddressSize){
        do{
            base->I2SR &= ~(1 << 1); /* 清除中断标志 */
            xfer->subaddressSize--;
            base->I2DR = ((xfer->subaddress) >> (8 * xfer->subaddressSize));
            while(!(base->I2SR & (1 << 1)));  	/* 等待传输完成 */
            /* 检查是否有错误发送 */
            ret = i2c_check_and_clear_error(base, base->I2SR);
            if(ret){
                i2c_master_stop(base); /* 发送停止信号 */
                return ret;
            }
        }while((xfer->subaddressSize > 0) && (ret == I2C_STATUS_OK));

        if(xfer->direction == KI2C_Read){
            base->I2SR &= ~(1 << 1); /* 清除中断标志位 */
            i2c_master_repeated_start(base, xfer->slaveAddress, KI2C_Read); /* 发送重新开始信号 */
            while(!(base->I2SR &(1 << 1))){}; /* 等待传输完成 */
            ret = i2c_check_and_clear_error(base, base->I2SR);

            /*检查是否有错误发生*/
            if(ret){
                ret = I2C_STATUS_ADDRNAK;
                i2c_master_stop(base);
                return ret;
            }
        }
    }
    /* 发送数据 */
    if((xfer->direction == KI2C_Write) && (xfer->dataSize > 0)){
        i2c_master_write(base, xfer->data, xfer->dataSize);
    }
    /* 读取数据 */
    if((xfer->direction == KI2C_Read) && (xfer->dataSize > 0)){
        i2c_master_read(base, xfer->data, xfer->dataSize);
    }
    return 0;
}