/*
 * cli.c
 *
 *  Created on: Dec 15, 2015
 *      Author: dev
 */

#include "cli.h"
#include <lib_cli.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <mqueue.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <drv_api.h>
#include <drv_gpio.h>
#include <reboot.h>

#include <setting.h>

#include <Test.h>
#include <Network.h>

extern sem_t              			g_sem_debug;
extern mqd_t              			g_debug_tx_buffer;
extern int                 			g_fd_debug;
extern struct mac_mrf24j40         	g_rf_mac;
extern struct setting_device       	g_setting_dev;

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
void CLI_display_help(){
	LREP("\r\n");
	LREP("\t ?/help  Display help\r\n");
	LREP("\t st      Setting\r\n");
	LREP("\t reboot  Reboot system\r\n");
	LREP("\t uname   Print system infomation\r\n");
	LREP("\t rand    Get num*32 bits random\r\n");
	LREP("\t mac     MAC layer tools\r\n");
    LREP("\t network Network layer tools\r\n");
    LREP("\t date    Print or set the system date and time\r\n");
}
void CLI_st_display_help(){
	LREP("\r\n");
	LREP("\t st ?                  Display help\r\n");
	LREP("\t st dump               Dump setting to stdout\r\n");
	LREP("\t st write param value  Write value to param\r\n");
}
void CLI_st_dump(){
	struct setting_value setting;
    setting_read(&g_setting_dev, &setting);
    LREP("\r\nSetting:\r\n");
    setting_dump_to_stdio(&setting);
}
void CLI_mac_help(){
    LREP("\r\n");
    LREP("\t mac ?     Display help\r\n");
    LREP("\t mac loop  Loop received packets to sender\r\n");
    LREP("\t mac send  Send and verify received packets from repeater\r\n");
}
void CLI_network_help(){
    LREP("\r\n");
    LREP("\t network ?     Display help\r\n");
    LREP("\t network scan  Scan free channel\r\n");
}
uint8_t CLI_hex_2_dec(uint8_t hex){
    if(hex >= '0' && hex <= '9') return hex - '0';
    else if(hex >= 'a' && hex <= 'f') return hex - 'a' + 10;
    else if(hex >= 'A' && hex <= 'F') return hex - 'A' + 10;
    return 0;
}
void CLI_st_write(const char* param, const char* value){
   	struct setting_value setting;
    int invalid = 0;
    int ival, i;
    setting_read(&g_setting_dev, &setting);
    if(strcmp(param, "type") == 0){
        ival = strtol(value, 0, 10);
        LREP("\r\nwrite type = %d\r\n", ival);
        setting.network_type = ival;
        invalid = 1;
    }
    if(strcmp(param, "address") == 0){
        if(strcmp(value, "auto") == 0){ 
            uint32_t u32Val = rand();
			setting.mac_long_address[0] = (uint8_t)(u32Val & (uint32_t)0x00FF);
            setting.mac_long_address[1] = (uint8_t)((u32Val>> 8) & (uint32_t)0x00FF);
			setting.mac_long_address[2] = (uint8_t)((u32Val>> 16) & (uint32_t)0x00FF);
			setting.mac_long_address[3] = (uint8_t)((u32Val>> 24) & (uint32_t)0x00FF);
			u32Val = rand();
			setting.mac_long_address[4] = (uint8_t)(u32Val & (uint32_t)0x00FF);
			setting.mac_long_address[5] = (uint8_t)((u32Val>> 8) & (uint32_t)0x00FF);
			setting.mac_long_address[6] = (uint8_t)((u32Val>> 16) & (uint32_t)0x00FF);
			setting.mac_long_address[7] = (uint8_t)((u32Val>> 24) & (uint32_t)0x00FF);
			LREP("\r\nnew address: ");
			for(i = 0; i < 7 ; i ++) LREP("%02X:", setting.mac_long_address[i]);
			LREP("%02X\r\n", setting.mac_long_address[i]);
            invalid = 1;
            LREP("\r\n");
        }else if(strlen(value)>= strlen("xx:xx:xx:xx:xx:xx:xx:xx")){
            setting.mac_long_address[0] = ((CLI_hex_2_dec(value[0]) & 0x0F) << 4) | (CLI_hex_2_dec(value[1]) & 0x0F);
            setting.mac_long_address[1] = ((CLI_hex_2_dec(value[3]) & 0x0F) << 4) | (CLI_hex_2_dec(value[4]) & 0x0F);
            setting.mac_long_address[2] = ((CLI_hex_2_dec(value[6]) & 0x0F) << 4) | (CLI_hex_2_dec(value[7]) & 0x0F);
            setting.mac_long_address[3] = ((CLI_hex_2_dec(value[9]) & 0x0F) << 4) | (CLI_hex_2_dec(value[10]) & 0x0F);
            setting.mac_long_address[4] = ((CLI_hex_2_dec(value[12]) & 0x0F) << 4) | (CLI_hex_2_dec(value[13]) & 0x0F);
            setting.mac_long_address[5] = ((CLI_hex_2_dec(value[15]) & 0x0F) << 4) | (CLI_hex_2_dec(value[16]) & 0x0F);
            setting.mac_long_address[6] = ((CLI_hex_2_dec(value[18]) & 0x0F) << 4) | (CLI_hex_2_dec(value[19]) & 0x0F);
            setting.mac_long_address[7] = ((CLI_hex_2_dec(value[21]) & 0x0F) << 4) | (CLI_hex_2_dec(value[22]) & 0x0F);
            invalid = 1;
            LREP("\r\n");
        }
    }
    if(invalid){
        setting_write(&g_setting_dev, &setting);
        LREP("Write done\r\n");
    }

}
int CLI_callback(int argc, char** argv, void* user){
	int ret = 0;
	if(argc > 0){
		if(		argv[0][0] == '?' ||
				strcmp(argv[0], "help") == 0){
			CLI_display_help();
		}
		else if(strcmp(argv[0], "st") == 0){
			if(argc < 2)
				CLI_st_display_help();
			else{
				if(strcmp(argv[1], "?") == 0){
					CLI_st_display_help();
				}
				if(strcmp(argv[1], "dump") == 0){
					CLI_st_dump();
				}
				else if(strcmp(argv[1], "write") == 0){
                    // write type val
                    // write address xx:xx:xx:xx:xx:xx:xx:xx
                    // write address auto
                    if(argc >= 4){
					    CLI_st_write(argv[2], argv[3]);
                    }else CLI_st_display_help();
				}
			}
		}
		else if(strcmp(argv[0], "reboot") == 0){
			LREP("\r\nrebooting...");
			usleep(1000*100);
			reboot();
		}
		else if(strcmp(argv[0], "uname") == 0){
			LREP("\r\nSystem version 0.1 build %s %s", __TIME__, __DATE__);
		}
		else if(strcmp(argv[0], "rand") == 0){
			if(argc >=  2){
				int num = strtol(argv[1], 0, 10);
				uint32_t u32Val;
				int i;
				if(num < 0) num = 0;
				if(num > 1024) num = 1024;
				for(i = 0; i < num; i++){
					if(i % 8 == 0) LREP("\r\n");
					u32Val = rand();
					LREP("%08X ", u32Val);
				}
				 LREP("\r\nFinish gen %d elements\r\n", num);
			}
		}
        else if(strcmp(argv[0], "mac") == 0){
            if(argc >= 2){
                if(strcmp(argv[1], "?") == 0) CLI_mac_help();
                else if(strcmp(argv[1], "loop") == 0){
                    LREP("\r\nloop packets\r\n");
                    MAC_test_loop_received_packets();
                }else if(strcmp(argv[1], "send") == 0){
                    LREP("\r\nsend and verify packets\r\n");
                    MAC_test_send_and_check_packets();
                }

            }else CLI_mac_help();
        }
        else if(strcmp(argv[0], "network") == 0){
            if(argc >= 2){
                if(strcmp(argv[1], "?") == 0) CLI_network_help();
                else if(strcmp(argv[1], "scan") == 0){
                    uint8_t noise_level[13];
                    LREP("\r\n");
                    Network_scan_channel(&g_rf_mac, 0x03FFF800, noise_level);
                    LREP("\r\n");
               }
                
            }else CLI_network_help();
        }
        else if(strcmp(argv[0], "date") == 0){
        	if(argc >= 2){
        		if(strcmp(argv[1], "-s") == 0 && argc >= 3 &&
        				strlen(argv[2]) >= strlen("yyyy/MM/dd hh/mm/ss")){
        			// format 'yyyy/MM/dd hh/mm/ss'
        			time_t tm_t;
        			struct tm s_tm;
        			s_tm.tm_year = (argv[2][0] - '0')*1000 +
        					(argv[2][1] - '0') * 100 +
							(argv[2][2] - '0') * 10 +
							(argv[2][3] - '0');
        			s_tm.tm_mon = (argv[2][5] - '0')*10+(argv[2][6] - '0');
        			s_tm.tm_mday = (argv[2][8] - '0')*10+(argv[2][9] - '0');
        			s_tm.tm_hour = (argv[2][11] - '0')*10+(argv[2][12] - '0');
        			s_tm.tm_min = (argv[2][14] - '0')*10+(argv[2][15] - '0');
        			s_tm.tm_sec = (argv[2][17] - '0')*10+(argv[2][18] - '0');

        			if(s_tm.tm_year >= 1970 && s_tm.tm_year <= 2099 &&
        					s_tm.tm_mon > 0 && s_tm.tm_mon <=12 &&
							s_tm.tm_mday > 0 && s_tm.tm_mday <= 31 &&
							s_tm.tm_hour >= 0 && s_tm.tm_hour < 24 &&
							s_tm.tm_min >= 0 && s_tm.tm_min < 60 &&
							s_tm.tm_sec >= 0 && s_tm.tm_sec < 60){
        				tm_t = mktime(&s_tm);
        				stime(&tm_t);
        			}else{
        				LREP("\r\nInvalid param\r\n");
        			}
        		}else {
        			LREP("\r\nInvalid param\r\n");
        		}
        	}else{
        		time_t tm_t = time(0);
        		struct tm tm_result;
        		localtime_r(&tm_t, &tm_result);
        		LREP("\r\n%d-%d-%d %d:%d:%d\r\n",
        				tm_result.tm_year,
						tm_result.tm_mon,
						tm_result.tm_mday,
						tm_result.tm_hour,
						tm_result.tm_min,
						tm_result.tm_sec);
        	}
        }
		else{
			LREP("\r\nUnknown command: %s\r\n", argv[0]);
		}
	}
	return ret;
}
int CLI_loop(){
    struct lib_cli libcli;
    lib_cli_init(&libcli);
    lib_cli_set_readfd(&libcli, g_fd_debug);
    lib_cli_set_writefd(&libcli, g_fd_debug);
    lib_cli_set_promptchar(&libcli, '$');
    lib_cli_set_hostname(&libcli, "cli");
    lib_cli_set_banner(&libcli, "\r\nWelcome to CLI\r\n");
    lib_cli_register_callback(&libcli, CLI_callback, &libcli);
    while(1){
    	lib_cli_loop(&libcli, 1000);
    }
}
// end of text
