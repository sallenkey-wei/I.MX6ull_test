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
#define ATOMICLED_NAME		"mutexled"

#define LED_ON 				1
#define LED_OFF				0

struct mutex_dev{
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class * mutex_class;
	struct device * mutex_dev;
	struct device_node * nd;
	int led_gpio;
	int dev_stats;
	struct mutex mutex;
};

static struct mutex_dev mutex = {
	.major = 0,
	.minor = 0,
	.dev_stats = 0,
};

static int mutex_open(struct inode *inode, struct file *file){
	struct mutex_dev * dev = container_of(inode->i_cdev, struct mutex_dev, cdev);
	file->private_data = dev;
	if(mutex_lock_interruptible(&dev->mutex)){
		return -ERESTARTSYS;
	}

	return 0;
}

static int mutex__release(struct inode *inode, struct file *file){
	struct mutex_dev * dev = file->private_data;
	mutex_unlock(&dev->mutex);
	return 0;
}


static ssize_t mutex_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos){

	u8 data[1];
	struct mutex_dev * dev = file->private_data;
	if(copy_from_user(data, buf, sizeof(data)))
		return -EFAULT;
	if(data[0] == LED_ON){
		gpio_set_value(dev->led_gpio, 0);
	}
	else if(data[0] == LED_OFF){
		gpio_set_value(dev->led_gpio, 1);
	}
	return 0;
}


static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = mutex_open,
	.write = mutex_write,
	.release = mutex__release,
};

static int gpio_init(struct mutex_dev * mutex){
	int ret = 0;
	mutex->nd = of_find_node_by_path("/gpioled");
	if(mutex->nd == NULL){
		printk("kernel: of_find_node failed.\n");
		ret = -ENODEV;
		goto failed_ret;
	}
	mutex->led_gpio = of_get_named_gpio(mutex->nd, "led-gpios", 0);
	if(mutex->led_gpio < 0){
		printk("kernel: of_get_named_gpio failed.\n");
		ret = -ENODEV;
		goto failed_ret;
	}
	ret = gpio_request(mutex->led_gpio, "mutex");
	if(ret){
		printk("kernel: gpio_request failed.\n");
		goto failed_ret;
	}

	ret = gpio_direction_output(mutex->led_gpio, 1);
	if(ret){
		printk("kernel: gpio_direction_output failed.\n");
		goto free_gpio;
	}
	return 0;
free_gpio:
	gpio_free(mutex->led_gpio);
failed_ret:
	return ret;
}



static int __init mutex__init(void){
	int ret = 0;
	mutex_init(&mutex.mutex);
	ret = gpio_init(&mutex);
	if(ret){
		goto failed_ret;
	}
	mutex.major = 0;
	if(mutex.major){
		mutex.devid = MKDEV(mutex.major, 0);
		mutex.minor = 0;
		ret = register_chrdev_region(mutex.devid, ATOMICLED_CNT, ATOMICLED_NAME);
	}
	else{
		ret = alloc_chrdev_region(&mutex.devid, 0, ATOMICLED_CNT, ATOMICLED_NAME);
	}
	if(ret){
		printk("kernel: register_chrdev_region failed.\n");
		goto failed_ret;
	}

	mutex.cdev.owner = THIS_MODULE;
	cdev_init(&mutex.cdev, &fops);
	ret = cdev_add(&mutex.cdev, mutex.devid, ATOMICLED_CNT);
	if(ret){
		printk("kernel: cdev_add failed.\n");
		goto unreg_chrdev;
	}
	mutex.mutex_class = class_create(THIS_MODULE, ATOMICLED_NAME);
	if(IS_ERR(mutex.mutex_class)){
		ret = PTR_ERR(mutex.mutex_class);
		printk("kernel: class_create failed.\n");
		goto del_cdev;
	}
	mutex.mutex_dev = device_create(mutex.mutex_class, NULL, mutex.devid, NULL, ATOMICLED_NAME);
	if(IS_ERR(mutex.mutex_dev)){
		ret = PTR_ERR(mutex.mutex_dev);
		printk("kernel: device_create failed.\n");
		goto dest_class;
	}
	
	
	return 0;
	
//dest_device:
	//device_destroy(mutex.mutex_class, mutex.devid);
dest_class:
	class_destroy(mutex.mutex_class);
del_cdev:
	cdev_del(&mutex.cdev);
unreg_chrdev:
	unregister_chrdev_region(mutex.devid, ATOMICLED_CNT);
failed_ret:
	return ret;
}

static void __exit mutex_exit(void){
	
	gpio_set_value(mutex.led_gpio, 1);
	gpio_free(mutex.led_gpio);
	device_destroy(mutex.mutex_class, mutex.devid);
	class_destroy(mutex.mutex_class);
	cdev_del(&mutex.cdev);
	unregister_chrdev_region(mutex.devid, ATOMICLED_CNT);
}

module_init(mutex__init);
module_exit(mutex_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("weiyuyin");

