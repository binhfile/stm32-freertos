/*
 * cli_app_rand.c
 *
 *  Created on: Dec 15, 2015
 *      Author: dev
 */
#include <cli.h>
#include <stdlib.h>
#include <stdint.h>
const char cli_app_rand_description[] = "Get num*32 bits random";

int cli_app_rand_callback(int argc, char** argv, void* user);
struct cli_app_info g_cli_app_rand = {
	.cmd = "rand",
	.desc = cli_app_rand_description,
	.callback = cli_app_rand_callback,
};
cli_app(g_cli_app_rand);
int cli_app_rand_callback(int argc, char** argv, void* user){
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
	return 0;
}



