/*
 * cli.cpp
 *
 *  Created on: Dec 15, 2015
 *      Author: dev
 */

#include "lib_cli.h"
#include <fcntl.h>
#include <string.h>
#include <debug.h>
#include <stdio.h>
#if defined(OS_LINUX)
#include <sys/select.h>
#endif

int lib_cli_init(struct lib_cli *inst){
	int ret = 0;
	inst->fd_write = -1;
	inst->cb_fxn = 0;
	inst->cb_user = 0;
	inst->prompt = '$';
	inst->hostname = "";
	inst->banner = "";
	return ret;
}
int lib_cli_register_callback(struct lib_cli *inst, lib_cli_callback cb, void* cb_user){
	int ret = 0;
	inst->cb_fxn = cb;
	inst->cb_user = cb_user;
	return ret;
}
int lib_cli_set_writefd(struct lib_cli *inst, int fd){
	int ret = 0;
	inst->fd_write = fd;
	return ret;
}
int lib_cli_set_promptchar(struct lib_cli *inst, char val){
	int ret = 0;
	inst->prompt = val;
	return ret;
}
int lib_cli_set_hostname(struct lib_cli *inst, const char* hostname){
	int ret = 0;
	inst->hostname = hostname;
	return ret;
}
int lib_cli_set_banner(struct lib_cli *inst, const char* banner){
	int ret = 0;
	inst->banner = banner;
	return ret;
}
inline uint8_t lib_cli_is_char_valid(uint8_t val){
	//return (val >= 32 && val <= 126);
	return (val >= '0' && val <= '9') ||
			(val >= 'A' && val <= 'Z') ||
			(val >= 'a' && val <= 'z') ||
			(val == '-' || val == ' ' || val == '?' ||  val == '/' ||  val == ':' ||  val == '"');
}
inline uint8_t lib_cli_is_space(uint8_t val){
	return ((val == ' '));
}
//#define lib_cli_write(x)	write(inst->write_fd, x, strlen(x))
#define lib_cli_write(x)	LREP("%s", x)

#define CLI_MAX_ARGV		10
#define CLI_BUF_MAX_LEN		64
int lib_cli_start(struct lib_cli *inst){
	uint8_t prompt[32];
	lib_cli_write(inst->banner);
	snprintf((char*)prompt, 31, "%s%c ", inst->hostname, inst->prompt);
	lib_cli_write(prompt);
	return 0;
}
int lib_cli_process(struct lib_cli *inst, unsigned char* data, int length){
	int ret = 0;
	int i;
    uint8_t prompt[32];
    char *argv[CLI_MAX_ARGV];

    static uint8_t buff_process[CLI_BUF_MAX_LEN+1];
    static int buff_index = 0;
    static int argc_index = 0;
    static int argc_cnt;
    static int full = 0;
    static int has_long_arg = 0;

	for(i = 0; i < length; i++){
		if(lib_cli_is_char_valid(data[i])){
			write(inst->fd_write, &data[i], 1);
			if(buff_index < CLI_BUF_MAX_LEN) buff_process[buff_index++] = data[i];
		}else if(data[i] == 13 || data[i] == 10){
			// enter
			argc_cnt = 0;
			if(buff_index > 0){
				buff_process[buff_index] = '\0';
				argc_index = 0;
				full = 0;
				i = 0;
				// ignore first space
				while(i < buff_index){
					if(lib_cli_is_space(buff_process[i]) == 0) break;
					i++;
				}
				if(i < buff_index){
					argv[0] = (char*)&buff_process[i];// assign first arg
					argc_cnt = 1;
				}
				while(i < buff_index){
					if(full) break;
					if(buff_process[i] == '"'){
						if(!has_long_arg && (i + 1 < buff_index)){
							has_long_arg = 1;
							i++;
						}
					}
					if(has_long_arg){
						if(buff_process[i] == '"')
						{
							has_long_arg = 0;
							buff_process[i] = '\0';
							if(argc_index == CLI_MAX_ARGV-1) {
								full = 1;
								break;
							}
							// new arg
							if(i+1 < buff_index &&
									!lib_cli_is_space(buff_process[i+1]) &&
									argc_index < CLI_MAX_ARGV-1){
								argc_index++;
								argv[argc_cnt] = (char*)&buff_process[i+1];
								argc_cnt++;
							}
						}
					}else{
						if(lib_cli_is_space(buff_process[i]))
						{
							buff_process[i] = '\0';
							if(argc_index == CLI_MAX_ARGV-1) {
								full = 1;
								break;
							}
							// new arg
							if(i+1 < buff_index &&
									!lib_cli_is_space(buff_process[i+1]) &&
									argc_index < CLI_MAX_ARGV-1){
								argc_index++;
								argv[argc_cnt] = (char*)&buff_process[i+1];
								argc_cnt++;
							}
						}
					}
					i++;
				}// end while
			}
			if(argc_cnt > 0){
				for(i = 1; i < argc_cnt; i++){
					if(*(argv[i]) == '"'){
						argv[i]++;
					}
				}
				if(inst->cb_fxn){
					inst->cb_fxn(argc_cnt, argv, inst->cb_user);
				}
			}
			lib_cli_write("\r\n");
			snprintf((char*)prompt, 31, "%s%c ", inst->hostname, inst->prompt);
			lib_cli_write(prompt);
			buff_index = 0;
			has_long_arg = 0;
		}// ENTER
		else
		{
			if(data[i] == 0x08){// backspace
				if(buff_index > 0){
					write(inst->fd_write, &data[i], 1);
					data[i] = ' ';
					write(inst->fd_write, &data[i], 1);
					data[i] = 0x08;
					write(inst->fd_write, &data[i], 1);
					buff_index--;
				}
			}
			else if(data[i] == 0x03){// end of text (^C)
				buff_index = 0;
				lib_cli_write("\r\n");
				lib_cli_write(prompt);
			}
			//else LREP("{%02X}", buff[i]);
		}
	}

	return ret;
}

// end of file
