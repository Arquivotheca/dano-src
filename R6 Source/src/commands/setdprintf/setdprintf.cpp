#include <stdio.h>
#include <stdlib.h>

extern "C" int _kset_dprintf_enabled_(bool);

int
main(int argc,char **argv)
{
	if(argc!= 2) {
		fprintf(stderr, "usage: %s 1|0\n");
		return 1;
	}

	int value= atoi(argv[1]);

	if((value!= 0) && (value!= 1)) {
		fprintf(stderr, "usage: %s 1|0\n");
		return 1;
	}

	_kset_dprintf_enabled_(bool(value));

	return 0;
}
