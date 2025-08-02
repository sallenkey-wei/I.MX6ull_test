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
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/regmap.h>
#include <linux/types.h>
#include <linux/iio/iio.h>
#include <linux/iio/common/st_sensors.h>
#include <linux/iio/common/st_sensors_spi.h>

#include "icm20608.h"

struct icm20608_dev{
	struct spi_device * spi;
	struct regmap * map;
	struct mutex lock;
};

static int icm20608_reginit(struct icm20608_dev *dev){
	unsigned int regvalue;
	int ret;

	/* reset icm20608, 并关闭睡眠模式 */
	regmap_write(dev->map, ICM20_PWR_MGMT_1, 0x80);
	mdelay(50);
	regmap_write(dev->map, ICM20_PWR_MGMT_1, 0x01);
	mdelay(50);
	//验证写函数是否正常
	//regvalue = icm20608_read_onereg(dev, ICM20_PWR_MGMT_1);
	//printk("icm20608 ICM20_PWR_MGMT_1 = %#X\n", regvalue);
	ret = regmap_read(dev->map, ICM20_WHO_AM_I, &regvalue);
	if(ret){
		printk("<icm20608_reginit> regmap_read failed.\n");
		return ret;
	}
	printk("icm20608 id = %#X\r\n", regvalue);
	if(regvalue != ICM20608D_ID && regvalue != ICM20608G_ID)
		return -1;
	
	
	regmap_write(dev->map, ICM20_SMPLRT_DIV, 0x00); 	/* 输出速率是内部采样率					*/
	regmap_write(dev->map, ICM20_GYRO_CONFIG, 0x18); 	/* 陀螺仪±2000dps量程 				*/
	regmap_write(dev->map, ICM20_ACCEL_CONFIG, 0x18); 	/* 加速度计±16G量程 					*/
	regmap_write(dev->map, ICM20_CONFIG, 0x04); 		/* 陀螺仪低通滤波BW=20Hz 				*/
	regmap_write(dev->map, ICM20_ACCEL_CONFIG2, 0x04); 	/* 加速度计低通滤波BW=21.2Hz 			*/
	regmap_write(dev->map, ICM20_PWR_MGMT_2, 0x00); 	/* 打开加速度计和陀螺仪所有轴 				*/
	regmap_write(dev->map, ICM20_LP_MODE_CFG, 0x00); 	/* 关闭低功耗 						*/
	regmap_write(dev->map, ICM20_FIFO_EN, 0x00);		/* 关闭FIFO						*/
	return 0;
}

int icm20608_sensor_show(struct icm20608_dev * dev, uint32_t reg, int axis, int * val) {
	uint32_t addr = reg + (axis - IIO_MOD_X) * 2;
	__be16 d;
	int ret = regmap_bulk_read(dev->map, addr, &d, sizeof(d));
	if (ret < 0) {
		dev_err(&dev->spi->dev, "Error in nvram read %d\n", ret);
		return ret;
	}

	*val = (int16_t)be16_to_cpup(&d); //一定要先转成有符号数，否则x和y数值不对
	return IIO_VAL_INT;
}

int icm20608_sensor_set(struct icm20608_dev * dev, uint32_t reg, int axis, int val) {
	uint32_t addr = reg + (axis - IIO_MOD_X) * 2;
	uint16_t temp = val;
	__be16 d = cpu_to_be16p(&temp);
	int ret = regmap_bulk_write(dev->map, addr, &d, sizeof(d));
	if (ret < 0) {
		dev_err(&dev->spi->dev, "Error in nvram write %d\n", ret);
		return ret;
	}

	return 0;
}

