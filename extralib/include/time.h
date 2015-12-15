#ifndef TIME_H__
#define TIME_H__
#define time_t	unsigned int
struct timespec {
   time_t tv_sec;      /* Seconds */
   long   tv_nsec;     /* Nanoseconds [0 .. 999999999] */
};
struct tm {
    int tm_sec;         /* seconds */
    int tm_min;         /* minutes */
    int tm_hour;        /* hours */
    int tm_mday;        /* day of the month */
    int tm_mon;         /* month */
    int tm_year;        /* year */
    int tm_wday;        /* day of the week */
    int tm_yday;        /* day in the year */
    int tm_isdst;       /* daylight saving time */
};
#define CLOCK_REALTIME	0
#define clockid_t	int

time_t time(time_t *t);
int stime(time_t *t);
struct tm *localtime_r(const time_t *timep, struct tm *result);
time_t mktime (struct tm * timeptr);
#endif
