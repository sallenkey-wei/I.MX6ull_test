#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h> 
#include <string.h>
#include <stdbool.h>
#include <assert.h>

typedef enum {
	INV_ICM20608_ACCL_SCALE  = 0,
	INV_ICM20608_ACCL_X_CAL,
	INV_ICM20608_ACCL_X_RAW,
	INV_ICM20608_ACCL_Y_CAL,
	INV_ICM20608_ACCL_Y_RAW,
	INV_ICM20608_ACCL_Z_CAL,
	INV_ICM20608_ACCL_Z_RAW,
	INV_ICM20608_GYRO_SCALE,
	INV_ICM20608_GYRO_X_CAL,
	INV_ICM20608_GYRO_X_RAW,
	INV_ICM20608_GYRO_Y_CAL,
	INV_ICM20608_GYRO_Y_RAW,
	INV_ICM20608_GYRO_Z_CAL,
	INV_ICM20608_GYRO_Z_RAW,
	INV_ICM20608_TEMP_OFFSET,
	INV_ICM20608_TEMP_RAW,
	INV_ICM20608_TEMP_SCALE,
} imv_icm20608_enum;

typedef struct {
	imv_icm20608_enum icm20608_iio_type;
	float data;
	char * iio_file_path;
	int fd;
	bool need_update;
} imv_icm20608_iio_struct;

#define ICM20608_IIO_MEMBER(path, type) \
	{ \
		.iio_file_path = path, \
		.icm20608_iio_type = type, \
	}

imv_icm20608_iio_struct icm_20608_iio_data[] = {
	ICM20608_IIO_MEMBER("/sys/bus/iio/devices/iio:device0/in_accel_scale", INV_ICM20608_ACCL_SCALE),
	ICM20608_IIO_MEMBER("/sys/bus/iio/devices/iio:device0/in_accel_x_calibbias", INV_ICM20608_ACCL_X_CAL),
	ICM20608_IIO_MEMBER("/sys/bus/iio/devices/iio:device0/in_accel_x_raw", INV_ICM20608_ACCL_X_RAW),
	ICM20608_IIO_MEMBER("/sys/bus/iio/devices/iio:device0/in_accel_y_calibbias", INV_ICM20608_ACCL_Y_CAL),
	ICM20608_IIO_MEMBER("/sys/bus/iio/devices/iio:device0/in_accel_y_raw", INV_ICM20608_ACCL_Y_RAW),
	ICM20608_IIO_MEMBER("/sys/bus/iio/devices/iio:device0/in_accel_z_calibbias", INV_ICM20608_ACCL_Z_CAL),
	ICM20608_IIO_MEMBER("/sys/bus/iio/devices/iio:device0/in_accel_z_raw", INV_ICM20608_ACCL_Z_RAW),
	ICM20608_IIO_MEMBER("/sys/bus/iio/devices/iio:device0/in_anglvel_scale", INV_ICM20608_GYRO_SCALE),
	ICM20608_IIO_MEMBER("/sys/bus/iio/devices/iio:device0/in_anglvel_x_calibbias", INV_ICM20608_GYRO_X_CAL),
	ICM20608_IIO_MEMBER("/sys/bus/iio/devices/iio:device0/in_anglvel_x_raw", INV_ICM20608_GYRO_X_RAW),
	ICM20608_IIO_MEMBER("/sys/bus/iio/devices/iio:device0/in_anglvel_y_calibbias", INV_ICM20608_GYRO_Y_CAL),
	ICM20608_IIO_MEMBER("/sys/bus/iio/devices/iio:device0/in_anglvel_y_raw", INV_ICM20608_GYRO_Y_RAW),
	ICM20608_IIO_MEMBER("/sys/bus/iio/devices/iio:device0/in_anglvel_z_calibbias", INV_ICM20608_GYRO_Z_CAL),
	ICM20608_IIO_MEMBER("/sys/bus/iio/devices/iio:device0/in_anglvel_z_raw", INV_ICM20608_GYRO_Z_RAW),
	ICM20608_IIO_MEMBER("/sys/bus/iio/devices/iio:device0/in_temp_offset", INV_ICM20608_TEMP_OFFSET),
	ICM20608_IIO_MEMBER("/sys/bus/iio/devices/iio:device0/in_temp_raw", INV_ICM20608_TEMP_RAW),
	ICM20608_IIO_MEMBER("/sys/bus/iio/devices/iio:device0/in_temp_scale", INV_ICM20608_TEMP_SCALE),
};

