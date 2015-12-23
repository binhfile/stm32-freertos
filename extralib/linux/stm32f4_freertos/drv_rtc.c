
#include <drv_api.h>
#include <string.h>

#include "stm32f4xx.h"
#include "stm32f4xx_rng.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_pwr.h"
#include "misc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"

#include <rtc.h>

int 	rtc_init	(void);
int 	rtc_open	(struct platform_device *dev, int flags);
int 	rtc_close	(struct platform_device *dev);
int		rtc_read	(struct platform_device *dev, void* buf, int count);
int		rtc_write	(struct platform_device *dev, const void* buf, int count);
int		rtc_ioctl	(struct platform_device *dev, int request, unsigned int arguments);

static struct platform_driver g_rtc_driver = {
	.driver		= {
		.name	= "rtc-drv",
		.devices = 0,
	},
	.archdata 	= 0,
	.probe		= 0,
	.remove		= 0,
	.resume 	= 0,
	.suspend	= 0,
	.shutdown	= 0,

	.open 		= &rtc_open,
	.close 		= &rtc_close,
	.read 		= &rtc_read,
	.write 		= &rtc_write,
	.ioctl 		= &rtc_ioctl,
	.select		= 0,

	.next 		= 0,
};
int rtc_init		(void){

	platform_driver_register(&g_rtc_driver);
	return 0;
}
module_init(rtc_init);
int 	rtc_open	(struct platform_device *dev, int flags){
	int ret;
	uint32_t status;
	RTC_TimeTypeDef s_time;
	RTC_DateTypeDef s_date;
	RTC_InitTypeDef s_init;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
	PWR_BackupAccessCmd(ENABLE);
	status = RTC_ReadBackupRegister(RTC_BKP_DR19);
#if 1
	// internal clock
	RCC_LSICmd(ENABLE);
	while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET){}
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
#else
	// exernal clock
	RCC_LSEConfig(RCC_LSE_ON);
	while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET){}
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
#endif
	// common
	RCC_RTCCLKCmd(ENABLE);
	RTC_WriteProtectionCmd(DISABLE);
	RTC_WaitForSynchro();
	RTC_WriteProtectionCmd(ENABLE);
	RTC_WriteBackupRegister(RTC_BKP_DR19, 0x1234);
	if(status == 0x4321 || status == 0x1234){
		// RTC time OK or init done
		RTC_WriteProtectionCmd(DISABLE);
		RTC_WaitForSynchro();
		RTC_WriteProtectionCmd(ENABLE);

		RTC_GetTime(RTC_Format_BIN, &s_time);
		RTC_GetDate(RTC_Format_BIN, &s_date);
	}
	else{
		/* Fill time */
		s_time.RTC_Hours = 0;
		s_time.RTC_Minutes = 0;
		s_time.RTC_Seconds = 0;
		/* Fill date */
		s_date.RTC_Date = 1;
		s_date.RTC_Month = 1;
		s_date.RTC_Year = 15;
		s_date.RTC_WeekDay = 1;
		/* Set the RTC time base to 1s and hours format to 24h */
		s_init.RTC_HourFormat = RTC_HourFormat_24;
		s_init.RTC_AsynchPrediv = 0x1F;
		s_init.RTC_SynchPrediv = 0x3FF;

		RTC_Init(&s_init);
		/* Set time */
		RTC_SetTime(RTC_Format_BIN, &s_time);
		/* Set date */
		RTC_SetDate(RTC_Format_BIN, &s_date);
		/* Write backup registers */
		RTC_WriteBackupRegister(RTC_BKP_DR19, 0x4321);
	}
	ret = 0;
	return ret;
}
int 	rtc_close	(struct platform_device *dev){
	int ret = 0;
	return ret;
}
int		rtc_read		(struct platform_device *dev, void* buf, int count){
	int ret = 0;

	return ret;
}
int		rtc_write	(struct platform_device *dev, const void* buf, int count){
	int ret = 0;

	return ret;
}
int		rtc_ioctl	(struct platform_device *dev, int request, unsigned int arguments){
	int ret = -1;
	RTC_TimeTypeDef s_time;
	RTC_DateTypeDef s_date;
	struct rtc_time* tm_rtc;

	switch(request){
		case RTC_RD_TIME:{
			RTC_GetTime(RTC_Format_BIN, &s_time);
			RTC_GetDate(RTC_Format_BIN, &s_date);
			tm_rtc = (struct rtc_time*)arguments;
			tm_rtc->tm_year = s_date.RTC_Year;
			tm_rtc->tm_mon = s_date.RTC_Month;
			tm_rtc->tm_mday = s_date.RTC_Date;

			tm_rtc->tm_hour = s_time.RTC_Hours;
			tm_rtc->tm_min = s_time.RTC_Minutes;
			tm_rtc->tm_sec = s_time.RTC_Seconds;
			ret = 0;
			break;
		}
		case RTC_SET_TIME:{
			tm_rtc = (struct rtc_time*)arguments;
			/* Fill time */
			s_time.RTC_Hours 	= tm_rtc->tm_hour;
			s_time.RTC_Minutes 	= tm_rtc->tm_min;
			s_time.RTC_Seconds 	= tm_rtc->tm_sec;
			/* Fill date */
			s_date.RTC_Date 	= tm_rtc->tm_mday;
			s_date.RTC_Month 	= tm_rtc->tm_mon;
			s_date.RTC_Year 	= tm_rtc->tm_year;
			s_date.RTC_WeekDay 	= tm_rtc->tm_mday;
			/* Set time */
			RTC_SetTime(RTC_Format_BIN, &s_time);
			/* Set date */
			RTC_SetDate(RTC_Format_BIN, &s_date);
			ret = 0;
			break;
		}
	}
	return ret;
}
// end of file

