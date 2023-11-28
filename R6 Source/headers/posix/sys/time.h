#ifndef _SYS_TIME_H
#define _SYS_TIME_H

#include <be_setup.h>
#include <time.h>

struct timeval { 
	long tv_sec;
	long tv_usec;
};

struct timezone {
	int tz_minuteswest;
	int tz_dsttime;
};


/* due to a circular dependency with sys/select.h, we must include
   it after timeval is defined */
#include <sys/select.h>


__extern_c_start

int gettimeofday(struct timeval *tv, struct timezone *tz);
int utimes(const char *name, const struct timeval times[2]);

#define timerclear(tvp)	((tvp)->tv_sec = (tvp)->tv_usec = 0)
#define timerisset(tvp) ((tvp)->tv_sec || (tvp)->tv_usec)
#define timercmp(tvp, uvp, cmp)                        \
                  ((tvp)->tv_sec cmp (uvp)->tv_sec ||  \
				   (tvp)->tv_sec == (uvp)->tv_sec &&   \
				   (tvp)->tv_usec cmp (uvp)->tv_usec)

/* XXXdbg getitimer() */
/* XXXdbg setitimer() */


__extern_c_end


#endif /* _SYS_TIME_H */
