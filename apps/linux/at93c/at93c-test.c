#include <stdio.h>
#include <fcntl.h>

#include <signal.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>
#include <pthread.h>
#include "at93c.h"

volatile int g_isTerminate = 0;
void ISR_INT(int iSig)
{
	printf("\nQuit signal\n");
	g_isTerminate = 1;
	signal(SIGINT, SIG_DFL);	/*^C*/
}
int main(int argc, char** argv){
	int fd;
	int len, i;
	unsigned char buff[512];
	unsigned int size = 0;

	signal(SIGINT, ISR_INT);
	
	fd = open("/dev/at93c.0", O_RDWR);
	if(fd < 0){
		printf("open device fail\r\n");
		return (0);
	}
	len = ioctl(fd, AT93C_IOC_RD_NAME, buff);
	printf("device name = %s ret[%d]\r\n", buff, len);
	len = ioctl(fd, AT93C_IOC_RD_SIZE, &size);
	printf("device size = %d ret[%d]\r\n", size, len);

	lseek(fd, 0, SEEK_SET);
	len = read(fd, buff, 32);
	printf("read %d\r\n", len);
	for(i = 0; i < len; i++){
		if(i%16 == 0) printf("\r\n");
		printf("%02X ", buff[i]);
	}
	printf("\r\n");

	lseek(fd, 16, SEEK_SET);
	buff[0] = '1';
	buff[1] = '2';
	buff[2] = '3';
	buff[3] = '4';
	len = write(fd, buff, 4); printf("write %d\r\n", len);
	lseek(fd, 0, SEEK_SET);
	len = read(fd, buff, 512);
	printf("read %d\r\n", len);
	for(i = 0; i < len; i++){
		if(i%16 == 0) printf("\r\n");
		printf("%02X ", buff[i]);
	}
	printf("\r\n");

	close(fd);
	printf("exit\r\n");
	return 0;
}
