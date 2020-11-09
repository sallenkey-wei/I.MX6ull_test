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

#define GPIOLED_CNT			1
#define GPIOLED_NAME		"gpioled"

#define LED_ON				1
#define LED_OFF				0

struct gpioled_dev{
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class * class;
	struct device * dev;
	struct device_node * nd;
	int gpio_index;
};

static struct gpioled_dev gpioled;

void led_switch(u8 status){
	if(status == LED_ON){
		gpio_set_value(gpioled.gpio_index, 0);
	}
	else if(status == LED_OFF){
		gpio_set_value(gpioled.gpio_index, 1);
	}
}

static int gpioled_open(struct inode *inode, struct file *filp){
	filp->private_data = (void *)container_of(inode->i_cdev, struct gpioled_dev, cdev);

	return 0;
}

static int gpioled_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t gpioled_write(struct file *file, const char __user *buffer,
			     size_t count, loff_t *ppos){
	int ret = 0;
	u8 data[1];
	ret = copy_from_user(data, buffer, count);
	if(ret)
		return -EFAULT;
	led_switch(data[0]);
	return 0;
}


static const struct file_operations fops ={
	.open = gpioled_open,
	.release = gpioled_release,
	.write = gpioled_write,
};





int __init gpioled_init(void){
	int ret = 0;
	gpioled.major = 0;
	if(gpioled.major){
		gpioled.devid = MKDEV(gpioled.major, 0);
		gpioled.minor = 0;
		ret = register_chrdev_region(gpioled.devid, GPIOLED_CNT, GPIOLED_NAME);
	}
	else{
		ret = alloc_chrdev_region(&gpioled.devid, 0, GPIOLED_CNT, GPIOLED_NAME);
		gpioled.major = MAJOR(gpioled.devid);
		gpioled.minor = MINOR(gpioled.devid);
	}
	if(ret){
		printk("kernel register_chrdev_region failed.\r\n");
		goto fail_ret;
	}

	gpioled.cdev.owner = THIS_MODULE;
	cdev_init(&gpioled.cdev, &fops);

	ret = cdev_add(&gpioled.cdev, gpioled.devid, GPIOLED_CNT);
	if(ret){
		printk("kernel cdev_add failed.\r\n");
		goto fail_unreg;
	}

	gpioled.class = class_create(THIS_MODULE, GPIOLED_NAME);
	if(IS_ERR(gpioled.class)){
		printk("kernel class_create failed.\r\n");
		ret = PTR_ERR(gpioled.class);
		goto fail_cdev_del;
	}

	gpioled.dev = device_create(gpioled.class, NULL, gpioled.devid, NULL, GPIOLED_NAME);
	if(IS_ERR(gpioled.dev)){
		printk("kernel device_create failed.\r\n");
		ret = PTR_ERR(gpioled.dev);
		goto fail_class_dest;
	}

	gpioled.nd = of_find_node_by_path("/gpioled");
	if(gpioled.nd == NULL){
		printk("kernel of_find_node_by_path failed.\r\n");
		ret = -ENODEV;
		goto fail_dev_dest;
	}

	gpioled.gpio_index = of_get_named_gpio(gpioled.nd, "led-gpios", 0);
	if(gpioled.gpio_index < 0){
		printk("kernel of_get_named_gpio failed.\r\n");
		ret = -EINVAL;
		goto fail_dev_dest;
	}

	ret = gpio_request(gpioled.gpio_index, "led");

	if(ret){
		printk("kernel gpio_request failed.\r\n");
		goto fail_dev_dest;
	}

	ret = gpio_direction_output(gpioled.gpio_index, 1);
	printk("kernel gpioled.gpio_index = %d\r\n", gpioled.gpio_index);
	if(ret){
		printk("kernel gpio_directon_output failed.\r\n");
		goto fail_gpio_free;
	}
	
	return 0;

fail_gpio_free:
	gpio_free(gpioled.gpio_index);
fail_dev_dest:
	device_destroy(gpioled.class, gpioled.devid);
fail_class_dest:
	class_destroy(gpioled.class);
fail_cdev_del:
	cdev_del(&gpioled.cdev);
fail_unreg:
	unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);
fail_ret:
	return ret;
}

void __exit gpioled_exit(void){
	led_switch(LED_OFF);
	gpio_free(gpioled.gpio_index);
	device_destroy(gpioled.class, gpioled.devid);
	class_destroy(gpioled.class);
	cdev_del(&gpioled.cdev);
	unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);
}

module_init(gpioled_init);
module_exit(gpioled_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("weiyuyin");
