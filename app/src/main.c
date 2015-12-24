/**
 * note:
 * 	do not include stdio.h (rx uart not work !!!)
 */
#include "project_config.h"
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <mqueue.h>
#include <string.h>
#include <stdlib.h>

#include <Test.h>
#include <Network.h>
#include <App.h>
#include <setting.h>
#include <at93c.h>
#include <cli.h>
#include <debug.h>

void *Thread_UserInput(void*);
void *Thread_PhyIntr(void*);
void *Thread_MiwiTask(void*);

#if defined(OS_FREERTOS)
#define             APP_THREAD_COUNT    	4
int                 g_fd_led[4]         = {-1};
int                 g_fd_button         = -1;
int					g_fd_rtc			= -1;
mqd_t               g_debug_tx_buffer   = 0;
void *Thread_DebugTX(void*);
#define DEFINE_THREAD(fxn, stack_size, priority) \
	pthread_t thread_##fxn;\
	pthread_attr_t thread_attr_##fxn;\
	{\
    pthread_attr_setstacksize(&thread_attr_##fxn, stack_size);\
    pthread_setschedprio(&thread_##fxn, priority);\
    pthread_create(&thread_##fxn, &thread_attr_##fxn, fxn, 0);\
}
#elif defined(OS_LINUX)
#define             APP_THREAD_COUNT    	3
#define DEFINE_THREAD(fxn, stack_size, priority) \
	pthread_t thread_##fxn;\
	{\
    pthread_create(&thread_##fxn, 0, fxn, 0);\
    g_thread_index++;\
}
#endif

sem_t               g_sem_debug;
sem_t				g_sem_debug_rx;
int                 g_fd_debug_tx          	= -1;
int                 g_fd_debug_rx          	= -1;

struct mac_mrf24j40_open_param  g_rf_mac_open;
struct mac_mrf24j40         	g_rf_mac;
struct network					g_nwk;
struct setting_device       	g_setting_dev;
struct setting_value			g_setting;

uint8_t g_kb_value = 0;
inline uint8_t kb_value(){
    return g_kb_value;
}
int main(void)
{
    int i, ival;
    struct termios2 opt;
    unsigned int uival;
    uint8_t u8aVal[32];

    sem_init(&g_sem_debug, 0, 1);
    sem_init(&g_sem_debug_rx, 0, 0);
    
#if defined(OS_FREERTOS)
    g_debug_tx_buffer = mq_open(0, 128);
    // open gpio
    g_fd_led[0] = open("led-green", 0);
    g_fd_led[1] = open("led-red",   0);
    g_fd_led[2] = open("led-blue", 0);
    g_fd_led[3] = open("led-orange", 0);
    g_fd_button = open("button",    0);
    // open usart
    g_fd_debug_tx = open("debug-dev", O_RDWR);
    if(g_fd_debug_tx >= 0){
        // configure
        ioctl(g_fd_debug_tx, TCGETS2, (unsigned int)&opt);
        opt.c_cc[VMIN]  = 1;
        opt.c_cc[VTIME] = 100;
        opt.c_ispeed = 115200;
        opt.c_ospeed = 115200;
        opt.c_cflag &= ~CBAUD;
        opt.c_cflag |= BOTHER;
        /*     no parity
            1 stop bit
            8 bit data
         */
        opt.c_cflag &= ~CSIZE;
        opt.c_cflag |= CS8;
        opt.c_cflag &= ~CSTOPB;
        opt.c_cflag &= ~PARENB;
        opt.c_iflag &= ~INPCK;

        //opt.c_cflag &= ~CRTSCTS;             // disable hardware flow control CTS/ RTS*/
        opt.c_cflag |= (CLOCAL | CREAD);    // ignore modem controls, enable reading
        opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // raw input
        opt.c_oflag &= ~(OPOST|ONLCR|OCRNL);  // raw output
        opt.c_iflag &= ~(IXON | IXOFF | IXANY | IGNBRK| INLCR| IGNCR| ICRNL); /* disable sofware flow */
        ioctl(g_fd_debug_tx, TCSETS2, (unsigned int)&opt);
        g_fd_debug_rx = g_fd_debug_tx;
    }else{
        while(1){
            LED_TOGGLE(RED);
            usleep(1000 * 100);
        };
    }
    g_fd_rtc = open("rtc0", 0);
    if(g_fd_rtc < 0) LREP("open rtc failed.\r\n");
    // eeprom
    g_setting_dev.dev.fd_cs   = open(DEV_EEPROM_CS_NAME, O_RDWR);
    g_setting_dev.dev.fd_sck  = open(DEV_EEPROM_SCK_NAME, O_RDWR);
    g_setting_dev.dev.fd_mosi = open(DEV_EEPROM_MOSI_NAME, O_RDWR);
    g_setting_dev.dev.fd_miso = open(DEV_EEPROM_MISO_NAME, O_RDWR);
    if(g_setting_dev.dev.fd_cs < 0 ||
            g_setting_dev.dev.fd_sck < 0 ||
            g_setting_dev.dev.fd_mosi < 0 ||
            g_setting_dev.dev.fd_miso < 0){
        LREP("open at93c failed cs:%d sck:%d mosi:%d miso:%d\r\n",
                g_setting_dev.dev.fd_cs, g_setting_dev.dev.fd_sck,
                g_setting_dev.dev.fd_mosi, g_setting_dev.dev.fd_miso);
    }
    g_rf_mac_open.fd_cs = open(DEV_RF_CS_NAME, O_WRONLY);
    if(g_rf_mac_open.fd_cs < 0) LREP("open spi cs device failed\r\n");
    g_rf_mac_open.fd_reset = open(DEV_RF_RESET_NAME, O_WRONLY);
    if(g_rf_mac_open.fd_reset < 0) LREP("open rf-reset device failed\r\n");
    g_rf_mac_open.fd_intr = open(DEV_RF_INTR_NAME, O_RDONLY);
    if(g_rf_mac_open.fd_intr < 0) LREP("open rf-intr device failed\r\n");
#elif defined(OS_LINUX)
    g_fd_debug_tx = STDOUT_FILENO;
    g_fd_debug_rx = STDIN_FILENO;
    ioctl(g_fd_debug_rx, TCGETS2, &opt);
	opt.c_cc[VMIN]  = 1;
	opt.c_cc[VTIME] = 0;
	opt.c_lflag &= ~(ICANON | ECHO);
	ioctl(g_fd_debug_rx, TCSETS2, &opt);

    int _fd = open("/sys/class/gpio/export", O_WRONLY);
    if(_fd >= 0){
    	write(_fd, "17", 2); // intr
    	close(_fd);
    }
    _fd = open("/sys/class/gpio/gpio17/edge", O_WRONLY);
	if(_fd >= 0){
		write(_fd, "both", 4);
		close(_fd);
	}
    _fd = open("/sys/class/gpio/export", O_WRONLY);
	if(_fd >= 0){
		write(_fd, "22", 2); // reset
		close(_fd);
	}
	_fd = open("/sys/class/gpio/gpio22/direction", O_WRONLY);
	if(_fd >= 0){
		write(_fd, "out", 3);
		close(_fd);
	}
	 _fd = open("/sys/class/gpio/export", O_WRONLY);
	if(_fd >= 0){
		write(_fd, "7", 2); // cs
		close(_fd);
	}
	_fd = open("/sys/class/gpio/gpio7/direction", O_WRONLY);
	if(_fd >= 0){
		write(_fd, "out", 3);
		close(_fd);
	}
    g_rf_mac_open.fd_cs = open("/sys/class/gpio/gpio7/value", O_WRONLY);
    if(g_rf_mac_open.fd_cs < 0) LREP("open spi cs device failed\r\n");
    g_rf_mac_open.fd_reset = open("/sys/class/gpio/gpio22/value", O_WRONLY);
    if(g_rf_mac_open.fd_reset < 0) LREP("open rf-reset device failed\r\n");
    g_rf_mac_open.fd_intr = open("/sys/class/gpio/gpio17/value", O_RDONLY);
    if(g_rf_mac_open.fd_intr < 0) LREP("open rf-intr device failed\r\n");
#endif
    g_rf_mac_open.fd_spi = open(DEV_RF_NAME, O_RDWR);
    if(g_rf_mac_open.fd_spi < 0){
        LREP("open spi device '%s' failed %d\r\n", DEV_RF_NAME, g_rf_mac_open.fd_spi);
    }
    else{
        uival = SPI_MODE_0;
        if(ioctl(g_rf_mac_open.fd_spi, SPI_IOC_WR_MODE, (unsigned int)&uival) != 0) LREP("ioctl spi mode failed\r\n");
        uival = 15000000;
        if(ioctl(g_rf_mac_open.fd_spi, SPI_IOC_WR_MAX_SPEED_HZ, (unsigned int)&uival) != 0) LREP("ioctl spi speed failed\r\n");
    }

    g_nwk.mac = &g_rf_mac;
    // Miwi network
    MAC_mrf24j40_open(&g_rf_mac, &g_rf_mac_open);
    Network_init(&g_nwk);
    srand(0);

#if defined(OS_FREERTOS)
    DEFINE_THREAD(Thread_DebugTX,   1024, tskIDLE_PRIORITY + 1UL);
#endif
    DEFINE_THREAD(Thread_UserInput, 2048, tskIDLE_PRIORITY + 1UL);
    DEFINE_THREAD(Thread_PhyIntr,   1024, tskIDLE_PRIORITY + 2UL);
    DEFINE_THREAD(Thread_MiwiTask,  2048, tskIDLE_PRIORITY + 4UL);
    usleep(1000* 100);

    LREP("\r\nHit any key to break boot sequence");

    uint8_t timeout = 30;
    uint8_t input = 0;
    while(timeout-- && input == 0){
    	if(timeout % 10 == 0)
    		LREP(".");
    	input = kb_value();
    	if(input == 0) usleep(1000* 100);
    }
    if(input != 0){
    	while(1){sleep(1);}
    }
    LREP("DONE\r\n");

    setting_read(&g_setting_dev, &g_setting);
    LREP("Setting:\r\n");
    setting_dump_to_stdio(&g_setting);
    uival = 25;
    MAC_mrf24j40_ioctl(&g_rf_mac, mac_mrf24j40_ioc_set_channel, (unsigned int)&uival);
    for(i = 0; i < 8 ; i++)
        u8aVal[i] = g_setting.mac_long_address[i];
    MAC_mrf24j40_ioctl(&g_rf_mac, mac_mrf24j40_ioc_set_long_address, (unsigned int)u8aVal);

    // PAN Coordinator
    if(g_setting.network_type == setting_network_type_pan_coordinator){
//        uint8_t noise_level[15];
        uint8_t channels[15];
        struct network_beacon_info nwk_info[1];

        LREP("Device as a PAN coordinator\r\n");
        LREP("Scan free channel\r\n");
//        for(i =0 ;i < 15; i++) channels[i] = i + 11;
//        Network_scan_channel(&g_rf_mac, 0x03fff800, noise_level);
//        // sort noise level
//        for(i = 0; i < 15-1; i++){
//            for(j = i+1; j < 15; j++){
//                if(noise_level[i] > noise_level[j]){
//                    noise_level[i] ^= noise_level[j];
//                    noise_level[j] ^= noise_level[i];
//                    noise_level[i] ^= noise_level[j];
//
//                    channels[i] ^= channels[j];
//                    channels[j] ^= channels[i];
//                    channels[i] ^= channels[j];
//                }
//            }
//        }
        //for(i =0 ;i < 15; i++) channels[14-i] = i + 11;
        i = 0;
        channels[i] = 25;
        //for(i = 0; i < 15; i++){
            LREP("Detect network on channel %d ...", channels[i]);
            ival = Network_detect_current_network(&g_nwk, channels[i], nwk_info, 1);
            if(ival > 0) LREP("found network panId %04X\r\n", nwk_info[0].panId);
            else {
            	LREP("not found\r\n");
//            	break;
            }
//        }
        if(i == 15) LREP("Not found free channel\r\n");
        else{
            uival = rand();
            u8aVal[0] = uival & 0x00FF;
            u8aVal[1] = ((uival & 0xFF00) >> 8);
            uival = channels[i];

            MAC_mrf24j40_ioctl(g_nwk.mac, mac_mrf24j40_ioc_set_channel, (unsigned int)&uival);
            MAC_mrf24j40_ioctl(g_nwk.mac, mac_mrf24j40_ioc_set_pan_id, (unsigned int)&u8aVal[0]);
            u8aVal[0] = 0; u8aVal[1] = 0;
            MAC_mrf24j40_ioctl(g_nwk.mac, mac_mrf24j40_ioc_set_short_address, (unsigned int)&u8aVal[0]);
            LREP("Create network channel %d PANId %02X%02X\r\n", uival, u8aVal[1], u8aVal[0]);
        }
    }else if(g_setting.network_type == setting_network_type_router){
    	struct network_beacon_info nwk_info[1];
    	LREP("Device as a Router device\r\n");
    	LREP("Join to new network\r\n");
    	while(1){
			// Request a network
    		i = 25;
			//for(i = 25; i >= 11; i--){
				LREP("Detect network on channel %d ...", i);
				MAC_mrf24j40_ioctl(g_nwk.mac, mac_mrf24j40_ioc_reset, 0);
				ival = Network_detect_current_network(&g_nwk, i, nwk_info, 1);
				if(ival > 0){
					LREP("found network panId %04X node %04X [rssi:%02X, lqi:%02X]\r\n",
							nwk_info[0].panId, nwk_info[0].address,
							nwk_info[0].rssi, nwk_info[0].lqi);
					//break;
				}
				else {
					LREP("not found\r\n");
				}
			//}
			if(ival <= 0) LREP("Not found any network\r\n");
			else{
				struct network_join_info join_info;
				// Request to join network
				if(Network_join_request(&g_nwk, i, nwk_info[0].panId, nwk_info[0].address, &join_info) == 0){
					LREP("Join to network with address %04X\r\n", join_info.address);
					u8aVal[0] = join_info.address & 0x00FF;
					u8aVal[1] = (join_info.address >> 8) & 0x00FF;
					MAC_mrf24j40_ioctl(g_nwk.mac, mac_mrf24j40_ioc_set_short_address, (unsigned int)&u8aVal[0]);
					u8aVal[0] = nwk_info[0].panId & 0x00FF;
					u8aVal[1] = (nwk_info[0].panId >> 8) & 0x00FF;
					MAC_mrf24j40_ioctl(g_nwk.mac, mac_mrf24j40_ioc_set_pan_id, (unsigned int)&u8aVal[0]);
					break;
				}else{
					LREP("Join failed\r\n");

				}
			}
    	}
    }else{
    	LREP("Type invalid\r\n");
    	while(1){sleep(1);}
    }

    while(1){
    	Network_loop(&g_nwk, 100);
        LED_TOGGLE(BLUE);
    }
    return 0;
}
#if defined(OS_FREERTOS)
void *Thread_DebugTX(void* param){
    uint8_t data;    
    struct timespec abs_timeout;
    abs_timeout.tv_sec = 1;
    abs_timeout.tv_nsec = 0;
    while(1){
        if(mq_timedreceive(g_debug_tx_buffer, (char*)&data, 1, 0, &abs_timeout) == 1)
            write(g_fd_debug_tx, &data, 1);
    }
}
#endif
void *Thread_UserInput(void *param){
    int len, fd;
    fd_set readfs;
    struct timeval timeout;
    unsigned char buff[32];
    
    FD_ZERO(&readfs);
    FD_SET(g_fd_debug_rx, &readfs);
    FD_SET(g_fd_button, &readfs);
    timeout.tv_sec      = 1;
    timeout.tv_usec     = 0;

    fd = g_fd_debug_rx;
    if(fd < g_fd_button) fd = g_fd_button;
    CLI_start();
    while (1) {
        len = select(fd+1, &readfs, 0, 0, &timeout);
        if(len > 0){
            if(FD_ISSET(g_fd_debug_rx, &readfs)){
				len = read(g_fd_debug_rx, buff, 32);
				if(len > 0){
					g_kb_value = buff[0];
					CLI_process(buff, len);
				}
			}
			if(FD_ISSET(g_fd_button, &readfs)){
				LREP("\r\nbutton pressed\r\n");
			}
        }else if(len == 0){
        	LED_TOGGLE(GREEN);
        }else{
            LREP("select intr pin failed.\r\n");
            break;
        }
    }
    while(1){sleep(1);}
}
void* Thread_PhyIntr(void* param){
	int len, fd;
	fd_set readfs;
	struct timeval timeout;

	FD_ZERO(&readfs);
	FD_SET(g_rf_mac_open.fd_intr, &readfs);
    fd = g_rf_mac_open.fd_intr;
    timeout.tv_sec      = 1;
    timeout.tv_usec     = 0;
    while(1){
        len = select(fd+1, &readfs, 0, 0, &timeout);
        if(len > 0){
            if(FD_ISSET(g_rf_mac_open.fd_intr, &readfs)){
                MAC_mrf24j40_ioctl(&g_rf_mac, mac_mrf24j40_ioc_trigger_interrupt, 0);
            }
        }
    }
    return 0;
}
void *Thread_MiwiTask(void*param){
    while(1){
        if(MAC_mrf24j40_select(&g_rf_mac, 100)){
            MAC_mrf24j40_task(&g_rf_mac);
        }
    }
    return 0;
}


