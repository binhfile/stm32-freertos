#if 1
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include <debug.h>
#include <rf_def.h>

#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <errno.h>

struct device_rf{
	int spi_fd;
	int intr_fd;
	int reset_fd;
};
struct device_eeprom{
	int cs_fd;
	int sck_fd;
	int di_fd;
	int do_fd;
};
struct device_fd{
	struct device_rf rf;
	struct device_eeprom eeprom;
};

struct device_fd g_devFD;

int device_init(struct device_fd* fd);

int gpio_write_low(int fd){
	return write(fd, "0", 1);
}
int gpio_write_high(int fd){
	return write(fd, "1", 1);
}
int spi_readwrite(int fd, void* tx, void* rx, int len){
	struct spi_ioc_transfer xfer;
	int ret;
	xfer.tx_buf			= (unsigned long)tx;
	xfer.rx_buf 		= (unsigned long)rx;;
	xfer.bits_per_word 	= 8;
	xfer.speed_hz 		= 500000;
	xfer.len			= len;
	xfer.cs_change		= 1;
	xfer.delay_usecs	= 100;
	ret = ioctl(fd, SPI_IOC_MESSAGE(1), (unsigned int)&xfer);
	if(ret < 0) LREP("ioctl %d len %d tx %p rx %p failed %d\r\n", fd, len, tx, rx, ret);
	return 0;
}
unsigned char rf_read_short_reg(struct device_rf *rf, unsigned char reg){
	unsigned char tx[2], rx[2];
	tx[0] 			= reg;
	tx[1]			= 0;
	rx[1] 			= 0;
	return spi_readwrite(rf->spi_fd, tx, rx, 2);
	return rx[1];
}
unsigned char rf_read_long_reg(struct device_rf *rf, unsigned short reg){
	unsigned char tx[3], rx[3];
	tx[0] = ((reg >> 3)&0x7F) | 0x80;
	tx[1] = ((reg << 5)&0xE0);
	tx[2] = 0;
	rx[2] = 0;
	return spi_readwrite(rf->spi_fd, tx, rx, 3);
	return rx[2];
}
int rf_write_short_reg(struct device_rf *rf, unsigned char reg, unsigned char val, int check){
	unsigned char tx[2], rx[2], fb;
	tx[0] = reg;
	tx[1] = val;
	spi_readwrite(rf->spi_fd, tx, rx, 2);
	if(check){
		usleep(1000* 10);
		fb = rf_read_short_reg(rf, reg & (~((unsigned char)0x01)));
		if(fb != val)
			LREP("PHY set reg %02X=%02X failed, read back %02X\r\n", reg, val, fb);
	}
	return 0;
}
int rf_write_long_reg(struct device_rf *rf, unsigned short reg, unsigned char val, int check){
	unsigned char tx[3], rx[3], fb;
	tx[0] = (((reg >> 3))&0x7F) | 0x80;
	tx[1] = (((reg << 5))&0xE0) | 0x10;
	tx[2] = val;
	spi_readwrite(rf->spi_fd, tx, rx, 2);

	if(check){
		usleep(1000* 10);
		fb = rf_read_short_reg(rf, reg);
		if(fb != val)
			LREP("PHY set reg %04X=%02X failed, read back %02X\r\n", reg, val, fb);
	}
	return 0;
}

int main(void)
{
	unsigned char ret;
	int i;

    debug_init();
    device_init(&g_devFD);

    // hard reset
    gpio_write_low(g_devFD.rf.reset_fd);
    usleep(1000*100);
    gpio_write_high(g_devFD.rf.reset_fd);
    usleep(1000*100);
    // soft reset
    rf_write_short_reg(&g_devFD.rf, PHY_WRITE_SOFTRST, 0x07, 0);
    int timeout = 1000;
    do
    {
    	usleep(1000* 10);
        ret = rf_read_short_reg(&g_devFD.rf, PHY_READ_SOFTRST);
        timeout-=10;
    } while (((ret & 0x07) != (unsigned char) 0x00) && (timeout > 0));
    usleep(1000 * 100);
    if(timeout <=0 ) LREP("Timeout\r\n");
    //else
    {
    	rf_write_short_reg(&g_devFD.rf, PHY_WRITE_SADRL, 12, 1);
    	rf_write_short_reg(&g_devFD.rf, PHY_WRITE_SADRH, 34, 1);
    	rf_write_short_reg(&g_devFD.rf, PHY_WRITE_PANIDL, 56, 1);
    	rf_write_short_reg(&g_devFD.rf, PHY_WRITE_PANIDH, 78, 1);

    	for (i = 0; i < (unsigned char) 8; i++)
    	{
    		rf_write_short_reg(&g_devFD.rf, PHY_WRITE_EADR0 + i * 2, i, 1);
    	}
    }

    while(1){
        sleep(1);
        return 0;
    }
    bcm2835_spi_end();
    bcm2835_close();
    LREP("EXIT\r\n");
}

