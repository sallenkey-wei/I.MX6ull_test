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
	unsigned keyvalue = 0;

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
		read(fd, &keyvalue, sizeof(keyvalue));
		if(keyvalue == 0xF0)
			printf("KEY0 Pressed, value = %#x \n", keyvalue);
	}

	
	close(fd);
	return 0;
}
