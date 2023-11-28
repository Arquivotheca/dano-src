#ifndef H_SELECT
#define H_SELECT

#include <sys/time.h>

__extern_c_start

#ifndef FD_SET
/* 
 * Select uses bit masks of file descriptors in uint32's.  These macros 
 * manipulate such bit fields (the filesystem macros use chars). 
 * FD_SETSIZE may be defined by the user, but the default here should 
 * be enough for most uses. 
 */ 
#ifndef        FD_SETSIZE 
#define        FD_SETSIZE      1024 
#endif 

typedef unsigned long   fd_mask; 
#define NFDBITS        (sizeof(fd_mask) * 8)       /* bits per mask */ 

#ifndef howmany 
#define        howmany(x, y)  (((x) + ((y) - 1)) / (y)) 
#endif 

typedef        struct fd_set { 
        fd_mask fds_bits[howmany(FD_SETSIZE, NFDBITS)]; 
} fd_set; 

#define        FD_SET(n, p)  ((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS))) 
#define        FD_CLR(n, p)    ((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS))) 
#define        FD_ISSET(n, p)  ((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS))) 
#define        FD_COPY(f, t)   memcpy(t, f, sizeof(*(f))) 
#define        FD_ZERO(p)      memset(p, 0, sizeof(*(p))) 

#endif /*FD_SET*/

#ifndef _KERNEL_MODE

int select(int fd, fd_set *rfds, fd_set *wfds, fd_set *efds, struct timeval *timeout);

#endif

__extern_c_end

#endif /* H_SELECT */
