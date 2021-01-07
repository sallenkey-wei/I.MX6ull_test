#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h> 
#include <string.h>

int main(int argc, char * argv[]){
	int fd, ret;
	char * file_name;
	unsigned short read_buf[3];
	unsigned short ir, als, ps;

	if(argc != 2){
		printf("Error Usage!\r\n");
	}

	file_name = argv[1];

	fd  = open(file_name, O_RDWR);
	if(fd < 0){
		printf("Can't open file %s \r\n", file_name);
		return -1;
	}

	while(1){
		ret = read(fd, &read_buf, sizeof(read_buf));
		if(ret == sizeof(read_buf)){
			ir = read_buf[0];
			als = read_buf[1];
			ps = read_buf[2];
			printf("ir = %d, als = %d, ps = %d\n", ir, als, ps);
		}
		else{
			printf("read error.\n");
		}
		usleep(200000);
	}

	close(fd);
	return 0;
}
