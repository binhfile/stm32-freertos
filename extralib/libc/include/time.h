#ifndef TIME_H_
#define TIME_H_
#include <stdint.h>
#include <stddef.h>
#define time_t	unsigned int
#define clockid_t	int
/* The value of this macro is the number of clock ticks per second measured by
 * the clock function.
 */
#define CLOCKS_PER_SEC     1000 //TODO CLOCKS_PER_SEC should receive from clock_getres()


/* This is an obsolete name for CLOCKS_PER_SEC. */
#define CLK_TCK            CLOCKS_PER_SEC

/* Parameters used to convert the time specific values */
#define MSEC_PER_SEC    1000L
#define USEC_PER_MSEC   1000L
#define NSEC_PER_USEC   1000L
#define USEC_PER_SEC    1000000L
#define NSEC_PER_SEC    1000000000L

struct tm {
	int    tm_sec;   /*Seconds [0,60].*/
	int    tm_min;   /*Minutes [0,59].*/
	int    tm_hour;  /*Hour [0,23].   */
	int    tm_mday;  /*Day of month [1,31]. */
	int    tm_mon;   /*Month of year [0,11]. */
	int    tm_year;  /*Years since 1900 */
	int    tm_wday;  /*Day of week [0,6] (Sunday =0). */
	int    tm_yday;  /*Day of year [0,365]. */
	int    tm_isdst; /*Daylight Savings flag. */
};

struct timespec {
	time_t tv_sec;  /*Seconds */
	long   tv_nsec; /*Nanoseconds */
};

/**
 * Converts the calendar time t into a null-terminated string of the form
 * "Wed Jun 30 21:49:08 1993\n".
 * It uses static allocated buffer which might be overwritten by subsequent
 * calls to any of the date and time functions
 */
char *ctime(const time_t *timep);
char *ctime_r(const time_t *t, char *buff);

struct tm *gmtime(const time_t *timep);
struct tm *gmtime_r(const time_t *timep, struct tm *result);

time_t mktime(struct tm *tm);

/* convert date and time to a string */
char *asctime(const struct tm *timeptr);

struct tm *localtime(const time_t *timep);
struct tm *localtime_r(const time_t *timep, struct tm *result);

#define CLOCK_REALTIME  3
#define TIMER_ABSTIME   2
#define CLOCK_MONOTONIC 1

int clock_getres(clockid_t clk_id, struct timespec *res);

int clock_gettime(clockid_t clk_id, struct timespec *tp);

int clock_settime(clockid_t clk_id, const struct timespec *tp);

/* seconds from beginning of start system */
time_t time(time_t *t);
int stime(time_t *t);
/** Format date and time */
size_t strftime(char *s, size_t max, const char *fmt, const struct tm *tm);

int nanosleep(const struct timespec *req, struct timespec *rem);

static inline double difftime(time_t time1, time_t time0) {
	return (time1 - time0);
}

#endif /* TIME_H_ */
