/**
 * note:
 * 	do not include stdio.h (rx uart not work !!!)
 */
#include <drv_api.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <semaphore.h>
#include <mqueue.h>
#include <string.h>
#include <stdlib.h>
#include <spidev.h>
#include <drv_gpio.h>

#include "network/mac/mac_mrf24j40.h"

#include <Test.h>
#include <Network.h>
#include <App.h>
#include <setting.h>
#include <at93c.h>
#include <cli.h>
#include <debug.h>

void *Thread_Startup(void*);
void *Thread_DebugTX(void*);
void *Thread_DebugRX(void*);
void *Thread_RFIntr(void*);
void *Thread_MiwiTask(void*);
void HW_Initalize();

#define             APP_THREAD_COUNT    	5
pthread_t           g_thread[APP_THREAD_COUNT];
pthread_attr_t      g_thread_attr[APP_THREAD_COUNT];
sem_t               g_thread_startup[APP_THREAD_COUNT-1];
sem_t               g_sem_debug;
sem_t				g_sem_debug_rx;
mqd_t               g_debug_tx_buffer   = 0;
int                 g_fd_debug          = -1;
int                 g_fd_led[4]         = {-1};
int                 g_fd_button         = -1;
int					g_fd_rtc			= -1;

struct mac_mrf24j40         g_rf_mac;
struct network				g_nwk;
struct setting_device       g_setting_dev;
struct setting_value		g_setting;

extern int board_register_devices();
int g_thread_index = 1;
#define DEFINE_THREAD(fxn, stack_size, priority) {\
    pthread_attr_setstacksize(&g_thread_attr[g_thread_index], stack_size);\
    pthread_setschedprio(&g_thread[g_thread_index], priority);\
    pthread_create(&g_thread[g_thread_index], &g_thread_attr[g_thread_index], fxn, &g_thread_startup[g_thread_index-1]);\
    g_thread_index++;\
}

uint8_t kbhit(int timeout){
    int8_t u8val = 0;
    int len;
    fd_set readfs;
    struct timeval s_timeout;

    FD_CLR(g_fd_debug, &readfs);
    s_timeout.tv_sec  = timeout/1000;
    s_timeout.tv_usec = (timeout%1000)*1000;

	len = select(g_fd_debug, (unsigned int*)&readfs, 0, 0, &s_timeout);
	if(len > 0){
		if(FD_ISSET(g_fd_debug, &readfs)){
			len = read(g_fd_debug, &u8val, 1);
			if(len <= 0){
				u8val = 0;
			}
		}
	}
	return u8val;
}
uint8_t kb_value(){
    uint8_t u8val = 0;
    int len = read(g_fd_debug, &u8val, 1);
    if(len <= 0) u8val = 0;
    return u8val;
}

