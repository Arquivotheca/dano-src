#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <Drivers.h>
#include <OS.h>
#include <ByteOrder.h>

const char *kDevice = "/dev/net/rangelan/0";

// Ethernet ioctrl opcodes
enum {
	ETHER_GETADDR = B_DEVICE_OP_CODES_END,	/* get ethernet address */
	ETHER_INIT,								/* set irq and port */
	ETHER_NONBLOCK,							/* set/unset nonblocking mode */
	ETHER_ADDMULTI,							/* add multicast addr */
	ETHER_REMMULTI,							/* rem multicast addr */
	ETHER_SETPROMISC,						/* set promiscuous */
	ETHER_GETFRAMESIZE,						/* get frame size */
	ETHER_DEVICE_END = ETHER_GETADDR + 1000
};

// Rangelan ioctrl opcodes
enum {
	ETHER_SET_SECURITY_ID = ETHER_DEVICE_END
};

int main( int argc, char *argv[]  )
{
	int			fd;
	status_t	err = 1;
	size_t		size;
	uint32		id;
	uint8		*codePtr;
	uint8		code[4];
	
	if( argc != 2 )
	{
		fprintf( stderr, "usage: %s id\n", argv[0] );
		fprintf( stderr, "id is a 3 digit hex value such as 0xA1B2C3\n" );
		return 1;
	}
	
	id = strtol( argv[1], NULL, 16 );
	id = B_HOST_TO_LENDIAN_INT32( id );
	codePtr = (uint8 *)&id;
	code[0] = codePtr[2];
	code[1] = codePtr[1];
	code[2] = codePtr[0];
	
	if( (fd = open( kDevice, O_RDONLY )) < 0 )
	{
		fprintf( stderr, "Could not open rangelan driver!: %s\n", strerror(fd) );
		return 1;
	}
	
	if( (err = ioctl( fd, ETHER_SET_SECURITY_ID, code, 3 )) == B_OK )
		fprintf( stderr, "ID Set to: 0x%.2X%.2X%.2X\n", (int32)code[0], (int32)code[1], (int32)code[2] );
	else
		fprintf( stderr, "Set Security ID Failed!: %s\n", strerror(err) );
	close(fd);
	return err;
}
