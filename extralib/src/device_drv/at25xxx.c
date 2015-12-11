#include <at25xxx.h>
#include <spidev.h>
#include <drv_api.h>
#include <string.h>
int at25_init(struct at25_device* dev){
	dev->fd_spi = 0;
	dev->fd_cs  = 0;
}
int at25_destroy(struct at25_device* dev){

}
int at25_read_status(struct at25_device* dev, struct at25_reg_status* status){
	int ret = 0;
	uint8_t u8val;
	struct spi_ioc_transfer tr;
	uint8_t txBuf[2], rxBuf[2];

	tr.tx_buf = (unsigned int)txBuf;
	tr.rx_buf = (unsigned int)rxBuf;
	tr.len    = 2;
	txBuf[0] = at25_ins_rdsr;
	txBuf[1] = 0;
	rxBuf[1] = 0;
	
	u8val = 0;
	write(dev->fd_cs, &u8val, 1);
	ret = ioctl(dev->fd_spi, SPI_IOC_MESSAGE(1), (unsigned int)&tr);
	status->status.Val = rxBuf[1];
	u8val = 1;
	write(dev->fd_cs, &u8val, 1);
	return ret;
}
int at25_write_status(struct at25_device* dev, struct at25_reg_status* status){
	int ret = 0;
	uint8_t u8val;
	struct spi_ioc_transfer tr;
	uint8_t txBuf[2];

	tr.tx_buf = (unsigned int)txBuf;
	tr.rx_buf = 0;
	tr.len    = 2;
	txBuf[0] = at25_ins_wrsr;
	txBuf[1] = status->status.Val;
	
	u8val = 0;
	write(dev->fd_cs, &u8val, 1);
	ret = ioctl(dev->fd_spi, SPI_IOC_MESSAGE(1), (unsigned int)&tr);
	u8val = 1;
	write(dev->fd_cs, &u8val, 1);
	return ret;
}
int at25_enable_write(struct at25_device* dev){
	int ret = 0;
	uint8_t u8val;
	struct spi_ioc_transfer tr;
	uint8_t txBuf[1];

	tr.tx_buf = (unsigned int)txBuf;
	tr.rx_buf = 0;
	tr.len    = 1;
	txBuf[0] = at25_ins_wren;
	
	u8val = 0;
	write(dev->fd_cs, &u8val, 1);
	ret = ioctl(dev->fd_spi, SPI_IOC_MESSAGE(1), (unsigned int)&tr);
	u8val = 1;
	write(dev->fd_cs, &u8val, 1);
	return ret;
}
int at25_disable_write(struct at25_device* dev){
	int ret = 0;
	uint8_t u8val;
	struct spi_ioc_transfer tr;
	uint8_t txBuf[1];

	tr.tx_buf = (unsigned int)txBuf;
	tr.rx_buf = 0;
	tr.len    = 1;
	txBuf[0] = at25_ins_wrdi;
	
	u8val = 0;
	write(dev->fd_cs, &u8val, 1);
	ret = ioctl(dev->fd_spi, SPI_IOC_MESSAGE(1), (unsigned int)&tr);
	u8val = 1;
	write(dev->fd_cs, &u8val, 1);
	return ret;
}
int at25_ioctl(struct at25_device* dev, int request, unsigned int args){
	int ret = -1;
	int *pi;
	switch(request){
		case at25_ioc_set_spi_fd:{
			pi = (int*)args;
			dev->fd_spi = *pi;
			ret = 0;
			break;
		}
		case at25_ioc_set_cs_fd:{
			pi = (int*)args;
			dev->fd_cs = *pi;
			ret = 0;
			break;
		}
	}
	return ret;
}
int at25_write(struct at25_device* dev, const void* payload, int length){
	int ret = 0, i;
	uint8_t u8val;
	struct 	spi_ioc_transfer tr;
	struct at25_reg_status reg_status;
	uint8_t txBuf[at25_page_size+1];
	uint8_t *pu8;
	uint8_t timeout;

	if(length <= 0) return 0;
	if(length > at25_page_size) length = at25_page_size;
	tr.tx_buf = (unsigned int)txBuf;
	tr.rx_buf = 0;
	tr.len = length + 1;
	txBuf[0] = at25_ins_write;
	pu8 = (uint8_t*)payload;
	for(i = 0; i < length; i++){
		txBuf[i+1] = pu8[i];
	}
	timeout = 100;
	// enable write
	at25_enable_write(dev);
	// read status
	at25_read_status(dev, &reg_status);
	if(reg_status.status.bits.wen){
		// cs = 0
		u8val = 0;
		write(dev->fd_cs, &u8val, 1);
		ioctl(dev->fd_spi, SPI_IOC_MESSAGE(1), (unsigned int)&tr);
		// cs = 1
		u8val = 1;
		write(dev->fd_cs, &u8val, 1);
		// wait done
		do{
			at25_read_status(dev, &reg_status);
		}while((reg_status.status.bits.rdy == 1) && (timeout -- > 0));
		if(timeout == 0)
			ret = -2;
		else
			ret = length;
	}else{
		ret = -1;
	}
	return ret;
}
int at25_read(struct at25_device* dev, const void* payload, int max_length){
	int ret = 0, i;
	uint8_t u8val;
	struct 	spi_ioc_transfer tr;
	struct at25_reg_status reg_status;
	uint8_t txBuf[1];
	uint8_t timeout;

	if(max_length <= 0) return 0;
	timeout = 100;
	
	// read status
	at25_read_status(dev, &reg_status);
	if(reg_status.status.bits.rdy == 0){
		// cs = 0
		u8val = 0;
		write(dev->fd_cs, &u8val, 1);

		tr.tx_buf = (unsigned int)txBuf;
		tr.rx_buf = 0;
		tr.len    = 1;	
		txBuf[0] = at25_ins_read;
		ioctl(dev->fd_spi, SPI_IOC_MESSAGE(1), (unsigned int)&tr);
		
		tr.tx_buf = 0;
		tr.rx_buf = (unsigned int)payload;
		tr.len    = max_length;	
		ioctl(dev->fd_spi, SPI_IOC_MESSAGE(1), (unsigned int)&tr);		
		
		// cs = 1
		u8val = 1;
		write(dev->fd_cs, &u8val, 1);
		ret = max_length;
	}else{
		ret = -1;
	}
	return ret;

}
// end of file
