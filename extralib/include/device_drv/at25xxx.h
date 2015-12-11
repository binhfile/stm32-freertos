#ifndef AT25XXX_H__
#define AT25XXX_H__
#include <stdint.h>
struct at25_device{
	int fd_spi;
	int fd_cs;
};
enum at25_ioc{
	at25_ioc_set_spi_fd = 1,
	at25_ioc_set_cs_fd
};
int at25_init(struct at25_device* dev);
int at25_destroy(struct at25_device* dev);
int at25_ioctl(struct at25_device* dev, int request, unsigned int args);
int at25_write(struct at25_device* dev, const void* payload, int length);
int at25_read(struct at25_device* dev, const void* payload, int max_length);

#define at25_ins_wren	0b00000110	// write enable
#define at25_ins_wrdi	0b00000100	// write disable
#define at25_ins_rdsr	0b00000101	// read status
#define at25_ins_wrsr	0b00000001	// write status
#define at25_ins_read	0b00000011	// read
#define at25_ins_write	0b00000010	// write
#define at25_page_size	(32)
struct at25_reg_status{
	union{
		uint8_t	Val;
		struct{
			uint8_t rdy : 1;	// ready
			uint8_t wen : 1;	// write enable
			uint8_t bp  : 2;	// block protected
			uint8_t reserver: 3;
			uint8_t wpen: 1;	// enable WP pin
		}bits;
	}status;
};

#endif
