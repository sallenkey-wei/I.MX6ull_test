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
#include <linux/i2c.h>
#include <linux/delay.h>
#include "ap3216c.h"

#define AP3216C_CNT		1
#define AP3216C_NAME	"ap3216c"

struct ap3216c_dev{
	struct cdev cdev;
	dev_t dev_id;
	int major;
	int minor;
	struct class * cls;
	struct device * device;
	struct i2c_client * client;
};

static int ap3216c_read_regs(struct ap3216c_dev * dev, u8 reg, u8 * buf, int len){
	struct i2c_client * client = dev->client;
	struct i2c_msg msgs[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf = &reg
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = len,
			.buf = buf
		}
	};
	int ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if(ret == ARRAY_SIZE(msgs)){
		return 0;
	}
	else{
		printk("i2c rd failed=%d reg=%06x len=%d\n", ret, reg, len);
		return -EREMOTEIO;
	}
}

static int ap3216c_write_regs(struct ap3216c_dev * dev, u8 reg, u8 * buf, int len){
	struct i2c_client * client = dev->client;
	u8 b[256];
	int ret;	
	struct i2c_msg msgs[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = len + 1,
			.buf = b
		},

	};
	b[0] = reg;
	memcpy(&b[1], buf, len);
	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if(ret == ARRAY_SIZE(msgs)){
		return 0;
	}
	else{
		printk("i2c rd failed=%d reg=%06x len=%d\n", ret, reg, len);
		return -EREMOTEIO;
	}
}

static int ap3216c_read_reg(struct ap3216c_dev *dev, u8 reg){
	struct i2c_client * client = dev->client;
	u8 data;
	ap3216c_read_regs(dev, reg, &data, 1);
	return data;
	return i2c_smbus_read_byte_data(client, reg);
}

static int ap3216c_write_reg(struct ap3216c_dev * dev, u8 reg, u8 data){
	struct i2c_client * client = dev->client;
	return ap3216c_write_regs(dev, reg, &data, 1);
	return i2c_smbus_write_byte_data(client, reg, data);
}

static int ap3216c_open(struct inode *inode, struct file *filp){
	int ret = 0;
	struct ap3216c_dev * dev = (struct ap3216c_dev *)container_of(inode->i_cdev, struct ap3216c_dev, cdev);
	filp->private_data = dev;	

    ret = ap3216c_write_reg(dev, AP3216C_SYSTEMCONG, 0x04); /* 复位AP3216c */
	if(ret < 0){
		printk("ap3216c_write_reg failed ret = %d.\n", ret);
		return ret;
	}
    mdelay(10);
    ap3216c_write_reg(dev, AP3216C_SYSTEMCONG, 0x03); /* 开启ALS, PS+IR */
	if(ret < 0)
		return ret;
    ret = ap3216c_read_reg(dev, AP3216C_SYSTEMCONG);
    if(ret == 0x03)
        return 0; /* 成功 */
    else
        return -EREMOTEIO; /* 失败 */
}

static int ap3216c_release(struct inode *inode, struct file *filp)
{
	return 0;
}

ssize_t ap3216c_read(struct file *file, char __user *to, size_t size, loff_t *ppos){
	
	unsigned char buf[6];
	unsigned char i;
	int ret = 0;
	struct ap3216c_dev * dev = file->private_data;
	unsigned short ir = 0, ps = 0, als = 0, data[3];

	/* 循环读取所有传感器数据 */	
	for(i = 0; i < 6; i++)
	{
		ret = ap3216c_read_reg(dev, AP3216C_IRDATALOW + i);
		if(ret < 0)
			return -EREMOTEIO;
		else
			buf[i] = (unsigned char)ret;
	}

	if(buf[0] & 0X80)	/* IR_OF位为1,则数据无效 */
		ir = 0;
	else				/* 读取IR传感器的数据			*/
		ir = ((unsigned short)buf[1] << 2) | (buf[0] & 0X03);

	als = ((unsigned short)buf[3] << 8) | buf[2];	/* 读取ALS传感器的数据			 */

	if(buf[4] & 0x40)	/* IR_OF位为1,则数据无效 		   */
		ps = 0;
	else				/* 读取PS传感器的数据	 */
		ps = ((unsigned short)(buf[5] & 0X3F) << 4) | (buf[4] & 0X0F);
	
	data[0] = ir;
	data[1] = als;
	data[2] = ps;
	ret = copy_to_user(to, data, sizeof(data));
	if(ret)
		return -EINVAL;
	else
		return sizeof(data);
}



