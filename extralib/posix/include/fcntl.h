#ifndef FCNTL_H__
#define FCNTL_H__
#include <stddef.h>
#include <stdint.h>
#undef fd_set

#define FD_SET_MAXCNT	8
typedef struct{
	int fd[FD_SET_MAXCNT];
	unsigned int flags;
}fd_set;

struct timeval {
	long	tv_sec;		/* seconds */
	long	tv_usec;	/* and microseconds */
};

#define O_RDONLY	0x0000
#define O_WRONLY	0x0001
#define O_RDWR		0x0002

int 	open(const char *pathname, int flags);
int 	close	(int fd);
int		read(int fd, void *buf, size_t count);
int		write	(int fd, const void *buf, size_t count);
int 	ioctl	(int d, int request, unsigned int arguments);
/*On  success,  select()  and pselect() return the number of 
 * file descriptors contained in the three returned descriptor sets 
 * (that is, the total number of bits that are set in readfds, writefds, exceptfds) 
 * which may be zero if the timeout expires before anything interesting happens.
 * On error, -1 is returned, and errno is set appropriately; 
 * the sets and timeout become undefined, so do not rely on their contents after an error.
 */ 
int 	select(int nfds, fd_set *readfds, fd_set *writefds,
		  fd_set *exceptfds, struct timeval *timeout);

//void 	FD_CLR	(int fd, fd_set *set);
//int  	FD_ISSET(int fd, fd_set *set);
//void 	FD_SET	(int fd, fd_set *set);
//void 	FD_ZERO	(fd_set *set);

static __inline__ int  	FD_ISSET(int fd, fd_set *set){
	int i = 0;
	int ret = 0;
	for(i = 0; i < FD_SET_MAXCNT; i++)
		if(set->fd[i] == fd){
			if(set->flags & (((unsigned int)1) << i))
				ret = 1;
			break;
		}
	return ret;
}
static __inline__ void 	FD_SET	(int fd, fd_set *set){
	int i = 0;
	for(i = 0; i < FD_SET_MAXCNT; i++)
		if(set->fd[i] == fd){
			break;
		}
	if(i == FD_SET_MAXCNT){
		for(i = 0; i < FD_SET_MAXCNT; i++){
			if(set->fd[i] == -1){
				set->fd[i] = fd;
				set->flags |= (((unsigned int)1) << i);
				break;
			}
		}
	}
}
static __inline__ void 	FD_ZERO	(fd_set *set){
	int i = 0;
	for(i = 0; i < FD_SET_MAXCNT; i++)
		set->fd[i] = -1;
	set->flags = 0;
}
static __inline__ void 	FD_CLR	(int fd, fd_set *set){
	int i = 0;
	for(i = 0; i < FD_SET_MAXCNT; i++)
		if(set->fd[i] == fd){
			set->flags &= ~(((unsigned int)1) << i);
			set->fd[i] = -1;
			break;
		}
}
#endif
