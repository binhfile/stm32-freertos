#include <stdio.h>
#include <fcntl.h>

#include <signal.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>
#include "rf_mac.h"

volatile int g_isTerminate = 0;
void ISR_INT(int iSig)
{
	printf("\nQuit signal\n");
	g_isTerminate = 1;
	signal(SIGINT, SIG_DFL);	/*^C*/
}
int main(int argc, char** argv){
	int fd;
	int len, i, ret;
	unsigned char tx[128], rx[128], ioc[32];
	struct rf_mac_write_packet *write_pkt;
	struct rf_mac_read_packet  *read_pkt;
	struct pollfd fds[1];
	struct timeval t_ref, t_now;
	int t_diff;
	float speed;

	signal(SIGINT, ISR_INT);
	
	fd = open("/dev/mrf24j40", O_RDWR);
	if(fd < 0){
		printf("open device fail\r\n");
		return (0);
	}
	
	write_pkt = (struct rf_mac_write_packet*)tx;
	read_pkt = (struct rf_mac_read_packet*)rx;
	for(len = 0; len < 8; len ++){
		write_pkt->header.dest_addr[len] = len;
	}
	write_pkt->header.flags.bits.ack_req 	= 0;
	write_pkt->header.flags.bits.broadcast 	= 1;
	write_pkt->header.flags.bits.intra_pan 	= 0;
	write_pkt->header.flags.bits.dest_addr_64bit 	= 0;
	write_pkt->header.flags.bits.src_addr_64bit 	= 1;
	write_pkt->data_len = 100;

	ioc[0] = 0x01;
	ioc[1] = 0x02;
	ret = ioctl(fd, RF_MAC_IOC_WR_SHORT_ADDRESS, ioc);
	if(ret < 0) printf("ioc ret = %d\r\n", ret);
	ioc[0] = 0x01;
	ioc[1] = 0x02;
	ioc[2] = 0x03;
	ioc[3] = 0x04;
	ioc[4] = 0x05;
	ioc[5] = 0x06;
	ioc[6] = 0x07;
	ioc[7] = 0x08;
	ret = ioctl(fd, RF_MAC_IOC_WR_LONG_ADDRESS, ioc);
	if(ret < 0) printf("ioc ret = %d\r\n", ret);
	ioc[0] = 0xFF;
	ioc[1] = 0xFF;
	ret = ioctl(fd, RF_MAC_IOC_WR_PAN_ID, ioc);
	if(ret < 0) printf("ioc ret = %d\r\n", ret);

	gettimeofday(&t_ref);
	for(i = 0; i < 100; i++){
		fds[0].fd = fd;
		fds[0].events = POLLOUT | POLLIN;

		ret = poll(fds, 1, 1000);
		if(ret < 0){
			printf("poll fail %d\r\n", ret);
			break;
		}else if(ret > 0){
			if(fds[0].revents & POLLOUT){
				len = ioctl(fd, RF_MAC_IOC_WR_PACKET, write_pkt);
				if(len < 0)
					printf("[%d] write ret = %d\r\n", i, len);
			}
			else if(fds[0].revents & POLLIN){
				len = ioctl(fd, RF_MAC_IOC_RD_PACKET, read_pkt);
				printf("[%d] read ret = %d\r\n", i, len);
			}
			else if(fds[0].revents & POLLERR){
				printf("[%d] error\r\n");
			}
			else{
				printf("poll not fd\r\n");
			}
		}else{
			printf(".");
		}
		fflush(stdout);
	}
	gettimeofday(&t_now);
	t_diff = t_now.tv_sec * 1000 + t_now.tv_usec / 1000 - (t_ref.tv_sec * 1000 + t_ref.tv_usec / 1000);
	if(t_diff > 0){
		speed = write_pkt->data_len * 100.0f / t_diff * 8;
		printf("diff %u ms speed %.02f kbps (%.02f%%)\r\n",
				t_diff,
				speed, speed*100 / 256.0f);
	}
	sleep(1);
	close(fd);
	printf("exit\r\n");
	return 0;
}
