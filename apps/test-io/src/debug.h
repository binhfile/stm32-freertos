/*
 * debug.h
 *
 *  Created on: Dec 30, 2015
 *      Author: dev
 */

#ifndef SRC_DEBUG_H_
#define SRC_DEBUG_H_
#include <stdio.h>

#define LREP(x, args...)	printf(x, ##args)
unsigned char kb_value();
int debug_init();
#endif /* SRC_DEBUG_H_ */
