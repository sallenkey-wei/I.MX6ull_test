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

#define			KEYIRQ_DEV_NUM		1
#define			KEYIRQ_DEV_NAME		"keyirq"
#define			KEY_VALUE			0x01

struct keyirq_dev{
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class * cls;
	struct device * dev;
	struct device_node * nd;
	int gpio;
	int irq_num;
	atomic_t key_value;
	atomic_t key_release_flag;
	struct timer_list timer;
	struct tasklet_struct tasklet;
	struct work_struct work;
};

static struct keyirq_dev keyirq = {
	.major = 0,
	.minor = 0,
	.key_value = ATOMIC_INIT(KEY_VALUE),
	.key_release_flag = ATOMIC_INIT(0),
	
};

static int keyirq_open(struct inode *inode, struct file *file)
{
		
	file->private_data = container_of(inode->i_cdev, struct keyirq_dev, cdev);
	
	return 0;
}


static ssize_t keyirq_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	struct keyirq_dev * dev = file->private_data;
	int ret = 0;
	
	if(atomic_read(&dev->key_release_flag)){
		atomic_set(&dev->key_release_flag, 0);
		int key_value = atomic_read(&dev->key_value);
		ret = copy_to_user(buf, &key_value, sizeof(key_value));
		if(ret)
			return -EFAULT;
		return sizeof(key_value);
	}
	else{
		return 0;
	}
	
}

static int keyirq_release(struct inode *inode, struct file *file)
{

	return 0;
}


static const struct file_operations ops = {
	.owner = THIS_MODULE,
	.read = keyirq_read,
	.open =keyirq_open,
	.release = keyirq_release,
};

static irqreturn_t keyirq_handler(int irq, void *dev_id){
	struct keyirq_dev * dev = dev_id;
	
	//tasklet_schedule(&dev->tasklet);
	schedule_work(&dev->work);
	return IRQ_HANDLED;
}


static int keyio_init(struct keyirq_dev * dev){
	int ret = 0;
	enum of_gpio_flags flag;
	dev->nd = of_find_node_by_path("/key");
	if(keyirq.nd == NULL){
		printk("of_find_node_by_path failed.\n");
		ret = -ENODEV;
		goto failed_ret;
	}

	dev->gpio = of_get_named_gpio_flags(dev->nd, "key-gpios", 0, &flag);
	if(!gpio_is_valid(dev->gpio)){
		printk("of_get_named_gpio_flags failed.\n");
		ret = dev->gpio;
		goto failed_ret;
	}

	ret = gpio_request(dev->gpio, "key_gpio");
	if(ret){
		printk("gpio_request failed.\n");
		goto failed_ret;
	}

	ret = gpio_direction_input(dev->gpio);
	if(ret){
		printk("gpio_direction_input failed.\n");
		goto free_gpio;
	}
#if 0
	dev->irq_num = irq_of_parse_and_map(dev->nd, 0);
#else
	dev->irq_num = gpio_to_irq(dev->gpio);
#endif
	ret = request_irq(dev->irq_num, keyirq_handler, IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, 
					"keyirq-handler", dev);
	if(ret){
		printk("request_irq failed.\r\n");
		goto free_gpio;
	}
	return 0;

free_gpio:
	gpio_free(dev->gpio);
failed_ret:
	return ret;
}
static void timer_function(unsigned long arg){
	struct keyirq_dev * dev = (struct keyirq_dev *)arg;
	int value = gpio_get_value(dev->gpio);
	if(value){
		atomic_set(&dev->key_release_flag, 1);
	}
}

void tasklet_func(unsigned long arg){
	struct keyirq_dev * dev = (struct keyirq_dev *)arg;
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));
}

void work_func(struct    work_struct * work){
	struct keyirq_dev * dev = container_of(work, struct keyirq_dev, work);
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));
}


static int __init keyirq_init(void){
	int ret = 0;
	if(keyirq.major){
		keyirq.devid = MKDEV(keyirq.major, 0);
		keyirq.minor = 0;
		ret = register_chrdev_region(keyirq.devid, KEYIRQ_DEV_NUM, KEYIRQ_DEV_NAME);
	}
	else{
		ret = alloc_chrdev_region(&keyirq.devid, 0, KEYIRQ_DEV_NUM, KEYIRQ_DEV_NAME);
		keyirq.major = MAJOR(keyirq.devid);
		keyirq.minor = MINOR(keyirq.devid);
	}
	if(ret){
		printk("alloc_chrdev_region failed.\n");
		goto failed_ret;
	}

	keyirq.cdev.owner = THIS_MODULE;
	cdev_init(&keyirq.cdev, &ops);

	ret = cdev_add(&keyirq.cdev, keyirq.devid, KEYIRQ_DEV_NUM);
	if(ret){
		printk("cdev_add error.\n");
		goto unregister_chrdev;
	}

	keyirq.cls = class_create(THIS_MODULE, KEYIRQ_DEV_NAME);
	if(IS_ERR(keyirq.cls)){
		printk("class_create failed.\n");
		ret = PTR_ERR(keyirq.cls);
		goto del_cdev;
	}

	keyirq.dev = device_create(keyirq.cls, NULL, keyirq.devid, NULL, KEYIRQ_DEV_NAME);
	if(IS_ERR(keyirq.dev)){
		printk("device_create failed.\n");
		ret = PTR_ERR(keyirq.dev);
		goto dest_class;
	}

	//tasklet_init(&keyirq.tasklet, tasklet_func, (unsigned long)&keyirq);
	INIT_WORK(&keyirq.work, work_func);
	ret = keyio_init(&keyirq);
	if(ret){
		goto dest_dev;
	}

	init_timer(&keyirq.timer);
	keyirq.timer.data = (unsigned long)&keyirq;
	keyirq.timer.function = timer_function;
	
	return 0;
	
dest_dev:
	device_destroy(keyirq.cls, keyirq.devid);
dest_class:
	class_destroy(keyirq.cls);
del_cdev:
	cdev_del(&keyirq.cdev);
unregister_chrdev:
	unregister_chrdev_region(keyirq.devid, KEYIRQ_DEV_NUM);
failed_ret:
	return ret;
}

static void __exit keyirq_exit(void){
	free_irq(keyirq.irq_num, &keyirq);
	gpio_free(keyirq.gpio);

	del_timer_sync(&keyirq.timer);

	device_destroy(keyirq.cls, keyirq.devid);

	class_destroy(keyirq.cls);

	cdev_del(&keyirq.cdev);

	unregister_chrdev_region(keyirq.devid, KEYIRQ_DEV_NUM);
}

module_init(keyirq_init);
module_exit(keyirq_exit);
MODULE_AUTHOR("weiyuyin");
MODULE_LICENSE("GPL");

