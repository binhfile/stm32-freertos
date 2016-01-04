#include <stdio.h>
#include <fcntl.h>

#include <signal.h>
#include <unistd.h>

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
		printf("open /dev/gpio_drv_%d fail %d\r\n", pin, fd);
		close(fd_gpio);
		return -1;
	}
	uival = dir;
	ret = ioctl(fd, GPIO_IOC_WR_DIR, &uival);
	printf("set dir %d fail %d\r\n", pin, ret);
	if(ret < 0){
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
	
EXIT:
	close(fd);
	close(fd_gpio);	
	printf("exit\r\n");
	return 0;
}