int icm20608_read_channel_data(struct iio_dev *indio_dev, struct iio_chan_spec const *chan, int *val) {
	struct icm20608_dev *dev = iio_priv(indio_dev);
	int ret;
	switch(chan->type) {
	case IIO_ACCEL:
		ret = icm20608_sensor_show(dev, ICM20_ACCEL_XOUT_H, chan->channel2, val);
		break;
	case IIO_ANGL_VEL:
		ret = icm20608_sensor_show(dev, ICM20_GYRO_XOUT_H, chan->channel2, val);
		break;
	case IIO_TEMP:
		ret = icm20608_sensor_show(dev, ICM20_TEMP_OUT_H, IIO_MOD_X, val);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static const int accel_scale_icm20608[] = {61025, 122070, 244140, 488281};
static const int gyro_scale_icm20608[] = {7629, 15258, 30517, 61035};
#define ICM20608_TEMP_SCALE     326800000
#define ICM20608_TEMP_OFFSET    0

static int icm20608_read_raw(struct iio_dev *indio_dev, 
			   struct iio_chan_spec const *chan,
			   int *val,
			   int *val2,
			   long m)
{
	struct icm20608_dev *dev = iio_priv(indio_dev);
	int ret;
	uint32_t reg_data;

	switch (m) {
	case IIO_CHAN_INFO_RAW:
		mutex_lock(&indio_dev->mlock);
		ret = icm20608_read_channel_data(indio_dev, chan, val);
		mutex_unlock(&indio_dev->mlock);
		return ret;
	case IIO_CHAN_INFO_SCALE:
		switch (chan->type) {
		case IIO_ACCEL:
			mutex_lock(&indio_dev->mlock);
			ret = regmap_read(dev->map, ICM20_ACCEL_CONFIG, &reg_data);
			reg_data = (reg_data & 0x18) >> 3;
			*val = 0;
			*val2 = accel_scale_icm20608[reg_data];
			mutex_unlock(&indio_dev->mlock);
			return ret ? ret : IIO_VAL_INT_PLUS_NANO;
		case IIO_ANGL_VEL:
			mutex_lock(&indio_dev->mlock);
			ret = regmap_read(dev->map, ICM20_GYRO_CONFIG, &reg_data);
			reg_data = (reg_data & 0x18) >> 3;
			*val = 0;
			*val2 = gyro_scale_icm20608[reg_data];
			mutex_unlock(&indio_dev->mlock);
			return ret ? ret : IIO_VAL_INT_PLUS_MICRO;
		case IIO_TEMP:
			mutex_lock(&indio_dev->mlock);
			*val = ICM20608_TEMP_SCALE / 1000000;
			*val2 = ICM20608_TEMP_SCALE % 1000000;
			mutex_unlock(&indio_dev->mlock);
			return IIO_VAL_INT_PLUS_MICRO;
		default:
			return -EINVAL;
		}
	case IIO_CHAN_INFO_OFFSET:
		mutex_lock(&indio_dev->mlock);
		switch(chan->type) {
		case IIO_TEMP:
			*val = ICM20608_TEMP_OFFSET;
			ret = IIO_VAL_INT;
			break;
		default:
			ret = -EINVAL;
			break;
		}
		mutex_unlock(&indio_dev->mlock);
		return ret;
	case IIO_CHAN_INFO_CALIBBIAS:
		mutex_lock(&indio_dev->mlock);
		switch(chan->type) {
		case IIO_ACCEL:
			ret = icm20608_sensor_show(dev, ICM20_XA_OFFSET_H, chan->channel2, val);
			break;
		case IIO_ANGL_VEL:
			ret = icm20608_sensor_show(dev, ICM20_XG_OFFS_USRH, chan->channel2, val);
			break;
		default:
			ret = -EINVAL;
			break;
		}
		mutex_unlock(&indio_dev->mlock);
		return ret;
	}

	return 0;
}

int icm20608_write_raw(struct iio_dev *indio_dev,
			 struct iio_chan_spec const *chan,
			 int val,
			 int val2,
			 long mask) {
	struct icm20608_dev *dev = iio_priv(indio_dev);
	int ret = 0;
	int i = 0;
	uint32_t reg_data;
	switch (mask) {
	case IIO_CHAN_INFO_SCALE:
		mutex_lock(&indio_dev->mlock);
		switch(chan->type) {
		case IIO_ACCEL:
			for (i = 0; i < ARRAY_SIZE(accel_scale_icm20608); i++) {
				if (val2 == accel_scale_icm20608[i]) {
					ret = regmap_read(dev->map, ICM20_ACCEL_CONFIG, &reg_data);
					if (ret) {
						dev_err(&dev->spi->dev, "Error in icm20608_write_raw regmap_read%d\n", ret);
						mutex_unlock(&indio_dev->mlock);
						return -EINVAL;
					}
					reg_data &= ~0x18;
					reg_data |= (i << 3);
					ret = regmap_write(dev->map, ICM20_ACCEL_CONFIG, reg_data);
					if (ret) {
						dev_err(&dev->spi->dev, "Error in icm20608_write_raw regmap_write%d\n", ret);
						mutex_unlock(&indio_dev->mlock);
						return -EINVAL;
					}
				}
			}
			break;
		case IIO_ANGL_VEL:
			for (i = 0; i < ARRAY_SIZE(gyro_scale_icm20608); i++) {
				if (val2 == gyro_scale_icm20608[i]) {
					ret = regmap_read(dev->map, ICM20_GYRO_CONFIG, &reg_data);
					if (ret) {
						dev_err(&dev->spi->dev, "Error in icm20608_write_raw regmap_read%d\n", ret);
						mutex_unlock(&indio_dev->mlock);
						return -EINVAL;
					}
					reg_data &= ~0x18;
					reg_data |= (i << 3);
					ret = regmap_write(dev->map, ICM20_GYRO_CONFIG, reg_data);
					if (ret) {
						dev_err(&dev->spi->dev, "Error in icm20608_write_raw regmap_write%d\n", ret);
						mutex_unlock(&indio_dev->mlock);
						return -EINVAL;
					}
				}
			}
			break;
		default:
			ret = -EINVAL;
			break;
		}
		mutex_unlock(&indio_dev->mlock);
		return ret;
	case IIO_CHAN_INFO_CALIBBIAS:
		mutex_lock(&indio_dev->mlock);
		switch(chan->type) {
		case IIO_ACCEL:
			ret = icm20608_sensor_set(dev, ICM20_XA_OFFSET_H, chan->channel2, val);
			break;
		case IIO_ANGL_VEL:
			ret = icm20608_sensor_set(dev, ICM20_XG_OFFS_USRH, chan->channel2, val);
			break;
		default:
			ret = -EINVAL;
			break;
		}
		mutex_unlock(&indio_dev->mlock);
		return ret;

	default:
		ret = -EINVAL;
		return ret;
	}
	

}

int icm20608_write_raw_get_fmt(struct iio_dev *indio_dev,
		 struct iio_chan_spec const *chan,
		 long mask) {
	int ret = 0;
	switch (mask) {
	case IIO_CHAN_INFO_SCALE:
		switch(chan->type) {
		case IIO_ACCEL:
			ret = IIO_VAL_INT_PLUS_NANO;
			break;
		case IIO_ANGL_VEL:
			ret = IIO_VAL_INT_PLUS_MICRO;
			break;
		default:
			ret = -EINVAL;
			break;
		}
		return ret;
	case IIO_CHAN_INFO_CALIBBIAS:
		switch(chan->type) {
		case IIO_ACCEL:
			ret = IIO_VAL_INT_PLUS_MICRO; // IIO_VAL_INT 会写入失败，原因未知
			break;
		case IIO_ANGL_VEL:
			ret = IIO_VAL_INT_PLUS_MICRO;
			break;
		default:
			ret = -EINVAL;
			break;
		}
		return ret;

	default:
		ret = -EINVAL;
		return ret;
	}

	return ret;
}

static const struct iio_info icm20608_info = {
	.read_raw = &icm20608_read_raw,
	.write_raw = &icm20608_write_raw,
	.write_raw_get_fmt = &icm20608_write_raw_get_fmt,
	.driver_module = THIS_MODULE,
};

enum imv_icm20608_scan {
	INV_ICM20608_SCAN_ACCL_X, 
	INV_ICM20608_SCAN_ACCL_Y, 
	INV_ICM20608_SCAN_ACCL_Z, 
	INV_ICM20608_SCAN_TEMP, 
	INV_ICM20608_SCAN_GYRO_X, 
	INV_ICM20608_SCAN_GYRO_Y, 
	INV_ICM20608_SCAN_GYRO_Z, 
	INV_ICM20608_SCAN_TIMESTAMP,
};

#define ICM20608_CHAN(_type, _channel2, _index)     \
    {                                               \
		.type = _type,                              \
		.channel2 =  _channel2,                     \
		.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE), \
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) \
				| BIT(IIO_CHAN_INFO_CALIBBIAS),         \
		.scan_index = _index,                        \
		.scan_type = {                               \
			.sign = 's',                             \
			.realbits = 16,                          \
			.storagebits = 16,                       \
			.shift = 0,                              \
			.endianness = IIO_BE,                    \
		},                                           \
		.modified = 1,                               \
	}

