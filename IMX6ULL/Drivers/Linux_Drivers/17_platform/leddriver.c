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


#define PLATFORMLED_NAME		"platformled"
#define PLATFORMLED_CNT		1

static void __iomem * IMX6U_CCM_CCGR1;
static void __iomem * SW_MUX_GPIO1_IO03;
static void __iomem * SW_PAD_GPIO1_IO03;
static void __iomem * GPIO1_GDIR;
static void __iomem * GPIO1_DR;

struct platformled{
	struct cdev newchrled_cdev;
	struct class * newchrled_class;
	struct device * newchrled_device;
	dev_t devid;
	u32 major;
	u32 minor;
};

static struct platformled platformled = {
	.major = 0,
	.minor = 0,
	.devid = 0,
};

static void led_switch(u8 status){
	u32 value = 0;
	switch(status){
		case 0:
			value = readl(GPIO1_DR);
			value |= (1 << 3);
			writel(value, GPIO1_DR);
			break;
		case 1:
			value = readl(GPIO1_DR);
			value &= ~(1 << 3);
			writel(value, GPIO1_DR);
			break;
	}
}

static int newchrled_open(struct inode * inode, struct file * file){
	file->private_data = (void *)container_of(inode->i_cdev, struct platformled, newchrled_cdev);
	return 0;
}

static ssize_t newchrled_write(struct file *file, const char __user *buf,
			 size_t count, loff_t *ppos){
	int ret = 0;
	u8 kernel_buf[1];
	ret = copy_from_user(kernel_buf, buf, 1);
	if(ret < 0){
		printk("kernel: len_write");
		return -EFAULT;
	}
	led_switch(kernel_buf[0]);
	return 0;
}

static int newchrled_release(struct inode * inode, struct file * file){
	return 0;
}


const struct file_operations fops = {
	.write = newchrled_write,
	.open = newchrled_open,
	.release = newchrled_release,
	.owner = THIS_MODULE,
};



static int led_probe(struct platform_device * dev){
	int result = 0;
	u32 value = 0;
	int i = 0;
	struct resource * ledsource[5];

	printk("led_probe\n");
	if(platformled.major){
		platformled.devid = MKDEV(platformled.major, platformled.minor);
		result = register_chrdev_region(platformled.devid, PLATFORMLED_CNT, PLATFORMLED_NAME);
	}
	else{
		result = alloc_chrdev_region(&platformled.devid, 0, PLATFORMLED_CNT, PLATFORMLED_NAME);
		platformled.major = MAJOR(platformled.devid);
		platformled.minor = MINOR(platformled.devid);
	}
	if(result){
		printk("kernel:register_chrdev failed.\n");
		return -EFAULT;
	}
	printk("platformled.major = %d platformled.minor = %d\n", platformled.major, platformled.minor);

	platformled.newchrled_cdev.owner = THIS_MODULE;
	cdev_init(&platformled.newchrled_cdev, &fops);
	
	result = cdev_add(&platformled.newchrled_cdev, platformled.devid, PLATFORMLED_CNT);
	if(result){
		printk("kernel:cdev_add failed.\n");
		goto fail_chrdev_unreg;
	}

	platformled.newchrled_class = class_create(THIS_MODULE, PLATFORMLED_NAME);
	if (IS_ERR(platformled.newchrled_class)){
		printk("kernel:class_create failed.\n");
		goto fail_cdev_del;
	}

	platformled.newchrled_device = device_create(platformled.newchrled_class, NULL, platformled.devid, NULL, PLATFORMLED_NAME);
	if(IS_ERR(platformled.newchrled_device)){
		printk("kernel:device_create failed.\n");
		goto fail_class_destr;
	}

	
	for(i = 0; i < 5; i++){
		ledsource[i] = platform_get_resource(dev, IORESOURCE_MEM, i);
		if(!ledsource[i]){
			dev_err(&dev->dev, "platform_get_resource failed.\n");
			goto fail_dev_destr;
		}
	}
	
	IMX6U_CCM_CCGR1 = ioremap(ledsource[0]->start, resource_size(ledsource[0]));
	SW_MUX_GPIO1_IO03 = ioremap(ledsource[1]->start, resource_size(ledsource[1]));
	SW_PAD_GPIO1_IO03 = ioremap(ledsource[2]->start, resource_size(ledsource[2]));
	GPIO1_GDIR = ioremap(ledsource[3]->start, resource_size(ledsource[3]));
	GPIO1_DR = ioremap(ledsource[4]->start, resource_size(ledsource[4]));
	
    /* 使能GPIO1时钟 */
    value = readl(IMX6U_CCM_CCGR1);
    value &= ~(3 << 26);
    value |= (3 << 26);
    writel(value, IMX6U_CCM_CCGR1);

    /* 设置io复用和电气属性 */
    writel(0x05, SW_MUX_GPIO1_IO03);
    writel(0x10B0, SW_PAD_GPIO1_IO03);
    value = readl(GPIO1_GDIR);
    value |= (1 << 3);
    writel(value, GPIO1_GDIR);

    /* 默认关闭led */
    led_switch(0);
	
	return 0;

fail_dev_destr:
	device_destroy(platformled.newchrled_class, platformled.devid);	
fail_class_destr:
	class_destroy(platformled.newchrled_class);
fail_cdev_del:
	cdev_del(&platformled.newchrled_cdev);
fail_chrdev_unreg:
	unregister_chrdev_region(platformled.devid, PLATFORMLED_CNT);
	return result;
}
static int led_remove(struct platform_device * dev){
	printk("led_remove\n");
	led_switch(0);

	iounmap(IMX6U_CCM_CCGR1);
	iounmap(SW_MUX_GPIO1_IO03);
	iounmap(SW_PAD_GPIO1_IO03);
	iounmap(GPIO1_GDIR);
	iounmap(GPIO1_DR);

	device_destroy(platformled.newchrled_class, platformled.devid);
	class_destroy(platformled.newchrled_class);
	cdev_del(&platformled.newchrled_cdev);
	unregister_chrdev_region(platformled.devid, PLATFORMLED_CNT);
	return 0;
}


static struct platform_driver led_driver = {
	.probe = led_probe,
	.remove = led_remove,
	.driver = {
		.name = "imx6ul-led",
	},
};

static int __init leddriver_init(void){
	int ret = 0;
	ret = platform_driver_register(&led_driver);
	
	return ret;
}

static void __exit leddriver_exit(void){
	platform_driver_unregister(&led_driver);
}

module_init(leddriver_init);
module_exit(leddriver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("weiyuyin");
