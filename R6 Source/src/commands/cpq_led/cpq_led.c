#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

int
main(int argc, char **argv)
{
	int		fd;
	int		err;
	int		op;
	char	c;

	if (argc != 3)
		goto usage;

	if (!strcmp(argv[1], "-sleep"))
		op = 0x6900;
	else if (!strcmp(argv[1], "-mail"))
		op = 0x6908;
	else if (!strcmp(argv[1], "-connect"))
		op = 0x690e;
	else
		goto usage;
		
	if (!strcmp(argv[2], "-off"))
		c = 1;
	else
		if (!strcmp(argv[2], "-on"))
			c = 0;
		else
			goto usage;

	fd = open("/dev/misc/cpq_gpio", O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "%s: Error opening GPIO device\n", argv[0]);
		return 1;
	}

	err = ioctl(fd, op, &c, sizeof(c));
	if (err < 0) {
		printf("%s: Error toggling LED\n", argv[0]);
		return 1;
	}

	close(fd);
	return 0;
	
usage:
	fprintf(stderr, "usage: %s { -mail, -sleep, -connect } { -on, -off }\n", argv[0]);
	return 1;
}
