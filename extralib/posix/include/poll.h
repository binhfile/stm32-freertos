/*
 * poll.h
 *
 *  Created on: Dec 31, 2015
 *      Author: dev
 */

#ifndef POSIX_INCLUDE_POLL_H_
#define POSIX_INCLUDE_POLL_H_

/* Event types that can be polled for.  These bits may be set in `events'
   to indicate the interesting event types; they will appear in `revents'
   to indicate the status of the file descriptor.  */
#define POLLIN		((short int)0x001)		/* There is data to read.  */
#define POLLPRI		((short int)0x002)		/* There is urgent data to read.  */
#define POLLOUT		((short int)0x004)		/* Writing now will not block.  */

/* Event types always implicitly polled for.  These bits need not be set in
   `events', but they will appear in `revents' to indicate the status of
   the file descriptor.  */
#define POLLERR		((short int)0x008)		/* Error condition.  */
#define POLLHUP		((short int)0x010)		/* Hung up.  */
#define POLLNVAL	((short int)0x020)		/* Invalid polling request.  */

/* Type used for the number of file descriptors.  */
typedef unsigned long int nfds_t;

/* Data structure describing a polling request.  */
struct pollfd
  {
    int fd;					/* File descriptor to poll.  */
    short int events;		/* Types of events poller cares about.  */
    short int revents;		/* Types of events that actually occurred.  */
  };
int poll (struct pollfd *__fds, nfds_t __nfds, int __timeout);


#endif /* POSIX_INCLUDE_POLL_H_ */
