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

#define		ASYNCNOTI_CNT		1
#define		ASYNCNOTI_NAME		"asyncnoti"

struct asyncnoti_dev{
	int major;
	int minor;
	dev_t devid;
	struct cdev cdev;
	struct class * cls;
	struct device * dev;
	struct device_node * nd;
	int gpio;
	int irq;
	struct timer_list timer;
	struct work_struct work;
	struct semaphore sem;
	wait_queue_head_t r_wait;
	struct fasync_struct * async_queue;
	uint8_t key_release;
	uint8_t key_value;
};

static struct asyncnoti_dev asyncnoti = {
	.major = 0,
	.minor = 0,
	.key_release = 0,
	.key_value = 1,
};

static int asyncnoti_open(struct inode *inode, struct file *file){
	file->private_data = container_of(inode->i_cdev, struct asyncnoti_dev, cdev);
	return 0;
}


static ssize_t asyncnoti_read(struct file *file, char __user *buf, size_t count, loff_t *ppos){
	int ret = 0;
	struct asyncnoti_dev * dev = (struct asyncnoti_dev *)file->private_data;
	if(down_interruptible(&dev->sem)){
		return -ERESTARTSYS;
	}

	while(!dev->key_release){
		DEFINE_WAIT(wait);

		up(&dev->sem);
		if(file->f_flags & O_NONBLOCK){
			return -EAGAIN;
		}
		prepare_to_wait(&dev->r_wait, &wait, TASK_INTERRUPTIBLE);
		if(!dev->key_release)
			schedule();
		finish_wait(&dev->r_wait, &wait);
		if(signal_pending(current))
			return -ERESTARTSYS;
		if(down_interruptible(&dev->sem)){
			return -ERESTARTSYS;
		}
	}

	if(dev->key_release){
		dev->key_release = 0;
		ret = copy_to_user(buf, &dev->key_value, sizeof(dev->key_value));
		if(ret){
			printk("copy_to_user failed.\n");
			up(&dev->sem);
			return -EFAULT;
		}
		ret = sizeof(dev->key_value);
		
	}
	up(&dev->sem);
	return ret;
}

static unsigned int asyncnoti_poll(struct file *file, poll_table *wait)
{
	struct asyncnoti_dev * dev = (struct asyncnoti_dev *)file->private_data;
	unsigned int mask = 0;

	down(&dev->sem);
	poll_wait(file, &dev->r_wait, wait);
	if(dev->key_release){
		mask |= POLLIN | POLLRDNORM;
	}
	up(&dev->sem);	

	return mask;
}

static int asyncnoti_fasync(int fd, struct file *file, int on){
	struct asyncnoti_dev * dev = (struct asyncnoti_dev *)file->private_data;
	return fasync_helper(fd, file, on, &dev->async_queue);
}

static int asyncnoti_release(struct inode *inode, struct file *file)
{
	return asyncnoti_fasync(-1, file, 0);
}


static const struct file_operations ops = {
	.owner = THIS_MODULE,
	.open  = asyncnoti_open,
	.read = asyncnoti_read,
	.release = asyncnoti_release,
	.poll = asyncnoti_poll,
	.fasync = asyncnoti_fasync,
};

static irqreturn_t asyncnoti_irq_handler(int irq, void * arg){
	struct asyncnoti_dev * dev = (struct asyncnoti_dev *)arg;
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));
	return IRQ_HANDLED;
}

static void timer_func(unsigned long arg){
	struct asyncnoti_dev * dev = (struct asyncnoti_dev *)arg;
	schedule_work(&dev->work);
}

static void work_func(struct work_struct * arg){
	struct asyncnoti_dev * dev = (struct asyncnoti_dev *)container_of(arg, struct asyncnoti_dev, work);
	int ret = 0;
	int release_flag = 0;
	if(down_interruptible(&dev->sem)){
		return;
	}

	ret = gpio_get_value(dev->gpio);
	if(ret == 1){
		dev->key_release = 1;
		release_flag = 1;
	}
	up(&dev->sem);
	if(release_flag){
		wake_up_interruptible(&dev->r_wait);
		kill_fasync(&dev->async_queue, SIGIO, POLL_IN);
	}
}

