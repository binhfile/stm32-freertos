#ifndef AT25XXX_H__
#define AT25XXX_H__
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
#endif