int device_init(struct device_fd* pfd){

	int fd = open("/sys/class/gpio/export", O_WRONLY);
	if(fd){
		write(fd, "17", 2);
		close(fd);
	}
	fd = open("/sys/class/gpio/gpio17/direction", O_WRONLY);
	if(fd){
		write(fd, "out", 3);
		close(fd);
	}
	pfd->rf.reset_fd = open("/sys/class/gpio/gpio17/value", O_WRONLY);
	pfd->rf.spi_fd = open("/dev/spidev0.0", O_RDWR);
	if(pfd->rf.spi_fd < 0){
		LREP("open spi device '%s' failed %d\r\n", "/dev/spidev0.0", pfd->rf.spi_fd);
	}
	else{
		LREP("pfd->rf.spi_fd=%d\r\n", pfd->rf.spi_fd);
		unsigned int uival = SPI_MODE_0;
		if(ioctl(pfd->rf.spi_fd, SPI_IOC_WR_MODE, (unsigned int)&uival) != 0) LREP("ioctl spi mode failed\r\n");
		uival = SPI_MODE_0;
		if(ioctl(pfd->rf.spi_fd, SPI_IOC_RD_MODE, (unsigned int)&uival) != 0) LREP("ioctl spi mode failed\r\n");
		uival = 1000000;
		if(ioctl(pfd->rf.spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, (unsigned int)&uival) != 0) LREP("ioctl spi speed failed\r\n");
		uival = 1000000;
		if(ioctl(pfd->rf.spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, (unsigned int)&uival) != 0) LREP("ioctl spi speed failed\r\n");
		uival = 8;
		if(ioctl(pfd->rf.spi_fd, SPI_IOC_WR_BITS_PER_WORD, (unsigned int)&uival) != 0) LREP("ioctl spi bpw failed\r\n");
		uival = 8;
		if(ioctl(pfd->rf.spi_fd, SPI_IOC_RD_BITS_PER_WORD, (unsigned int)&uival) != 0) LREP("ioctl spi bpw failed\r\n");
	}
    return 0;
}
#else
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#define ZIGBEE_SPI_NUM    0            // SPI port number used (by default 0)
#define ZIGBEE_SPI_FREQ   1000000      // example with 1MHz CLK line
#define ZIGBEE_SPI_BYTE_PER_WORD 8     // 8 Bits per word
demoSPI() {
    int mySPI;  // spidev file descriptor
    // Assuming spi_bcm2708 module has already been loaded
    if ( ZIGBEE_SPI_NUM == 0 ) {
        mySPI = open("/dev/spidev0.0", O_RDWR);
    } else {
        mySPI = open("/dev/spidev0.1", O_RDWR);
    }
    if ( mySPI < 0 ) {
    	printf("failed\r\n");
        return -1;
    }

    //SPI_MODE_0 (0,0)     CPOL=0 (Clock Idle low level), CPHA=0 (SDO transmit/change edge active to idle)
    int spiWRMode = SPI_MODE_0;
    int spiRDMode = SPI_MODE_0;
    int spiBitsPerWord = ZIGBEE_SPI_BYTE_PER_WORD;
    int spiSpeed = ZIGBEE_SPI_FREQ;


    if (   ( ioctl(mySPI, SPI_IOC_WR_MODE, &spiWRMode) < 0 )
        || ( ioctl(mySPI, SPI_IOC_WR_BITS_PER_WORD, &spiBitsPerWord) < 0 )
        || ( ioctl(mySPI, SPI_IOC_WR_MAX_SPEED_HZ, &spiSpeed)  < 0 )
        || ( ioctl(mySPI, SPI_IOC_RD_BITS_PER_WORD, &spiBitsPerWord) < 0 )
        || ( ioctl(mySPI, SPI_IOC_RD_MODE, &spiRDMode) < 0 )
        || ( ioctl(mySPI, SPI_IOC_RD_MAX_SPEED_HZ, &spiSpeed)  < 0 )
        ) {
    	printf("set fail\r\n");
        return -1;
    }

    // Read data @ address 0x12
    unsigned char buff[2];
    buff[0] = (0x0 << 7) | (0x12 << 1 ) | 0x0;
    buff[1] = 0x0;

    struct spi_ioc_transfer spi[1];
    spi[0].tx_buf        = (unsigned long)(buff) ;  // transmit from "data"
    spi[0].rx_buf        = (unsigned long)(buff) ;  // receive into "data"
    spi[0].len           = 2;
    spi[0].speed_hz      = ZIGBEE_SPI_FREQ;
    spi[0].bits_per_word = ZIGBEE_SPI_BYTE_PER_WORD;
    spi[0].cs_change     = 0;                       // this keep CS active between the different transfers
    spi[0].delay_usecs   = 0;                       // delay between two transfer

    int ret = ioctl(mySPI, SPI_IOC_MESSAGE(1), &spi);
    if ( ret < 0 ) {
    	printf("ioctl fail %d\r\n", ret);
        return -1;
    }
    printf(" # Read value %d \n",buff[1]);
    return 0;
}
int main(){
	demoSPI();
	return 0;
}
#endif
