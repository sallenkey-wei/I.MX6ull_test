#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h> 
#include <string.h>


int main(int argc, char * argv[]){
	int fd, retvalue;
	char * file_name;
	unsigned char write_buf[1];

	if(argc != 3){
		printf("Error Usage!\r\n");
	}

	file_name = argv[1];

	fd  = open(file_name, O_RDWR);
	if(fd < 0){
		printf("Can't open file %s \r\n", file_name);
		return -1;
	}

	write_buf[0] = atoi(argv[2]);
	retvalue = write(fd, write_buf, 1);
	if(retvalue < 0)
		printf("app write file %s failed!\r\n", file_name);

	
	for(int i = 0; i < 25; i++){
		sleep(1);
		write_buf[0] ^= 1;
		retvalue = write(fd, write_buf, 1);
		if(retvalue < 0)
			printf("app write file %s failed!\r\n", file_name);
		if(i % 5 == 0)
			printf("app running...\n");
	}
	close(fd);
	return 0;
}
