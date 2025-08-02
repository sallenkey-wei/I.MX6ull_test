#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h> 
#include <string.h>
#include <math.h>
#include <termios.h> 
#include <stdbool.h>
#include <assert.h>
#include "ekf.h"

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
			}
			sscanf(buf, "%f", &iio_data[i].data);
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
				break;
			default:
				break;
			}
		}
	}
	return 0;
}


#define PI acos(-1)

typedef unsigned char u8;

int uart_fd;

int hal_set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop) 
{ 
		struct termios newtio,oldtio; 
		if  ( tcgetattr( fd,&oldtio)  !=  0) {  
				perror("SetupSerial 1");
				printf("tcgetattr( fd,&oldtio) -> %d\n",tcgetattr( fd,&oldtio)); 
				return -1; 
		} 
		bzero( &newtio, sizeof( newtio ) ); 
		newtio.c_cflag  |=  CLOCAL | CREAD;  
		newtio.c_cflag &= ~CSIZE;  
		switch( nBits ) 
		{ 
			case 7: 
					newtio.c_cflag |= CS7; 
				break; 
			
			case 8: 
					newtio.c_cflag |= CS8; 
				break; 
		} 
		switch( nEvent ) 
		{ 
			case 'o':
			case 'O':
					newtio.c_cflag |= PARENB; 
					newtio.c_cflag |= PARODD; 
					newtio.c_iflag |= (INPCK | ISTRIP); 
				break; 
			case 'e':
			case 'E': 
					newtio.c_iflag |= (INPCK | ISTRIP); 
					newtio.c_cflag |= PARENB; 
					newtio.c_cflag &= ~PARODD; 
				break;
			case 'n':
			case 'N':  
					newtio.c_cflag &= ~PARENB; 
				break;
			default:
				break;
		} 
		switch( nSpeed ) 
		{ 
			case 2400: 
					cfsetispeed(&newtio, B2400); 
					cfsetospeed(&newtio, B2400); 
				break; 
			case 4800: 
					cfsetispeed(&newtio, B4800); 
					cfsetospeed(&newtio, B4800); 
				break; 
			case 9600: 
					cfsetispeed(&newtio, B9600); 
					cfsetospeed(&newtio, B9600); 
				break; 
			case 115200: 
					cfsetispeed(&newtio, B115200); 
					cfsetospeed(&newtio, B115200); 
				break; 
			case 460800: 
					cfsetispeed(&newtio, B460800); 
					cfsetospeed(&newtio, B460800); 
				break; 
			case 500000: 
					cfsetispeed(&newtio, B500000); 
					cfsetospeed(&newtio, B500000); 
				break; 
			default: 
					cfsetispeed(&newtio, B9600); 
					cfsetospeed(&newtio, B9600); 
				break; 
		} 
		if( nStop == 1 ) 
			newtio.c_cflag &=  ~CSTOPB; 
		else if ( nStop == 2 ) 
			newtio.c_cflag |=  CSTOPB; 
		newtio.c_cc[VTIME]  = 0; 
		newtio.c_cc[VMIN] = 0; 
		tcflush(fd,TCIFLUSH); 
		if((tcsetattr(fd,TCSANOW,&newtio))!=0) 
		{ 
			perror("com set error"); 
			return -1; 
		} 
		return 0; 
} 


int uart_init(){
	int fd, ret;
	char *uart3 = "/dev/ttymxc2";

	fd = open(uart3,O_RDWR|O_NOCTTY|O_NONBLOCK);//O_NONBLOCK设置为非阻塞模式，在read是不会阻塞模式，在读的时候将read放在while循环中
	if(fd == -1){
		printf("open error.\n");
		return fd;
	}
	else
	{ 
	  //设这串口
		ret = hal_set_opt(fd,115200, 8, 'N', 1); 
		if(ret == -1){
			return ret;
		}
		return fd;
	}

}

