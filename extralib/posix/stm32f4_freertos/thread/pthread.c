#include "pthread.h"

pthread_t *g_pthread_list_thread = 0;
int pthread_create (pthread_t * __newthread,
			   const pthread_attr_t * __attr,
			   void *(*__start_routine) (void *),
			   void * __arg){
	pthread_t *p;
	__newthread->attr = (pthread_attr_t*)__attr;
	__newthread->start_routine = __start_routine;
	__newthread->arg = __arg;
	taskENTER_CRITICAL();
	if(!g_pthread_list_thread) g_pthread_list_thread = __newthread;
	else{
		p = g_pthread_list_thread;
		while(p->next != 0) p = p->next;
		p->next = __newthread;
	}
	taskEXIT_CRITICAL();
	__newthread->next = 0;

	  xTaskCreate(
			  (void (*)(void*))__newthread->start_routine,/* Function pointer */
			  "",                          				  /* Task name - for debugging only*/
			  __newthread->attr ?
					  __newthread->attr->stack_size
					  : configMINIMAL_STACK_SIZE,         /* Stack depth in words */
			  (void*) __newthread->arg,                   /* Pointer to tasks arguments (parameter) */
			  __newthread->prio,           			      /* Task priority*/
			  &__newthread->handle         				  /* Task handle */
	  );

	  return 0;
}
int pthread_attr_getstackaddr (const pthread_attr_t *
				      __attr, void ** __stackaddr){
	*__stackaddr = __attr->stack;
	return 0;
}
int pthread_attr_setstackaddr (pthread_attr_t *__attr,
				      void *__stackaddr){
	__attr->stack = __stackaddr;
	return 0;
}
int pthread_attr_getstacksize (const pthread_attr_t *
				      __attr, size_t * __stacksize){
	*__stacksize = __attr->stack_size;
	return 0;
}
int pthread_attr_setstacksize (pthread_attr_t *__attr,
				      size_t __stacksize){
	__attr->stack_size = __stacksize;
	return 0;
}
int pthread_setschedprio (pthread_t *__target_thread, int __prio){
	__target_thread->prio = __prio;
	return 0;
}
pthread_t pthread_self (void){
	pthread_t ret;
	pthread_t *p;
	TaskHandle_t handle = 0;

	ret.handle = 0;

	handle = xTaskGetCurrentTaskHandle();
	p = g_pthread_list_thread;
	if(p){
		do{
			if(p->handle == handle){
				ret = *p;
				break;
			}
			p = p->next;
		}while(p);
	}
	return ret;
}
