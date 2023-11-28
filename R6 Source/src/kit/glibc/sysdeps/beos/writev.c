#include <unistd.h>
#include <sys/socket.h>
#define __write(fd,buf,n) send (fd, buf, n, 0)
#include <sysdeps/posix/writev.c>
