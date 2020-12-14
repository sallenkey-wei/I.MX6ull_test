#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h> 
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>

#define USE_POLL 1

int main(int argc, char * argv[]){
	int fd, ret = 0;
	char * file_name;
	unsigned char key_value;
	fd_set readfds;
	struct timeval timeout;
	unsigned char data;


	if(argc != 2){
		printf("Error Usage!\r\n");
	}

	file_name = argv[1];

#if USE_POLL
	fd  = open(file_name, O_RDWR|O_NONBLOCK);
	if(fd < 0){
		printf("Can't open file %s \r\n", file_name);
		return -1;
	}

	while(1){
		FD_ZERO(&readfds);
		FD_SET(fd, &readfds);
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;
		ret = select(fd + 1, &readfds, NULL, NULL, &timeout);
		switch(ret){
			case 0: /* 超时 */
				break;
			case -1: /* 错误 */
				break;
			default:
				if(FD_ISSET(fd, &readfds)){
					ret = read(fd, &data, sizeof(data));
					if(ret > 0)
						printf("key value = %d\r\n", data);
				}

		}

		ret = read(fd, &key_value, sizeof(key_value));
		if(ret == sizeof(key_value))
			printf("key_value = %d.\n", key_value);

	}
#else
	fd  = open(file_name, O_RDWR);
	if(fd < 0){
		printf("Can't open file %s \r\n", file_name);
		return -1;
	}

	while(1){
		ret = read(fd, &key_value, sizeof(key_value));
		if(ret == sizeof(key_value))
			printf("key_value = %d.\n", key_value);

	}
#endif
	
	close(fd);
	return 0;
}
