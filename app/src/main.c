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

#include <sys/reboot.h>

#include <debug.h>

#include "network/mac/mac_mrf24j40.h"

#include <Test.h>
#include <Network.h>
#include <App.h>
#include <setting.h>
#include <at93c.h>

void *Thread_Startup(void*);
void *Thread_DebugTX(void*);
void *Thread_DebugRx(void *);
void *Thread_RFIntr(void*);
void *Thread_MiwiTask(void*);
void HW_Initalize();
void LREP(char* s, ...);

#define             APP_THREAD_COUNT    5
pthread_t           g_thread[APP_THREAD_COUNT];
pthread_attr_t      g_thread_attr[APP_THREAD_COUNT];
sem_t               g_thread_startup[APP_THREAD_COUNT-1];
sem_t               g_sem_debug;
mqd_t               g_debug_tx_buffer   = 0;
int                 g_fd_debug          = -1;
int                 g_fd_led[4]         = {-1};
int                 g_fd_button         = -1;
volatile uint8_t    g_debug_cmd = 0;

struct mac_mrf24j40         g_rf_mac;
struct setting_device       g_setting_dev;

extern int board_register_devices();
int g_thread_index = 1;
#define DEFINE_THREAD(fxn, stack_size, priority) {\
    pthread_attr_setstacksize(&g_thread_attr[g_thread_index], stack_size);\
    pthread_setschedprio(&g_thread[g_thread_index], priority);\
    pthread_create(&g_thread[g_thread_index], &g_thread_attr[g_thread_index], fxn, &g_thread_startup[g_thread_index-1]);\
    g_thread_index++;\
}