static const struct file_operations  ops = {
	.owner = THIS_MODULE,
	.read = ap3216c_read,
	.open = ap3216c_open,
	.release = ap3216c_release
};

static int ap3216c_probe(struct i2c_client * client, const struct i2c_device_id * id){
	int ret = 0;
	struct ap3216c_dev * dev = devm_kzalloc(&client->dev, sizeof(struct ap3216c_dev), GFP_KERNEL);
	if(!dev)
		return -ENOMEM;

	dev->client = client;
	
	if(dev->major){
		dev->dev_id = MKDEV(dev->major, 0);
		ret = register_chrdev_region(dev->dev_id, AP3216C_CNT, AP3216C_NAME);
		dev->minor = MINOR(dev->dev_id);
	}
	else{
		ret = alloc_chrdev_region(&dev->dev_id, 0, AP3216C_CNT, AP3216C_NAME);
		dev->major = MAJOR(dev->dev_id);
		dev->minor = MINOR(dev->dev_id);
	}
	if(ret){
		printk("register_chrdev_region failed.\n");
		return ret;
	}

	dev->cdev.owner = THIS_MODULE;
	cdev_init(&dev->cdev, &ops);
	ret = cdev_add(&dev->cdev, dev->dev_id, AP3216C_CNT);
	if(ret){
		printk("cdev_add failed.\n");
		goto unreg_chrdev;
	}

	dev->cls = class_create(THIS_MODULE, AP3216C_NAME);
	if(IS_ERR(dev->cls)){
		printk("kernel class_create failed.\r\n");
		ret = PTR_ERR(dev->cls);
		goto failed_cdev_del;
	}
	
	dev->device = device_create(dev->cls, NULL, dev->dev_id, NULL, AP3216C_NAME);
	if(IS_ERR(dev->cls)){
		printk("kernel device_create failed.\r\n");
		ret = PTR_ERR(dev->device);
		goto failed_dest_class;
	}

	dev_set_drvdata(&client->dev, dev);
	
	return 0;
failed_dest_class:
	class_destroy(dev->cls);
failed_cdev_del:
	cdev_del(&dev->cdev);
unreg_chrdev:
	unregister_chrdev_region(dev->dev_id, AP3216C_CNT);
	return ret;
}
static int ap3216c_remove(struct i2c_client * client){
	struct ap3216c_dev * dev = dev_get_drvdata(&client->dev);
	device_destroy(dev->cls, dev->dev_id);
	class_destroy(dev->cls);
	cdev_del(&dev->cdev);
	unregister_chrdev_region(dev->dev_id, AP3216C_CNT);
	return 0;
}

static const struct of_device_id ap3216c_of_match[] = {
	{
		.compatible = "sallenkey,ap3216c",
	},
	{ /* Sentinel */ }
};

static const struct i2c_device_id ap3216c_id[] = {
	{.name = "sallenkey,ap3216c", 0},
	{ /* Sentinel */ }
};


static struct i2c_driver ap3216c = {
	.driver = {
		.name = "ap3216c",
		.of_match_table = ap3216c_of_match
	},
	.id_table = ap3216c_id,
	.probe = ap3216c_probe,
	.remove = ap3216c_remove,
};


static int __init ap3216c_init(void){
	return i2c_add_driver(&ap3216c);
}

static void __exit ap3216c_exit(void){
	i2c_del_driver(&ap3216c);
}

module_init(ap3216c_init);
module_exit(ap3216c_exit);
MODULE_AUTHOR("sallenkey");
MODULE_LICENSE("GPL");

