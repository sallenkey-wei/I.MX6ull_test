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
#include <linux/input/mt.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include "ft5x06.h"

struct ft5x06_dev {
	struct device_node * nd;
	int irq_pin, reset_pin;
	int irqnum;
	struct input_dev * input;
	struct i2c_client * client;
};

static struct ft5x06_dev ft5x06;

static int ft5x06_read_regs(struct ft5x06_dev * dev, u8 reg, u8 * buf, int len){
	struct i2c_client * client = dev->client;
	struct i2c_msg msgs[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf = &reg
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = len,
			.buf = buf
		}
	};
	int ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if(ret == ARRAY_SIZE(msgs)){
		return 0;
	}
	else{
		printk("i2c rd failed=%d reg=%06x len=%d\n", ret, reg, len);
		return -EREMOTEIO;
	}
}

static int ft5x06_write_regs(struct ft5x06_dev * dev, u8 reg, u8 * buf, int len){
	struct i2c_client * client = dev->client;
	u8 b[256];
	int ret;	
	struct i2c_msg msgs[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = len + 1,
			.buf = b
		},

	};
	b[0] = reg;
	memcpy(&b[1], buf, len);
	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if(ret == ARRAY_SIZE(msgs)){
		return 0;
	}
	else{
		printk("i2c wr failed=%d reg=%06x len=%d\n", ret, reg, len);
		return -EREMOTEIO;
	}
}

static int ft5x06_read_reg(struct ft5x06_dev *dev, u8 reg){
	struct i2c_client * client = dev->client;
	u8 data;
	ft5x06_read_regs(dev, reg, &data, 1);
	return data;
	return i2c_smbus_read_byte_data(client, reg);
}

static int ft5x06_write_reg(struct ft5x06_dev * dev, u8 reg, u8 data){
	struct i2c_client * client = dev->client;
	//return ft5x06_write_regs(dev, reg, &data, 1);
	return i2c_smbus_write_byte_data(client, reg, data);
}

static irqreturn_t ft5x06_handler_t(int irq, void * param){
	struct ft5x06_dev * dev = (struct ft5x06_dev *)param;
	u8 rdbuf[29];
	int i, type, x, y, id;
	int offset, tplen;
	int ret;
	bool down;

	offset = 1;
	tplen = 6;
	
	memset(rdbuf, 0, sizeof(rdbuf));
	ret = ft5x06_read_regs(dev, FT5X06_TD_STATUS_REG, rdbuf, FT5X06_READLEN);
	if(ret)
		return IRQ_HANDLED;
	for(i = 0; i < MAX_SUPPORT_POINTS; i++){
		u8 * buf = &rdbuf[i * tplen + offset];
		type = buf[0] >> 6;
		if(type == TOUCH_EVENT_RESERVED)
			continue;
		x = ((buf[2] << 8) | buf[3]) & 0x0fff;
		y = ((buf[0] << 8) | buf[1]) & 0x0fff;
		id = (buf[2] >> 4) & 0x0f;
		down = type != TOUCH_EVENT_UP;
		input_mt_slot(dev->input, id);
		input_mt_report_slot_state(dev->input, MT_TOOL_FINGER, down);
		if(!down)
			continue;
		input_report_abs(dev->input, ABS_MT_POSITION_X, x);
		input_report_abs(dev->input, ABS_MT_POSITION_Y, y);
	}
	input_mt_report_pointer_emulation(dev->input, true);
	input_sync(dev->input);
	return IRQ_HANDLED;
}

static int ft5x06_ts_reset(struct ft5x06_dev *dev){
	struct i2c_client * client = dev->client;
	int ret = 0;

	ret = devm_gpio_request_one(&client->dev, dev->reset_pin, GPIOF_OUT_INIT_LOW, "edt-ft5x06 reset");
	if(ret)
		return ret;
	msleep(5);
	gpio_set_value(dev->reset_pin, 1); /* 输出高电平停止复位 */ 
	msleep(300);
	return 0;
}

