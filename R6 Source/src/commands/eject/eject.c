#include <stdio.h>
#include <Drivers.h>
#include <errno.h>

#define	FLOPPY_DEVICE	"/dev/disk/floppy/raw"

main(int argc, char **argv)
{
	char	*device = FLOPPY_DEVICE;
	int	fd, err;

	if (argc > 1)
		device = argv[1];

	fd = open(device, 0);

	if (fd < 0) {
		perror(device);
		exit(-1);
	}

	err = ioctl(fd, B_EJECT_DEVICE, 0);

	if (err < 0) {
		if (err == EBUSY) {
			printf("%s: is in used -- device not ejected.\n", device);
			exit(-2);
		} else {
			perror(device);
			exit(-3);
		}
	}

	exit (0);
}