void usart1_niming_report(u8 fun,u8*data,u8 len)
{
	u8 send_buf[32];
	u8 i;
	if(len>28)return;	//最多28字节数据 
	send_buf[len+3]=0;	//校验数置零
	send_buf[0]=0X88;	//帧头
	send_buf[1]=fun;	//功能字
	send_buf[2]=len;	//数据长度
	for(i=0;i<len;i++)send_buf[3+i]=data[i];			//复制数据
	for(i=0;i<len+3;i++)send_buf[len+3]+=send_buf[i];	//计算校验和	
	write(uart_fd, send_buf, 32);	//发送数据到串口1 
}

//通过串口1上报结算后的姿态数据给电脑
//aacx,aacy,aacz:x,y,z三个方向上面的加速度值
//gyrox,gyroy,gyroz:x,y,z三个方向上面的陀螺仪值
//roll:横滚角.单位0.01度。 -18000 -> 18000 对应 -180.00  ->  180.00度
//pitch:俯仰角.单位 0.01度。-9000 - 9000 对应 -90.00 -> 90.00 度
//yaw:航向角.单位为0.1度 0 -> 3600  对应 0 -> 360.0度
void usart_report_imu(short aacx,short aacy,short aacz,short gyrox,short gyroy,short gyroz,short roll,short pitch,short yaw)
{
	u8 tbuf[28]; 
	u8 i;
	for(i=0;i<28;i++)tbuf[i]=0;//清0
	tbuf[0]=(aacx>>8)&0XFF;
	tbuf[1]=aacx&0XFF;
	tbuf[2]=(aacy>>8)&0XFF;
	tbuf[3]=aacy&0XFF;
	tbuf[4]=(aacz>>8)&0XFF;
	tbuf[5]=aacz&0XFF; 
	tbuf[6]=(gyrox>>8)&0XFF;
	tbuf[7]=gyrox&0XFF;
	tbuf[8]=(gyroy>>8)&0XFF;
	tbuf[9]=gyroy&0XFF;
	tbuf[10]=(gyroz>>8)&0XFF;
	tbuf[11]=gyroz&0XFF;	
	tbuf[18]=(roll>>8)&0XFF;
	tbuf[19]=roll&0XFF;
	tbuf[20]=(pitch>>8)&0XFF;
	tbuf[21]=pitch&0XFF;
	tbuf[22]=(yaw>>8)&0XFF;
	tbuf[23]=yaw&0XFF;
	usart1_niming_report(0XAF,tbuf,28);//飞控显示帧,0XAF
} 


int main(int argc, char * argv[]){
	int ret, i;
	float gyro_x_adc, gyro_y_adc, gyro_z_adc; 
	float accel_x_adc, accel_y_adc, accel_z_adc; 
	float temp_adc;

	float gyro_x_act, gyro_y_act, gyro_z_act; 
	float accel_x_act, accel_y_act, accel_z_act;
	float temp_act;
	float roll, pitch, yaw;

	IMU_init();

	uart_fd = uart_init();
	if(uart_fd < 0){
		printf("Open uart failed.\n");
	}

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
		gyro_x_act = gyro_x_adc * icm_20608_iio_data[INV_ICM20608_GYRO_SCALE].data;
		gyro_y_act = gyro_y_adc * icm_20608_iio_data[INV_ICM20608_GYRO_SCALE].data;
		gyro_z_act = gyro_z_adc * icm_20608_iio_data[INV_ICM20608_GYRO_SCALE].data;
		accel_x_act = accel_x_adc * icm_20608_iio_data[INV_ICM20608_ACCL_SCALE].data;
		accel_y_act = accel_y_adc * icm_20608_iio_data[INV_ICM20608_ACCL_SCALE].data;
		accel_z_act = accel_z_adc * icm_20608_iio_data[INV_ICM20608_ACCL_SCALE].data;
		//temp_act = ((temp_adc) - 25 ) / icm_20608_iio_data[INV_ICM20608_TEMP_SCALE].data + 25;
		IMU_Update(gyro_x_act, gyro_y_act, gyro_z_act, accel_x_act, accel_y_act, accel_z_act, &roll, &pitch, &yaw);
		usart_report_imu(accel_x_act,accel_y_act,accel_z_act,gyro_x_act,gyro_y_act,gyro_z_act,(int)(roll*100),(int)(pitch*100),(int)(yaw*10));
		
		usleep(SAMPLE_PERIOD * 1000);
	}

	return 0;
}
