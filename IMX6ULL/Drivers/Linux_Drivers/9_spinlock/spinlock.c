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
#define ATOMICLED_NAME		"spinlockled"

#define LED_ON 				1
#define LED_OFF				0

struct spinlock_dev{
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class * spinlock_class;
	struct device * spinlock_dev;
	struct device_node * nd;
	int led_gpio;
	int dev_stats;
	spinlock_t lock;
};

static struct spinlock_dev spinlock = {
	.major = 0,
	.minor = 0,
	.dev_stats = 0,
};

static int spinlock_open(struct inode *inode, struct file *file){
	struct spinlock_dev * dev = container_of(inode->i_cdev, struct spinlock_dev, cdev);
	unsigned long flags;
	file->private_data = dev;
	spin_lock_irqsave(&dev->lock, flags);
	if(dev->dev_stats){
		spin_unlock_irqrestore(&dev->lock, flags);
		return -EBUSY;
	}
	dev->dev_stats++;
	spin_unlock_irqrestore(&dev->lock, flags);

	return 0;
}

static int spinlock_release(struct inode *inode, struct file *file){
	struct spinlock_dev * dev = file->private_data;
	unsigned long flags;
	spin_lock_irqsave(&dev->lock, flags);
	if(dev->dev_stats){
		dev->dev_stats--;
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return 0;
}


static ssize_t spinlock_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos){

	u8 data[1];
	struct spinlock_dev * dev = file->private_data;
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
	.open = spinlock_open,
	.write = spinlock_write,
	.release = spinlock_release,
};

static int gpio_init(struct spinlock_dev * spinlock){
	int ret = 0;
	spinlock->nd = of_find_node_by_path("/gpioled");
	if(spinlock->nd == NULL){
		printk("kernel: of_find_node failed.\n");
		ret = -ENODEV;
		goto failed_ret;
	}
	spinlock->led_gpio = of_get_named_gpio(spinlock->nd, "led-gpios", 0);
	if(spinlock->led_gpio < 0){
		printk("kernel: of_get_named_gpio failed.\n");
		ret = -ENODEV;
		goto failed_ret;
	}
	ret = gpio_request(spinlock->led_gpio, "spinlock");
	if(ret){
		printk("kernel: gpio_request failed.\n");
		goto failed_ret;
	}

	ret = gpio_direction_output(spinlock->led_gpio, 1);
	if(ret){
		printk("kernel: gpio_direction_output failed.\n");
		goto free_gpio;
	}
	return 0;
free_gpio:
	gpio_free(spinlock->led_gpio);
failed_ret:
	return ret;
}



static int __init spinlock_init(void){
	int ret = 0;
	spin_lock_init(&spinlock.lock);
	ret = gpio_init(&spinlock);
	if(ret){
		goto failed_ret;
	}
	spinlock.major = 0;
	if(spinlock.major){
		spinlock.devid = MKDEV(spinlock.major, 0);
		spinlock.minor = 0;
		ret = register_chrdev_region(spinlock.devid, ATOMICLED_CNT, ATOMICLED_NAME);
	}
	else{
		ret = alloc_chrdev_region(&spinlock.devid, 0, ATOMICLED_CNT, ATOMICLED_NAME);
	}
	if(ret){
		printk("kernel: register_chrdev_region failed.\n");
		goto failed_ret;
	}

	spinlock.cdev.owner = THIS_MODULE;
	cdev_init(&spinlock.cdev, &fops);
	ret = cdev_add(&spinlock.cdev, spinlock.devid, ATOMICLED_CNT);
	if(ret){
		printk("kernel: cdev_add failed.\n");
		goto unreg_chrdev;
	}
	spinlock.spinlock_class = class_create(THIS_MODULE, ATOMICLED_NAME);
	if(IS_ERR(spinlock.spinlock_class)){
		ret = PTR_ERR(spinlock.spinlock_class);
		printk("kernel: class_create failed.\n");
		goto del_cdev;
	}
	spinlock.spinlock_dev = device_create(spinlock.spinlock_class, NULL, spinlock.devid, NULL, ATOMICLED_NAME);
	if(IS_ERR(spinlock.spinlock_dev)){
		ret = PTR_ERR(spinlock.spinlock_dev);
		printk("kernel: device_create failed.\n");
		goto dest_class;
	}
	
	
	return 0;
	
//dest_device:
	//device_destroy(spinlock.spinlock_class, spinlock.devid);
dest_class:
	class_destroy(spinlock.spinlock_class);
del_cdev:
	cdev_del(&spinlock.cdev);
unreg_chrdev:
	unregister_chrdev_region(spinlock.devid, ATOMICLED_CNT);
failed_ret:
	return ret;
}

static void __exit spinlock_exit(void){
	
	gpio_set_value(spinlock.led_gpio, 1);
	gpio_free(spinlock.led_gpio);
	device_destroy(spinlock.spinlock_class, spinlock.devid);
	class_destroy(spinlock.spinlock_class);
	cdev_del(&spinlock.cdev);
	unregister_chrdev_region(spinlock.devid, ATOMICLED_CNT);
}

module_init(spinlock_init);
module_exit(spinlock_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("weiyuyin");

