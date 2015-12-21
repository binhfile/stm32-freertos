/*
 * at93c.c
 *
 *  Created on: Dec 12, 2015
 *      Author: dev
 */
#include <at93c.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include "gpio.h"

#define DRV_AT93C_SET_GPIO(fd, val)	{\
	u8val = val;\
	write(fd, &u8val, 1);\
}
// Fosc = 180.000.000
// counter_1us = Fosc * / 1.000.000 / 4 = 45
#define DRV_AT93C_DELAY_SCK(dev) {volatile int __i = 45; while(__i--){}}	// 1us
#define DRV_AT93C_DELAY_CS(dev) {volatile int __i = 45; while(__i--){}}		// 250 ns
#define DRV_AT93C_W_BIT(dev, val)	{\
	DRV_AT93C_SET_GPIO(dev->fd_mosi, val); \
	DRV_AT93C_SET_GPIO(dev->fd_sck, 1); \
	DRV_AT93C_DELAY_SCK(dev); \
	DRV_AT93C_SET_GPIO(dev->fd_sck, 0); \
	DRV_AT93C_DELAY_SCK(dev); \
}
#define DRV_AT93C_R_BIT(dev, ret) {\
	DRV_AT93C_SET_GPIO(dev->fd_sck, 1); \
	DRV_AT93C_DELAY_SCK(dev); \
	read(dev->fd_miso, &ret, 1);\
	DRV_AT93C_SET_GPIO(dev->fd_sck, 0); \
	DRV_AT93C_DELAY_SCK(dev); \
}
#define DRV_AT93C_WAIT_W_DONE(dev) {\
	do{\
		read(dev->fd_miso, &u8val, 1);\
		if(u8val == 0) usleep(1000);\
	}while(u8val == 0);\
}
int at93c_enable_write(struct at93c_device* dev){
	uint8_t u8val;

	// sck = 0
	DRV_AT93C_SET_GPIO(dev->fd_cs, 0);
	DRV_AT93C_SET_GPIO(dev->fd_sck, 0);
	DRV_AT93C_SET_GPIO(dev->fd_mosi, 0);
	// cs = 1
	DRV_AT93C_SET_GPIO(dev->fd_cs, 1);
	DRV_AT93C_DELAY_CS(dev);
	// write 100 110000000
	DRV_AT93C_W_BIT(dev, 1);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);

	DRV_AT93C_W_BIT(dev, 1);
	DRV_AT93C_W_BIT(dev, 1);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);
	// cs = 0
	DRV_AT93C_DELAY_CS(dev);
	DRV_AT93C_SET_GPIO(dev->fd_cs, 0);
	DRV_AT93C_DELAY_CS(dev);
	DRV_AT93C_SET_GPIO(dev->fd_cs, 1);
	DRV_AT93C_DELAY_CS(dev);
	return 0;
}
int at93c_disable_write(struct at93c_device* dev){
	uint8_t u8val;

	// sck = 0
	DRV_AT93C_SET_GPIO(dev->fd_cs, 0);
	DRV_AT93C_SET_GPIO(dev->fd_sck, 0);
	DRV_AT93C_SET_GPIO(dev->fd_mosi, 0);
	// cs = 1
	DRV_AT93C_SET_GPIO(dev->fd_cs, 1);
	DRV_AT93C_DELAY_CS(dev);
	// write 100 000000000
	DRV_AT93C_W_BIT(dev, 1);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);

	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);
	// cs = 0
	DRV_AT93C_DELAY_CS(dev);
	DRV_AT93C_SET_GPIO(dev->fd_cs, 0);
	DRV_AT93C_DELAY_CS(dev);
	DRV_AT93C_SET_GPIO(dev->fd_cs, 1);
	DRV_AT93C_DELAY_CS(dev);
	return 0;
}
int at93c_erase_all(struct at93c_device* dev){
	uint8_t u8val;

	at93c_enable_write(dev);

	DRV_AT93C_SET_GPIO(dev->fd_cs, 0);
	DRV_AT93C_SET_GPIO(dev->fd_sck, 0);
	DRV_AT93C_SET_GPIO(dev->fd_mosi, 0);
	// cs = 1
	DRV_AT93C_SET_GPIO(dev->fd_cs, 1);
	DRV_AT93C_DELAY_CS(dev);
	// write 100 100000000
	DRV_AT93C_W_BIT(dev, 1);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);

	DRV_AT93C_W_BIT(dev, 1);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);
	// cs = 0
	DRV_AT93C_DELAY_CS(dev);
	DRV_AT93C_SET_GPIO(dev->fd_cs, 0);
	DRV_AT93C_DELAY_CS(dev);
	//wait done
	DRV_AT93C_SET_GPIO(dev->fd_cs, 1);
	DRV_AT93C_DELAY_CS(dev);
	DRV_AT93C_WAIT_W_DONE(dev);
	DRV_AT93C_SET_GPIO(dev->fd_cs, 0);
	DRV_AT93C_DELAY_CS(dev);
	return 0;
}
int at93c_erase(struct at93c_device* dev, unsigned int address){
	uint8_t u8val;

	at93c_enable_write(dev);
	// sck = 0
	DRV_AT93C_SET_GPIO(dev->fd_cs, 0);
	DRV_AT93C_SET_GPIO(dev->fd_sck, 0);
	DRV_AT93C_SET_GPIO(dev->fd_mosi, 0);
	// cs = 1
	DRV_AT93C_SET_GPIO(dev->fd_cs, 1);
	DRV_AT93C_DELAY_CS(dev);
	// write 111 A8-A0
	DRV_AT93C_W_BIT(dev, 1);
	DRV_AT93C_W_BIT(dev, 1);
	DRV_AT93C_W_BIT(dev, 1);

	DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 8) & (uint16_t)0x0001));//8
	DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 7) & (uint16_t)0x0001));//7
	DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 6) & (uint16_t)0x0001));//6
	DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 5) & (uint16_t)0x0001));//5
	DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 4) & (uint16_t)0x0001));//4
	DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 3) & (uint16_t)0x0001));//3
	DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 2) & (uint16_t)0x0001));//2
	DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 1) & (uint16_t)0x0001));//1
	DRV_AT93C_W_BIT(dev, (((uint16_t)address) & (uint16_t)0x0001));//0
	// cs = 0
	DRV_AT93C_DELAY_CS(dev);
	DRV_AT93C_SET_GPIO(dev->fd_cs, 0);
	DRV_AT93C_DELAY_CS(dev);
	//wait done
	DRV_AT93C_SET_GPIO(dev->fd_cs, 1);
	DRV_AT93C_DELAY_CS(dev);
	DRV_AT93C_WAIT_W_DONE(dev);
	DRV_AT93C_SET_GPIO(dev->fd_cs, 0);
	DRV_AT93C_DELAY_CS(dev);
	return 0;
}
int at93c_fill(struct at93c_device* dev, unsigned char pattern){
	uint8_t u8val;

	at93c_enable_write(dev);
	// sck = 0
	DRV_AT93C_SET_GPIO(dev->fd_cs, 0);
	DRV_AT93C_SET_GPIO(dev->fd_sck, 0);
	DRV_AT93C_SET_GPIO(dev->fd_mosi, 0);
	// cs = 1
	DRV_AT93C_SET_GPIO(dev->fd_cs, 1);
	DRV_AT93C_DELAY_CS(dev);
	// write 111 A8-A0
	DRV_AT93C_W_BIT(dev, 1);
	DRV_AT93C_W_BIT(dev, 0);
	DRV_AT93C_W_BIT(dev, 0);

	DRV_AT93C_W_BIT(dev, 0);//8
	DRV_AT93C_W_BIT(dev, 1);//7
	DRV_AT93C_W_BIT(dev, 0);//6
	DRV_AT93C_W_BIT(dev, 0);//5
	DRV_AT93C_W_BIT(dev, 0);//4
	DRV_AT93C_W_BIT(dev, 0);//3
	DRV_AT93C_W_BIT(dev, 0);//2
	DRV_AT93C_W_BIT(dev, 0);//1
	DRV_AT93C_W_BIT(dev, 0);//0
	// D7-D0
	DRV_AT93C_W_BIT(dev, ((pattern & (((uint8_t)1) << 7)) != 0));//7
	DRV_AT93C_W_BIT(dev, ((pattern & (((uint8_t)1) << 6)) != 0));//6
	DRV_AT93C_W_BIT(dev, ((pattern & (((uint8_t)1) << 5)) != 0));//5
	DRV_AT93C_W_BIT(dev, ((pattern & (((uint8_t)1) << 4)) != 0));//4
	DRV_AT93C_W_BIT(dev, ((pattern & (((uint8_t)1) << 3)) != 0));//3
	DRV_AT93C_W_BIT(dev, ((pattern & (((uint8_t)1) << 2)) != 0));//2
	DRV_AT93C_W_BIT(dev, ((pattern & (((uint8_t)1) << 1)) != 0));//1
	DRV_AT93C_W_BIT(dev, ((pattern & (((uint8_t)1) << 0)) != 0));//0
	// cs = 0
	DRV_AT93C_DELAY_CS(dev);
	DRV_AT93C_SET_GPIO(dev->fd_cs, 0);
	DRV_AT93C_DELAY_CS(dev);
	//wait done
	DRV_AT93C_SET_GPIO(dev->fd_cs, 1);
	DRV_AT93C_DELAY_CS(dev);
	DRV_AT93C_WAIT_W_DONE(dev);
	DRV_AT93C_DELAY_CS(dev);
	return 0;
}

