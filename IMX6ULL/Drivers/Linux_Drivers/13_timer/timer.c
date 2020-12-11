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

#define ATOMICLED_CNT		1
#define ATOMICLED_NAME		"timer"

#define LED_ON 				1
#define LED_OFF				0

#define TIMER_IOCTL_BASE	'Z'

#define	OPEN_TIMER		_IO(TIMER_IOCTL_BASE, 0)
#define	CLOSE_TIMER		_IO(TIMER_IOCTL_BASE, 1)
#define	SET_PERIOR		_IOW(TIMER_IOCTL_BASE, 2, int)


struct timer_dev{
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class * timer_class;
	struct device * timer_dev;
	struct device_node * nd;
	int led_gpio;
	struct timer_list timer;
	atomic_t timer_period;
};

static struct timer_dev timer = {
	.major = 0,
	.minor = 0,
	.timer_period = ATOMIC_INIT(500),	
};

static int timer_open(struct inode *inode, struct file *file){
	struct timer_dev * dev = container_of(inode->i_cdev, struct timer_dev, cdev);
	file->private_data = dev;
	return 0;
}

static int timer_release(struct inode *inode, struct file *file){
	//struct timer_dev * dev = file->private_data;
	return 0;
}

static long timer_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
	struct timer_dev * dev = file->private_data;
	unsigned int timer_period = 0;
	int ret = 0;
	/* 获取cmd中指定的数据长度 */
	int size = _IOC_SIZE(SET_PERIOR);
	
	switch(cmd){
		case OPEN_TIMER:
			mod_timer(&dev->timer, msecs_to_jiffies(atomic_read(&dev->timer_period)) + jiffies);
			break;
		case CLOSE_TIMER:
			del_timer_sync(&dev->timer);
			break;
		case SET_PERIOR:
			if(size != sizeof(int))
				return -EINVAL;
			ret = copy_from_user(&timer_period, (unsigned int *)arg, sizeof(int));
			if(ret){
				printk("kernel: copy_frome_user failed.\n");
				return -EFAULT;
			}
			atomic_set(&dev->timer_period, timer_period);
			break;
	}
	return 0;
}


static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = timer_open,
	.release = timer_release,
	.unlocked_ioctl = timer_ioctl,
};

static int gpio_init(struct timer_dev * timer){
	int ret = 0;
	timer->nd = of_find_node_by_path("/gpioled");
	if(timer->nd == NULL){
		printk("kernel: of_find_node failed.\n");
		ret = -ENODEV;
		goto failed_ret;
	}
	timer->led_gpio = of_get_named_gpio(timer->nd, "led-gpios", 0);
	if(timer->led_gpio < 0){
		printk("kernel: of_get_named_gpio failed.\n");
		ret = -ENODEV;
		goto failed_ret;
	}
	ret = gpio_request(timer->led_gpio, "timer");
	if(ret){
		printk("kernel: gpio_request failed.\n");
		goto failed_ret;
	}

	ret = gpio_direction_output(timer->led_gpio, 1);
	if(ret){
		printk("kernel: gpio_direction_output failed.\n");
		goto free_gpio;
	}
	return 0;
free_gpio:
	gpio_free(timer->led_gpio);
failed_ret:
	return ret;
}

void timer_server_func(unsigned long arg){
	struct timer_dev * dev = (struct timer_dev *)arg;
	static int led_stat = 1;
	led_stat = !led_stat;
	gpio_set_value(dev->led_gpio, led_stat);
	mod_timer(&timer.timer, msecs_to_jiffies(atomic_read(&timer.timer_period)) + jiffies);
}

static int __init timer_init(void){
	int ret = 0;
	ret = gpio_init(&timer);
	if(ret){
		goto failed_ret;
	}
	timer.major = 0;
	if(timer.major){
		timer.devid = MKDEV(timer.major, 0);
		timer.minor = 0;
		ret = register_chrdev_region(timer.devid, ATOMICLED_CNT, ATOMICLED_NAME);
	}
	else{
		ret = alloc_chrdev_region(&timer.devid, 0, ATOMICLED_CNT, ATOMICLED_NAME);
	}
	if(ret){
		printk("kernel: register_chrdev_region failed.\n");
		goto failed_ret;
	}

	timer.cdev.owner = THIS_MODULE;
	cdev_init(&timer.cdev, &fops);
	ret = cdev_add(&timer.cdev, timer.devid, ATOMICLED_CNT);
	if(ret){
		printk("kernel: cdev_add failed.\n");
		goto unreg_chrdev;
	}
	timer.timer_class = class_create(THIS_MODULE, ATOMICLED_NAME);
	if(IS_ERR(timer.timer_class)){
		ret = PTR_ERR(timer.timer_class);
		printk("kernel: class_create failed.\n");
		goto del_cdev;
	}
	timer.timer_dev = device_create(timer.timer_class, NULL, timer.devid, NULL, ATOMICLED_NAME);
	if(IS_ERR(timer.timer_dev)){
		ret = PTR_ERR(timer.timer_dev);
		printk("kernel: device_create failed.\n");
		goto dest_class;
	}

	init_timer(&timer.timer);
	timer.timer.data = (unsigned long)&timer;
	timer.timer.function = timer_server_func;
	
	mod_timer(&timer.timer, msecs_to_jiffies(atomic_read(&timer.timer_period)) + jiffies);
	
	return 0;
	
//dest_device:
	//device_destroy(timer.timer_class, timer.devid);
dest_class:
	class_destroy(timer.timer_class);
del_cdev:
	cdev_del(&timer.cdev);
unreg_chrdev:
	unregister_chrdev_region(timer.devid, ATOMICLED_CNT);
failed_ret:
	return ret;
}

static void __exit timer_exit(void){
	del_timer_sync(&timer.timer);
	gpio_set_value(timer.led_gpio, 1);
	gpio_free(timer.led_gpio);
	device_destroy(timer.timer_class, timer.devid);
	class_destroy(timer.timer_class);
	cdev_del(&timer.cdev);
	unregister_chrdev_region(timer.devid, ATOMICLED_CNT);
}

module_init(timer_init);
module_exit(timer_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("weiyuyin");

