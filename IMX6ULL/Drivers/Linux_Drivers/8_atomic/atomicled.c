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
#define ATOMICLED_NAME		"atomicled"

#define LED_ON 				1
#define LED_OFF				0

struct atomicled_dev{
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class * atomicled_class;
	struct device * atomicled_dev;
	struct device_node * nd;
	int led_gpio;
	atomic_t lock;
};

static struct atomicled_dev atomicled = {
	.major = 0,
	.minor = 0,
	.lock = ATOMIC_INIT(1),	
};

static int atomiceld_open(struct inode *inode, struct file *file){
	struct atomicled_dev * dev = container_of(inode->i_cdev, struct atomicled_dev, cdev);
	file->private_data = dev;
	if(atomic_dec_and_test(&dev->lock) == 0){
		atomic_inc(&dev->lock);
		return -EBUSY;
	}
	return 0;
}

static int atomicled_release(struct inode *inode, struct file *file){
	struct atomicled_dev * dev = file->private_data;
	atomic_inc(&dev->lock);
	return 0;
}


static ssize_t atomicled_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos){

	u8 data[1];
	struct atomicled_dev * dev = file->private_data;
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
	.open = atomiceld_open,
	.write = atomicled_write,
	.release = atomicled_release,
};

static int gpio_init(struct atomicled_dev * atomicled){
	int ret = 0;
	atomicled->nd = of_find_node_by_path("/gpioled");
	if(atomicled->nd == NULL){
		printk("kernel: of_find_node failed.\n");
		ret = -ENODEV;
		goto failed_ret;
	}
	atomicled->led_gpio = of_get_named_gpio(atomicled->nd, "led-gpios", 0);
	if(atomicled->led_gpio < 0){
		printk("kernel: of_get_named_gpio failed.\n");
		ret = -ENODEV;
		goto failed_ret;
	}
	ret = gpio_request(atomicled->led_gpio, "atomicled");
	if(ret){
		printk("kernel: gpio_request failed.\n");
		goto failed_ret;
	}

	ret = gpio_direction_output(atomicled->led_gpio, 1);
	if(ret){
		printk("kernel: gpio_direction_output failed.\n");
		goto free_gpio;
	}
	return 0;
free_gpio:
	gpio_free(atomicled->led_gpio);
failed_ret:
	return ret;
}



static int __init atomic_init(void){
	int ret = 0;
	ret = gpio_init(&atomicled);
	if(ret){
		goto failed_ret;
	}
	atomicled.major = 0;
	if(atomicled.major){
		atomicled.devid = MKDEV(atomicled.major, 0);
		atomicled.minor = 0;
		ret = register_chrdev_region(atomicled.devid, ATOMICLED_CNT, ATOMICLED_NAME);
	}
	else{
		ret = alloc_chrdev_region(&atomicled.devid, 0, ATOMICLED_CNT, ATOMICLED_NAME);
	}
	if(ret){
		printk("kernel: register_chrdev_region failed.\n");
		goto failed_ret;
	}

	atomicled.cdev.owner = THIS_MODULE;
	cdev_init(&atomicled.cdev, &fops);
	ret = cdev_add(&atomicled.cdev, atomicled.devid, ATOMICLED_CNT);
	if(ret){
		printk("kernel: cdev_add failed.\n");
		goto unreg_chrdev;
	}
	atomicled.atomicled_class = class_create(THIS_MODULE, ATOMICLED_NAME);
	if(IS_ERR(atomicled.atomicled_class)){
		ret = PTR_ERR(atomicled.atomicled_class);
		printk("kernel: class_create failed.\n");
		goto del_cdev;
	}
	atomicled.atomicled_dev = device_create(atomicled.atomicled_class, NULL, atomicled.devid, NULL, ATOMICLED_NAME);
	if(IS_ERR(atomicled.atomicled_dev)){
		ret = PTR_ERR(atomicled.atomicled_dev);
		printk("kernel: device_create failed.\n");
		goto dest_class;
	}
	
	
	return 0;
	
//dest_device:
	//device_destroy(atomicled.atomicled_class, atomicled.devid);
dest_class:
	class_destroy(atomicled.atomicled_class);
del_cdev:
	cdev_del(&atomicled.cdev);
unreg_chrdev:
	unregister_chrdev_region(atomicled.devid, ATOMICLED_CNT);
failed_ret:
	return ret;
}

static void __exit atomic_exit(void){
	
	gpio_set_value(atomicled.led_gpio, 1);
	gpio_free(atomicled.led_gpio);
	device_destroy(atomicled.atomicled_class, atomicled.devid);
	class_destroy(atomicled.atomicled_class);
	cdev_del(&atomicled.cdev);
	unregister_chrdev_region(atomicled.devid, ATOMICLED_CNT);
}

module_init(atomic_init);
module_exit(atomic_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("weiyuyin");

