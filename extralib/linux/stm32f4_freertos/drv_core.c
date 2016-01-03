#include "drv_api.h"
#include "drv_errno.h"
#include "string.h"
#include "fcntl.h"
#include "FreeRTOS.h"

#include <poll.h>

int    errno = 0;
struct platform_driver* g_list_drivers	= 0;
extern init_fxn		___drv_init_begin;
extern init_fxn		___drv_init_end;

extern void*		___dev_lookup_begin;
extern void*		___dev_lookup_end;

int platform_driver_register(struct platform_driver *driver){
	int ret = -EPERM;
	struct platform_driver *drv = 0;

	drv = g_list_drivers;
	if(drv){
		while(drv->next)
			drv = drv->next;
		drv->next = driver;
		ret = 0;
	}else{
		g_list_drivers = driver;
		ret = 0;
	}
	driver->next = 0;
	return ret;
}
int platform_device_register(struct platform_device *pdev){
	int ret = -EPERM;
	struct platform_driver *drv 	= 0;
	struct platform_device *p 		= 0;
	struct platform_driver *p_drv 	= g_list_drivers;
	void** p_dev;

	static int s_dev_index = 0;

	p_dev = &___dev_lookup_begin;
	while(p_drv){
		if(strcmp(p_drv->driver.name, pdev->name) == 0){
			drv = p_drv;
			break;
		}
		p_drv = p_drv->next;
	}
	if(drv){
		p = drv->driver.devices;
		if(p){
			while(p->next) p = p->next;
			p->next = pdev;
			p_dev[s_dev_index] = pdev;
			s_dev_index++;
		}else{
			drv->driver.devices = pdev;
			p_dev[s_dev_index] = pdev;
			s_dev_index++;
		}
		pdev->next = 0;
		pdev->driver = drv;
		ret = 0;
	}
	return ret;
}
int driver_probe(){
	init_fxn *elem;
    elem = &___drv_init_begin;
    while(elem < &___drv_init_end){
        (*elem)();
        elem++;
    }
    return 0;
}
int open(const char *pathname, int flags){
#if 0
	int ret = -EPERM;
	struct platform_driver *drv = g_list_drivers;
	struct platform_device *pdev = 0;
	int found = 0;
	int drv_index = 0;
	int dev_index = 0;

	if(!g_list_drivers) {
		return ret;
	}
	while(drv){
		dev_index = 0;
		pdev = drv->driver.devices;
		while(pdev){
			if(strcmp(pdev->dev_name, pathname) == 0){
				found = 1;
				break;
			}
			pdev = pdev->next;
			dev_index++;
		}
		if(found) break;
		drv = drv->next;
		drv_index++;
	}
	if(found){
		found = drv->open(pdev, flags);
		if(found >= 0){
			ret = ((((uint16_t)(drv_index)) & 0x00FF) << 8) | (((uint16_t)dev_index) & 0x00FF);
		}else{
			ret = -EPERM;
		}
	}else{
	}
	return ret;
#else
	int ret = -EPERM;
	struct platform_driver *drv = 0;
	struct platform_device **pdev = (struct platform_device **)&___dev_lookup_begin;
	int found = 0;
	int dev_index = 0;

	while(pdev < (struct platform_device **)&___dev_lookup_end){
		if(strcmp((*pdev)->dev_name, pathname) == 0){
			found = 1;
			break;
		}
		pdev++;
		dev_index++;
	}
	if(found){
		drv = (*pdev)->driver;
		found = drv->open((*pdev), flags);
		if(found >= 0){
			ret = dev_index;
		}else{
			ret = -EPERM;
		}
	}else{
	}
	return ret;
#endif
}
int 	close	(int fd){
#if 0
	int ret = -EPERM;
	struct platform_driver *drv = g_list_drivers;
	struct platform_device *pdev = 0;
	int drv_index = 0;
	int dev_index = 0;

	drv_index = (((uint16_t)fd) & 0xFF00) >> 8;
	dev_index = (((uint16_t)fd) & 0x00FF);

	while(drv_index > 0){
		if(drv){
			drv = drv->next;
		}else break;
		drv_index --;
	}
	if(drv_index > 0 || !drv) return ret;
	pdev = drv->driver.devices;
	while(dev_index){
		if(pdev){
			pdev = pdev->next;
		}else break;
		dev_index--;
	}
	if(dev_index > 0 || !pdev) return ret;
	if(drv->close)
		ret = drv->close(pdev);
	return ret;
#else
	int ret = -EPERM;
	struct platform_driver *drv = 0;
	struct platform_device **pdev = (struct platform_device **)&___dev_lookup_begin;

	if(fd >= 0 && fd <= (&___dev_lookup_end - &___dev_lookup_begin)){
		drv = (pdev[fd])->driver;
		if(drv->close)
			ret = drv->close(pdev[fd]);
	}
	return ret;
#endif
}
int 	write	(int fd, const void *buf, size_t count){
#if 0
	int ret = -EPERM;
	struct platform_driver *drv = g_list_drivers;
	struct platform_device *pdev = 0;
	int drv_index = 0;
	int dev_index = 0;

	drv_index = (((uint16_t)fd) & 0xFF00) >> 8;
	dev_index = (((uint16_t)fd) & 0x00FF);

	while(drv_index > 0){
		if(drv){
			drv = drv->next;
		}else break;
		drv_index --;
	}
	if(drv_index > 0 || !drv) return ret;
	pdev = drv->driver.devices;
	while(dev_index){
		if(pdev){
			pdev = pdev->next;
		}else break;
		dev_index--;
	}
	if(dev_index > 0 || !pdev) return ret;
	if(drv->write)
		ret = drv->write(pdev, buf, count);
	return ret;
#else
	int ret = -EPERM;
	struct platform_driver *drv = 0;
	struct platform_device **pdev = (struct platform_device **)&___dev_lookup_begin;

	if(fd >= 0 && fd <= (&___dev_lookup_end - &___dev_lookup_begin)){
		drv = (pdev[fd])->driver;
		if(drv->write)
			ret = drv->write(pdev[fd], buf, count);
	}
	return ret;
#endif
}
int 	read	(int fd, void *buf, size_t count){
#if 0
	int ret = -EPERM;
	struct platform_driver *drv = g_list_drivers;
	struct platform_device *pdev = 0;
	int drv_index = 0;
	int dev_index = 0;

	drv_index = (((uint16_t)fd) & 0xFF00) >> 8;
	dev_index = (((uint16_t)fd) & 0x00FF);

	while(drv_index > 0){
		if(drv){
			drv = drv->next;
		}else break;
		drv_index --;
	}
	if(drv_index > 0 || !drv) return ret;
	pdev = drv->driver.devices;
	while(dev_index){
		if(pdev){
			pdev = pdev->next;
		}else break;
		dev_index--;
	}
	if(dev_index > 0 || !pdev) return ret;
	if(drv->read)
		ret = drv->read(pdev, buf, count);
	return ret;
#else
	int ret = -EPERM;
	struct platform_driver *drv = 0;
	struct platform_device **pdev = (struct platform_device **)&___dev_lookup_begin;

	if(fd >= 0 && fd <= (&___dev_lookup_end - &___dev_lookup_begin)){
		drv = (pdev[fd])->driver;
		if(drv->read)
			ret = drv->read(pdev[fd], buf, count);
	}
	return ret;
#endif
}
int 	ioctl	(int fd, int request, unsigned int arguments){
#if 0
	int ret = -EPERM;
	struct platform_driver *drv = g_list_drivers;
	struct platform_device *pdev = 0;
	int drv_index = 0;
	int dev_index = 0;

	drv_index = (((uint16_t)fd) & 0xFF00) >> 8;
	dev_index = (((uint16_t)fd) & 0x00FF);
	
	while(drv_index > 0){
		if(drv){
			drv = drv->next;
		}else break;
		drv_index --;
	}
	if(drv_index > 0 || !drv) return ret;
	pdev = drv->driver.devices;
	while(dev_index){
		if(pdev){
			pdev = pdev->next;
		}else break;
		dev_index--;
	}
	if(dev_index > 0 || !pdev) return ret;
	if(drv->ioctl)
		ret = drv->ioctl(pdev, request, arguments);
	return ret;
#else
	int ret = -EPERM;
	struct platform_driver *drv = 0;
	struct platform_device **pdev = (struct platform_device **)&___dev_lookup_begin;

	if(fd >= 0 && fd <= (&___dev_lookup_end - &___dev_lookup_begin)){
		drv = (pdev[fd])->driver;
		if(drv->ioctl)
			ret = drv->ioctl(pdev[fd], request, arguments);
	}
	return ret;
#endif
}
int 	select(int fd, fd_set *readfds, fd_set *writefds,
		  fd_set *exceptfds, struct timeval *timeout){
#if 0
	int ret = -EPERM;
	struct platform_driver *drv = g_list_drivers;
	struct platform_device *pdev = 0;
	int drv_index = 0;
	int dev_index = 0;
	int readfd = 0, writefd = 0, errorfd = 0;
	int s_timeout = 0;

	drv_index = (((uint16_t)fd) & 0xFF00) >> 8;
	dev_index = (((uint16_t)fd) & 0x00FF);
	
	while(drv_index > 0){
		if(drv){
			drv = drv->next;
		}else break;
		drv_index --;
	}
	if(drv_index > 0 || !drv) return ret;
	pdev = drv->driver.devices;
	while(dev_index){
		if(pdev){
			pdev = pdev->next;
		}else break;
		dev_index--;
	}
	if(dev_index > 0 || !pdev) return ret;
	if(drv->select){
		if(readfds) readfd 		= 1;
		if(writefds) writefd 	= 1;
		if(exceptfds) errorfd 	= 1;
		
		s_timeout = 0;
		if(timeout){
			s_timeout = timeout->tv_sec * 1000 / portTICK_PERIOD_MS;
			s_timeout += timeout->tv_usec /1000 / portTICK_PERIOD_MS;
		}		
		ret = drv->select(pdev, &readfd, &writefd, &errorfd, s_timeout);
		if(ret > 0){
			if(readfd) 	FD_SET(fd, readfds);
			if(writefd) FD_SET(fd, writefds);
			if(errorfd) FD_SET(fd, exceptfds);
		}
	}
	return ret;
#else
	int ret = -EPERM;
	struct platform_driver *drv = 0;
	struct platform_device **pdev = (struct platform_device **)&___dev_lookup_begin;
	int readfd = 0, writefd = 0, errorfd = 0;
	int s_timeout = 0;

	fd = fd - 1;
	if(fd >= 0 && fd <= (&___dev_lookup_end - &___dev_lookup_begin)){
		drv = (pdev[fd])->driver;
		if(drv->select){
			if(readfds) {
				FD_CLR(fd, readfds);
				readfd 		= 1;
			}
			if(writefds){
				FD_CLR(fd, writefds);
				writefd 	= 1;
			}
			if(exceptfds){
				FD_CLR(fd, exceptfds);
				errorfd 	= 1;
			}

			s_timeout = 0;
			if(timeout){
#if 1
				s_timeout = timeout->tv_sec * 1000 * portTICK_PERIOD_MS;
				s_timeout += timeout->tv_usec / 1000 * portTICK_PERIOD_MS;
#else
				s_timeout = (timeout->tv_sec << 10);
				s_timeout += ((timeout->tv_usec > 1024) ? (timeout->tv_usec >> 10) : 1);	// 1000us(10^-6) = 1ms(10^-3)
#endif
			}
			ret = drv->select(pdev[fd], &readfd, &writefd, &errorfd, s_timeout);
			if(ret > 0){
				if(readfds && readfd) 	FD_SET(fd, readfds);
				if(writefds && writefd) FD_SET(fd, writefds);
				if(exceptfds && errorfd) FD_SET(fd, exceptfds);
			}
		}
	}
	return ret;
#endif
}
int poll (struct pollfd *__fds, nfds_t __nfds, int __timeout){
	int ret = -EPERM;
	struct platform_driver *drv = 0;
	struct platform_device **pdev = (struct platform_device **)&___dev_lookup_begin;
	int readfd = 0, writefd = 0, errorfd = 0;
	int s_timeout = 0;

	int fd = __fds[0].fd;
	__fds[0].revents = 0;
	if(fd >= 0 && fd <= (&___dev_lookup_end - &___dev_lookup_begin)){
		drv = (pdev[fd])->driver;
		if(drv->select){
			s_timeout = __timeout * portTICK_PERIOD_MS;
			ret = drv->select(pdev[fd], &readfd, &writefd, &errorfd, s_timeout);
			if(ret > 0){
				__fds[0].revents = __fds[0].events;
			}
		}
	}
	return ret;
}

void 	FD_CLR	(int fd, fd_set *set){
	*set = FD_INVALID;
}
int  	FD_ISSET(int fd, fd_set *set){
	return (fd == (int)(*set));
}
void 	FD_SET	(int fd, fd_set *set){
	*set = fd;
}
void 	FD_ZERO	(fd_set *set){
	*set = FD_INVALID;
}

//end of file
