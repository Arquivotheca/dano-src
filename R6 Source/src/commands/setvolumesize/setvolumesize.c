#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <Drivers.h>
#include <OS.h>

int main( int argc, char *argv[]  )
{
	int			fd;
	status_t	err = 1;
	size_t		size;
	
	if( argc != 3 )
	{
		fprintf(stderr, "usage: %s device_path size\n", argv[0]);
		return 1;
	}
	
	size = strtol( argv[2], NULL, 10 );
	size <<= 20;
	
	if( (fd = open(argv[1], O_RDONLY)) < 0 )
	{
		fprintf( stderr, "Could not open device: %s\n", strerror(fd) );
		return 1;
	}
	
	if( (err = ioctl( fd, B_SET_DEVICE_SIZE, &size, sizeof(size_t) )) == B_OK )
		fprintf( stderr, "Device \"%s\" set to %lu bytes\n", argv[1], size );
	else
		fprintf( stderr, "Set size failed: %s\n", strerror(err) );
	close(fd);
	return err;
}
