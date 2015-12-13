/*
 * at93c.h
 *
 *  Created on: Dec 12, 2015
 *      Author: dev
 */

#ifndef INCLUDE_DEVICE_DRV_AT93C_H_
#define INCLUDE_DEVICE_DRV_AT93C_H_

struct at93c_device{
	int fd_cs;
	int fd_sck;
	int fd_mosi;
	int fd_miso;
};
enum at93c_ioc{
	at93c_ioc_set_fd = 1,	// args = &at93c_set_fd_param
	at93c_ioc_erase_all,
};
struct at93c_set_fd_param{
	int fd_cs;
	int fd_sck;
	int fd_mosi;
	int fd_miso;
};
#define AT93C66_SIZE	(512)

int at93c_init(struct at93c_device* dev);
int at93c_destroy(struct at93c_device* dev);
int at93c_ioctl(struct at93c_device* dev, int request, unsigned int args);
int at93c_read(struct at93c_device* dev, unsigned int address, void* payload, int payload_max_len);
int at93c_write(struct at93c_device* dev, unsigned int address, void* payload, int payload_len);

#endif /* INCLUDE_DEVICE_DRV_AT93C_H_ */
