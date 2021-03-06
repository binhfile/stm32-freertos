/* 
 * File:   unistd.h
 * Author: dev
 *
 * Created on October 26, 2015, 10:04 AM
 */

#ifndef UNISTD_H
#define	UNISTD_H

#ifdef	__cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <time.h>
#ifndef __useconds_t
#define __useconds_t    uint32_t
#endif

#define SEEK_SET	0

unsigned int 	sleep (unsigned int __seconds);
int 			usleep (unsigned int __useconds);/*security issue*/
int 			clock_gettime(clockid_t clk_id, struct timespec *tp);
unsigned long   lseek (int __fd, unsigned long __offset, int __whence);
#ifdef	__cplusplus
}
#endif

#endif	/* UNISTD_H */

