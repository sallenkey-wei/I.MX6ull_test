#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/ide.h>
#include <linux/cdev.h>
#include <linux/of.h>
#include <linux/of_address.h>

#define 		DTSLED_CHRDEV_CNT		1
#define 		DTSLED_CHRDEV_NAME		"dtsled"

struct dtsled{
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class * dtsled_class;
	struct device* dtsled_device;
	struct device_node * dtsled_device_node; 
};

static struct dtsled dtsled;

static void __iomem * IMX6U_CCM_CCGR1;
static void __iomem * SW_MUX_GPIO1_IO03;
static void __iomem * SW_PAD_GPIO1_IO03;
static void __iomem * GPIO1_GDIR;
static void __iomem * GPIO1_DR;

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


static int dtsled_open(struct inode *inode, struct file *file){
	file->private_data = container_of(inode->i_cdev, struct dtsled, cdev);
	return 0;
}

static int dtsled_release(struct inode *inode, struct file *file){
	return 0;
}

static ssize_t dtsled_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos){
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


static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = dtsled_open,
	.release = dtsled_release,
	.write = dtsled_write,
	
};

static int __init dtsled_init(void){
	int ret = 0;
	int value = 0;
	int regdata[10] = {0};
	const char * string;
	struct property * status_property;
	
	dtsled.major = 0;
	
	if(dtsled.major){
		dtsled.devid = MKDEV(dtsled.major, 0);
		ret = register_chrdev_region(dtsled.devid, DTSLED_CHRDEV_CNT, DTSLED_CHRDEV_NAME);
	}
	else{
		ret = alloc_chrdev_region(&dtsled.devid, 0, DTSLED_CHRDEV_CNT, DTSLED_CHRDEV_NAME);
		dtsled.major = MAJOR(dtsled.devid);
		dtsled.minor = MINOR(dtsled.devid);
	}
	if(ret){
		printk("kernel register/alloc_chrdev_region failed.\r\n");
		goto fail_ret;
	}

	dtsled.cdev.owner = THIS_MODULE;
	cdev_init(&dtsled.cdev, &fops);
	ret = cdev_add(&dtsled.cdev, dtsled.devid, DTSLED_CHRDEV_CNT);
	if(ret){
		printk("kernel cdev_add failed.\r\n");
		goto fail_unreg_chrdev;
	}

	dtsled.dtsled_class = class_create(THIS_MODULE, DTSLED_CHRDEV_NAME);
	if(IS_ERR(dtsled.dtsled_class)){
		printk("kernel class_create failed.\r\n");
		goto fail_cdev_del;
	}
	
	dtsled.dtsled_device = device_create(dtsled.dtsled_class, NULL, dtsled.devid, NULL, DTSLED_CHRDEV_NAME);
	if(IS_ERR(dtsled.dtsled_device)){
		printk("kernel device_create failed.\r\n");
		goto fail_class_dest;
	}

	dtsled.dtsled_device_node = of_find_node_by_path("/alphaled");
	if(dtsled.dtsled_device_node == NULL){
		printk("kernel find_node failed.\r\n");
		ret = -ENODEV;
		goto fail_device_dest;
	}

	ret = of_property_read_u32_array(dtsled.dtsled_device_node, "reg", regdata, 10);
	if(ret) {
		printk("kernel reg property read failed!\r\n");
		goto fail_device_dest;
	} else {
		u8 i = 0;
		printk("reg data:\r\n");
		for(i = 0; i < 10; i++)
			printk("%#X ", regdata[i]);
		printk("\r\n");
	}

	ret = of_property_read_string(dtsled.dtsled_device_node, "compatible", &string);
	if(ret){
		printk("kernel read compatible failed.\r\n");
		goto fail_device_dest;
	}
	else{
		printk("kernel read compatible is %s\r\n", string);
	}

	ret = of_property_read_string(dtsled.dtsled_device_node, "status", &string);
	if(ret){
		printk("kernel read status failed.\r\n");
		goto fail_device_dest;
	}
	else{
		printk("kernel read status is %s.\r\n", string);
	}

	status_property = of_find_property(dtsled.dtsled_device_node, "status", NULL);
	if(status_property == NULL){
		printk("kernel read status failed.\r\n");
		ret = -EINVAL;
		goto fail_device_dest;
	}
	else{
		printk("kernel read status is %s\r\n", (char *)status_property->value);
	}
	

#if 1
	IMX6U_CCM_CCGR1 = of_iomap(dtsled.dtsled_device_node, 0);
	SW_MUX_GPIO1_IO03 = of_iomap(dtsled.dtsled_device_node, 1);
	SW_PAD_GPIO1_IO03 = of_iomap(dtsled.dtsled_device_node, 2);
	GPIO1_DR = of_iomap(dtsled.dtsled_device_node, 3);
	GPIO1_GDIR = of_iomap(dtsled.dtsled_device_node, 4);
#else
	IMX6U_CCM_CCGR1 = ioremap(regdata[0], regdata[1]);
	SW_MUX_GPIO1_IO03 = ioremap(regdata[2], regdata[3]);
	SW_PAD_GPIO1_IO03 = ioremap(regdata[4], regdata[5]);
	GPIO1_DR = ioremap(regdata[6], regdata[7]);
	GPIO1_GDIR = ioremap(regdata[8], regdata[9]);
#endif

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

fail_device_dest:
	device_destroy(dtsled.dtsled_class, dtsled.devid);
fail_class_dest:
	class_destroy(dtsled.dtsled_class);
fail_cdev_del:
	cdev_del(&dtsled.cdev);
fail_unreg_chrdev:
	unregister_chrdev_region(dtsled.devid, DTSLED_CHRDEV_CNT);
fail_ret:
	return ret;
}

static void __exit dtsled_exit(void){
	led_switch(0);

	iounmap(IMX6U_CCM_CCGR1);
	iounmap(SW_MUX_GPIO1_IO03);
	iounmap(SW_PAD_GPIO1_IO03);
	iounmap(GPIO1_GDIR);
	iounmap(GPIO1_DR);
	
	device_destroy(dtsled.dtsled_class, dtsled.devid);
	class_destroy(dtsled.dtsled_class);
	cdev_del(&dtsled.cdev);
	unregister_chrdev_region(dtsled.devid, DTSLED_CHRDEV_CNT);

}


module_init(dtsled_init);
module_exit(dtsled_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("weiyuyin");
