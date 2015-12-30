/*
 * debug.c
 *
 *  Created on: Dec 30, 2015
 *      Author: dev
 */

#include "debug.h"
#include <unistd.h>
#include <asm/termios.h>


unsigned char g_kb_value = 0;
int     g_fd_debug_tx;
int 	g_fd_debug_rx;

unsigned char kb_value(){
    return g_kb_value;
}

int debug_init(){
	struct termios2 opt;

    g_fd_debug_tx = STDOUT_FILENO;
    g_fd_debug_rx = STDIN_FILENO;
    ioctl(g_fd_debug_rx, TCGETS2, &opt);
	opt.c_cc[VMIN]  = 1;
	opt.c_cc[VTIME] = 0;
	opt.c_lflag &= ~(ICANON | ECHO);
	ioctl(g_fd_debug_rx, TCSETS2, &opt);
	return 0;
}
// end of file
