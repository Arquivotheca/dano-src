#ifndef _TIME_H_
#define _TIME_H_

#include <be_setup.h>
#include <null.h>
#include <size_t.h>


typedef long clock_t;
typedef long time_t;

#define CLOCKS_PER_SEC	1000
#define CLK_TCK         CLOCKS_PER_SEC


struct tm {
	int	tm_sec;
	int	tm_min;
	int	tm_hour;
	int	tm_mday;
	int	tm_mon;
	int	tm_year;
	int	tm_wday;
	int	tm_yday;
	int	tm_isdst;
	int tm_gmtoff;
	char *tm_zone;
};


__extern_c_start

clock_t		clock(void);
double		difftime(time_t time1, time_t time2);
time_t		mktime(struct tm *timeptr);
time_t		time(time_t *timer);
char 	   *asctime(const struct tm *timeptr);
char 	   *asctime_r(const struct tm *timeptr, char *buf);
char 	   *ctime(const time_t *timer);
char 	   *ctime_r(const time_t *timer, char *buf);
struct tm  *gmtime(const time_t *timer);
struct tm  *gmtime_r(const time_t *timer, struct tm *t); 
struct tm  *localtime(const time_t *timer);
struct tm  *localtime_r(const time_t *timer, struct tm *t);
size_t		strftime(char *s, size_t maxsize, const char *format, const struct tm *timeptr);

#define MAX_TIMESTR	70	/* maximum length of a string returned by asctime or ctime */

#if __POWERPC__
clock_t	__get_clock(void);
time_t	__get_time(void);
#endif /* __POWERPC__ */


extern void tzset(void);
extern char *tzname[2];

/*
 * For BeOS timezone support
 */
extern int 	daylight;
extern long	timezone;
extern int	stime(const time_t *t);

__extern_c_end

#endif /* _TIME_H_ */
