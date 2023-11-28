#ifndef _SYS_TIMEB_H
#define _SYS_TIMEB_H

#include <be_setup.h>
#include <time.h>


struct timeb {
    time_t			time;
    unsigned short	millitm;
    short			timezone;
    short			dstflag;
};


__extern_c_start

int ftime(struct timeb *tp);

__extern_c_end


#endif /* _SYS_TIMEB_H */

