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


#define		BLOCKIO_NAME	"blockio"
#define		BLOCKIO_CNT		1

struct blockio_dev{
	int major;
	int minor;
	dev_t dev_id;
	struct cdev cdev;
	struct class * cls;
	struct device * dev;
	struct device_node *nd;
	int gpio;
	int irq_num;
	struct work_struct work;
	struct timer_list timer;
	struct semaphore sem;
	wait_queue_head_t r_wait;
	uint8_t key_release;
	uint8_t key_value;
};

static struct blockio_dev blockio = {
	.major = 0,
	.minor = 0,
	.key_release = 0,
	.key_value = 1,
};

static int blockio_open(struct inode *inode, struct file *file)
{

	file->private_data = container_of(inode->i_cdev, struct blockio_dev, cdev);
	return 0;
}

#if 0
static ssize_t blockio_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	struct blockio_dev * dev = (struct blockio_dev *)file->private_data;
	int ret = 0;
	ret = down_interruptible(&dev->sem);
	if(ret){
		return -ERESTARTSYS;
	}

	while(!dev->key_release){
		up(&dev->sem);
		if(file->f_flags & O_NONBLOCK){
			return -EAGAIN;
		}
		//printk("\"%s\" reading: going to sleep.\n", current->comm);
		if(wait_event_interruptible(dev->r_wait, dev->key_release))
			return -ERESTARTSYS;
		if(down_interruptible(&dev->sem))
			return -ERESTARTSYS;
	}
	if(dev->key_release){
		dev->key_release = 0;
		ret = copy_to_user(buf, &dev->key_value, sizeof(dev->key_value));
		if(ret){
			up(&dev->sem);
			return -EFAULT;
		}
		up(&dev->sem);
		return sizeof(dev->key_value);
	}

	up(&dev->sem);
	return 0;
}
#else
static ssize_t blockio_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	struct blockio_dev * dev = (struct blockio_dev *)file->private_data;
	int ret = 0;
	ret = down_interruptible(&dev->sem);
	if(ret){
		return -ERESTARTSYS;
	}

	while(!dev->key_release){
		DEFINE_WAIT(wait);
		
		up(&dev->sem);
		if(file->f_flags & O_NONBLOCK){
			return -EAGAIN;
		}
		//printk("\"%s\" reading: going to sleep.\n", current->comm);
		prepare_to_wait(&dev->r_wait, &wait, TASK_INTERRUPTIBLE);
		if(!dev->key_release)
			schedule();
		finish_wait(&dev->r_wait, &wait);
		if(signal_pending(current))
			return -ERESTARTSYS;
		if(down_interruptible(&dev->sem))
			return -ERESTARTSYS;
	}
	if(dev->key_release){
		dev->key_release = 0;
		ret = copy_to_user(buf, &dev->key_value, sizeof(dev->key_value));
		if(ret){
			up(&dev->sem);
			return -EFAULT;
		}
		up(&dev->sem);
		return sizeof(dev->key_value);
	}

	up(&dev->sem);
	return 0;
}

#endif
static int blockio_release(struct inode *inode, struct file *file)
{
	return 0;
}

static unsigned int blockio_poll(struct file *file, poll_table *wait)
{
	struct blockio_dev * dev = (struct blockio_dev *)file->private_data;
	unsigned int mask = 0;

	down(&dev->sem);
	poll_wait(file, &dev->r_wait, wait);
	if(dev->key_release){
		mask |= POLLIN | POLLRDNORM;
	}
	up(&dev->sem);	

	return mask;
}


static const struct file_operations ops = {
	.owner = THIS_MODULE,
	.open = blockio_open,
	.read = blockio_read,
	.release = blockio_release,
	.poll = blockio_poll,
};

static irqreturn_t blockio_irq_handler(int irq_num, void * arg){
	struct blockio_dev * dev = (struct blockio_dev *)arg;
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));
	return IRQ_HANDLED;
}

static void blockio_irq_work(struct work_struct *work){
	struct blockio_dev * dev = container_of(work, struct blockio_dev, work);
	int flag = 0;
	int ret = down_interruptible(&dev->sem);
	if(ret){
		return;
	}
	if(gpio_get_value(dev->gpio) == 1){
		dev->key_release = 1;
		flag = 1;
	}
	up(&dev->sem);
	if(flag){
		wake_up_interruptible(&dev->r_wait);
	}
}

