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
#include <linux/miscdevice.h>

#define MISC_BEEP_NAME			"miscbeep"
#define BEEP_ON					1
#define BEEP_OFF				0

struct misc_beep_dev {
	int gpio;
	struct device_node * nd;
	struct miscdevice misc_beep;
};

static int misc_beep_open(struct inode *inode, struct file *filp);
static int misc_beep_release(struct inode *inode, struct file *filp);
static ssize_t misc_beep_write(struct file *file, const char __user *buffer,
			     size_t count, loff_t *ppos);


static const struct file_operations fops ={
	.open = misc_beep_open,
	.release = misc_beep_release,
	.write = misc_beep_write,
};


static struct misc_beep_dev misc_beep_dev = {	 
	.nd = NULL,
	.misc_beep = {
		.name = MISC_BEEP_NAME,
		.minor = MISC_DYNAMIC_MINOR,
		.fops = &fops,
	},
};


static void beep_switch(u8 status){
	if(status == BEEP_ON){
		gpio_set_value(misc_beep_dev.gpio, 0);
	}
	else if(status == BEEP_OFF){
		gpio_set_value(misc_beep_dev.gpio, 1);
	}
}

static int misc_beep_open(struct inode *inode, struct file *filp){
	filp->private_data = (void *)&misc_beep_dev;
	return 0;
}

static int misc_beep_release(struct inode *inode, struct file *filp){
	return 0;
}

static ssize_t misc_beep_write(struct file *file, const char __user *buffer,
			     size_t count, loff_t *ppos){
	int ret = 0;
	u8 data[1];
	ret = copy_from_user(data, buffer, count);
	if(ret)
		return -EFAULT;
	beep_switch(data[0]);
	return 0;
}

static int misc_beep_probe(struct platform_device * dev){
	int ret = 0;
	/* 1.获取设备节点 */
	misc_beep_dev.nd = dev->dev.of_node;

	/* 2.初始化io */
	misc_beep_dev.gpio = of_get_named_gpio(misc_beep_dev.nd, "beep-gpios", 0);
	if(misc_beep_dev.gpio < 0){
		printk("of_get_named_gpio failed.\n");
		return -ENODEV;
	}
	
	ret = gpio_request(misc_beep_dev.gpio, "beep-gpio");
	if(ret){
		printk("gpio_request failed.\n");
		return ret;
	}
	
	ret = gpio_direction_output(misc_beep_dev.gpio, 1);
	if(ret){
		printk("gpio_direction_output failed.\n");
		goto fail_free_gpio;
	}

	/* 3.注册misc */
	misc_register(&misc_beep_dev.misc_beep);
	
fail_free_gpio:	
	gpio_free(misc_beep_dev.gpio);
	return ret;
}

static int misc_beep_remove(struct platform_device * dev){
	gpio_set_value(misc_beep_dev.gpio, 1);
	misc_deregister(&misc_beep_dev.misc_beep);
	gpio_free(misc_beep_dev.gpio);
	return 0;
}


static const struct of_device_id misc_beep_match_table[] = {
	{.compatible = "alientek,alk-beep"},
	{/* sentinel */},
};

static struct platform_driver misc_beep_driver = {
	.driver = {
		.name = "misc_beep",
		.of_match_table = misc_beep_match_table,
	},
	.probe = misc_beep_probe,
	.remove = misc_beep_remove,
};

static int __init misc_beep_init(void){
	platform_driver_register(&misc_beep_driver);
	return 0;
}

static void __exit misc_beep_exit(void){
	platform_driver_unregister(&misc_beep_driver);
}

module_init(misc_beep_init);
module_exit(misc_beep_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("weiyuyin");
