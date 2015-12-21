/*
 * flag_event.c
 *
 *  Created on: Dec 19, 2015
 *      Author: dev
 */
#include "flag_event.h"
#include <sys/eventfd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int flag_event_init(flag_event_t* event){
	*event = eventfd(0, 0);
	return *event;
}
int flag_event_destroy(flag_event_t *event){
	close(*event);
	return 0;
}
int flag_event_post(flag_event_t *event){
	uint8_t u8val = 1;
	write(*event, &u8val, 1);
	return 0;
}
int flag_event_wait(flag_event_t *event){
	int ret;
	uint8_t u8val = 0;
	ret = read(*event, &u8val, 1);
	if(u8val == 1) return 1;
	return 0;
}
int flag_event_timedwait(flag_event_t *event, const struct timespec *abs_timeout){
	fd_set rfds;
	struct timeval tv;
	int retval;
	int ret = -1;
	uint8_t u8val;

	/* Watch stdin (fd 0) to see when it has input. */
	FD_ZERO(&rfds);
	FD_SET(*event, &rfds);

	/* Wait up to five seconds. */
	tv.tv_sec = abs_timeout->tv_sec;
	tv.tv_usec = abs_timeout->tv_nsec / 1000;

	retval = select(*event+1, &rfds, 0, 0, &tv);
	/* Don't rely on the value of tv now! */

	if (retval == -1){

	}
	else if (retval){
		read(*event, &u8val, 1);
		ret = 1;
	}
	else{
		ret = 0;
	}
	return ret;
}



