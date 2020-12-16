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
#include <signal.h>

#define USE_POLL 0

static void sigio_signal_func(int signum){
	printf("key release.\n");
}

int main(int argc, char * argv[]){
	int fd, ret = 0;
	char * file_name;
	unsigned char key_value;
	fd_set readfds;
	struct timeval timeout;
	unsigned char data;
	int flags;


	if(argc != 2){
		printf("Error Usage!\r\n");
	}

	file_name = argv[1];

	fd  = open(file_name, O_RDWR);
	if(fd < 0){
		printf("Can't open file %s \r\n", file_name);
		return -1;
	}

	signal(SIGIO, sigio_signal_func);
	fcntl(fd, F_SETOWN, getpid());
	flags = fcntl(fd, F_GETFL); 
	fcntl(fd, F_SETFL, flags | FASYNC);

	while(1){
		sleep(10);
	}
	
	close(fd);
	return 0;
}
