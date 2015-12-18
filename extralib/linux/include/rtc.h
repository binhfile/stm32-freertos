/*
 * rtc.h
 *
 *  Created on: Dec 15, 2015
 *      Author: dev
 */

#ifndef INCLUDE_RTC_H_
#define INCLUDE_RTC_H_
#undef RTC_UIE_ON
#undef RTC_UIE_OFF
#undef RTC_RD_TIME
#undef RTC_SET_TIME
#undef rtc_time

#define RTC_UIE_ON		1	// args = 0
#define RTC_UIE_OFF		2	// args = 0
#define RTC_RD_TIME		3	// args = &rtc_time
#define RTC_SET_TIME	4	// args = &rtc_time

struct rtc_time {
    int tm_sec;
    int tm_min;
    int tm_hour;

    int tm_mday;
    int tm_mon;
    int tm_year;
};

#endif /* INCLUDE_RTC_H_ */
