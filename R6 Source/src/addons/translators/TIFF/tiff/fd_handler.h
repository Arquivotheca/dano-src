
#if !defined(LNG_FDHANDLER)
#define	LNG_FDHANDLER

#include <stdio.h>
// #include <Unix.h>

void	fd_init();
void	fd_terminate();

int		fd_open(const char *name, const char *mode);
int		fd_close(int fd);

size_t	fd_read(int fd, void *data, size_t size);
size_t	fd_write(int fd, void *data, size_t size);
off_t 	fd_seek(int fd, off_t offset, int whence);
long 	fd_size(int fd);

#endif	// LNG_FDHANDLER

