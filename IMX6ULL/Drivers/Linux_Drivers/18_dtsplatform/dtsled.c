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
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/pm_domain.h>


#define PLATFORMLED_NAME	"dtsplatformled"
#define PLATFORMLED_CNT		1

#define LED_ON				1
#define LED_OFF				0

#define USE_LEGACY_GPIO_API 0

struct platformled_dev{
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class * class;
	struct device * dev;
	struct device_node * nd;
	struct platform_device * platform_dev;
 #if USE_LEGACY_GPIO_API
	int gpio_index;
#else
	struct gpio_desc * gpio_desc;
#endif
};

static struct platformled_dev platformled = {
	.major = 0,
	.minor = 0,
};

void led_switch(u8 status){
 #if USE_LEGACY_GPIO_API
	if(status == LED_ON){
		gpio_set_value(platformled.gpio_index, 0);
	}
	else if(status == LED_OFF){
		gpio_set_value(platformled.gpio_index, 1);
	}
#else
	if(status == LED_ON){
		gpiod_set_value(platformled.gpio_desc, 1);
	}
	else if(status == LED_OFF){
		gpiod_set_value(platformled.gpio_desc, 0);
	}
#endif
}

static int platformled_open(struct inode *inode, struct file *filp){
	filp->private_data = (void *)container_of(inode->i_cdev, struct platformled_dev, cdev);
#if 0
	int ret;
	struct platformled_dev * platformled = filp->private_data;
	struct platform_device * pdev = platformled->platform_dev;

	pm_runtime_enable(&pdev->dev);

    ret = pm_runtime_get_sync(&pdev->dev);
    if (ret < 0) printk("===pm_runtime_get_sync error===\n");
#endif

	return 0;
}

static int platformled_release(struct inode *inode, struct file *filp)
{
#if 0
	struct platformled_dev * platformled = filp->private_data;
	struct platform_device * pdev = platformled->platform_dev;
	int ret;
	ret = pm_runtime_put_sync(&pdev->dev);
	if (ret < 0) printk("===pm_runtime_put_sync error===\n");
	pm_runtime_disable(&pdev->dev);
#endif

	return 0;
}

static ssize_t platformled_write(struct file *file, const char __user *buffer,
			     size_t count, loff_t *ppos){
	int ret = 0;
	u8 data[1];
	ret = copy_from_user(data, buffer, count);
	if(ret)
		return -EFAULT;
	led_switch(data[0]);
	return 0;
}


static const struct file_operations fops ={
	.open = platformled_open,
	.release = platformled_release,
	.write = platformled_write,
};


static int led_probe(struct platform_device * dev){
	int ret = 0;
	printk("led_probe.\n");
	if(platformled.major){
		platformled.devid = MKDEV(platformled.major, 0);
		platformled.minor = 0;
		ret = register_chrdev_region(platformled.devid, PLATFORMLED_CNT, PLATFORMLED_NAME);
	}
	else{
		ret = alloc_chrdev_region(&platformled.devid, 0, PLATFORMLED_CNT, PLATFORMLED_NAME);
		platformled.major = MAJOR(platformled.devid);
		platformled.minor = MINOR(platformled.devid);
	}
	if(ret){
		printk("kernel register_chrdev_region failed.\r\n");
		goto fail_ret;
	}

	platformled.cdev.owner = THIS_MODULE;
	cdev_init(&platformled.cdev, &fops);

	ret = cdev_add(&platformled.cdev, platformled.devid, PLATFORMLED_CNT);
	if(ret){
		printk("kernel cdev_add failed.\r\n");
		goto fail_unreg;
	}

	platformled.class = class_create(THIS_MODULE, PLATFORMLED_NAME);
	if(IS_ERR(platformled.class)){
		printk("kernel class_create failed.\r\n");
		ret = PTR_ERR(platformled.class);
		goto fail_cdev_del;
	}

	platformled.dev = device_create(platformled.class, NULL, platformled.devid, NULL, PLATFORMLED_NAME);
	if(IS_ERR(platformled.dev)){
		printk("kernel device_create failed.\r\n");
		ret = PTR_ERR(platformled.dev);
		goto fail_class_dest;
	}

	platformled.nd = dev->dev.of_node;

#if USE_LEGACY_GPIO_API
	platformled.gpio_index = of_get_named_gpio(platformled.nd, "led-gpios", 0);
	if(platformled.gpio_index < 0){
		printk("kernel of_get_named_gpio failed.\r\n");
		ret = -EINVAL;
		goto fail_dev_dest;
	}

	ret = gpio_request(platformled.gpio_index, "led");

	if(ret){
		printk("kernel gpio_request failed.\r\n");
		goto fail_dev_dest;
	}

	ret = gpio_direction_output(platformled.gpio_index, 1);
	if(ret){
		printk("kernel gpio_directon_output failed.\r\n");
		goto fail_gpio_free;
	}
#else
	platformled.gpio_desc = devm_gpiod_get(&dev->dev, "led", GPIOD_OUT_HIGH); // default logic level high, the led default status is on.
	if (IS_ERR(platformled.gpio_desc)) {
		ret = PTR_ERR(platformled.gpio_desc);
		goto fail_dev_dest;
	}

	//ret = gpiod_direction_output(platformled.gpio_desc, 1); // 1: default logic level high, already set gpio GPIOD_OUT_HIGH previous, this code is not essential.
	if (ret) {
		goto fail_dev_dest;
	}


#endif

	platformled.platform_dev = dev;
	
	return 0;

#if USE_LEGACY_GPIO_API
fail_gpio_free:
	gpio_free(platformled.gpio_index);
#endif
fail_dev_dest:
	device_destroy(platformled.class, platformled.devid);
fail_class_dest:
	class_destroy(platformled.class);
fail_cdev_del:
	cdev_del(&platformled.cdev);
fail_unreg:
	unregister_chrdev_region(platformled.devid, PLATFORMLED_CNT);
fail_ret:
	return ret;
}

static int led_remove(struct platform_device * dev){
	led_switch(LED_OFF);
#if USE_LEGACY_GPIO_API
	gpio_free(platformled.gpio_index);
#endif
	device_destroy(platformled.class, platformled.devid);
	class_destroy(platformled.class);
	cdev_del(&platformled.cdev);
	unregister_chrdev_region(platformled.devid, PLATFORMLED_CNT);
	return 0;
}

#if 0
static int viv_dev_system_suspend(struct device *dev)
{
    printk("=============> Enter viv_dev_system_suspend");
    return 0;
}

static int viv_dev_system_resume(struct device *dev)
{
	printk("=============> Enter viv_dev_system_resume");
    return 0;
}
#endif

static const struct of_device_id led_of_match[] = {
	{.compatible = "alientek,gpio-led"},
	{/*Sentinel*/}
};

#if 0
static const struct dev_pm_ops viv_dev_pm_ops = {
    SET_SYSTEM_SLEEP_PM_OPS(viv_dev_system_suspend, viv_dev_system_resume)
};
#endif

static struct platform_driver led_driver = {
	.probe = led_probe,
	.remove = led_remove,
	.driver = {
		.name = "imx6ul-led",/* 必须要有，要不然内核会段错误 */
		.of_match_table = led_of_match,
#if 0
		.pm     = &viv_dev_pm_ops,
#endif
	},
};

static int __init platform_led_init(void){
	return platform_driver_register(&led_driver);
}

static void __exit platform_led_exit(void){
	platform_driver_unregister(&led_driver);
}

module_init(platform_led_init);
module_exit(platform_led_exit);
MODULE_AUTHOR("weiyuyin");
MODULE_LICENSE("GPL");