uint8_t kbhit(int timeout){
    g_debug_cmd = 0;
    while(g_debug_cmd == 0 && timeout > 0){
        usleep(1000* 100);
        timeout -= 100;
    }
    return g_debug_cmd;
}
uint8_t kb_value(){ return g_debug_cmd;}
uint8_t is_char(uint8_t val){ return ((val >= 'a' && val <= 'z') || (val >= 'A' && val <= 'Z')); }
uint8_t is_number(uint8_t val){ return ((val >= '0' && val <= '9')); }
uint8_t kb_cmd(const char* cmd){
    uint8_t u8_cmd = 0;
    LREP("%s? ", cmd); 
    do{
        u8_cmd = kbhit(1000);
        if(u8_cmd == 13){
            LREP("\r\n");
            LREP("%s? ", cmd);
            cmd = 0;
        }
        if(!is_char(u8_cmd) && !is_number(u8_cmd)) u8_cmd = 0;
    }while(u8_cmd == 0);
    LREP("%c\r\n", u8_cmd);
    return u8_cmd;
}
int main(void)
{
    int i;    
    
    HW_Initalize();    
    g_debug_tx_buffer = mq_open(0, 512);

    for(i = 0; i < APP_THREAD_COUNT-1; i++)
        sem_init(&g_thread_startup[i], 0, 0);
    sem_init(&g_sem_debug, 0, 1);
//    sem_init(&g_mimac_access, 0, 1);
    pthread_attr_setstacksize(&g_thread_attr[0], configMINIMAL_STACK_SIZE*32);
    pthread_setschedprio(&g_thread[0], tskIDLE_PRIORITY + 2UL);
    pthread_create(&g_thread[0], &g_thread_attr[0], Thread_Startup, 0);
    
    DEFINE_THREAD(Thread_DebugTX, configMINIMAL_STACK_SIZE*8,  tskIDLE_PRIORITY + 1UL);
    DEFINE_THREAD(Thread_DebugRx, configMINIMAL_STACK_SIZE*32, tskIDLE_PRIORITY + 1UL);
    DEFINE_THREAD(Thread_RFIntr,  configMINIMAL_STACK_SIZE*16, tskIDLE_PRIORITY + 3UL);
    DEFINE_THREAD(Thread_MiwiTask,configMINIMAL_STACK_SIZE*32, tskIDLE_PRIORITY + 4UL);
    
    /* Start the RTOS Scheduler */
    vTaskStartScheduler();

    /* HALT */
    while(1);
}
#include <stdarg.h>
void LREP(char* s, ...){
    char szBuffer[128];
    int len;
    va_list arglist;
    va_start(arglist, s);
    memset(szBuffer, 0, 128);
    vsnprintf(szBuffer, 127, s, arglist);
    len = strlen(szBuffer);
    sem_wait(&g_sem_debug);
    mq_send(g_debug_tx_buffer, szBuffer, len, 0);
    sem_post(&g_sem_debug);
}
struct mac_mrf24j40_open_param  rf_mac_init;
extern void*		___dev_lookup_begin;
extern void*		___dev_lookup_end;
void *Thread_Startup(void *pvParameters){
    int i,j;
    struct termios2 opt;
    unsigned int uival;
    uint8_t userInput;
    struct setting_value setting;
    uint8_t u8aVal[8];

    
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
        LREP("\r\n____________________________");
        LREP("\r\n|-------- startup ---------|\r\n");
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
    // Miwi network
    MAC_mrf24j40_open(&g_rf_mac, &rf_mac_init);
    // signal all other thread startup
    //LREP("Thread startup is running\r\n");
    for(i = 0; i < APP_THREAD_COUNT-1; i++){
        sem_post(&g_thread_startup[i]);
    }
    srand(0);
    usleep(1000* 100);
MAIN_MENU:
    LREP("------- Main menu ------\r\n");
    LREP("1. Test\r\n");
    LREP("2. Setting\r\n");
    LREP("cmd? ");
    i = 3;
    do{
        userInput = kbhit(1000);
        if(userInput == 13){
            LREP("\r\n");
            LREP("%s? ", userInput);
            userInput = 0;
        }
        if(!is_char(userInput) && !is_number(userInput)) userInput = 0;
        LREP(".");
        i--;
    }while(userInput == 0 && i > 0);
    LREP("%c\r\n", userInput);
    switch(userInput){
        case '1':{
            Test_menu();
            goto MAIN_MENU;
            break;
        }
        case '2':{
            setting_menu();
            goto MAIN_MENU;
            break;
        }
    }
    LREP("\r\nGoto main app\r\n");
    setting_read(&g_setting_dev, &setting);
    LREP("Setting:\r\n");
    setting_dump_to_stdio(&setting);
    uival = 25;
    MAC_mrf24j40_ioctl(&g_rf_mac, mac_mrf24j40_ioc_set_channel, (unsigned int)&uival);
    for(i = 0; i < 8 ; i++)
        u8aVal[i] = setting.mac_long_address[i];
    MAC_mrf24j40_ioctl(&g_rf_mac, mac_mrf24j40_ioc_set_long_address, (unsigned int)u8aVal);
    if(setting.network_type == setting_network_type_pan_coordinator){
        uint8_t noise_level[15];
        uint8_t channels[15];
        LREP("Device as PAN coordinator\r\n");
        LREP("Scan free channel\r\n");
        for(i =0 ;i < 15; i++) channels[i] = i + 11;
        Network_scan_channel(&g_rf_mac, 0x03fff800, noise_level);
        // sort noise level
        for(i = 0; i < 15-1; i++){
            for(j = i+1; j < 15; j++){
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
        for(i = 0; i < 15; i++){
            LREP("Detect network on channel %d ...", channels[i]);
            LREP("not found\r\n");
            break;
        }
        if(i == 15) LREP("Not found free channel\r\n");
        else{
            LREP("Select free channel: %d\r\n", channels[i]);
        }
    }
    App_Initialize();
    while(1){sleep(1);}
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
void *Thread_DebugRx(void *pvParameters){
    int8_t u8val;
    int len;
    sem_t* sem_startup = (sem_t*)pvParameters;
    fd_set readfs;
    struct timeval timeout;

    FD_CLR(g_fd_debug, &readfs);
    timeout.tv_sec  = 1;
    timeout.tv_usec = 0;

    sem_wait(sem_startup);
    //LREP("Thread DebugRx is running\r\n");
    while (1) {
        len = select(g_fd_debug, (unsigned int*)&readfs, 0, 0, &timeout);
        if(len > 0){
            if(FD_ISSET(g_fd_debug, &readfs)){
                len = read(g_fd_debug, &u8val, 1);
                if(len > 0){
                    g_debug_cmd = u8val;
                    if(u8val == 'r') reboot();
                }
            }
        }else if(len == 0){
        }
        else{
            LREP("select uart failed %d.\r\n", len);
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
        LED_TOGGLE(ORANGE);
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


