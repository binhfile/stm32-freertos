#include "drv_api.h"
#include "drv_errno.h"
#include "string.h"
#include "fcntl.h"
#include "FreeRTOS.h"

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
		pdev->i_event = 0;
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
}
int 	close	(int fd){
	int ret = -EPERM;
	struct platform_driver *drv = 0;
	struct platform_device **pdev = (struct platform_device **)&___dev_lookup_begin;

	if(fd >= 0 && fd <= (&___dev_lookup_end - &___dev_lookup_begin)){
		drv = (pdev[fd])->driver;
		if(drv->close)
			ret = drv->close(pdev[fd]);
	}
	return ret;
}
int 	write	(int fd, const void *buf, size_t count){
	int ret = -EPERM;
	struct platform_driver *drv = 0;
	struct platform_device **pdev = (struct platform_device **)&___dev_lookup_begin;

	if(fd >= 0 && fd <= (&___dev_lookup_end - &___dev_lookup_begin)){
		drv = (pdev[fd])->driver;
		if(drv->write)
			ret = drv->write(pdev[fd], buf, count);
	}
	return ret;
}
int 	read	(int fd, void *buf, size_t count){
	int ret = -EPERM;
	struct platform_driver *drv = 0;
	struct platform_device **pdev = (struct platform_device **)&___dev_lookup_begin;

	if(fd >= 0 && fd <= (&___dev_lookup_end - &___dev_lookup_begin)){
		drv = (pdev[fd])->driver;
		if(drv->read)
			ret = drv->read(pdev[fd], buf, count);
	}
	return ret;
}
int 	ioctl	(int fd, int request, unsigned int arguments){
	int ret = -EPERM;
	struct platform_driver *drv = 0;
	struct platform_device **pdev = (struct platform_device **)&___dev_lookup_begin;

	if(fd >= 0 && fd <= (&___dev_lookup_end - &___dev_lookup_begin)){
		drv = (pdev[fd])->driver;
		if(drv->ioctl)
			ret = drv->ioctl(pdev[fd], request, arguments);
	}
	return ret;
}
int 	select(int fd, fd_set *readfds, fd_set *writefds,
		  fd_set *exceptfds, struct timeval *timeout){
	int ret = -EPERM;
	struct platform_device **pdev = (struct platform_device **)&___dev_lookup_begin;
	int s_timeout = 0, i, tmp_fd;
	uint32_t ulNotificationValue;
	pthread_t thread;

	if(timeout){
		s_timeout = timeout->tv_sec * 1000 * portTICK_PERIOD_MS;
		s_timeout += timeout->tv_usec / 1000 * portTICK_PERIOD_MS;
	}

	if(readfds){
		thread.handle = xTaskGetCurrentTaskHandle();
		readfds->flags = 0;
		taskENTER_CRITICAL();
		for(i = 0; i < FD_SET_MAXCNT; i++){
			tmp_fd = readfds->fd[i];
			if(tmp_fd != -1){
				if(pdev[tmp_fd]->i_event){
					readfds->flags |= (((unsigned int)1) << i);
					pdev[tmp_fd]->event |= pdev[tmp_fd]->event_mask;
				}else{
					pdev[tmp_fd]->event 		= 0x00;
					pdev[tmp_fd]->event_mask 	= 0x01;
					pdev[tmp_fd]->current_thread.handle = thread.handle;
				}
			}
		}
		taskEXIT_CRITICAL();
		if(readfds->flags){
			ulNotificationValue = 1;
		}else{
			ulNotificationValue = ulTaskNotifyTake( pdTRUE, s_timeout);
		}
		if(ulNotificationValue == 1){
			taskENTER_CRITICAL();
			for(i = 0; i < FD_SET_MAXCNT; i++){
				tmp_fd = readfds->fd[i];
				if(tmp_fd != -1){
					pdev[tmp_fd]->i_event = 0;
					if(pdev[tmp_fd]->event & pdev[tmp_fd]->event_mask){
						readfds->flags |= (((unsigned int)1) << i);
						ret = 1;
					}
					pdev[tmp_fd]->current_thread.handle = 0;
				}
			}
			taskEXIT_CRITICAL();
		}
		else ret = 0;
	}
	return ret;
}

//void 	FD_CLR	(int fd, fd_set *set){
//	int i = 0;
//	for(i = 0; i < FD_SET_MAXCNT; i++)
//		if(set->fd[i] == fd){
//			set->flags &= ~(((unsigned int)1) << i);
//			set->fd[i] = -1;
//			break;
//		}
//}
//inline int  	FD_ISSET(int fd, fd_set *set){
//	int i = 0;
//	int ret = 0;
//	for(i = 0; i < FD_SET_MAXCNT; i++)
//		if(set->fd[i] == fd){
//			if(set->flags & (((unsigned int)1) << i))
//				ret = 1;
//			break;
//		}
//	return ret;
//}
//void 	FD_SET	(int fd, fd_set *set){
//	int i = 0;
//	for(i = 0; i < FD_SET_MAXCNT; i++)
//		if(set->fd[i] == fd){
//			break;
//		}
//	if(i == FD_SET_MAXCNT){
//		for(i = 0; i < FD_SET_MAXCNT; i++){
//			if(set->fd[i] == -1){
//				set->fd[i] = fd;
//				set->flags |= (((unsigned int)1) << i);
//				break;
//			}
//		}
//	}
//}
//void 	FD_ZERO	(fd_set *set){
//	int i = 0;
//	for(i = 0; i < FD_SET_MAXCNT; i++)
//		set->fd[i] = -1;
//	set->flags = 0;
//}

//end of file
