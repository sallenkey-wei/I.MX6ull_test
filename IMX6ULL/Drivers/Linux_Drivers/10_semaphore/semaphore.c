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
#define ATOMICLED_NAME		"semled"

#define LED_ON 				1
#define LED_OFF				0

struct sem_dev{
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class * sem_class;
	struct device * sem_dev;
	struct device_node * nd;
	int led_gpio;
	int dev_stats;
	struct semaphore sem;
};

static struct sem_dev sem = {
	.major = 0,
	.minor = 0,
	.dev_stats = 0,
};

static int sem_open(struct inode *inode, struct file *file){
	struct sem_dev * dev = container_of(inode->i_cdev, struct sem_dev, cdev);
	file->private_data = dev;
	if(down_interruptible(&dev->sem)){
		return -ERESTARTSYS;
	}

	return 0;
}

static int sem_release(struct inode *inode, struct file *file){
	struct sem_dev * dev = file->private_data;
	up(&dev->sem);
	return 0;
}


static ssize_t sem_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos){

	u8 data[1];
	struct sem_dev * dev = file->private_data;
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
	.open = sem_open,
	.write = sem_write,
	.release = sem_release,
};

static int gpio_init(struct sem_dev * sem){
	int ret = 0;
	sem->nd = of_find_node_by_path("/gpioled");
	if(sem->nd == NULL){
		printk("kernel: of_find_node failed.\n");
		ret = -ENODEV;
		goto failed_ret;
	}
	sem->led_gpio = of_get_named_gpio(sem->nd, "led-gpios", 0);
	if(sem->led_gpio < 0){
		printk("kernel: of_get_named_gpio failed.\n");
		ret = -ENODEV;
		goto failed_ret;
	}
	ret = gpio_request(sem->led_gpio, "sem");
	if(ret){
		printk("kernel: gpio_request failed.\n");
		goto failed_ret;
	}

	ret = gpio_direction_output(sem->led_gpio, 1);
	if(ret){
		printk("kernel: gpio_direction_output failed.\n");
		goto free_gpio;
	}
	return 0;
free_gpio:
	gpio_free(sem->led_gpio);
failed_ret:
	return ret;
}



static int __init sem_init(void){
	int ret = 0;
	sema_init(&sem.sem, 1);
	ret = gpio_init(&sem);
	if(ret){
		goto failed_ret;
	}
	sem.major = 0;
	if(sem.major){
		sem.devid = MKDEV(sem.major, 0);
		sem.minor = 0;
		ret = register_chrdev_region(sem.devid, ATOMICLED_CNT, ATOMICLED_NAME);
	}
	else{
		ret = alloc_chrdev_region(&sem.devid, 0, ATOMICLED_CNT, ATOMICLED_NAME);
	}
	if(ret){
		printk("kernel: register_chrdev_region failed.\n");
		goto failed_ret;
	}

	sem.cdev.owner = THIS_MODULE;
	cdev_init(&sem.cdev, &fops);
	ret = cdev_add(&sem.cdev, sem.devid, ATOMICLED_CNT);
	if(ret){
		printk("kernel: cdev_add failed.\n");
		goto unreg_chrdev;
	}
	sem.sem_class = class_create(THIS_MODULE, ATOMICLED_NAME);
	if(IS_ERR(sem.sem_class)){
		ret = PTR_ERR(sem.sem_class);
		printk("kernel: class_create failed.\n");
		goto del_cdev;
	}
	sem.sem_dev = device_create(sem.sem_class, NULL, sem.devid, NULL, ATOMICLED_NAME);
	if(IS_ERR(sem.sem_dev)){
		ret = PTR_ERR(sem.sem_dev);
		printk("kernel: device_create failed.\n");
		goto dest_class;
	}
	
	
	return 0;
	
//dest_device:
	//device_destroy(sem.sem_class, sem.devid);
dest_class:
	class_destroy(sem.sem_class);
del_cdev:
	cdev_del(&sem.cdev);
unreg_chrdev:
	unregister_chrdev_region(sem.devid, ATOMICLED_CNT);
failed_ret:
	return ret;
}

static void __exit sem_exit(void){
	
	gpio_set_value(sem.led_gpio, 1);
	gpio_free(sem.led_gpio);
	device_destroy(sem.sem_class, sem.devid);
	class_destroy(sem.sem_class);
	cdev_del(&sem.cdev);
	unregister_chrdev_region(sem.devid, ATOMICLED_CNT);
}

module_init(sem_init);
module_exit(sem_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("weiyuyin");

