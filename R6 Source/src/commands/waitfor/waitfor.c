#include <stdio.h>
#include <OS.h>


int
main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s thread_name\n", argv[0]);
		return 1;
	}

	while(1) {
		if (find_thread(argv[1]) > 0)
			break;

		snooze(100000);
	}
	
	return 0;
}
