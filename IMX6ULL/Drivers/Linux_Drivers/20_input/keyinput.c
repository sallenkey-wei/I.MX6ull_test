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
#include <linux/input.h>

#define KEYINPUT_NAME		"keyinput"

struct keyinput_dev{
	struct device_node * nd;
	int gpio;
	int irq_num;
	struct timer_list timer;
	struct input_dev * input_dev;
};

struct keyinput_dev keyinput;

static irqreturn_t keyinput_handler(int irq_num, void * arg){
	mod_timer(&keyinput.timer, jiffies + msecs_to_jiffies(10));
	return IRQ_HANDLED;
}

static void timer_func(unsigned long arg){
	struct keyinput_dev * dev = (struct keyinput_dev *)arg;
	if(gpio_get_value(dev->gpio)){
		input_event(dev->input_dev, EV_KEY, KEY_ENTER, 0);
		input_sync(dev->input_dev);
	}
	else{
		input_event(dev->input_dev, EV_KEY, KEY_ENTER, 1);
		input_sync(dev->input_dev);
	}
}

static int gpio_init(struct platform_device *dev){
	int ret = 0;
	/* 1.获取设备节点 */
	keyinput.nd = dev->dev.of_node;
	/* 2.获取gpio */
	keyinput.gpio = of_get_named_gpio(keyinput.nd, "key-gpios", 0);
	if(!gpio_is_valid(keyinput.gpio)){
		printk("of_get_named_gpio_failed.\n");
		ret = -ENODEV;
		goto failed_ret;
	}
	/* 3.申请gpio */
	ret = gpio_request(keyinput.gpio, "key-gpio");
	if(ret){
		printk("gpio_request failed.\n");
		goto failed_ret;
	}

	ret = gpio_direction_input(keyinput.gpio);
	if(ret){
		printk("gpio_direction_input failed.\n");
		goto fail_gpio_free;
	}

	/* 初始化定时器 */
	init_timer(&keyinput.timer);
	keyinput.timer.data = (unsigned long)&keyinput;
	keyinput.timer.function = timer_func;
	

	/* 4.获取中断 */
	keyinput.irq_num = irq_of_parse_and_map(keyinput.nd, 0);
	if(keyinput.irq_num <= 0){
		printk("irq_of_parse_and_map failed.\n");
		goto fail_gpio_free;
	}

	/* 5.申请中断 */
	ret = request_irq(keyinput.irq_num, keyinput_handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,  \
					"keyinput-handler", &keyinput);
	if(ret){
		printk("request_irq failed.\n");
		goto fail_gpio_free;
	}
	return 0;
fail_gpio_free:
	gpio_free(keyinput.gpio);
failed_ret:
	return ret;
}

static int keyinput_probe(struct platform_device *dev){
	int ret = 0;
	/* 1.初始化gpio */
	ret = gpio_init(dev);
	if(ret){
		goto failed_ret;
	}

	/* 2. 申请并设置input */
	keyinput.input_dev = input_allocate_device();
	if(!keyinput.input_dev){
		printk("input_allocate_device failed.\n");
		ret = -ENOMEM;
	}
	/* 3.注册input */
	keyinput.input_dev->name = KEYINPUT_NAME;
	__set_bit(EV_KEY, keyinput.input_dev->evbit);
	__set_bit(EV_REP, keyinput.input_dev->evbit);
	__set_bit(KEY_ENTER, keyinput.input_dev->keybit);
	ret = input_register_device(keyinput.input_dev);
	if(ret){
		printk("input_register_device failed.\n");
		goto fail_free_input;
	}
	return 0;
fail_free_input:
	input_free_device(keyinput.input_dev);
failed_ret:	
	return ret;
}
int keyinput_remove(struct platform_device *dev){
	input_unregister_device(keyinput.input_dev);
	input_free_device(keyinput.input_dev);
	free_irq(keyinput.irq_num, &keyinput);
	del_timer_sync(&keyinput.timer);
	gpio_free(keyinput.gpio);
	return 0;
}


static const struct of_device_id	keyinput_match_table[] = {
	{.compatible = "sallenkey,key-input"},
	{/* sentinel */}
};

static struct platform_driver keyinput_driver = {
	.driver = {
		.name = "keyinput",
		.of_match_table = keyinput_match_table,
	},
	.probe = keyinput_probe,
	.remove = keyinput_remove,
};

static int __init keyinput_init(void){
	return platform_driver_register(&keyinput_driver);
}

static void __exit keyinput_exit(void){
	platform_driver_unregister(&keyinput_driver);
}

module_init(keyinput_init);
module_exit(keyinput_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sallenkey");

