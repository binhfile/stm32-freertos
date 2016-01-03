#include "reboot.h"

volatile int g_app_term = 0;
void reboot(){
	g_app_term = 1;
}
