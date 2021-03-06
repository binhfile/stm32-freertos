#include <stdio.h>
#include <fcntl.h>

#include <signal.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>

#include "gpio.h"

volatile int g_isTerminate = 0;
void ISR_INT(int iSig)
{
	printf("\nQuit signal\n");
	g_isTerminate = 1;
	signal(SIGINT, SIG_DFL);	/*^C*/
}
int main(int argc, char** argv){
	if(argc < 3){
		printf("Usage: %s pin_num in/out [rising/falling/both/none]\r\n", argv[0]);
		return 0;
	}
	signal(SIGINT, ISR_INT);
	
	const char* sz_num = argv[1];
	const char* sz_dir = argv[2];
	const char* sz_intr = "none";
	if(argc >= 4) sz_intr = argv[3];
	int pin = atoi(sz_num);
	enum GPIO_DIR dir = GPIO_DIR_INPUT;
	enum GPIO_INTR intr = GPIO_INTR_NONE;
	
	if(strcmp(sz_dir, "out") == 0) dir = GPIO_DIR_OUTPUT;
	if(strcmp(sz_intr, "rising") == 0) intr = GPIO_INTR_RISING;
	if(strcmp(sz_intr, "falling") == 0) intr = GPIO_INTR_FALLING;
	if(strcmp(sz_intr, "both") == 0) intr = GPIO_INTR_BOTH;
		
	printf("Pin %d dir %d interrupt %d\r\n", pin, dir, intr);
	
	int ret;
	unsigned int uival;
	int fd_gpio = open("/dev/gpio_drv", O_WRONLY);
	if(fd_gpio < 0){
		printf("open /dev/gpio_drv fail\r\n");
		return -1;
	}
	uival = pin;
	ret = ioctl(fd_gpio, GPIO_IOC_EXPORT_PIN, &uival);
	printf("export pin %d ret %d\r\n", uival, ret);
	if(ret < 0){
		close(fd_gpio);
		return -1;
	}
	char buffer[32];
	snprintf(buffer, 31, "/dev/gpio_drv_%d", pin);
	int fd = open(buffer, O_RDWR);
	if(fd < 0){
		printf("open %s fail %d\r\n", buffer, fd);
		close(fd_gpio);
		return -1;
	}
	uival = dir;
	ret = ioctl(fd, GPIO_IOC_WR_DIR, &uival);	
	if(ret < 0){
		printf("set dir %d fail %d\r\n", pin, ret);
		goto EXIT;		
	}
	
	if(dir == GPIO_DIR_OUTPUT){
		unsigned char ucval = 0;
		while(!g_isTerminate){
			ret = write(fd, &ucval, 1);
			printf("write %d ret %d\r\n", ucval, ret);
			ucval = !ucval;
			sleep(1);
		}
	}
	else if(dir == GPIO_DIR_INPUT){
		unsigned char ucval = 0;		
		uival = intr;
		ret = ioctl(fd, GPIO_IOC_WR_INTR, &uival);	
		if(ret < 0){
			printf("set interrupt %d fail %d\r\n", pin, ret);
			goto EXIT;		
		}
		if(intr == GPIO_INTR_NONE){
			printf("no select\r\n");
			while(!g_isTerminate){
				ucval = 0;
				ret = read(fd, &ucval, 1);
				printf("read %d len %d\r\n", ucval, ret);
				sleep(1);
				fflush(stdout);
			}
		}else{
			struct pollfd fds[1];
			struct timeval timeout;
			
			fds[0].fd = fd;
			fds[0].events = POLLIN | POLLRDNORM;
			
			printf("use poll\r\n");
			while(!g_isTerminate){
				ret = poll(fds, 1, 1000);
				if(ret < 0){
					printf("poll fail %d\r\n", ret);
					break;
				}else if(ret > 0){
					if(fds[0].revents & POLLIN){
						ucval = 0;
						ret = read(fd, &ucval, 1);
						printf("read %d len %d\r\n", ucval, ret);
					}else{
						printf("poll not fd\r\n");
					}
				}else{
					printf(".");
				}
				fflush(stdout);
			}
		}
	}
	
EXIT:
	close(fd);
	close(fd_gpio);	
	printf("exit\r\n");
	return 0;
}
