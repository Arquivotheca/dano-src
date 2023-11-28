#ifndef _SYS_UIO_H_
#define _SYS_UIO_H_

#include <be_setup.h>
#include <sys/types.h>
#include <size_t.h>

typedef struct iovec {
	void	*iov_base;
	size_t	iov_len;
} iovec;


__extern_c_start

extern ssize_t readv(int fd, const struct iovec *vector, size_t count);
extern ssize_t readv_pos(int fd, off_t pos, const struct iovec *vec, size_t count);
extern ssize_t writev(int fd, const struct iovec *vector, size_t count);
extern ssize_t writev_pos(int fd, off_t pos, const struct iovec *vec, size_t count);

__extern_c_end

#endif /* _SYS_UIO_H_ */
