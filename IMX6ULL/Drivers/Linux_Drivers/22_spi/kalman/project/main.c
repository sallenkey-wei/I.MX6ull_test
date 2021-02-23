#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h> 
#include <string.h>
#include <math.h>
#include "ekf.h"

#define PI acos(-1)

int main(int argc, char * argv[]){
	int fd, ret;
	char * file_name;
	signed int databuf[7];
	unsigned char data[14];
	signed int gyro_x_adc, gyro_y_adc, gyro_z_adc; 
	signed int accel_x_adc, accel_y_adc, accel_z_adc; 
	signed int temp_adc;

	float gyro_x_act, gyro_y_act, gyro_z_act; 
	float accel_x_act, accel_y_act, accel_z_act;
	float temp_act;
	float roll, pitch, yaw;

	if(argc != 2){
		printf("Error Usage!\r\n");
	}

	file_name = argv[1];

	IMU_init();

	fd  = open(file_name, O_RDWR);
	if(fd < 0){
		printf("Can't open file %s \r\n", file_name);
		return -1;
	}

	while(1){
		ret = read(fd, &databuf, sizeof(databuf));
		if(ret == sizeof(databuf)){
			gyro_x_adc = databuf[0];
			gyro_y_adc = databuf[1];
			gyro_z_adc = databuf[2];
			accel_x_adc = databuf[3];
			accel_y_adc = databuf[4];
			accel_z_adc = databuf[5];
			temp_adc = databuf[6];
			/* 计算实际值 */
			gyro_x_act = (float)(gyro_x_adc) / 16.4 * (PI/180);
			gyro_y_act = (float)(gyro_y_adc) / 16.4 * (PI/180);
			gyro_z_act = (float)(gyro_z_adc) / 16.4 * (PI/180);
			accel_x_act = (float)(accel_x_adc) / 2048;
			accel_y_act = (float)(accel_y_adc) / 2048;
			accel_z_act = (float)(accel_z_adc) / 2048;
			temp_act = ((float)(temp_adc) - 25 ) / 326.8 + 25;
			printf("\r\n原始值:\r\n");
			printf("gx = %d, gy = %d, gz = %d\r\n", gyro_x_adc, gyro_y_adc, gyro_z_adc);
			printf("ax = %d, ay = %d, az = %d\r\n", accel_x_adc, accel_y_adc, accel_z_adc);
			printf("temp = %d\r\n", temp_adc);
			printf("实际值:");
			printf("act gx = %.2f°/S, act gy = %.2f°/S, act gz = %.2f°/S\r\n", gyro_x_act, gyro_y_act, gyro_z_act);
			printf("act ax = %.2fg, act ay = %.2fg, act az = %.2fg\r\n", accel_x_act, accel_y_act, accel_z_act);
			printf("act temp = %.2f°C\r\n", temp_act);
			IMU_Update(gyro_x_act, gyro_y_act, gyro_z_act, accel_x_act, accel_y_act, accel_z_act, &roll, &pitch, &yaw);
		}
		else{
			printf("read error.\n");
		}
		while(1);
		usleep(SAMPLE_PERIOD * 1000);
	}

	close(fd);
	return 0;
}