static int ft5x06_ts_irq(struct ft5x06_dev * dev){
	struct i2c_client * client= dev->client;
	int ret = 0;
	/* 申请中断GPIO */
	ret = devm_gpio_request_one(&client->dev, dev->irq_pin, GPIOF_IN, "edt-ft5x06 irq");
	if(ret){
		dev_err(&client->dev, "Failed to request GPIO %d, error %d\n", dev->irq_pin, ret);
		return ret;
	}
	
	/* 申请中断 */
	ret = devm_request_threaded_irq(&client->dev, client->irq, NULL, ft5x06_handler_t, 
						IRQF_TRIGGER_FALLING|IRQF_ONESHOT, client->name, dev);/* IRQF_ONESHOT表示中断线程化的thread_fn没有
																			   * 执行之前，中断不会再次被触发，这样thread_fn
																			   * 函数就不用考虑重入的问题*/
	if(ret)
		return ret;
	return 0;
}

static int ft5x06_probe(struct i2c_client * client, const struct i2c_device_id * id){
	int ret = 0;
	u8 tmp;
	ft5x06.client = client;
	ft5x06.nd = client->dev.of_node;

	/* 获取设备树中的中断和复位引脚 */
	ft5x06.irq_pin = of_get_named_gpio(ft5x06.nd, "reset-gpios", 0);
	if(!gpio_is_valid(ft5x06.irq_pin)){
		printk("<ft5x06_probe> get reset-gpios failed\n");
		return -ENODEV;
	}
	ft5x06.reset_pin = of_get_named_gpio(ft5x06.nd, "interrupt-gpios", 0);
	if(!gpio_is_valid(ft5x06.irq_pin)){
		printk("<ft5x06_probe> get interrupt-gpios failed.\n");
		return -ENODEV;
	}

	/* 复位触摸屏 */
	ret = ft5x06_ts_reset(&ft5x06);
	if(ret){
		return ret;
	}

	/* 初始化中断 */
	ret = ft5x06_ts_irq(&ft5x06);
	if(ret)
		return ret;

	/* 初始化FT5x6 */
	ft5x06_write_reg(&ft5x06, FT6X06_DEVICE_MODE_REG, 0);
	ft5x06_write_reg(&ft5x06, FT5426_IDG_MODE_REG, 1);
	tmp = ft5x06_read_reg(&ft5x06, FT5426_IDG_MODE_REG);
	printk("tmp = %d\n", tmp);

	/* input设备注册 */
	ft5x06.input = devm_input_allocate_device(&client->dev);
	if(!ft5x06.input){
		ret = -ENOMEM;
		return ret;
	}

	ft5x06.input->name = client->name;
	ft5x06.input->id.bustype = BUS_I2C;
	ft5x06.input->dev.parent = &client->dev;
	__set_bit(EV_KEY, ft5x06.input->evbit);
	__set_bit(EV_ABS, ft5x06.input->evbit);
	__set_bit(BTN_TOUCH, ft5x06.input->keybit);

	input_set_abs_params(ft5x06.input, ABS_X, 0, 480, 0, 0);
	input_set_abs_params(ft5x06.input, ABS_Y, 0, 272, 0, 0);
	input_set_abs_params(ft5x06.input, ABS_MT_POSITION_X, 0, 480, 0, 0);
	input_set_abs_params(ft5x06.input, ABS_MT_POSITION_Y, 0, 272, 0, 0);

	ret = input_mt_init_slots(ft5x06.input, MAX_SUPPORT_POINTS, 0);
	if(ret)
		return ret;

	ret = input_register_device(ft5x06.input);
	if(ret)
		return ret;
	
	return 0;
}

static int ft5x06_remove(struct i2c_client * client){
	input_unregister_device(ft5x06.input);
	return 0;
}



static const struct of_device_id ft5x06_of_match[] = {
	{
		.compatible = "edt,edt-ft5426",
	},
	{ /* Sentinel */ }
};

static const struct i2c_device_id ft5x06_id_table[] = {
	{.name = "edt,edt-ft5426", 0},
	{ /* Sentinel */ }
};


static struct i2c_driver ft5x06_driver = {
	.driver = {
		.name = "ft5x06",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ft5x06_of_match),
		},
	.probe = ft5x06_probe,
	.remove = ft5x06_remove,
	.id_table = ft5x06_id_table,
};


static int __init ft5x06_init(void){
	return i2c_add_driver(&ft5x06_driver);
}

static void __exit ft5x06_exit(void){
	i2c_del_driver(&ft5x06_driver);
}

module_init(ft5x06_init);
module_exit(ft5x06_exit);

MODULE_AUTHOR("wyy");
MODULE_LICENSE("GPL");

