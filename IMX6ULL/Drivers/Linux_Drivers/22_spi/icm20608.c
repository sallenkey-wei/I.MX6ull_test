#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/ide.h>
#include <linux/cdev.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include "icm20608.h"


#define ICM20608_CNT		1
#define ICM20608_NAME		"icm20608"

struct icm20608_dev{
	int major;
	int minor;
	dev_t devid;
	struct cdev cdev;
	struct class * cls;
	struct device * dev;
	struct spi_device * spi;
	struct device_node *parent_nd;
	int cs_gpio;
};

static int icm20608_read_regs(struct icm20608_dev *dev, u8 reg, void *buf, int len){
	int ret;
	unsigned char txdata[len];
	struct spi_message m;
	struct spi_transfer *t;
	struct spi_device * spi = dev->spi;

	gpio_set_value(dev->cs_gpio, 0);/* 片选拉低，选中icm20608 */
	t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
	txdata[0] = reg | 0x80; /* icm20608写数据时寄存器地址bit7要置1 */
	t->tx_buf = txdata;
	t->len = 1;
	spi_message_init(&m);
	spi_message_add_tail(t, &m);
	ret = spi_sync(spi, &m);

	txdata[0] = 0xff;     /* 此处随便写一个值，没有意义 */
	t->rx_buf = buf;
	t->len = len;
	spi_message_init(&m);
	spi_message_add_tail(t, &m);
	ret = spi_sync(spi, &m);

	kfree(t);
	gpio_set_value(dev->cs_gpio, 1);
	return ret;
	
}

static int icm20608_wirte_regs(struct icm20608_dev *dev, u8 reg, void *buf, int len){
	int ret;
	unsigned char txdata[len];
	struct spi_message m;
	struct spi_transfer *t;
	struct spi_device * spi = dev->spi;

	gpio_set_value(dev->cs_gpio, 0);/* 片选拉低，选中icm20608 */
	t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
	txdata[0] = reg & ~0x80; /* icm20608读数据时寄存器地址bit7要清零 */
	t->tx_buf = txdata;
	t->len = 1;
	spi_message_init(&m);
	spi_message_add_tail(t, &m);
	ret = spi_sync(spi, &m);

	t->tx_buf = buf;
	t->len = len;
	spi_message_init(&m);
	spi_message_add_tail(t, &m);
	ret = spi_sync(spi, &m);

	kfree(t);
	gpio_set_value(dev->cs_gpio, 1);
	return ret;
}

static unsigned char icm20608_read_onereg(struct icm20608_dev *dev, u8 reg){
	unsigned char data = 0;
	int ret;
	ret = icm20608_read_regs(dev, reg, &data, 1);
	if(ret){
		printk("<icm20608_read_onereg> failed.\n");
	}
	return data;
}

static void icm20608_write_onereg(struct icm20608_dev *dev, u8 reg, u8 value){
	int ret;
	ret = icm20608_wirte_regs(dev, reg, &value, 1);
	if(ret){
		printk("<icm20608_write_onereg> failed.\n");
	}
}

static int icm20608_reginit(struct icm20608_dev *dev){
	unsigned char regvalue;

	/* reset icm20608, 并关闭睡眠模式 */
	icm20608_write_onereg(dev, ICM20_PWR_MGMT_1, 0x80);
	mdelay(50);
	icm20608_write_onereg(dev, ICM20_PWR_MGMT_1, 0x01);
	mdelay(50);
	regvalue = icm20608_read_onereg(dev, ICM20_WHO_AM_I);
	printk("icm20608 id = %#X\r\n", regvalue);
	if(regvalue != ICM20608D_ID && regvalue != ICM20608G_ID)
		return -1;
	
	icm20608_write_onereg(dev, ICM20_SMPLRT_DIV, 0x00); 	/* 输出速率是内部采样率					*/
	icm20608_write_onereg(dev, ICM20_GYRO_CONFIG, 0x18); 	/* 陀螺仪±2000dps量程 				*/
	icm20608_write_onereg(dev, ICM20_ACCEL_CONFIG, 0x18); 	/* 加速度计±16G量程 					*/
	icm20608_write_onereg(dev, ICM20_CONFIG, 0x04); 		/* 陀螺仪低通滤波BW=20Hz 				*/
	icm20608_write_onereg(dev, ICM20_ACCEL_CONFIG2, 0x04); 	/* 加速度计低通滤波BW=21.2Hz 			*/
	icm20608_write_onereg(dev, ICM20_PWR_MGMT_2, 0x00); 	/* 打开加速度计和陀螺仪所有轴 				*/
	icm20608_write_onereg(dev, ICM20_LP_MODE_CFG, 0x00); 	/* 关闭低功耗 						*/
	icm20608_write_onereg(dev, ICM20_FIFO_EN, 0x00);		/* 关闭FIFO						*/
	return 0;
}

