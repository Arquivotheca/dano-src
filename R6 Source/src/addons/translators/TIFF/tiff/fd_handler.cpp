
#include "fd_handler.h"

#define	NBELTS	256

typedef struct {
	FILE	*file;
} fd_file;

fd_file	master[NBELTS];

static int fd_checkfree()
{
	int i;
	int free_fd = -1;
	for (i=0;i<NBELTS;i++) {
		if (master[i].file==NULL)			{free_fd = i; break;}
	}
	return free_fd;
}

void fd_init()
{
	int i;
	for (i=0;i<NBELTS;i++)		master[i].file = NULL;
}

int fd_open(const char *name, const char *mode)
{
	int 	fd;
	FILE	*file;
	fd = fd_checkfree();
	if (fd==-1)		return -1;
	file = fopen(name,mode);
	if (!file)		return -1;
	master[fd].file = file;
fprintf(stderr,"opening file %s as fd %d\n",name,fd);
	return fd;
}

int	fd_close(int fd)
{
	int error = 0;
	if (master[fd].file)		
		if ((error = fclose(master[fd].file)) == 0)		master[fd].file = NULL;
	return error;
}

size_t fd_read(int fd, void *data, size_t size)
{
	if (master[fd].file)		return fread(data,1,size,master[fd].file);
	else						return 0;
}

size_t fd_write(int fd, void *data, size_t size)
{
	if (master[fd].file)		return fwrite(data,1,size,master[fd].file);
	else						return 0;
}

off_t fd_seek(int fd, off_t offset, int whence)
{
	off_t	cpos = 0;
	if (master[fd].file) {
		if (fseek(master[fd].file,offset,whence)==0)	cpos = ftell(master[fd].file);
	}
	return cpos;
}

long fd_size(int fd)
{
	long size = 0;
	long cpos; 
	if (master[fd].file) {
		cpos = ftell(master[fd].file);
		size = fd_seek(fd,0,SEEK_END);
		fd_seek(fd,cpos,SEEK_SET);
	}
	return size;
}

void fd_terminate()
{
	int i;
	for (i=0;i<NBELTS;i++) 		{fd_close(i);}
}

