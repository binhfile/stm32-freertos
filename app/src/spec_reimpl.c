/*
 * spec_reimpl.c
 *
 *  Created on: Dec 14, 2015
 *      Author: dev
 */

#if defined(STM32F4XX)
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <rtc.h>
#include <debug.h>

int g_fd_random = 0;
extern int g_fd_rtc;

int rand(void){
	uint32_t u32Val = 0;
	int ret = read(g_fd_random, &u32Val, sizeof(u32Val));
	if(ret != sizeof(u32Val)){
		LREP("read random failed %d\r\n", ret);
	}
	return u32Val;
}
void srand(unsigned int seed){
	g_fd_random = open("random-dev", 0);
	if(g_fd_random < 0) LREP("open random device failed\r\n");
}

#define RTC_LEAP_YEAR(year)             ((((year) % 4 == 0) && ((year) % 100 != 0)) || ((year) % 400 == 0))
#define RTC_DAYS_IN_YEAR(x)             RTC_LEAP_YEAR(x) ? 366 : 365
uint8_t RTC_Months_days[2][12] = {
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},	/* Not leap year */
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}	/* Leap year */
};
#define RTC_SECONDS_PER_DAY             86400
#define RTC_SECONDS_PER_HOUR            3600
#define RTC_SECONDS_PER_MINUTE          60
time_t time(time_t *t){
	time_t ret = 0;
	time_t days = 0;
	struct rtc_time rtc_tm;
	int i;

	if(g_fd_rtc >= 0){
		if(ioctl(g_fd_rtc, RTC_RD_TIME, (unsigned int)&rtc_tm) == 0){
			rtc_tm.tm_year+=2000;
			/* Days in back years */
			for (i = 1970; i < rtc_tm.tm_year; i++) {
				days += RTC_DAYS_IN_YEAR(i);
			}
			/* Days in current year */
			for (i = 1; i < rtc_tm.tm_mon; i++) {
				days += RTC_Months_days[RTC_LEAP_YEAR(rtc_tm.tm_year)][i - 1];
			}
			/* Day starts with 1 */
			days += rtc_tm.tm_mday - 1;
			ret = days * RTC_SECONDS_PER_DAY;
			ret += rtc_tm.tm_hour * RTC_SECONDS_PER_HOUR;
			ret += rtc_tm.tm_min * RTC_SECONDS_PER_MINUTE;
			ret += rtc_tm.tm_sec;
		}
	}

	if(t) *t = ret;
	return ret;
}
int stime(time_t *t){
	int ret = 0;
	struct tm tm_t;
	struct rtc_time rtc_tm;

	localtime_r(t, &tm_t);
	if(g_fd_rtc >= 0){
		rtc_tm.tm_year = tm_t.tm_year - 2000;
		rtc_tm.tm_mon = tm_t.tm_mon;
		rtc_tm.tm_mday = tm_t.tm_mday;

		rtc_tm.tm_hour = tm_t.tm_hour;
		rtc_tm.tm_min = tm_t.tm_min;
		rtc_tm.tm_sec = tm_t.tm_sec;
		ioctl(g_fd_rtc, RTC_SET_TIME, (unsigned int)&rtc_tm);
	}
	return ret;
}
struct tm *localtime_r(const time_t *timep, struct tm *result){
	uint16_t year;
	time_t unix = *timep;

	/* Get seconds from unix */
	result->tm_sec = unix % 60;
	/* Go to minutes */
	unix /= 60;
	/* Get minutes */
	result->tm_min = unix % 60;
	/* Go to hours */
	unix /= 60;
	/* Get hours */
	result->tm_hour = unix % 24;
	/* Go to days */
	unix /= 24;

	/* Get week day */
	/* Monday is day one */
	result->tm_mday = (unix + 3) % 7 + 1;

	/* Get year */
	year = 1970;
	while (1) {
		if (RTC_LEAP_YEAR(year)) {
			if (unix >= 366) {
				unix -= 366;
			} else {
				break;
			}
		} else if (unix >= 365) {
			unix -= 365;
		} else {
			break;
		}
		year++;
	}
		/* Get year in xx format */
	result->tm_year = year;
	/* Get month */
	for (result->tm_mon = 0; result->tm_mon < 12; result->tm_mon++) {
		if (RTC_LEAP_YEAR(year) && unix >= (uint32_t)RTC_Months_days[1][result->tm_mon]) {
			unix -= RTC_Months_days[1][result->tm_mon];
		} else if (unix >= (uint32_t)RTC_Months_days[0][result->tm_mon]) {
			unix -= RTC_Months_days[0][result->tm_mon];
		} else {
			break;
		}
	}
	/* Get month */
	/* Month starts with 1 */
	result->tm_mon++;
	/* Get date */
	/* Date starts with 1 */
	result->tm_mday = unix + 1;
	return result;
}
time_t mktime (struct tm * timeptr){
	time_t ret = 0;
	time_t days = 0;
	int i;

//	timeptr->tm_year+=2000;
	/* Days in back years */
	for (i = 1970; i < timeptr->tm_year; i++) {
		days += RTC_DAYS_IN_YEAR(i);
	}
	/* Days in current year */
	for (i = 1; i < timeptr->tm_mon; i++) {
		days += RTC_Months_days[RTC_LEAP_YEAR(timeptr->tm_year)][i - 1];
	}
	/* Day starts with 1 */
	days += timeptr->tm_mday - 1;
	ret = days * RTC_SECONDS_PER_DAY;
	ret += timeptr->tm_hour * RTC_SECONDS_PER_HOUR;
	ret += timeptr->tm_min * RTC_SECONDS_PER_MINUTE;
	ret += timeptr->tm_sec;
	return ret;
}
#endif