int load_icm20608_iio_fd(imv_icm20608_iio_struct * iio_data, int iio_array_len) {
	assert(iio_data && (iio_array_len > 0));
	int i = 0;
	for (i = 0; i < iio_array_len; i++) {
		iio_data[i].fd = open(iio_data[i].iio_file_path, O_RDWR);
		if (iio_data[i].fd < 0) {
			printf("load_icm20608_iio_fd failed.\n");
			return -1;
		}
	}
	return 0;
}

void close_icm20608_iio_fd(imv_icm20608_iio_struct * iio_data, int iio_array_len) {
	assert(iio_data && (iio_array_len > 0));
	int i = 0;
	for (i = 0; i < iio_array_len; i++) {
		if (iio_data[i].fd > 0)
			close(iio_data[i].fd);
		else
			printf("dobule close iio fd.\n");
		iio_data[i].fd = 0;
	}
}

int updata_icm20608_iio_struct(imv_icm20608_iio_struct * iio_data, int iio_array_len, bool is_update_all) {
	int i = 0;
	assert(iio_data && (iio_array_len > 0));
	if (is_update_all) {
		for (i = 0; i < iio_array_len; i++) {
			char buf[1024];
			int count = read(iio_data[i].fd, buf, sizeof(buf));
			if (count < 0) {
				printf("updata_icm20608_iio_struct failed.\n");
				return -1;
			}
			if (i == INV_ICM20608_GYRO_SCALE) {
				printf("==========buf = %s\n", buf);
			}
			sscanf(buf, "%f", &iio_data[i].data);
			printf("data = %f\n", iio_data[i].data);
		}
	}
	else {
		for (i = 0; i < iio_array_len; i++) {
			char buf[1024] = {0};
			int count = 0;
			switch (iio_data[i].icm20608_iio_type) {
			case INV_ICM20608_ACCL_X_RAW:
			case INV_ICM20608_ACCL_Y_RAW:
			case INV_ICM20608_ACCL_Z_RAW:
			case INV_ICM20608_GYRO_X_RAW:
			case INV_ICM20608_GYRO_Y_RAW:
			case INV_ICM20608_GYRO_Z_RAW:
			case INV_ICM20608_TEMP_RAW:
				count = read(iio_data[i].fd, buf, sizeof(buf));
				if (count < 0) {
					printf("updata_icm20608_iio_struct failed.\n");
					return -1;
				}
				sscanf(buf, "%f", &iio_data[i].data);
				printf("data = %f\n", iio_data[i].data);
				break;
			default:
				break;
			}
		}
	}
	return 0;
}

