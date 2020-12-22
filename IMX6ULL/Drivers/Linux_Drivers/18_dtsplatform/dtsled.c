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


#define PLATFORMLED_NAME	"dtsplatformled"
#define PLATFORMLED_CNT		1

#define LED_ON				1
#define LED_OFF				0

struct platformled_dev{
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class * class;
	struct device * dev;
	struct device_node * nd;
	int gpio_index;
};

static struct platformled_dev platformled = {
	.major = 0,
	.minor = 0,
};

void led_switch(u8 status){
	if(status == LED_ON){
		gpio_set_value(platformled.gpio_index, 0);
	}
	else if(status == LED_OFF){
		gpio_set_value(platformled.gpio_index, 1);
	}
}

static int platformled_open(struct inode *inode, struct file *filp){
	filp->private_data = (void *)container_of(inode->i_cdev, struct platformled_dev, cdev);

	return 0;
}

static int platformled_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t platformled_write(struct file *file, const char __user *buffer,
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
	.open = platformled_open,
	.release = platformled_release,
	.write = platformled_write,
};


int led_probe(struct platform_device * dev){
	int ret = 0;
	printk("led_probe.\n");
	if(platformled.major){
		platformled.devid = MKDEV(platformled.major, 0);
		platformled.minor = 0;
		ret = register_chrdev_region(platformled.devid, PLATFORMLED_CNT, PLATFORMLED_NAME);
	}
	else{
		ret = alloc_chrdev_region(&platformled.devid, 0, PLATFORMLED_CNT, PLATFORMLED_NAME);
		platformled.major = MAJOR(platformled.devid);
		platformled.minor = MINOR(platformled.devid);
	}
	if(ret){
		printk("kernel register_chrdev_region failed.\r\n");
		goto fail_ret;
	}

	platformled.cdev.owner = THIS_MODULE;
	cdev_init(&platformled.cdev, &fops);

	ret = cdev_add(&platformled.cdev, platformled.devid, PLATFORMLED_CNT);
	if(ret){
		printk("kernel cdev_add failed.\r\n");
		goto fail_unreg;
	}

	platformled.class = class_create(THIS_MODULE, PLATFORMLED_NAME);
	if(IS_ERR(platformled.class)){
		printk("kernel class_create failed.\r\n");
		ret = PTR_ERR(platformled.class);
		goto fail_cdev_del;
	}

	platformled.dev = device_create(platformled.class, NULL, platformled.devid, NULL, PLATFORMLED_NAME);
	if(IS_ERR(platformled.dev)){
		printk("kernel device_create failed.\r\n");
		ret = PTR_ERR(platformled.dev);
		goto fail_class_dest;
	}

	platformled.nd = dev->dev.of_node;

	platformled.gpio_index = of_get_named_gpio(platformled.nd, "led-gpios", 0);
	if(platformled.gpio_index < 0){
		printk("kernel of_get_named_gpio failed.\r\n");
		ret = -EINVAL;
		goto fail_dev_dest;
	}

	ret = gpio_request(platformled.gpio_index, "led");

	if(ret){
		printk("kernel gpio_request failed.\r\n");
		goto fail_dev_dest;
	}

	ret = gpio_direction_output(platformled.gpio_index, 1);
	if(ret){
		printk("kernel gpio_directon_output failed.\r\n");
		goto fail_gpio_free;
	}
	
	return 0;

fail_gpio_free:
	gpio_free(platformled.gpio_index);
fail_dev_dest:
	device_destroy(platformled.class, platformled.devid);
fail_class_dest:
	class_destroy(platformled.class);
fail_cdev_del:
	cdev_del(&platformled.cdev);
fail_unreg:
	unregister_chrdev_region(platformled.devid, PLATFORMLED_CNT);
fail_ret:
	return ret;
}

int led_remove(struct platform_device * dev){
	led_switch(LED_OFF);
	gpio_free(platformled.gpio_index);
	device_destroy(platformled.class, platformled.devid);
	class_destroy(platformled.class);
	cdev_del(&platformled.cdev);
	unregister_chrdev_region(platformled.devid, PLATFORMLED_CNT);
	return 0;
}

static const struct of_device_id led_of_match[] = {
	{.compatible = "alientek,gpio-led"},
	{/*Sentinel*/}
};


static struct platform_driver led_driver = {
	.probe = led_probe,
	.remove = led_remove,
	.driver = {
		.name = "imx6ul-led",/* 必须要有，要不然内核会段错误 */
		.of_match_table = led_of_match
	},
};

static int __init platform_led_init(void){
	return platform_driver_register(&led_driver);
}

static void __exit platform_led_exit(void){
	platform_driver_unregister(&led_driver);
}

module_init(platform_led_init);
module_exit(platform_led_exit);
MODULE_AUTHOR("weiyuyin");
MODULE_LICENSE("GPL");
