#if 1
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include <debug.h>
#include <rf_def.h>

struct device_rf{
	int spi_cs;
	int spi_sck;
	int spi_mosi;
	int spi_miso;

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

static inline int gpio_write_low(int fd){
	lseek(fd, 0, SEEK_SET);
	return write(fd, "0", 1);
}
static inline int gpio_write_high(int fd){
	lseek(fd, 0, SEEK_SET);
	return write(fd, "1", 1);
}
static inline int gpio_read(int fd){
	unsigned char rx;
	lseek(fd, 0, SEEK_SET);
	read(fd, &rx, 1);
	if(rx == '1') return 1;
	return 0;
}
static inline void spi_bitbang_delay(){
	volatile int i = 100;
	while(i--){}
}
static inline unsigned char spi_bitbang_xfer(int sck_fd, int mosi_fd, int miso_fd, unsigned char tx){
	unsigned char rx = 0;
	int i = 7;
	// write low SCK
	gpio_write_low(sck_fd);
	while(i >= 0){
		// write to MOSI
		if(tx & (((unsigned int)1) << i)) gpio_write_high(mosi_fd);
		else gpio_write_low(mosi_fd);
		spi_bitbang_delay();
		spi_bitbang_delay();
		// write SCK to HIGH
		gpio_write_high(sck_fd);
		// read MISO
		spi_bitbang_delay();
		rx |= ((((unsigned char)gpio_read(miso_fd)) & (unsigned char)0x01) << i);
		spi_bitbang_delay();
		gpio_write_low(sck_fd);
		i--;
	}
	return rx;
}

int spi_readwrite(struct device_rf *rf, void* tx, void* rx, int len){
	int ret;
	int i;
	unsigned char* ptx = (unsigned char*)tx;
	unsigned char* prx = (unsigned char*)rx;
	i = 0;
	gpio_write_low(rf->spi_cs);
	spi_bitbang_delay();
	while(i < len){
		prx[i] = spi_bitbang_xfer(rf->spi_sck, rf->spi_mosi, rf->spi_miso, ptx[i]);
		i++;
	}
	gpio_write_high(rf->spi_cs);
	spi_bitbang_delay();
	return 0;
}
unsigned char rf_read_short_reg(struct device_rf *rf, unsigned char reg){
	unsigned char tx[2], rx[2];
	tx[0] 			= reg;
	tx[1]			= 0xCC;
	rx[1] 			= 0;
	spi_readwrite(rf, tx, rx, 2);
	return rx[1];
}
unsigned char rf_read_long_reg(struct device_rf *rf, unsigned short reg){
	unsigned char tx[3], rx[3];
	tx[0] = ((reg >> 3)&0x7F) | 0x80;
	tx[1] = ((reg << 5)&0xE0);
	tx[2] = 0;
	rx[2] = 0;
	spi_readwrite(rf, tx, rx, 3);
	return rx[2];
}
int rf_write_short_reg(struct device_rf *rf, unsigned char reg, unsigned char val, int check){
	unsigned char tx[2], rx[2], fb;
	tx[0] = reg;
	tx[1] = val;
	spi_readwrite(rf, tx, rx, 2);
	if(check){
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
	spi_readwrite(rf, tx, rx, 2);

	if(check){
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
    int cnt = 10;
    while(cnt--){
       sleep(1);
       return 0;
    }
    LREP("EXIT\r\n");
}

int device_init(struct device_fd* pfd){

	struct gpio_info{
		int num;
		int dir;
		int edge;
	};
	struct gpio_info gpios[] = {
			{
					.num = 17,
					.dir = 0,
			},// intr
			{
					.num = 11,
					.dir = 0,
			},// sck
			{
					.num = 10,
					.dir = 0,
			},// mosi
			{
					.num = 9,
					.dir = 1,
					.edge = 0,
			},// miso
			{
					.num = 8,
					.dir = 0,
			},// cs
	};

	int i;
	int fd;
	char buffer[64];
	for(i = 0; i < sizeof(gpios) / sizeof(struct gpio_info); i++){
		fd = open("/sys/class/gpio/export", O_WRONLY);
		if(fd >= 0){
			snprintf(buffer, 63, "%d", gpios[i].num);
			write(fd, buffer, strlen(buffer));
			close(fd);
			snprintf(buffer, 63, "/sys/class/gpio/gpio%d/direction", gpios[i].num);
			fd = open(buffer, O_WRONLY);
			if(fd >= 0){
				if(gpios[i].dir == 0)
					snprintf(buffer, 63, "out");
				else
					snprintf(buffer, 63, "in");
				write(fd, buffer, strlen(buffer));
				close(fd);
			}
			if(gpios[i].dir == 1){
				snprintf(buffer, 63, "/sys/class/gpio/gpio%d/edge", gpios[i].num);
				fd = open(buffer, O_WRONLY);
				if(fd >= 0){
					if(gpios[i].edge == 1) snprintf(buffer, 63, "falling");
					else if(gpios[i].edge == 2) snprintf(buffer, 63, "rising");
					else snprintf(buffer, 63, "none");
					write(fd, buffer, strlen(buffer));
					close(fd);
				}
			}
		}
	}

	pfd->rf.reset_fd = open("/sys/class/gpio/gpio17/value", O_WRONLY);
	pfd->rf.spi_sck  = open("/sys/class/gpio/gpio11/value", O_WRONLY);
	pfd->rf.spi_mosi = open("/sys/class/gpio/gpio10/value", O_WRONLY);
	pfd->rf.spi_miso = open("/sys/class/gpio/gpio9/value", O_RDONLY);
	pfd->rf.spi_cs   = open("/sys/class/gpio/gpio8/value", O_WRONLY);

//	unsigned char tx[32], rx[32];
//	LREP("\r\n");
//	for(i = 0; i < 32; i++){
//		tx[i] = i;
//		rx[i] = 0;
//
//		rx[i] = spi_bitbang_xfer(pfd->rf.spi_sck, pfd->rf.spi_mosi, pfd->rf.spi_miso, tx[i]);
//		if(i % 16 == 0) LREP("\r\n");
//		LREP("%02X ", rx[i]);
//	}
//	LREP("\r\n");
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