static const struct iio_chan_spec icm20608_channels[] = {
	{
		.type = IIO_TEMP,
		.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_RAW) 
				| BIT(IIO_CHAN_INFO_OFFSET)
				| BIT(IIO_CHAN_INFO_SCALE),
		.scan_index = INV_ICM20608_SCAN_TEMP,
		.scan_type = {
			.sign = 's',
			.realbits = 16,
			.storagebits = 16,
			.shift = 0,
			.endianness = IIO_BE,
		}
	},
	ICM20608_CHAN(IIO_ACCEL, IIO_MOD_X, INV_ICM20608_SCAN_ACCL_X),
	ICM20608_CHAN(IIO_ACCEL, IIO_MOD_Y, INV_ICM20608_SCAN_ACCL_Y),
	ICM20608_CHAN(IIO_ACCEL, IIO_MOD_Z, INV_ICM20608_SCAN_ACCL_Z),
	ICM20608_CHAN(IIO_ANGL_VEL, IIO_MOD_X, INV_ICM20608_SCAN_GYRO_X),
	ICM20608_CHAN(IIO_ANGL_VEL, IIO_MOD_Y, INV_ICM20608_SCAN_GYRO_Y),
	ICM20608_CHAN(IIO_ANGL_VEL, IIO_MOD_Z, INV_ICM20608_SCAN_GYRO_Z),
};

