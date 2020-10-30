#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h> 
#include <string.h>

static char usrdata[] = {"usr data!"};

int main(int argc, char * argv[]){
	int fd, retvalue;
	char * file_name;
	char read_buf[100], write_buf[100];

	if(argc != 3){
		printf("Error Usage!\r\n");
	}

	file_name = argv[1];

	fd  = open(file_name, O_RDWR);
	if(fd < 0){
		printf("Can't open file %s \r\n", file_name);
		return -1;
	}
	if(atoi(argv[2]) == 1){
		retvalue = read(fd, read_buf, 50);
		if(retvalue < 0)
			printf("app read file %s failed!\r\n", file_name);
		else
			printf("app read data:%s\r\n", read_buf);
	}

	if(atoi(argv[2]) == 2){
		memcpy(write_buf, usrdata, sizeof(usrdata));
		retvalue = write(fd, write_buf, 50);
		if(retvalue < 0)
			printf("app write file %s failed!\r\n", file_name);

	}

	retvalue = close(fd);
	if(retvalue < 0){
		printf("Can't close file %s\n", file_name);
		return -1;
	}
	return 0;
}
