/*
 * cli_app_setting.c
 *
 *  Created on: Dec 15, 2015
 *      Author: dev
 */
#include <cli.h>
#include <unistd.h>
#include <string.h>
#include <setting.h>
#include <stdlib.h>

extern struct setting_device       	g_setting_dev;

const char cli_app_setting_description[] = "Setting system";

int cli_app_setting_callback(int argc, char** argv, void* user);
struct cli_app_info g_cli_app_setting = {
	.cmd = "st",
	.desc = cli_app_setting_description,
	.callback = cli_app_setting_callback,
};
cli_app(g_cli_app_setting);

uint8_t cli_app_hex_2_dec(uint8_t hex){
    if(hex >= '0' && hex <= '9') return hex - '0';
    else if(hex >= 'a' && hex <= 'f') return hex - 'a' + 10;
    else if(hex >= 'A' && hex <= 'F') return hex - 'A' + 10;
    return 0;
}
void cli_app_st_help(){
	LREP("\r\n");
	LREP("\t %s ?                  Display help\r\n", g_cli_app_setting.cmd);
	LREP("\t %s dump               Dump setting to stdout\r\n", g_cli_app_setting.cmd);
	LREP("\t %s write param value  Write value to param\r\n", g_cli_app_setting.cmd);
}
void cli_app_st_dump(){
	struct setting_value setting;
    setting_read(&g_setting_dev, &setting);
    LREP("\r\nSetting:\r\n");
    setting_dump_to_stdio(&setting);
}
void cli_app_st_write(const char* param, const char* value){
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
            setting.mac_long_address[0] = ((cli_app_hex_2_dec(value[0]) & 0x0F) << 4) | (cli_app_hex_2_dec(value[1]) & 0x0F);
            setting.mac_long_address[1] = ((cli_app_hex_2_dec(value[3]) & 0x0F) << 4) | (cli_app_hex_2_dec(value[4]) & 0x0F);
            setting.mac_long_address[2] = ((cli_app_hex_2_dec(value[6]) & 0x0F) << 4) | (cli_app_hex_2_dec(value[7]) & 0x0F);
            setting.mac_long_address[3] = ((cli_app_hex_2_dec(value[9]) & 0x0F) << 4) | (cli_app_hex_2_dec(value[10]) & 0x0F);
            setting.mac_long_address[4] = ((cli_app_hex_2_dec(value[12]) & 0x0F) << 4) | (cli_app_hex_2_dec(value[13]) & 0x0F);
            setting.mac_long_address[5] = ((cli_app_hex_2_dec(value[15]) & 0x0F) << 4) | (cli_app_hex_2_dec(value[16]) & 0x0F);
            setting.mac_long_address[6] = ((cli_app_hex_2_dec(value[18]) & 0x0F) << 4) | (cli_app_hex_2_dec(value[19]) & 0x0F);
            setting.mac_long_address[7] = ((cli_app_hex_2_dec(value[21]) & 0x0F) << 4) | (cli_app_hex_2_dec(value[22]) & 0x0F);
            invalid = 1;
            LREP("\r\n");
        }
    }
    if(invalid){
        setting_write(&g_setting_dev, &setting);
        LREP("Write done\r\n");
    }

}
int cli_app_setting_callback(int argc, char** argv, void* user){
	if(argc < 2)
		cli_app_st_help();
	else{
		if(strcmp(argv[1], "?") == 0){
			cli_app_st_help();
		}
		if(strcmp(argv[1], "dump") == 0){
			cli_app_st_dump();
		}
		else if(strcmp(argv[1], "write") == 0){
			// write type val
			// write address xx:xx:xx:xx:xx:xx:xx:xx
			// write address auto
			if(argc >= 4){
				cli_app_st_write(argv[2], argv[3]);
			}else cli_app_st_help();
		}
	}
	return 0;
}



