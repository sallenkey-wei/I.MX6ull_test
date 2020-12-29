#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h> 
#include <string.h>
#include <linux/input.h>

static struct input_event inputevent;

int main(int argc, char * argv[]){
	int fd, ret;
	char * file_name;
	unsigned char write_buf[1];

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
		ret = read(fd, &inputevent, sizeof(inputevent));
		if(ret > 0){
			switch(inputevent.type){
				case EV_KEY:
					if(inputevent.code < BTN_MISC){
						printf("key %d %s\n", inputevent.code, inputevent.value ? "press":"release");
					}
					else{
						printf("button %d %s\n", inputevent.code, inputevent.value ? "press":"release");
					}
					break;
			}
		}
	}

	close(fd);
	return 0;
}
