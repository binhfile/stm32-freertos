/*
 * spec_reimpl.c
 *
 *  Created on: Dec 14, 2015
 *      Author: dev
 */
#include <stdlib.h>
#include <fcntl.h>
#include <debug.h>

int g_fd_random = 0;
int rand(void){
	uint32_t u32Val = 0;
	int ret = read(g_fd_random, &u32Val, sizeof(u32Val));
	if(ret != sizeof(u32Val)){
		LREP("read random failed %d\r\n", ret);
	}
	return u32Val;
}
void srand(unsigned int seed){
	g_fd_random = open("random-dev", 0);
	if(g_fd_random < 0) LREP("open random device failed\r\n");
}