void icm20608_readdata(struct icm20608_dev * dev, struct icm20608_data * icm20608_data){
	unsigned char data[14];
	//float gyroscale;
	//unsigned short accescale;
	icm20608_read_regs(dev, ICM20_ACCEL_XOUT_H, data, sizeof(data));
	
	//gyroscale = icm20608_gyro_scaleget();
	//accescale = icm20608_accel_scaleget();

	/* 内核中最好不要进行浮点运算，所以这里简单读取了原始数据 */
	icm20608_data->accel_x_adc = (signed short)((data[0] << 8) | data[1]); 
	icm20608_data->accel_y_adc = (signed short)((data[2] << 8) | data[3]); 
	icm20608_data->accel_z_adc = (signed short)((data[4] << 8) | data[5]); 
	icm20608_data->temp_adc    = (signed short)((data[6] << 8) | data[7]); 
	icm20608_data->gyro_x_adc  = (signed short)((data[8] << 8) | data[9]); 
	icm20608_data->gyro_y_adc  = (signed short)((data[10] << 8) | data[11]);
	icm20608_data->gyro_z_adc  = (signed short)((data[12] << 8) | data[13]);
}

static int icm20608_open(struct inode *inode, struct file *filp){
	struct icm20608_dev * dev = (struct icm20608_dev *)container_of(inode->i_cdev, struct icm20608_dev, cdev);
	filp->private_data = dev;	

	return 0;
}


static int icm20608_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t icm20608_read(struct file *file, char __user * buf, size_t size, loff_t *ppos){
	struct icm20608_dev * dev = (struct icm20608_dev *)file->private_data;
	struct icm20608_data icm20608_data;
	signed int data[7];
	int ret;
	icm20608_readdata(dev, &icm20608_data);

	data[0] = icm20608_data.gyro_x_adc;
	data[1] = icm20608_data.gyro_y_adc;
	data[2] = icm20608_data.gyro_z_adc;
	data[3] = icm20608_data.accel_x_adc;
	data[4] = icm20608_data.accel_y_adc;
	data[5] = icm20608_data.accel_z_adc;
	data[6] = icm20608_data.temp_adc;

	ret = copy_to_user(buf, data, sizeof(data));
	if(ret){
		printk("copy_to_user failed.\n");
		return ret;
	}

	return sizeof(data);
}


static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = icm20608_read,
	.open = icm20608_open,
	.release = icm20608_release
};


