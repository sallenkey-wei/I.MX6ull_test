#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h> 
#include <string.h>
#include <sys/ioctl.h>



int main(int argc, char * argv[]){
	int fd, ret;
	char * file_name;
	int key_value;


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
		ret = read(fd, &key_value, sizeof(key_value));
		if(ret == sizeof(key_value))
			printf("key_value = %d.\n", key_value);

	}
	
	close(fd);
	return 0;
}
