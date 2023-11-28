#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <Drivers.h>

enum {
	ETHER_GETADDR = B_DEVICE_OP_CODES_END,	/* get ethernet address */
	ETHER_INIT,								/* set irq and port */
	ETHER_NONBLOCK,							/* set/unset nonblocking mode */
	ETHER_ADDMULTI,							/* add multicast addr */
	ETHER_REMMULTI,							/* rem multicast addr */
	ETHER_SETPROMISC,						/* set promiscuous */
	ETHER_GETFRAMESIZE						/* get frame size */
};

int
do_put_mac_address(int argc, char **argv)
{
	extern int do_setenv(int, char **);

	unsigned char mac_addr[64];
	unsigned char mac_addr_txt[64];
	unsigned char *setenv_argv[3];
	short result;
	int fd;
	
	if (argc != 2) {
		printf("usage: %s <device> <var>\n", "putmacaddress");
		return -1;
	}
	
	fd = open(argv[0], O_RDONLY);
	if (fd < 0 ) {
		printf("open %s %x failed\n", (char *) *argv, fd);
		return -1;
	}
 
 	result = ioctl(fd, ETHER_INIT, mac_addr, 0);
	if (result < 0) {
		printf("Unable to initialize driver %x.\n",result);
		return -1;
	}
	
	result = ioctl(fd, ETHER_GETADDR, mac_addr, 6);
	if (result < 0) {
		printf("I/O control failed %x\n",result);
		return -1;
	}

	close(fd);

	sprintf(
		mac_addr_txt,
		"%02x%02x%02x%02x%02x%02x",
		mac_addr[0], mac_addr[1],
		mac_addr[2], mac_addr[3],
		mac_addr[4], mac_addr[5]
	);

	setenv_argv[0]= argv[1];
	setenv_argv[1]= mac_addr_txt;
	setenv_argv[2]= NULL;

	return do_setenv(2, (char**)setenv_argv);
}
