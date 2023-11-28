/*
 * kernel exported routines for use by filesystems to
 * implement named pipes.
 */

int		fifo_open(void **_vn, int omode, void **cookie);
void	fifo_close(void ** _vn, void *cookie);
int		fifo_read(void *_vn, void * cookie, void *buf, size_t *len);
int		fifo_write(void *_vn, void * cookie, const void *buf, size_t *len);
