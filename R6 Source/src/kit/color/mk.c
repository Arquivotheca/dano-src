#include <stdio.h>

int	main(int argc, char **argv)
{
	long	i;
	long	cnt;
	
	cnt = argc;

	for (i = 1; i < cnt; i++) {
		printf("cvt < %s > tmp\n", argv[i]);
		printf("mv tmp %s\n", argv[i]);
	}
}

