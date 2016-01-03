#ifndef REBOOT_H__
#define REBOOT_H__
extern volatile int g_app_term;
void reboot();
static inline int  is_app_term(){
	return g_app_term;
}
#endif
