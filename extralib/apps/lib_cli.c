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
#if defined(OS_LINUX)
#include <stdio.h>
#include <sys/select.h>
#endif

int lib_cli_init(struct lib_cli *inst){
	int ret = 0;
	inst->fd_write = -1;
	inst->fd_read = -1;
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
int lib_cli_set_readfd(struct lib_cli *inst, int fd){
	int ret = 0;
	inst->fd_read = fd;
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
			(val == '-' || val == ' ' || val == '?');
}
inline uint8_t lib_cli_is_space(uint8_t val){
	return ((val == ' '));
}
//#define lib_cli_write(x)	write(inst->write_fd, x, strlen(x))
#define lib_cli_write(x)	LREP("%s", x)

#define CLI_MAX_ARGV		10
#define CLI_BUF_MAX_LEN		64
int lib_cli_loop(struct lib_cli *inst, int timeout){
	int ret = 0;
	int len, i;
    fd_set readfs;
    struct timeval s_timeout;
    uint8_t buff[32];
    uint8_t prompt[32];
    uint8_t buff_process[CLI_BUF_MAX_LEN+1];
    int argc_index;
    char *argv[CLI_MAX_ARGV];
    int buff_index;
    int argc_cnt;
    int full;

    lib_cli_write(inst->banner);
    snprintf((char*)prompt, 31, "%s%c ", inst->hostname, inst->prompt);
    lib_cli_write(prompt);
    buff_index = 0;
    while(1){
        s_timeout.tv_sec  = 1;
        s_timeout.tv_usec = 0;
        FD_ZERO(&readfs);
        FD_SET(inst->fd_read, &readfs);

		len = select(inst->fd_read+1, &readfs, 0, 0, &s_timeout);
		if(len > 0){
			if(FD_ISSET(inst->fd_read, &readfs)){
				len = read(inst->fd_read, buff, 32);
				if(len > 0){
					for(i = 0; i < len; i++){
						if(lib_cli_is_char_valid(buff[i])){
							write(inst->fd_write, &buff[i], 1);
							if(buff_index < CLI_BUF_MAX_LEN) buff_process[buff_index++] = buff[i];
						}else if(buff[i] == 13 || buff[i] == 10){
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
									argv[0] = (char*)&buff_process[i];
									argc_cnt = 1;
								}
								while(i < buff_index){
									if(full) break;
									if(lib_cli_is_space(buff_process[i])){
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
									i++;
								}// end while
							}
							if(argc_cnt > 0){
								if(inst->cb_fxn){
									inst->cb_fxn(argc_cnt, argv, inst->cb_user);
								}
							}
							lib_cli_write("\r\n");
							lib_cli_write(prompt);
							buff_index = 0;
						}// ENTER
						else
						{
							if(buff[i] == 0x08){// backspace
								if(buff_index > 0){
									write(inst->fd_write, &buff[i], 1);
									buff[i] = ' ';
									write(inst->fd_write, &buff[i], 1);
									buff[i] = 0x08;
									write(inst->fd_write, &buff[i], 1);
									buff_index--;
								}
							}
							else if(buff[i] == 0x03){// end of text (^C)
								buff_index = 0;
								lib_cli_write("\r\n");
								lib_cli_write(prompt);
							}
                            //else LREP("{%02X}", buff[i]);
						}
					}
				}
			}
		}else if(len == 0){
		}
		else{
			lib_cli_write("select uart failed.\r\n");
			break;
		}
    }

	return ret;
}

// end of file