void timer_func(unsigned long arg){
	struct blockio_dev * dev = (struct blockio_dev *)arg;
	schedule_work(&dev->work);
}

static int blockio_gpio_init(struct blockio_dev * dev){
	int ret = 0;
	dev->nd = of_find_node_by_path("/key");
	if(dev->nd == NULL){
		ret = -ENODEV;
		printk("of_find_node_by_path failed.\n");
		goto failed_ret;
	}

	dev->gpio = of_get_named_gpio(dev->nd, "key-gpios", 0);
	if(!gpio_is_valid(dev->gpio)){
		ret = -ENODEV;
		printk("of_get_name_gpio failed.\n");
		goto failed_ret;
	}

	ret = gpio_request(dev->gpio, "key-gpio");
	if(ret){
		printk("gpio_request failed.\n");
		goto failed_ret;
	}

	ret = gpio_direction_input(dev->gpio);
	if(ret){
		printk("gpio_direction_input failed.\n");
		goto free_gpios;
	}

	dev->irq_num = irq_of_parse_and_map(dev->nd, 0);
	if(!dev->irq_num){
		ret = -EINVAL;
		printk("irq_of_parse_and_map failed.\n");
		goto free_gpios;
	}

	ret = request_irq(dev->irq_num, blockio_irq_handler, \
				IRQF_TRIGGER_RISING, "blockio-irq-handler", dev);
	if(ret){
		printk("request_irq failed.\n");
		goto free_gpios;
	}

	return 0;

free_gpios:
	gpio_free(dev->gpio);
failed_ret:	
	return ret;
}

static void blockio_gpio_destroy(struct blockio_dev * dev){
	free_irq(dev->irq_num, dev);
	gpio_free(dev->gpio);
}


static int __init blockio_init(void){
	int ret = 0;
	if(blockio.major){
		blockio.dev_id = MKDEV(blockio.major, blockio.minor);
		ret = register_chrdev_region(blockio.dev_id, BLOCKIO_CNT, BLOCKIO_NAME);
	}
	else{
		alloc_chrdev_region(&blockio.dev_id, 0, BLOCKIO_CNT, BLOCKIO_NAME);
		blockio.major = MAJOR(blockio.dev_id);
		blockio.minor = MINOR(blockio.dev_id);
	}
	if(ret){
		printk("register chrdev region failed.\n");
		goto failed_ret;
	}

	blockio.cdev.owner = THIS_MODULE;
	cdev_init(&blockio.cdev, &ops);

	ret = cdev_add(&blockio.cdev, blockio.dev_id, BLOCKIO_CNT);
	if(ret){
		printk("cdev_add failed.\n");
		goto unreg_chrdev_region;
	}

	blockio.cls = class_create(THIS_MODULE, BLOCKIO_NAME);
	if(IS_ERR(blockio.cls)){
		printk("class_create failed.\n");
		ret = PTR_ERR(blockio.cls);
		goto del_cdev;
	}

	blockio.dev = device_create(blockio.cls, NULL, blockio.dev_id, NULL, BLOCKIO_NAME);
	if(IS_ERR(blockio.dev)){
		printk("device_create failed.\n");
		ret = PTR_ERR(blockio.dev);
		goto dest_class;
	}
	
	init_timer(&blockio.timer);
	blockio.timer.function = timer_func;
	blockio.timer.data = (unsigned long)&blockio;

	sema_init(&blockio.sem, 1);

	init_waitqueue_head(&blockio.r_wait);

	INIT_WORK(&blockio.work, blockio_irq_work);
	
	ret = blockio_gpio_init(&blockio);
	if(ret){
		goto dest_dev;
	}

	

	return 0;

dest_dev:
	device_destroy(blockio.cls, blockio.dev_id);
dest_class:
	class_destroy(blockio.cls);
del_cdev:
	cdev_del(&blockio.cdev);
unreg_chrdev_region:
	unregister_chrdev_region(blockio.dev_id, BLOCKIO_CNT);
failed_ret:	
	return ret;
}

static void __exit blockio_exit(void){
	blockio_gpio_destroy(&blockio);
	del_timer_sync(&blockio.timer);
	device_destroy(blockio.cls, blockio.dev_id);
	class_destroy(blockio.cls);
	cdev_del(&blockio.cdev);
	unregister_chrdev_region(blockio.dev_id, BLOCKIO_CNT);
}

module_init(blockio_init);
module_exit(blockio_exit);
MODULE_AUTHOR("weiyuyin");
MODULE_LICENSE("GPL");