static int asyncnoti_gpio_init(struct asyncnoti_dev * dev){
	int ret = 0;
	dev->nd = of_find_node_by_path("/key");
	if(dev->nd == NULL){
		printk("of_find_node_by_path failed.\n");
		ret = -ENODEV;
		goto failed_ret;
	}
	
	dev->gpio = of_get_named_gpio(dev->nd, "key-gpios", 0);
	if(!gpio_is_valid(dev->gpio)){
		printk("of_get_named_gpio failed.\n");
		ret = -ENODEV;
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

	dev->irq = irq_of_parse_and_map(dev->nd, 0);
	if(!dev->irq){
		printk("irq_of_parse_and_map failed");		
		goto free_gpio;
	}

	ret = request_irq(dev->irq, asyncnoti_irq_handler, IRQF_TRIGGER_RISING, "asyncnoti-irq", dev);
	if(ret){
		printk("request_irq failed.\n");
		goto free_gpio;
	}
	
	return 0;
free_gpio:
	gpio_free(dev->gpio);
failed_ret:
	return ret;
}

static void asyncnoti_gpio_destroy(struct asyncnoti_dev * dev){
	free_irq(dev->irq, dev);
	gpio_free(dev->gpio);
}


static int __init asyncnoti_init(void){
	int ret = 0;
	if(asyncnoti.major){
		asyncnoti.devid = MKDEV(asyncnoti.major, asyncnoti.minor);
		ret = register_chrdev_region(asyncnoti.devid, ASYNCNOTI_CNT, ASYNCNOTI_NAME);
	}
	else{
		ret = alloc_chrdev_region(&asyncnoti.devid, 0, ASYNCNOTI_CNT, ASYNCNOTI_NAME);
		asyncnoti.major = MAJOR(asyncnoti.devid);
		asyncnoti.minor = MINOR(asyncnoti.devid);
	}
	if(ret){
		printk("register_chrdev_region failed.\n");
		goto failed_ret;
	}
	
	asyncnoti.cdev.owner = THIS_MODULE;
	cdev_init(&asyncnoti.cdev, &ops);
	ret = cdev_add(&asyncnoti.cdev, asyncnoti.devid, ASYNCNOTI_CNT);
	if(ret){
		printk("cdev_add failed.\n");
		goto unreg_chrdev;
	}

	asyncnoti.cls = class_create(THIS_MODULE, ASYNCNOTI_NAME);
	if(IS_ERR(asyncnoti.cls)){
		ret = PTR_ERR(asyncnoti.cls);
		goto del_cdev;
	}
	asyncnoti.dev = device_create(asyncnoti.cls, NULL, asyncnoti.devid, NULL, ASYNCNOTI_NAME);
	if(IS_ERR(asyncnoti.dev)){
		ret = PTR_ERR(asyncnoti.dev);
		goto dest_cls;
	}

	init_timer(&asyncnoti.timer);
	asyncnoti.timer.function = timer_func;
	asyncnoti.timer.data = (unsigned long)&asyncnoti;

	sema_init(&asyncnoti.sem, 1);

	INIT_WORK(&asyncnoti.work, work_func);

	init_waitqueue_head(&asyncnoti.r_wait);

	ret = asyncnoti_gpio_init(&asyncnoti);
	if(ret){
		goto del_timer_label;
	}
	
	return 0;
	
del_timer_label:	
	del_timer_sync(&asyncnoti.timer);	
//dest_dev:
	device_destroy(asyncnoti.cls, asyncnoti.devid);
dest_cls:
	class_destroy(asyncnoti.cls);
del_cdev:
	cdev_del(&asyncnoti.cdev);
unreg_chrdev:
	unregister_chrdev_region(asyncnoti.devid, ASYNCNOTI_CNT);
failed_ret:	
	return ret;
}

static void __exit asyncnoti_exit(void){
	del_timer_sync(&asyncnoti.timer);
	asyncnoti_gpio_destroy(&asyncnoti);
	device_destroy(asyncnoti.cls, asyncnoti.devid);
	class_destroy(asyncnoti.cls);
	cdev_del(&asyncnoti.cdev);
	unregister_chrdev_region(asyncnoti.devid, ASYNCNOTI_CNT);
}

module_init(asyncnoti_init);
module_exit(asyncnoti_exit);
MODULE_AUTHOR("weiyuyin");
MODULE_LICENSE("GPL");

