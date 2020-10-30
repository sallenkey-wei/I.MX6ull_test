#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/ide.h>
#include <linux/cdev.h>


#define CHRDEVLED_MAJOR		200
#define NEWCHRLED_NAME		"newchrled"
#define NEWCHRLED_CNT		1

#define	CCM_CCGR1_BASE					0x020c406c
#define SW_MUX_GPIO1_IO03_BASE			0x02030068
#define SW_PAD_GPIO1_IO03_BASE			0x020e02f4
#define GPIO1_GDIR_BASE					0x0209c004
#define GPIO1_DR_BASE					0x0209c000

static void __iomem * IMX6U_CCM_CCGR1;
static void __iomem * SW_MUX_GPIO1_IO03;
static void __iomem * SW_PAD_GPIO1_IO03;
static void __iomem * GPIO1_GDIR;
static void __iomem * GPIO1_DR;

struct newchrled{
	struct cdev newchrled_cdev;
	struct class * newchrled_class;
	struct device * newchrled_device;
	dev_t devid;
	u32 major;
	u32 minor;
};

static struct newchrled newchrled = {
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
	file->private_data = (void *)container_of(inode->i_cdev, struct newchrled, newchrled_cdev);
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


static int __init newchrled_init(void){
	int result = 0;
	u32 value = 0;
	if(newchrled.major){
		newchrled.devid = MKDEV(newchrled.major, newchrled.minor);
		result = register_chrdev_region(newchrled.devid, NEWCHRLED_CNT, NEWCHRLED_NAME);
	}
	else{
		result = alloc_chrdev_region(&newchrled.devid, 0, NEWCHRLED_CNT, NEWCHRLED_NAME);
		newchrled.major = MAJOR(newchrled.devid);
		newchrled.minor = MINOR(newchrled.devid);
	}
	if(result){
		printk("kernel:register_chrdev failed.\n");
		return -EFAULT;
	}
	printk("newchrled.major = %d newchrled.minor = %d\n", newchrled.major, newchrled.minor);

	newchrled.newchrled_cdev.owner = THIS_MODULE;
	cdev_init(&newchrled.newchrled_cdev, &fops);
	
	result = cdev_add(&newchrled.newchrled_cdev, newchrled.devid, NEWCHRLED_CNT);
	if(result){
		printk("kernel:cdev_add failed.\n");
		goto fail_chrdev_unreg;
	}

	newchrled.newchrled_class = class_create(THIS_MODULE, NEWCHRLED_NAME);
	if (IS_ERR(newchrled.newchrled_class)){
		printk("kernel:class_create failed.\n");
		goto fail_cdev_del;
	}

	newchrled.newchrled_device = device_create(newchrled.newchrled_class, NULL, newchrled.devid, NULL, NEWCHRLED_NAME);
	if(IS_ERR(newchrled.newchrled_device)){
		printk("kernel:device_create failed.\n");
		goto fail_class_destr;
	}
	IMX6U_CCM_CCGR1 = ioremap(CCM_CCGR1_BASE, 4);
	SW_MUX_GPIO1_IO03 = ioremap(SW_MUX_GPIO1_IO03_BASE, 4);
	SW_PAD_GPIO1_IO03 = ioremap(SW_PAD_GPIO1_IO03_BASE, 4);
	GPIO1_GDIR = ioremap(GPIO1_GDIR_BASE, 4);
	GPIO1_DR = ioremap(GPIO1_DR_BASE, 4);
	
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
	
	
fail_class_destr:
	class_destroy(newchrled.newchrled_class);
fail_cdev_del:
	cdev_del(&newchrled.newchrled_cdev);
fail_chrdev_unreg:
	unregister_chrdev_region(newchrled.devid, NEWCHRLED_CNT);
	return result;
}

static void __exit newchrled_exit(void){
	/*关闭led灯*/
	led_switch(0);

	iounmap(IMX6U_CCM_CCGR1);
	iounmap(SW_MUX_GPIO1_IO03);
	iounmap(SW_PAD_GPIO1_IO03);
	iounmap(GPIO1_GDIR);
	iounmap(GPIO1_DR);

	device_destroy(newchrled.newchrled_class, newchrled.devid);
	class_destroy(newchrled.newchrled_class);
	cdev_del(&newchrled.newchrled_cdev);
	unregister_chrdev_region(newchrled.devid, NEWCHRLED_CNT);
}

module_init(newchrled_init);
module_exit(newchrled_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("weiyuyin");


