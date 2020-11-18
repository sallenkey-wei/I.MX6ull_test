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

#define KEY_CNT		1
#define KEY_NAME		"key"

#define KEY_VALUE 			0xF0
#define INVAKEY				0x00

struct key_dev{
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class * key_class;
	struct device * key_dev;
	struct device_node * nd;
	int key_gpio;
	atomic_t keyvalue;
};

static struct key_dev key = {
	.major = 0,
	.minor = 0,
	.keyvalue = ATOMIC_INIT(INVAKEY),
};

static int key_open(struct inode *inode, struct file *file){
	struct key_dev * dev = container_of(inode->i_cdev, struct key_dev, cdev);
	file->private_data = dev;
	

	return 0;
}

static int key__release(struct inode *inode, struct file *file){
	//struct key_dev * dev = file->private_data;

	return 0;
}

ssize_t key_read(struct file *file, char __user *buf, size_t size, loff_t *ppos){
	unsigned char value;
	int ret = 0;
	struct key_dev * dev = file->private_data;
	if(gpio_get_value(dev->key_gpio) == 0){
		while(gpio_get_value(dev->key_gpio) == 0)
			;
		atomic_set(&dev->keyvalue, KEY_VALUE);
	}
	else{
		atomic_set(&dev->keyvalue, INVAKEY);
	}

	value = atomic_read(&dev->keyvalue);
	ret = copy_to_user(buf, &value, sizeof(value));
	if(ret)
		return -EINVAL;
	return ret;
}




static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = key_open,
	.read = key_read,
	.release = key__release,
};


static int gpio_init(struct key_dev * key){
	int ret = 0;
	key->nd = of_find_node_by_path("/key");
	if(key->nd == NULL){
		printk("kernel: of_find_node failed.\n");
		ret = -ENODEV;
		goto failed_ret;
	}
	key->key_gpio = of_get_named_gpio(key->nd, "key-gpios", 0);
	if(key->key_gpio < 0){
		printk("kernel: of_get_named_gpio failed.\n");
		ret = -ENODEV;
		goto failed_ret;
	}
	ret = gpio_request(key->key_gpio, "key");
	if(ret){
		printk("kernel: gpio_request failed.\n");
		goto failed_ret;
	}

	ret = gpio_direction_input(key->key_gpio);
	if(ret){
		printk("kernel: gpio_direction_input failed.\n");
		goto free_gpio;
	}
	return 0;
free_gpio:
	gpio_free(key->key_gpio);
failed_ret:
	return ret;
}



static int __init key__init(void){
	int ret = 0;

	ret = gpio_init(&key);
	if(ret){
		goto failed_ret;
	}
	key.major = 0;
	if(key.major){
		key.devid = MKDEV(key.major, 0);
		key.minor = 0;
		ret = register_chrdev_region(key.devid, KEY_CNT, KEY_NAME);
	}
	else{
		ret = alloc_chrdev_region(&key.devid, 0, KEY_CNT, KEY_NAME);
	}
	if(ret){
		printk("kernel: register_chrdev_region failed.\n");
		goto failed_ret;
	}

	key.cdev.owner = THIS_MODULE;
	cdev_init(&key.cdev, &fops);
	ret = cdev_add(&key.cdev, key.devid, KEY_CNT);
	if(ret){
		printk("kernel: cdev_add failed.\n");
		goto unreg_chrdev;
	}
	key.key_class = class_create(THIS_MODULE, KEY_NAME);
	if(IS_ERR(key.key_class)){
		ret = PTR_ERR(key.key_class);
		printk("kernel: class_create failed.\n");
		goto del_cdev;
	}
	key.key_dev = device_create(key.key_class, NULL, key.devid, NULL, KEY_NAME);
	if(IS_ERR(key.key_dev)){
		ret = PTR_ERR(key.key_dev);
		printk("kernel: device_create failed.\n");
		goto dest_class;
	}
	
	
	return 0;
	
//dest_device:
	//device_destroy(key.key_class, key.devid);
dest_class:
	class_destroy(key.key_class);
del_cdev:
	cdev_del(&key.cdev);
unreg_chrdev:
	unregister_chrdev_region(key.devid, KEY_CNT);
failed_ret:
	return ret;
}

static void __exit key_exit(void){
	
	gpio_set_value(key.key_gpio, 1);
	gpio_free(key.key_gpio);
	device_destroy(key.key_class, key.devid);
	class_destroy(key.key_class);
	cdev_del(&key.cdev);
	unregister_chrdev_region(key.devid, KEY_CNT);
}

module_init(key__init);
module_exit(key_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("weiyuyin");

