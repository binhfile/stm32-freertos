/*
 * spec_reimpl.c
 *
 *  Created on: Dec 14, 2015
 *      Author: dev
 */
#include <stdlib.h>
#include <fcntl.h>

int g_fd_random = 0;
int rand(void){
	uint32_t u32Val = 0;
	read(g_fd_random, &u32Val, sizeof(u32Val));
	return u32Val;
}
void srand(unsigned int seed){
	g_fd_random = open("random-dev", 0);
}