int main(void)
{
    int i;    
    
    HW_Initalize();    
    g_debug_tx_buffer = mq_open(0, 512);

    for(i = 0; i < APP_THREAD_COUNT-1; i++)
        sem_init(&g_thread_startup[i], 0, 0);
    sem_init(&g_sem_debug, 0, 1);
    sem_init(&g_sem_debug_rx, 0, 0);
//    sem_init(&g_mimac_access, 0, 1);
    pthread_attr_setstacksize(&g_thread_attr[0], configMINIMAL_STACK_SIZE*32);
    pthread_setschedprio(&g_thread[0], tskIDLE_PRIORITY + 1UL);
    pthread_create(&g_thread[0], &g_thread_attr[0], Thread_Startup, 0);
    
    DEFINE_THREAD(Thread_DebugTX, configMINIMAL_STACK_SIZE*8,  tskIDLE_PRIORITY + 1UL);
    DEFINE_THREAD(Thread_RFIntr,  configMINIMAL_STACK_SIZE*16, tskIDLE_PRIORITY + 3UL);
    DEFINE_THREAD(Thread_MiwiTask,configMINIMAL_STACK_SIZE*32, tskIDLE_PRIORITY + 4UL);
    DEFINE_THREAD(Thread_DebugRX, configMINIMAL_STACK_SIZE*32,  tskIDLE_PRIORITY + 1UL);
    
    /* Start the RTOS Scheduler */
    vTaskStartScheduler();

    /* HALT */
    while(1);
}
struct mac_mrf24j40_open_param  rf_mac_init;
void *Thread_Startup(void *pvParameters){
    int i, ival;
    struct termios2 opt;
    unsigned int uival;
    uint8_t u8aVal[32];
    
    // register drivers & devices
    driver_probe();
    board_register_devices();
    // open gpio
    g_fd_led[0] = open("led-green", 0);
    g_fd_led[1] = open("led-red",   0);
    g_fd_led[2] = open("led-blue", 0);
    g_fd_led[3] = open("led-orange", 0);
    g_fd_button = open("button",    0);
    // open usart
    g_fd_debug = open("debug-dev", O_RDWR);
    if(g_fd_debug >= 0){
        // configure
        ioctl(g_fd_debug, TCGETS2, (unsigned int)&opt);
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
        ioctl(g_fd_debug, TCSETS2, (unsigned int)&opt);        
    }else{
        while(1){
            LED_TOGGLE(RED);
            usleep(1000 * 100);
        };
    }
    
    rf_mac_init.fd_spi = open("rf", O_RDWR);
    if(rf_mac_init.fd_spi < 0){
        LREP("open spi device failed\r\n");
    }
    else{
        uival = SPI_MODE_0;
        if(ioctl(rf_mac_init.fd_spi, SPI_IOC_WR_MODE, (unsigned int)&uival) != 0) LREP("ioctl spi mode failed\r\n");
        uival = 15000000;
        if(ioctl(rf_mac_init.fd_spi, SPI_IOC_WR_MAX_SPEED_HZ, (unsigned int)&uival) != 0) LREP("ioctl spi speed failed\r\n");
        else{
            //uival = 0;
            //if(ioctl(rf_mac_init.fd_spi, SPI_IOC_RD_MAX_SPEED_HZ, (unsigned int)&uival) == 0) LREP("ioctl spi speed = %u\r\n", uival);
        }
    }
    rf_mac_init.fd_cs = open("rf-cs", 0);
    if(rf_mac_init.fd_cs < 0) LREP("open spi cs device failed\r\n");
    rf_mac_init.fd_reset = open("rf-reset", 0);
    if(rf_mac_init.fd_reset < 0) LREP("open rf-reset device failed\r\n");
    rf_mac_init.fd_intr = open("rf-intr", 0);
    if(rf_mac_init.fd_intr < 0) LREP("open rf-intr device failed\r\n");
    // eeprom
    g_setting_dev.dev.fd_cs   = open("at93c-cs", 0);
    g_setting_dev.dev.fd_sck  = open("at93c-sck", 0);
    g_setting_dev.dev.fd_mosi = open("at93c-mosi", 0);
    g_setting_dev.dev.fd_miso = open("at93c-miso", 0);
    if(g_setting_dev.dev.fd_cs < 0 ||
            g_setting_dev.dev.fd_sck < 0 ||
            g_setting_dev.dev.fd_mosi < 0 ||
            g_setting_dev.dev.fd_miso < 0){
        LREP("open at93c failed cs:%d sck:%d mosi:%d miso:%d\r\n",
                g_setting_dev.dev.fd_cs, g_setting_dev.dev.fd_sck,
                g_setting_dev.dev.fd_mosi, g_setting_dev.dev.fd_miso);
    }
    g_fd_rtc = open("rtc0", 0);
    if(g_fd_rtc < 0) LREP("open rtc failed.\r\n");

    g_nwk.mac = &g_rf_mac;
    // Miwi network
    MAC_mrf24j40_open(&g_rf_mac, &rf_mac_init);
    Network_init(&g_nwk);
    srand(0);

    // signal all other thread startup
    //LREP("Thread startup is running\r\n");
    for(i = 0; i < APP_THREAD_COUNT-1; i++){
        sem_post(&g_thread_startup[i]);
    }
    usleep(1000* 100);
    LREP("\r\nHit any key to break boot sequence");
    uint8_t timeout = 3;
    uint8_t input = 0;
    while(timeout-- && input == 0){
    	LREP(".");
    	input = kbhit(1000);
    }
    if(input != 0){
    	CLI_loop();
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

            LREP("Create network channel %d PANId %02X%02X\r\n", uival, u8aVal[1], u8aVal[0]);
            MAC_mrf24j40_ioctl(g_nwk.mac, mac_mrf24j40_ioc_set_channel, (unsigned int)&uival);
            MAC_mrf24j40_ioctl(g_nwk.mac, mac_mrf24j40_ioc_set_pan_id, (unsigned int)&u8aVal[0]);
            u8aVal[0] = 0; u8aVal[1] = 0;
            MAC_mrf24j40_ioctl(g_nwk.mac, mac_mrf24j40_ioc_set_short_address, (unsigned int)&u8aVal[0]);
        }
    }else if(g_setting.network_type == setting_network_type_router){
    	struct network_beacon_info nwk_info[1];
    	LREP("Device as a Router device\r\n");
    	LREP("Join to new network\r\n");
    	while(1){
			// Request a network
			for(i = 25; i >= 11; i--){
				LREP("Detect network on channel %d ...", i);
				ival = Network_detect_current_network(&g_nwk, i, nwk_info, 1);
				if(ival > 0){
					LREP("found network panId %04X node %04X [rssi:%02X, lqi:%02X]\r\n",
							nwk_info[0].panId, nwk_info[0].address,
							nwk_info[0].rssi, nwk_info[0].lqi);
					break;
				}
				else {
					LREP("not found\r\n");
				}
			}
			if(i == 26) LREP("Not found any network\r\n");
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
    }

    sem_post(&g_sem_debug_rx);
    while(1){
    	Network_loop(&g_nwk, 100);
        LED_TOGGLE(GREEN);
    }
    return 0;
}
void *Thread_DebugTX(void* pvParameters){
    uint8_t data;    
    struct timespec abs_timeout;
    abs_timeout.tv_sec = 1;
    abs_timeout.tv_nsec = 0;
    while(1){
        if(mq_timedreceive(g_debug_tx_buffer, (char*)&data, 1, 0, &abs_timeout) == 1)
            write(g_fd_debug, &data, 1);
    }
}
void *Thread_RFIntr(void *pvParameters){
    int len;
    sem_t* sem_startup = (sem_t*)pvParameters;
    fd_set readfs;
    struct timeval timeout;
    
    sem_wait(sem_startup);
    //LREP("Thread RFIntr is running\r\n");
    
    FD_CLR(rf_mac_init.fd_intr, &readfs);
    timeout.tv_sec      = 1;
    timeout.tv_usec     = 0;

    while (1) {        
        len = select(rf_mac_init.fd_intr, (unsigned int*)&readfs, 0, 0, &timeout);
        if(len > 0){
            if(FD_ISSET(rf_mac_init.fd_intr, &readfs)){
                MAC_mrf24j40_ioctl(&g_rf_mac, mac_mrf24j40_ioc_trigger_interrupt, 0);
            }
        }else if(len == 0){
        }else{
            LREP("select intr pin failed.\r\n");
            break;
        }
    }
    while(1){sleep(1);}
}
void *Thread_MiwiTask(void* pvParameters){
    sem_t* sem_startup = (sem_t*)pvParameters;

    sem_wait(sem_startup);
    //LREP("Thread MiwiTask is running\r\n");
    while(1){
        if(MAC_mrf24j40_select(&g_rf_mac, 100)){
            MAC_mrf24j40_task(&g_rf_mac);
        }
    }
    return 0;
}
void *Thread_DebugRX(void* pvParameters){
    sem_t* sem_startup = (sem_t*)pvParameters;
    sem_wait(sem_startup);
    sem_wait(g_sem_debug_rx);
    while(1){
    	CLI_loop();
    }
    return 0;
}
/**
 * Init HW
 */
/* Board includes */
#include "stm32f4_discovery.h"
#include "stm32f4xx_rcc.h"
void HW_Initalize()
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
}


