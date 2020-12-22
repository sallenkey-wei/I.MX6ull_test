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

#define	CCM_CCGR1_BASE					0x020c406c
#define SW_MUX_GPIO1_IO03_BASE			0x02030068
#define SW_PAD_GPIO1_IO03_BASE			0x020e02f4
#define GPIO1_GDIR_BASE					0x0209c004
#define GPIO1_DR_BASE					0x0209c000

#define REGISTER_LEN					4


static struct resource led_resources[] = {
	[0] = {
		.start 	= CCM_CCGR1_BASE,
		.end	= CCM_CCGR1_BASE + REGISTER_LEN -1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start 	= SW_MUX_GPIO1_IO03_BASE,
		.end	= SW_MUX_GPIO1_IO03_BASE + REGISTER_LEN -1,
		.flags	= IORESOURCE_MEM,
	},
	[2] = {
		.start 	= SW_PAD_GPIO1_IO03_BASE,
		.end	= SW_PAD_GPIO1_IO03_BASE + REGISTER_LEN -1,
		.flags	= IORESOURCE_MEM,
	},
	[3] = {
		.start 	= GPIO1_GDIR_BASE,
		.end	= GPIO1_GDIR_BASE + REGISTER_LEN -1,
		.flags	= IORESOURCE_MEM,
	},
	[4] = {
		.start 	= GPIO1_DR_BASE,
		.end	= GPIO1_DR_BASE + REGISTER_LEN -1,
		.flags	= IORESOURCE_MEM,
	},
};

static void led_release(struct device * dev){
	printk("led_release.\n");
}

static struct platform_device led_device = {
	.name = "imx6ul-led",
	.id = -1, /* 此设备无id */
	.dev = {
		.release = led_release,
	},
	.num_resources = ARRAY_SIZE(led_resources),
	.resource = led_resources,
};

static int __init leddevice_init(void){
	int ret = 0;
	ret = platform_device_register(&led_device);
	return ret;
}

static void __exit leddevice_exit(void){
	platform_device_unregister(&led_device);
}

module_init(leddevice_init);
module_exit(leddevice_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("weiyuyin");
