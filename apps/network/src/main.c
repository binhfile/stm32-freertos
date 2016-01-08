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
#include <poll.h>
#include <reboot.h>

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

int                 g_fd_led[4]         = {-1};
int                 g_fd_button         = -1;

#if defined(OS_FREERTOS)
#define             APP_THREAD_COUNT    	4
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
#include <stdio.h>
#define             APP_THREAD_COUNT    	3
#define DEFINE_THREAD(fxn, stack_size, priority) \
	pthread_t thread_##fxn;\
	{\
    pthread_create(&thread_##fxn, 0, fxn, 0);\
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
struct termios2 g_stdin_opt;
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
    g_rf_mac_open.fd_cs = -1;//open(DEV_RF_CS_NAME, O_WRONLY);
//    if(g_rf_mac_open.fd_cs < 0) LREP("open spi cs device failed\r\n");
    g_rf_mac_open.fd_reset = open(DEV_RF_RESET_NAME, O_WRONLY);
    if(g_rf_mac_open.fd_reset < 0) LREP("open rf-reset device failed\r\n");
    g_rf_mac_open.fd_intr = open(DEV_RF_INTR_NAME, O_RDONLY);
    if(g_rf_mac_open.fd_intr < 0) LREP("open rf-intr device failed\r\n");
    g_rf_mac_open.fd_spi = open("rf", O_RDWR);

#elif defined(OS_LINUX)
    g_fd_debug_tx = STDOUT_FILENO;
    g_fd_debug_rx = STDIN_FILENO;
    ioctl(g_fd_debug_rx, TCGETS2, &opt);
    memcpy(&g_stdin_opt, &opt, sizeof(g_stdin_opt));
	opt.c_cc[VMIN]  = 1;
	opt.c_cc[VTIME] = 0;
	opt.c_lflag &= ~(ICANON | ECHO);
	ioctl(g_fd_debug_rx, TCSETS2, &opt);

	/*
	 * rf-reset 	- 17
	 * rf-intr  	- 13
	 *
	 * eeprom-cs	- 22
	 * eeprom-sck	- 23
	 * eeprom-mosi	- 24
	 * eeprom-miso	- 25
	 */
	struct gpio_info{
			int num;
			int dir;
			int edge;
	};
	struct gpio_info gpios[] = {
			{
					.num = 17,
					.dir = 0,
			},// reset
			{
					.num = 27,
					.dir = 1,
					.edge = 1,
			},// intr
			{
					.num = 22,
					.dir = 0,
			},// cs
			{
					.num = 23,
					.dir = 0,
			},// sck
			{
					.num = 24,
					.dir = 0,
			},// mosi
			{
					.num = 25,
					.dir = 1,
					.edge = 0,
			},// miso
			{
					.num = 5,
					.dir = 0,
			},// led-1
			{
					.num = 6,
					.dir = 0,
			},// led-2
			{
					.num = 13,
					.dir = 0,
			},// led-3
			{
					.num = 19,
					.dir = 0,
			},// led-4
			{
					.num = 16,
					.dir = 1,
					.edge = 1,
			},// button
	};

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
    g_rf_mac_open.fd_cs = -1;
    g_rf_mac_open.fd_reset = open("/sys/class/gpio/gpio17/value", O_WRONLY);
    if(g_rf_mac_open.fd_reset < 0) LREP("open rf-reset device failed\r\n");
    g_rf_mac_open.fd_intr = open("/sys/class/gpio/gpio27/value", O_RDONLY);
    if(g_rf_mac_open.fd_intr < 0) LREP("open rf-intr device failed\r\n");
    g_rf_mac_open.fd_spi = open("/dev/spidev0.0", O_RDWR);
    g_setting_dev.dev.fd_cs   = open("/sys/class/gpio/gpio22/value", O_WRONLY);
    g_setting_dev.dev.fd_sck  = open("/sys/class/gpio/gpio23/value", O_WRONLY);
    g_setting_dev.dev.fd_mosi = open("/sys/class/gpio/gpio24/value", O_WRONLY);
    g_setting_dev.dev.fd_miso =  open("/sys/class/gpio/gpio25/value", O_RDONLY);

    g_fd_led[0] = open("/sys/class/gpio/gpio5/value", O_RDWR);
    g_fd_led[1] = open("/sys/class/gpio/gpio6/value", O_RDWR);
    g_fd_led[2] = open("/sys/class/gpio/gpio13/value", O_RDWR);
    g_fd_led[3] = open("/sys/class/gpio/gpio19/value", O_RDWR);
//    g_fd_button = open("/sys/class/gpio/gpio26/value", O_RDONLY);
#endif
    LED_OFF(RED);
    LED_OFF(GREEN);
    LED_OFF(BLUE);
    LED_OFF(ORANGE);
    if(g_rf_mac_open.fd_spi < 0){
        LREP("open spi device '%s' failed %d\r\n", "/dev/spidev0.0", g_rf_mac_open.fd_spi);
    }
    else{
        uival = SPI_MODE_0;
        if(ioctl(g_rf_mac_open.fd_spi, SPI_IOC_WR_MODE, (unsigned int)&uival) != 0) LREP("ioctl spi mode failed\r\n");
        uival = 15000000;
        if(ioctl(g_rf_mac_open.fd_spi, SPI_IOC_WR_MAX_SPEED_HZ, (unsigned int)&uival) != 0) LREP("ioctl spi speed failed\r\n");
        uival = 8;
        if(ioctl(g_rf_mac_open.fd_spi, SPI_IOC_WR_BITS_PER_WORD, (unsigned int)&uival) != 0) LREP("ioctl bit per word failed\r\n");
    }

    g_nwk.mac = &g_rf_mac;
    // Miwi network
    MAC_mrf24j40_open(&g_rf_mac, &g_rf_mac_open);
    Network_init(&g_nwk);
    srand(0);

#if defined(OS_FREERTOS)
    DEFINE_THREAD(Thread_DebugTX,   1024, tskIDLE_PRIORITY + 1UL);
#endif
    DEFINE_THREAD(Thread_UserInput, 2048, tskIDLE_PRIORITY + 3UL);
    DEFINE_THREAD(Thread_MiwiTask,  2048, tskIDLE_PRIORITY + 4UL);
    DEFINE_THREAD(Thread_PhyIntr,   2048, tskIDLE_PRIORITY + 1UL);
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
    uival = 16;
    MAC_mrf24j40_ioctl(&g_rf_mac, mac_mrf24j40_ioc_set_channel, (unsigned int)&uival);
    for(i = 0; i < 8 ; i++)
        u8aVal[i] = g_setting.mac_long_address[i];
    MAC_mrf24j40_ioctl(&g_rf_mac, mac_mrf24j40_ioc_set_long_address, (unsigned int)u8aVal);

    // PAN Coordinator
    if(g_setting.network_type == setting_network_type_pan_coordinator){
        uint8_t channels[NWK_CHANNEL_CNT];

        LREP("Device as a PAN coordinator\r\n");
#if 0
        struct network_beacon_info nwk_info[1];
        uint8_t noise_level[NWK_CHANNEL_CNT] = {0xff};
        int j;

        LREP("Scan free channel\r\n");
        for(i = 0 ;i < NWK_CHANNEL_CNT; i++) channels[i] = i + NWK_CHANNEL_MIN;
        Network_scan_channel(&g_rf_mac, NWK_CHANNEL_MAP, noise_level);
        // sort noise level
        for(i = 0; i < NWK_CHANNEL_CNT-1; i++){
            for(j = i+1; j < NWK_CHANNEL_CNT; j++){
                if(noise_level[i] > noise_level[j]){
                    noise_level[i] ^= noise_level[j];
                    noise_level[j] ^= noise_level[i];
                    noise_level[i] ^= noise_level[j];

                    channels[i] ^= channels[j];
                    channels[j] ^= channels[i];
                    channels[i] ^= channels[j];
                }
            }
        }
        for(i = 0; i < NWK_CHANNEL_CNT; i++){
        	if(NWK_CHANNEL_MAP & ((unsigned int)1) << channels[i]){
				LREP("Detect network on channel %d ...", channels[i]);
				ival = Network_detect_current_network(&g_nwk, channels[i], nwk_info, 1);
				if(ival > 0){
					struct network_echo_info info;
					LREP("found network panId %04X\r\n", nwk_info[0].panId);
					u8aVal[0] = nwk_info[0].panId & 0x00FF;
					u8aVal[1] = ((nwk_info[0].panId & 0xFF00) >> 8);
					LREP("Try to ping PAN coordinator ...");
					MAC_mrf24j40_ioctl(g_nwk.mac, mac_mrf24j40_ioc_set_pan_id, (unsigned int)&u8aVal[0]);
					Network_echo_request(&g_nwk, 0, 10, 95, &info);
					if(info.passed < 8) LREP("FAIL\r\n");

				}
				else {
					LREP("not found\r\n");
					break;
				}
        	}
        }
        if(i == NWK_CHANNEL_CNT) LREP("Not found free channel\r\n");
        else
#else
        i = 0;
        channels[i] = NWK_CHANNEL_MIN;
#endif
        {
            uival = rand();
            u8aVal[0] = uival & 0x00FF;
            u8aVal[1] = ((uival & 0xFF00) >> 8);
            uival = channels[i];
            LREP("Create network channel %d PANId %02X%02X\r\n", uival, u8aVal[1], u8aVal[0]);

            MAC_mrf24j40_ioctl(g_nwk.mac, mac_mrf24j40_ioc_set_channel, (unsigned int)&uival);
            MAC_mrf24j40_ioctl(g_nwk.mac, mac_mrf24j40_ioc_set_pan_id, (unsigned int)&u8aVal[0]);
            u8aVal[0] = 0; u8aVal[1] = 0;
            MAC_mrf24j40_ioctl(g_nwk.mac, mac_mrf24j40_ioc_set_short_address, (unsigned int)&u8aVal[0]);
        }
    }else
    	//if(g_setting.network_type == setting_network_type_router)
    	{
    	struct network_beacon_info nwk_info[3];
		struct network_join_info join_info;
		struct network_echo_info info;

		if(g_setting.network_type != setting_network_type_router){
		    for(i = 0; i < 8 ; i++)
		        u8aVal[i] = i;
		    MAC_mrf24j40_ioctl(&g_rf_mac, mac_mrf24j40_ioc_set_long_address, (unsigned int)u8aVal);
		}

    	int found = 0;
    	LREP("Device as a Router device\r\n");
    	LREP("Join to new network\r\n");
    	while(!found){
			// Request a network
    		i = NWK_CHANNEL_MIN;
			while(!found && (i < NWK_CHANNEL_MAX)){
				if(NWK_CHANNEL_MAP & ((unsigned int)1) << i){
					LREP("Detect network on channel %d ...", i);
					ival = Network_detect_current_network(&g_nwk, i, nwk_info, sizeof(nwk_info) / sizeof(struct network_beacon_info));
					if(ival > 0){
						int nwk_index;
						LREP(" %d networks\r\n", ival);
						for(nwk_index = 0 ;nwk_index < ival; nwk_index++){
							LREP("found network[%02d] panId %04X node %04X [rssi:%02X, lqi:%02X]\r\n",
									nwk_index,
									nwk_info[nwk_index].panId, nwk_info[nwk_index].address,
									nwk_info[nwk_index].rssi, nwk_info[nwk_index].lqi);
						}
						for(nwk_index = 0 ;nwk_index < ival; nwk_index++){
							// try to join
							usleep(1000* 100);
							LREP("Try join to network panId %04X parent %02X ...",
									nwk_info[nwk_index].panId, nwk_info[nwk_index].address);
							if(Network_join_request(&g_nwk, i, nwk_info[nwk_index].panId, nwk_info[nwk_index].address, &join_info) == 0){
								LREP("done with address %04X\r\n", join_info.address);
								u8aVal[0] = join_info.address & 0x00FF;
								u8aVal[1] = (join_info.address >> 8) & 0x00FF;
								MAC_mrf24j40_ioctl(g_nwk.mac, mac_mrf24j40_ioc_set_short_address, (unsigned int)&u8aVal[0]);
								u8aVal[0] = nwk_info[nwk_index].panId & 0x00FF;
								u8aVal[1] = (nwk_info[nwk_index].panId >> 8) & 0x00FF;
								MAC_mrf24j40_ioctl(g_nwk.mac, mac_mrf24j40_ioc_set_pan_id, (unsigned int)&u8aVal[0]);
								// ping test
								LREP("Ping test connection ...");
								Network_echo_request(&g_nwk, nwk_info[nwk_index].address, 10, 95, &info, 0);
								if(info.passed < 8) LREP("FAIL\r\n");
								else {
									LREP("done\r\n");
									g_nwk.parent_id = nwk_info[nwk_index].address;
									Network_join_send_done(&g_nwk);
									found = 1;
									break;
								}
							}else{
								LREP(" FAIL\r\n");
							}
						}
					}
					else {
						LREP("not found\r\n");
					}
				}
				i++;
			}// end for
    	}// end while
    }
//    else{
//    	LREP("Type invalid\r\n");
//    	while(1){sleep(1);}
//    }

    while(!is_app_term()){
    	Network_loop(&g_nwk, 1000);
        LED_TOGGLE(BLUE);
    }
	ioctl(g_fd_debug_rx, TCSETS2, &g_stdin_opt);
    sleep(2);
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
    int len;
    unsigned char buff[32];
    CLI_start();
	int timeout = 1000;
	struct pollfd poll_fd[1];
	while(!is_app_term()){
		poll_fd[0].fd = g_fd_debug_rx;
		poll_fd[0].events = POLLIN;
		poll_fd[0].revents = 0;
		int ret = poll(&poll_fd[0], 1, timeout);
		if (ret > 0 && ((poll_fd[0].revents & POLLIN) != 0))  {
			len = read(g_fd_debug_rx, buff, 32);
			if(len > 0){
				g_kb_value = buff[0];
				CLI_process(buff, len);
			}
		}
	}
    while(1){sleep(1);}
}
void* Thread_PhyIntr(void* param){
#if defined(OS_LINUX)
	unsigned char rx[5];
#endif
    int timeout = 1000;
    struct pollfd poll_fd[1];
    while(!is_app_term()){
		poll_fd[0].fd = g_rf_mac_open.fd_intr;
		poll_fd[0].events = POLLPRI;
		poll_fd[0].revents = 0;
		lseek(g_rf_mac_open.fd_intr, 0, SEEK_SET);
		int ret = poll(&poll_fd[0], 1, timeout);
		if (ret > 0 && ((poll_fd[0].revents & POLLPRI) != 0))  {
#if defined(OS_LINUX)
			ret = read(g_rf_mac_open.fd_intr, &rx, 5);
#endif
			MAC_mrf24j40_ioctl(&g_rf_mac, mac_mrf24j40_ioc_trigger_interrupt, 0);
		}
    }
    return 0;
}
void *Thread_MiwiTask(void*param){
    while(!is_app_term()){
        if(MAC_mrf24j40_select(&g_rf_mac, 1000)){
            MAC_mrf24j40_task(&g_rf_mac);
        }
    }
    return 0;
}


