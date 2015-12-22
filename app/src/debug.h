#ifndef DEBUG_H__
#define DEBUG_H__
#include <stdint.h>
enum LED{
	LED_GREEN = 0,
	LED_RED,
	LED_BLUE,
	LED_ORANGE
};
#if defined(STM32F4XX)
#include <gpio.h>
#include <fcntl.h>
extern int g_fd_led[];
#define LED_ON(led) {uint8_t val = 1; write(g_fd_led[LED_##led], &val, 1);}
#define LED_OFF(led) {uint8_t val = 0; write(g_fd_led[LED_##led], &val, 1);}
#define LED_TOGGLE(led) {ioctl(g_fd_led[LED_##led], GPIO_IOC_TOGGLE, 0);}
#else
#define LED_ON(led)
#define LED_OFF(led)
#define LED_TOGGLE(led)
#endif

void LREP(char* s, ...);
#define LREP_WARN(s, args...) LREP("%d@%s " s, __LINE__, __FILE__, ##args)



extern uint8_t kbhit(int timeout);
extern uint8_t kb_value();
extern uint8_t kb_cmd(const char* cmd);

#if defined(OS_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
extern volatile TickType_t t_profile_ref, t_profile_now;
#define PROFILE_BEGIN()	t_profile_ref = xTaskGetTickCount()
#define PROFILE_BEGIN_ISR()	t_profile_ref = xTaskGetTickCountFromISR()

#define PROFILE_CHECK()	{ \
		t_profile_now = xTaskGetTickCount();\
	LREP("profile: %u\r\n", t_profile_now-t_profile_ref);\
	}
#endif
#endif