int main(int argc, char * argv[]){
	int fd, ret, i;
	char * file_name;
	float gyro_x_adc, gyro_y_adc, gyro_z_adc; 
	float accel_x_adc, accel_y_adc, accel_z_adc; 
	float temp_adc;

	float gyro_x_act, gyro_y_act, gyro_z_act; 
	float accel_x_act, accel_y_act, accel_z_act; 
	float temp_act;

	ret = load_icm20608_iio_fd(icm_20608_iio_data, sizeof(icm_20608_iio_data)/sizeof(icm_20608_iio_data[0]));
	if (ret) {
		printf("load_icm20608_iio_fd failed.\n");
		return -1;
	}

	ret = updata_icm20608_iio_struct(icm_20608_iio_data, sizeof(icm_20608_iio_data)/sizeof(icm_20608_iio_data[0]), true);
	if (ret) {
		printf("updata_icm20608_iio_struct failed.\n");
		return -1;
	}

	while(1){
		ret = updata_icm20608_iio_struct(icm_20608_iio_data, sizeof(icm_20608_iio_data)/sizeof(icm_20608_iio_data[0]), false);
		if (ret) {
			return -1;
		}
		for (i = 0; i < sizeof(icm_20608_iio_data)/sizeof(icm_20608_iio_data[0]); i++) {
			char buf[1024];
			switch (icm_20608_iio_data[i].icm20608_iio_type) {
			case INV_ICM20608_ACCL_X_RAW:
				accel_x_adc = icm_20608_iio_data[i].data;
				break;
			case INV_ICM20608_ACCL_Y_RAW:
				accel_y_adc = icm_20608_iio_data[i].data;
				break;
			case INV_ICM20608_ACCL_Z_RAW:
				accel_z_adc = icm_20608_iio_data[i].data;
				break;
			case INV_ICM20608_GYRO_X_RAW:
				gyro_x_adc = icm_20608_iio_data[i].data;
				break;
			case INV_ICM20608_GYRO_Y_RAW:
				gyro_y_adc = icm_20608_iio_data[i].data;
				break;
			case INV_ICM20608_GYRO_Z_RAW:
				gyro_z_adc = icm_20608_iio_data[i].data;
				break;
			case INV_ICM20608_TEMP_RAW:
				temp_adc = icm_20608_iio_data[i].data;
				break;
			default:
				break;
			}
		}
		
		/* 计算实际值 */
		printf("icm_20608_iio_data[INV_ICM20608_GYRO_SCALE].data = %f\n", icm_20608_iio_data[INV_ICM20608_GYRO_SCALE].data);
		printf("icm_20608_iio_data[INV_ICM20608_ACCL_SCALE].data = %f\n", icm_20608_iio_data[INV_ICM20608_ACCL_SCALE].data);
		printf("icm_20608_iio_data[INV_ICM20608_TEMP_OFFSET].data = %f\n", icm_20608_iio_data[INV_ICM20608_TEMP_OFFSET].data);
		printf("icm_20608_iio_data[INV_ICM20608_TEMP_SCALE].data = %f\n", icm_20608_iio_data[INV_ICM20608_TEMP_SCALE].data);
		gyro_x_act = gyro_x_adc * icm_20608_iio_data[INV_ICM20608_GYRO_SCALE].data;
		gyro_y_act = gyro_y_adc * icm_20608_iio_data[INV_ICM20608_GYRO_SCALE].data;
		gyro_z_act = gyro_z_adc * icm_20608_iio_data[INV_ICM20608_GYRO_SCALE].data;
		accel_x_act = accel_x_adc * icm_20608_iio_data[INV_ICM20608_ACCL_SCALE].data;
		accel_y_act = accel_y_adc * icm_20608_iio_data[INV_ICM20608_ACCL_SCALE].data;
		accel_z_act = accel_z_adc * icm_20608_iio_data[INV_ICM20608_ACCL_SCALE].data;
		temp_act = ((temp_adc) - 25 ) / icm_20608_iio_data[INV_ICM20608_TEMP_SCALE].data + 25;
		printf("\r\n原始值:\r\n");
		printf("gx = %f, gy = %f, gz = %f\r\n", gyro_x_adc, gyro_y_adc, gyro_z_adc);
		printf("ax = %f, ay = %f, az = %f\r\n", accel_x_adc, accel_y_adc, accel_z_adc);
		printf("temp = %f\r\n", temp_adc);
		printf("实际值:");
		printf("act gx = %.2f°/S, act gy = %.2f°/S, act gz = %.2f°/S\r\n", gyro_x_act, gyro_y_act, gyro_z_act);
		printf("act ax = %.2fg, act ay = %.2fg, act az = %.2fg\r\n", accel_x_act, accel_y_act, accel_z_act);
		printf("act temp = %.2f°C\r\n", temp_act);

		usleep(100000);
	}

	close_icm20608_iio_fd(icm_20608_iio_data, sizeof(icm_20608_iio_data)/sizeof(icm_20608_iio_data[0]));
	return 0;
}
