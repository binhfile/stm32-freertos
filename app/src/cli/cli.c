/*
 * cli.c
 *
 *  Created on: Dec 15, 2015
 *      Author: dev
 */

#include "project_config.h"
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

extern sem_t              			g_sem_debug;
extern mqd_t              			g_debug_tx_buffer;
extern int                 			g_fd_debug_rx;
extern int                 			g_fd_debug_tx;

extern struct cli_app_info *___cli_app_begin;
extern struct cli_app_info *___cli_app_end;
#if defined(OS_FREERTOS)
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
#elif defined(OS_LINUX)
#include <stdio.h>
void LREP(char* s, ...){
    char szBuffer[128];
    int len;
    va_list arglist;
    va_start(arglist, s);
    memset(szBuffer, 0, 128);
    vsnprintf(szBuffer, 127, s, arglist);
    len = strlen(szBuffer);
    sem_wait(&g_sem_debug);
    printf("%s", szBuffer);fflush(stdout);
    sem_post(&g_sem_debug);
}
#endif
void CLI_display_help(){
	struct cli_app_info **app;
	LREP("\r\n");
	LREP("\t?/help\tDisplay help\r\n");
	app = &___cli_app_begin;
	while(app < &___cli_app_end){
		LREP("\t%s\t%s\r\n", (*app)->cmd, (*app)->desc);
		app++;
	}

}
int CLI_callback(int argc, char** argv, void* user){
	int ret = 0;
	struct cli_app_info **app;
	int found = 0;

	if(argc > 0){
		if(		argv[0][0] == '?' ||
				strcmp(argv[0], "help") == 0){
			CLI_display_help();
		}
		else{
			app = &___cli_app_begin;
			while(app < &___cli_app_end){
				if(strcmp((*app)->cmd, argv[0]) == 0){
					if((*app)->callback){
						(*app)->callback(argc, argv, user);
						found = 1;
						break;
					}
				}
				app++;
			}
			if(found == 0){
				LREP("\r\nUnknown command: %s\r\n", argv[0]);
			}
		}
	}
	return ret;
}
const char g_cli_baner[] = "\r\n"\
"__/\\__\r\n\
\\    /\r\n\
/_  _\\\r\n\
  \\/\r\n";
int CLI_loop(){
    struct lib_cli libcli;
    lib_cli_init(&libcli);
    lib_cli_set_readfd(&libcli, g_fd_debug_rx);
    lib_cli_set_writefd(&libcli, g_fd_debug_tx);
    lib_cli_set_promptchar(&libcli, '$');
    lib_cli_set_hostname(&libcli, "cli");
    lib_cli_set_banner(&libcli, g_cli_baner);
    lib_cli_register_callback(&libcli, CLI_callback, &libcli);
    while(1){
    	lib_cli_loop(&libcli, 1000);
    }
}
// end of text
