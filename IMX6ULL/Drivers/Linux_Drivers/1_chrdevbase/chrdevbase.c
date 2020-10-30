#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/ide.h>

#define CHRDEVBASE_MAJOR		200

static unsigned char write_buf[100] = {0};
static unsigned char read_buf[100] = {0};
static char chrdevbase_data[] = "kernel data!\n";

static int chrdevbase_open(struct inode *inode, struct file *file){
	printk("chrdevbase: open\n");
	return 0;
}

static int chrdevbase_release(struct inode *inode, struct file *file){
	printk("chrdevbase: release\n");
	return 0;
}

static ssize_t chrdevbase_read(struct file *file, char __user *buf, size_t count,
					   loff_t *ppos){
	if(copy_to_user(buf, read_buf, count))
		return -EFAULT;
	return 0;
}

static ssize_t chrdevbase_write(struct file *file, const char __user *buf, size_t count,
						    loff_t *ppos){
	int ret = 0;
	ret = copy_from_user(write_buf, buf, count);
	if(ret == 0)
		printk("chrdevbase: recvdata:%s\n", write_buf);
	return 0;
}
static const struct file_operations fops = {
	.owner		= THIS_MODULE,
	.read		= chrdevbase_read,
	.write		= chrdevbase_write,
	.open		= chrdevbase_open,
	.release	= chrdevbase_release,
};

static int __init chrdevbase_init(void){
	if(register_chrdev(CHRDEVBASE_MAJOR, "chrdevbase", &fops) < 0){
		printk("chrdevbase: Unable to register driver\n");
		return -ENODEV;
	}

	printk("chrdevbase: init success\n");
	return 0;
}

static void __exit chrdevbase_cleanup(void){
	unregister_chrdev(CHRDEVBASE_MAJOR, "chrdevbase");
	printk("chrdevbase: cleanup\n");
}

module_init(chrdevbase_init);
module_exit(chrdevbase_cleanup);

MODULE_LICENSE("GPL");
