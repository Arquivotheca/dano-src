#ifndef _SYS_TIMES_H_
#define _SYS_TIMES_H_

#include <be_setup.h>
#include <time.h>

struct tms {
    clock_t tms_utime;      /* user time */
    clock_t tms_stime;      /* system time */
    clock_t tms_cutime;     /* user time, children */
    clock_t tms_cstime;     /* system time, children */
};


__extern_c_start

clock_t times(struct tms *buffer); 

__extern_c_end


#endif /* _SYS_TIMES_H_ */