static int	icm20608_probe(struct spi_device *spi){
	int ret = 0;
	struct icm20608_dev * dev= devm_kzalloc(&spi->dev, sizeof(struct icm20608_dev), GFP_KERNEL);
	if(!dev)
		return -ENOMEM;
	if(dev->major){
		dev->devid = MKDEV(dev->major, 0);
		ret = register_chrdev_region(dev->devid, ICM20608_CNT, ICM20608_NAME);
		dev->minor = MINOR(dev->devid);
	}
	else{
		ret = alloc_chrdev_region(&dev->devid, 0, ICM20608_CNT, ICM20608_NAME);
		dev->major = MAJOR(dev->devid);
		dev->minor = MINOR(dev->devid);
	}
	if(ret){
		printk("register_chrdev_region failed.\n");
		goto failed_and_ret;
	}

	dev->cdev.owner = THIS_MODULE;
	cdev_init(&dev->cdev, &fops);
	ret = cdev_add(&dev->cdev, dev->devid, ICM20608_CNT);
	if(ret){
		printk("cdev_add failed.\n");
		goto failed_and_unreg_chrdev;
	}

	dev->cls = class_create(THIS_MODULE, ICM20608_NAME);
	if(IS_ERR(dev->cls)){
		ret = PTR_ERR(dev->cls);
		printk("class_create failed.\n");
		goto failed_and_del_cdev;
	}

	dev->dev = device_create(dev->cls, NULL, dev->devid, NULL, ICM20608_NAME);
	if(IS_ERR(dev->dev)){
		ret = PTR_ERR(dev->dev);
		printk("device_create failed.\n");
		goto failed_and_dest_cls;
	}

	dev->parent_nd = of_get_parent(spi->dev.of_node);
	if(!dev->parent_nd){
		printk("of_get_parent failed");
		ret = -ENODEV;
		goto failed_and_dest_dev;
	}

	//printk("parent full name is %s\n", dev->parent_nd->full_name);

	dev->cs_gpio = of_get_named_gpio(dev->parent_nd, "cs-gpio", 0);
	if(!gpio_is_valid(dev->cs_gpio)){
		printk("of_get_named_gpio failed.\n");
		ret = -EINVAL;
		goto failed_and_dest_dev;
	}

	ret = gpio_request(dev->cs_gpio, "icm20608-cs");
	if(ret){
		printk("kernel gpio_request failed.\r\n");
		goto failed_and_dest_dev;
	}

	ret = gpio_direction_output(dev->cs_gpio, 1);
	if(ret < 0){
		printk("can't set gpio!\n");
		goto failed_and_free_gpio;
	}

	/* 将我们自定义的结构体变量放入dev中的driver_data */	
	dev_set_drvdata(&spi->dev, dev);

	spi->mode = SPI_MODE_0;
	spi_setup(spi);
	
	dev->spi = spi;

	ret = icm20608_reginit(dev);
	if(ret < 0){
		ret = -EINVAL;
		goto failed_and_free_gpio;
	}
	
	return 0;

failed_and_free_gpio:
	gpio_free(dev->cs_gpio);
failed_and_dest_dev:
	device_destroy(dev->cls, dev->devid);
failed_and_dest_cls:
	class_destroy(dev->cls);
failed_and_del_cdev:
	cdev_del(&dev->cdev);
failed_and_unreg_chrdev:
	unregister_chrdev_region(dev->devid, ICM20608_CNT);
failed_and_ret:
	return ret;
	
}
static int icm20608_remove(struct spi_device *spi){
	struct icm20608_dev * dev = (struct icm20608_dev *)dev_get_drvdata(&spi->dev);

	gpio_free(dev->cs_gpio);
	device_destroy(dev->cls, dev->devid);
	class_destroy(dev->cls);
	cdev_del(&dev->cdev);
	unregister_chrdev_region(dev->devid, ICM20608_CNT);

	return 0;
}


static const struct spi_device_id icm20608_id[] = {
	{
		.name = "alientek,icm20608",
		.driver_data = 0,
	},
	{	/* Sentinel */ },
};

const struct of_device_id icm20608_of_match[] = {
	{
		.compatible = "alientek,icm20608",
	},
	{ /* Sentienl */ }
};


static struct spi_driver icm20608 = {
	.id_table = icm20608_id,
	.driver = {
		.name = "alientek,icm20608",
		.of_match_table = icm20608_of_match
	},
	.probe = icm20608_probe,
	.remove = icm20608_remove,
};


static int __init icm20608_init(void){
	return spi_register_driver(&icm20608);
	
}

static void __exit icm20608_exit(void){
	spi_unregister_driver(&icm20608);
}

module_init(icm20608_init);
module_exit(icm20608_exit);
MODULE_AUTHOR("weiyuyin");
MODULE_LICENSE("GPL");
