#include <drv_api.h>
#include <string.h>

#include "stm32f4xx.h"
#include "stm32f4xx_rng.h"
#include "stm32f4xx_rcc.h"
#include "misc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"

int 	random_init		(void);
int 	random_open		(struct platform_device *dev, int flags);
int 	random_close	(struct platform_device *dev);
int		random_read		(struct platform_device *dev, void* buf, int count);

static struct platform_driver g_random_driver = {
	.driver		= {
		.name	= "random-drv",
		.devices = 0,
	},
	.archdata 	= 0,
	.probe		= 0,
	.remove		= 0,
	.resume 	= 0,
	.suspend	= 0,
	.shutdown	= 0,

	.open 		= &random_open,
	.close 		= &random_close,
	.read 		= &random_read,
	.write 		= 0,
	.ioctl 		= 0,
	.select		= 0,

	.next 		= 0,
};
int random_init		(void){

	platform_driver_register(&g_random_driver);
	return 0;
}
module_init(random_init);
int 	random_open	(struct platform_device *dev, int flags){
	int ret;
	RNG_Cmd(ENABLE);
	RNG_ITConfig(DISABLE);
	ret = 0;
	return ret;
}
int 	random_close	(struct platform_device *dev){
	int ret = 0;
	return ret;
}
int		random_read		(struct platform_device *dev, void* buf, int count){
	int ret = 0;
	uint8_t* pu8 = (uint8_t*)buf;
	uint32_t u32Val;
	while(count > 0){
		RNG_Cmd(ENABLE);
		while(RNG_GetITStatus(RNG_FLAG_DRDY | RNG_FLAG_CECS | RNG_FLAG_SECS) == RESET){}
		if(RNG_GetFlagStatus(RNG_FLAG_DRDY) == SET){
			u32Val = RNG_GetRandomNumber();
			if(count > 0){ *pu8 = u32Val; pu8++; count--; ret++;}
			if(count > 0){ *pu8 = ((u32Val >> 8)& 0x00FF); pu8++; count--; ret++;}
			if(count > 0){ *pu8 = ((u32Val >> 16)& 0x00FF); pu8++; count--; ret++;}
			if(count > 0){ *pu8 = ((u32Val >> 24)& 0x00FF); pu8++; count--; ret++;}
		}else break;
	}
	return ret;
}
// end of file
