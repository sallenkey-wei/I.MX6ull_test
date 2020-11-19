#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h> 
#include <string.h>
#include <sys/ioctl.h>

#define TIMER_IOCTL_BASE	'Z'

#define	OPEN_TIMER		_IO(TIMER_IOCTL_BASE, 0)
#define	CLOSE_TIMER		_IO(TIMER_IOCTL_BASE, 1)
#define	SET_PERIOR		_IOW(TIMER_IOCTL_BASE, 2, int)


int main(int argc, char * argv[]){
	int fd, ret;
	char * file_name;
	unsigned int cmd, arg;
	char str[100];


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
		printf("Input CMD:");
		ret = scanf("%d", &cmd);
		if(ret != 1){
			gets(str);
		}

		if(cmd == 1)
			cmd = CLOSE_TIMER;
		else if(cmd == 2)
			cmd = OPEN_TIMER;
		else if(cmd == 3){
			cmd = SET_PERIOR;
			printf("Input Timer Period:");
			ret = scanf("%d", &arg);
			if(ret != 1){
				gets(str);
			}
		}
		ioctl(fd, cmd, &arg);
	}
	
	close(fd);
	return 0;
}
