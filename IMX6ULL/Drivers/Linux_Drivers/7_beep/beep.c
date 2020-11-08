#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/ide.h>
#include <linux/cdev.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/string.h>
#include <linux/of_gpio.h>


#define BEEP_CHRDEV_CNT		1
#define	BEEP_CHRDEV_NAME	"beep"

struct beep_dev{
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class * beep_class;
	struct device * beep_device;
	struct device_node * nd;
	int beep_gpio;
	int led_gpio;
	enum of_gpio_flags beep_flag;
	enum of_gpio_flags led_flag;
};

static struct beep_dev beep;

static int beep_open(struct inode *inode, struct file *file){
	file->private_data = container_of(inode->i_cdev, struct beep_dev, cdev);
	return 0;
}

static int beep_release(struct inode *inode, struct file *file){
	return 0;
}


static ssize_t beep_write(struct file *fp, const char __user *user_buffer, size_t count, loff_t *position){
	struct beep_dev * dev = fp->private_data;
	int ret = 0;
	u8 buf[1];
	ret = copy_from_user(buf, user_buffer, 1);
	if(ret < 0){
		printk("kernel copy_from_user failed.\n");
		return -EINVAL;
	}
	gpio_set_value(dev->beep_gpio, buf[0]^dev->beep_flag);
	gpio_set_value(dev->led_gpio, buf[0]^dev->led_flag);
	return 0;
}



const static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = beep_open,
	.write = beep_write,
	.release = beep_release,
	
};




static int __init beep_init(void){
	int ret = 0;
	
	memset(&beep, 0, sizeof(beep));

	if(beep.major){
		beep.devid = MKDEV(beep.major, beep.minor);
		ret = register_chrdev_region(beep.devid, BEEP_CHRDEV_CNT, BEEP_CHRDEV_NAME);
	}
	else{
		ret = alloc_chrdev_region(&beep.devid, 0, BEEP_CHRDEV_CNT, BEEP_CHRDEV_NAME);
		beep.major = MAJOR(beep.devid);
		beep.minor = MINOR(beep.devid);
	}

	if(ret){
		printk("register_chrdev_region failed.\r\n");
		goto failed_ret;
	}

	beep.cdev.owner = THIS_MODULE;
	cdev_init(&beep.cdev, &fops);
	ret = cdev_add(&beep.cdev, beep.devid, BEEP_CHRDEV_CNT);
	if(ret){
		printk("cdev_add failed.\r\n");
		goto unregister_region;
	}

	beep.beep_class = class_create(THIS_MODULE, BEEP_CHRDEV_NAME);
	if(IS_ERR(beep.beep_class)){
		printk("class_create failed.\r\n");
		ret = PTR_ERR(beep.beep_class);
		goto cdev_delete;
	}

	beep.beep_device = device_create(beep.beep_class, NULL, beep.devid, NULL, BEEP_CHRDEV_NAME);
	if(IS_ERR(beep.beep_device)){
		printk("device_create failed.\r\n");
		ret = PTR_ERR(beep.beep_device);
		goto class_dest;
	}

	beep.nd = of_find_node_by_path("/beep");
	if(beep.nd == NULL){
		printk("of_find_node_by_path failed.\r\n");
		ret = -ENODEV;
		goto device_dest;
	}

	
	beep.beep_gpio = of_get_named_gpio_flags(beep.nd, "beep-gpios", 0, &beep.beep_flag);
	if(beep.beep_gpio < 0){
		printk("of_get_named_gpio failed.\r\n");
		ret = -ENODEV;
		goto device_dest;
	}

	printk("beep.beep_gpio = %d\n", beep.beep_gpio);
	printk("beep_flag = %d\n", beep.beep_flag);

	beep.led_gpio = of_get_named_gpio_flags(beep.nd, "led_gpios", 0, &beep.led_flag);
	if(beep.led_gpio < 0){
		printk("of_get_named_gpio led_gpios failed.\n");
		ret = -ENODEV;
		goto device_dest;
	}
	printk("led_gpio = %d\n", beep.led_gpio);
	printk("led_flag = %d\n", beep.led_flag);

	ret = gpio_request(beep.beep_gpio, "beep-gpio");
	if(ret){
		printk("gpio_request failed.\n");
		ret = -EBUSY;
		goto device_dest;
	}

	ret = gpio_request(beep.led_gpio, "led-gpio");
	if(ret){
		printk("gpio_request failed. \n");
		goto failed_gpio_free;
	}
	

	ret = gpio_direction_output(beep.beep_gpio, 1^beep.beep_flag);
	if(ret){
		goto gpio_led_free;
	}
	ret = gpio_direction_output(beep.led_gpio, 1^beep.led_flag);
	if(ret){
		goto gpio_led_free;
	}
	
	return 0;
	
gpio_led_free:
	gpio_free(beep.led_gpio);
failed_gpio_free:
	gpio_free(beep.beep_gpio);
device_dest:
	device_destroy(beep.beep_class, beep.devid);
class_dest:
	class_destroy(beep.beep_class);
cdev_delete:
	cdev_del(&beep.cdev);
unregister_region:
	unregister_chrdev_region(beep.devid, BEEP_CHRDEV_CNT);
failed_ret:
	return ret;		
}

static void __exit beep_exit(void){
	gpio_set_value(beep.led_gpio, 0^beep.led_flag);
	gpio_set_value(beep.beep_gpio, 0^beep.beep_flag);
	gpio_free(beep.led_gpio);
	gpio_free(beep.beep_gpio);
	device_destroy(beep.beep_class, beep.devid);
	class_destroy(beep.beep_class);
	cdev_del(&beep.cdev);
	unregister_chrdev_region(beep.devid, BEEP_CHRDEV_CNT);
}

module_init(beep_init);
module_exit(beep_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("weiyuyin");
