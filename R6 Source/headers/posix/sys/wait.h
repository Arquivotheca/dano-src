#ifndef _SYS_WAIT_H_
#define _SYS_WAIT_H_

#include <be_setup.h>
#include <sys/types.h>

__extern_c_start

pid_t wait(int *statloc);
pid_t waitpid(pid_t pid, int *stat_loc, int options);

__extern_c_end


#define WNOHANG    0x1
#define WUNTRACED  0x2

#define WIFEXITED(val)    (((val) & ~(0xff)) == 0)
#define WEXITSTATUS(val)  ((val) & 0xff)
#define WIFSIGNALED(val)  ((((val) >>  8) & 0xff) != 0)
#define WTERMSIG(val)     (((val)  >>  8) & 0xff)
#define WIFSTOPPED(val)   ((((val) >> 16) & 0xff) != 0)
#define WSTOPSIG(val)     (((val)  >> 16) & 0xff)
#define WIFCORED(val)     ((val) & 0x10000)

						 
#endif /* _SYS_WAIT_H_ */
