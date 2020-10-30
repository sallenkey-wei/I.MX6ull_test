#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/ide.h>

#define CHRDEVBASE_MAJOR		200

#define	CCM_CCGR1_BASE					0x020c406c
#define SW_MUX_GPIO1_IO03_BASE			0x02030068
#define SW_PAD_GPIO1_IO03_BASE			0x020e02f4
#define GPIO1_GDIR_BASE					0x0209c004
#define GPIO1_DR_BASE					0x0209c000

static void __iomem * IMX6U_CCM_CCGR1;
static void __iomem * SW_MUX_GPIO1_IO03;
static void __iomem * SW_PAD_GPIO1_IO03;
static void __iomem * GPIO1_GDIR;
static void __iomem * GPIO1_DR;

static void led_switch(u8 status){
	u32 value = 0;
	switch(status){
		case 0:
			value = readl(GPIO1_DR);
			value |= (1 << 3);
			writel(value, GPIO1_DR);
			break;
		case 1:
			value = readl(GPIO1_DR);
			value &= ~(1 << 3);
			writel(value, GPIO1_DR);
			break;
		default:
			break;
	}
}

static int led_open(struct inode *inode, struct file *file){
	return 0;
}
static int led_release(struct inode *inode, struct file *filp){
	return 0;
}
static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offp){
	int ret = 0;
	u8 kernel_buf[1];
	ret = copy_from_user(kernel_buf, buf, 1);
	if(ret < 0){
		printk("kernel: len_write");
		return -EFAULT;
	}
	led_switch(kernel_buf[0]);
	return 0;
}

static const struct file_operations fops = {
	.write = led_write,
	.open = led_open,
	.release = led_release,
	.owner = THIS_MODULE,
};

static int __init led_init(void){
	int ret = 0;
	u32 value = 0;

	/* 地址重映射  */
	IMX6U_CCM_CCGR1 = ioremap(CCM_CCGR1_BASE, 4);
	SW_MUX_GPIO1_IO03 = ioremap(SW_MUX_GPIO1_IO03_BASE, 4);
	SW_PAD_GPIO1_IO03 = ioremap(SW_PAD_GPIO1_IO03_BASE, 4);
	GPIO1_GDIR = ioremap(GPIO1_GDIR_BASE, 4);
	GPIO1_DR = ioremap(GPIO1_DR_BASE, 4);

	/* 使能GPIO1时钟 */
	value = readl(IMX6U_CCM_CCGR1);
	value &= ~(3 << 26);
	value |= (3 << 26);
	writel(value, IMX6U_CCM_CCGR1);

	/* 设置io复用和电气属性 */
	writel(0x05, SW_MUX_GPIO1_IO03);
	writel(0x10B0, SW_PAD_GPIO1_IO03);
	value = readl(GPIO1_GDIR);
	value |= (1 << 3);
	writel(value, GPIO1_GDIR);

	/* 默认关闭led */
	led_switch(0);

	ret = register_chrdev(CHRDEVBASE_MAJOR, "led", &fops);
	if(ret < 0){
		printk("kernel: led_init failed!\n");
		return -EFAULT;
	}

	return 0;
}

module_init(led_init);

static void __exit led_exit(void){

	/* 关闭led灯 */
	led_switch(0);

	/* 解除地址映射 */
	iounmap(IMX6U_CCM_CCGR1);
	iounmap(SW_MUX_GPIO1_IO03);
	iounmap(SW_PAD_GPIO1_IO03);
	iounmap(GPIO1_GDIR);
	iounmap(GPIO1_DR);

	unregister_chrdev(CHRDEVBASE_MAJOR, "led");
}

module_exit(led_exit);

MODULE_AUTHOR("WeiYuyin<weiyuyinn@gmail.com>");
MODULE_LICENSE("GPL");