static int	icm20608_probe(struct spi_device *spi){
	int ret = 0;
	struct regmap_config config;
	struct icm20608_dev * dev;
	struct iio_dev *indio_dev;

	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*dev));
	if (!indio_dev)
		return -ENOMEM;

	dev = iio_priv(indio_dev);

	dev_set_drvdata(&spi->dev, indio_dev);

	indio_dev->dev.parent = &spi->dev;
	indio_dev->name = spi_get_device_id(spi)->name;
	indio_dev->modes = INDIO_DIRECT_MODE; /*直连模式，提供sysfs接口*/
	indio_dev->info = &icm20608_info;

	indio_dev->channels = icm20608_channels;
	indio_dev->num_channels = ARRAY_SIZE(icm20608_channels);

	spi->mode = SPI_MODE_0;
	spi_setup(spi);
	
	dev->spi = spi;

	memset(&config, 0, sizeof(config));
	config.reg_bits = 8;
	config.val_bits = 8;
	//config.write_flag_mask = 0x80;
	config.read_flag_mask = 0x80;

	dev->map = devm_regmap_init_spi(spi, &config);
	if (IS_ERR(dev->map)) {
		dev_err(&spi->dev, "icm20608 spi regmap init failed\n");
		return PTR_ERR(dev->map);
	}

	ret = icm20608_reginit(dev);
	if(ret < 0){
		return -EINVAL;
	}

	mutex_init(&dev->lock);

	ret = iio_device_register(indio_dev);
	
	return ret;
}
static int icm20608_remove(struct spi_device *spi){
	struct iio_dev * indio_dev = (struct iio_dev *)dev_get_drvdata(&spi->dev);
	//struct icm20608_dev *dev = iio_priv(indio_dev);
	iio_device_unregister(indio_dev);
	return 0;
}


static const struct spi_device_id icm20608_id[] = {
	{
		.name = "alientek,icm20608",
		.driver_data = 0,
	},
	{	/* Sentinel */ },
};

const struct of_device_id icm20608_of_match[] = {
	{
		.compatible = "alientek,icm20608",
	},
	{ /* Sentienl */ }
};


static struct spi_driver icm20608 = {
	.id_table = icm20608_id,
	.driver = {
		.name = "alientek,icm20608",
		.of_match_table = icm20608_of_match
	},
	.probe = icm20608_probe,
	.remove = icm20608_remove,
};


static int __init icm20608_init(void){
	return spi_register_driver(&icm20608);
	
}

static void __exit icm20608_exit(void){
	spi_unregister_driver(&icm20608);
}

module_init(icm20608_init);
module_exit(icm20608_exit);
MODULE_AUTHOR("weiyuyin");
MODULE_LICENSE("GPL");