int at93c_init(struct at93c_device* dev){

	return 0;
}
int at93c_destroy(struct at93c_device* dev){
	return 0;
}
int at93c_ioctl(struct at93c_device* dev, int request, unsigned int args){
	int ret = -1;
	struct at93c_set_fd_param *set_fd;

	switch(request){
		case at93c_ioc_set_fd:{
			set_fd = (struct at93c_set_fd_param*)args;
			dev->fd_cs = set_fd->fd_cs;
			dev->fd_sck = set_fd->fd_sck;
			dev->fd_miso = set_fd->fd_miso;
			dev->fd_mosi = set_fd->fd_mosi;
			ret = 0;
			break;
		}
		case at93c_ioc_erase_all:{
			at93c_erase_all(dev);
			ret = 0;
			break;
		}
	}
	return ret;
}
int at93c_read(struct at93c_device* dev, unsigned int address, void* payload, int payload_max_len){
	uint8_t u8val, u8read, u8total;
	uint8_t *pu8;
	int ret;

	pu8 = (uint8_t*)payload;
	ret = payload_max_len;
	// sck = 0
	DRV_AT93C_SET_GPIO(dev->fd_cs, 0);
	DRV_AT93C_SET_GPIO(dev->fd_sck, 0);
	DRV_AT93C_SET_GPIO(dev->fd_mosi, 0);
	// cs = 1
	DRV_AT93C_SET_GPIO(dev->fd_cs, 1);
	DRV_AT93C_DELAY_CS(dev);
	// write 111 A8-A0
	DRV_AT93C_W_BIT(dev, 1);
	DRV_AT93C_W_BIT(dev, 1);
	DRV_AT93C_W_BIT(dev, 0);

	DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 8) & (uint16_t)0x0001));//8
	DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 7) & (uint16_t)0x0001));//7
	DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 6) & (uint16_t)0x0001));//6
	DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 5) & (uint16_t)0x0001));//5
	DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 4) & (uint16_t)0x0001));//4
	DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 3) & (uint16_t)0x0001));//3
	DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 2) & (uint16_t)0x0001));//2
	DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 1) & (uint16_t)0x0001));//1
	DRV_AT93C_W_BIT(dev, (((uint16_t)address) & (uint16_t)0x0001));//0
	// read data
	while(payload_max_len > 0){
		u8total = 0;
		DRV_AT93C_R_BIT(dev, u8read);
		u8total |= (uint8_t)((u8read == 1) ? 1 : 0); u8total =  u8total<<1;
		DRV_AT93C_R_BIT(dev, u8read);
		u8total |= (uint8_t)((u8read == 1) ? 1 : 0); u8total =  u8total<<1;
		DRV_AT93C_R_BIT(dev, u8read);
		u8total |= (uint8_t)((u8read == 1) ? 1 : 0); u8total =  u8total<<1;
		DRV_AT93C_R_BIT(dev, u8read);
		u8total |= (uint8_t)((u8read == 1) ? 1 : 0); u8total =  u8total<<1;
		DRV_AT93C_R_BIT(dev, u8read);
		u8total |= (uint8_t)((u8read == 1) ? 1 : 0); u8total =  u8total<<1;
		DRV_AT93C_R_BIT(dev, u8read);
		u8total |= (uint8_t)((u8read == 1) ? 1 : 0); u8total =  u8total<<1;
		DRV_AT93C_R_BIT(dev, u8read);
		u8total |= (uint8_t)((u8read == 1) ? 1 : 0); u8total =  u8total<<1;
		DRV_AT93C_R_BIT(dev, u8read);
		u8total |= (uint8_t)((u8read == 1) ? 1 : 0);

		*pu8 = u8total;
		pu8++;
		payload_max_len--;
	}
	// cs = 0
	DRV_AT93C_DELAY_CS(dev);
	DRV_AT93C_SET_GPIO(dev->fd_cs, 0);
	DRV_AT93C_DELAY_CS(dev);
	DRV_AT93C_SET_GPIO(dev->fd_cs, 1);
	DRV_AT93C_DELAY_CS(dev);
	return ret;
}
int at93c_write(struct at93c_device* dev, unsigned int address, void* payload, int payload_len){
	uint8_t* pu8;
	uint8_t u8val, u8w;
	int ret;

	ret = payload_len;
	pu8 = (uint8_t*)payload;
	at93c_enable_write(dev);

	while(payload_len > 0){
		u8w = *pu8;
		DRV_AT93C_SET_GPIO(dev->fd_cs, 0);
		DRV_AT93C_SET_GPIO(dev->fd_sck, 0);
		DRV_AT93C_SET_GPIO(dev->fd_mosi, 0);
		DRV_AT93C_DELAY_CS(dev);
		// cs = 1
		DRV_AT93C_SET_GPIO(dev->fd_cs, 1);
		DRV_AT93C_DELAY_CS(dev);
		// write 111 A8-A0
		DRV_AT93C_W_BIT(dev, 1);
		DRV_AT93C_W_BIT(dev, 0);
		DRV_AT93C_W_BIT(dev, 1);

		DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 8) & (uint16_t)0x0001));//8
		DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 7) & (uint16_t)0x0001));//7
		DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 6) & (uint16_t)0x0001));//6
		DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 5) & (uint16_t)0x0001));//5
		DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 4) & (uint16_t)0x0001));//4
		DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 3) & (uint16_t)0x0001));//3
		DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 2) & (uint16_t)0x0001));//2
		DRV_AT93C_W_BIT(dev, (((uint16_t)address >> 1) & (uint16_t)0x0001));//1
		DRV_AT93C_W_BIT(dev, (((uint16_t)address) & (uint16_t)0x0001));//0
		// D7-D0
		DRV_AT93C_W_BIT(dev, ((u8w >> 7) & (uint8_t)0x01));//7
		DRV_AT93C_W_BIT(dev, ((u8w >> 6) & (uint8_t)0x01));//6
		DRV_AT93C_W_BIT(dev, ((u8w >> 5) & (uint8_t)0x01));//5
		DRV_AT93C_W_BIT(dev, ((u8w >> 4) & (uint8_t)0x01));//4
		DRV_AT93C_W_BIT(dev, ((u8w >> 3) & (uint8_t)0x01));//3
		DRV_AT93C_W_BIT(dev, ((u8w >> 2) & (uint8_t)0x01));//2
		DRV_AT93C_W_BIT(dev, ((u8w >> 1) & (uint8_t)0x01));//1
		DRV_AT93C_W_BIT(dev, ((u8w) & 0x01));//0
		// cs = 0
		DRV_AT93C_DELAY_CS(dev);
		DRV_AT93C_SET_GPIO(dev->fd_cs, 0);
		DRV_AT93C_DELAY_CS(dev);
		//wait done
		DRV_AT93C_SET_GPIO(dev->fd_cs, 1);
		DRV_AT93C_DELAY_CS(dev);
		DRV_AT93C_WAIT_W_DONE(dev);
		DRV_AT93C_SET_GPIO(dev->fd_cs, 0);
		DRV_AT93C_DELAY_CS(dev);
		//
		payload_len--;
		pu8++;
		address++;
	}
	return ret;
}



