#include "../HTTPStream.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	if (argc != 3) {
		printf("USAGE: testhttpstream <machine> <filepath>\n");
		return -1;
	}

	HTTPStream stream(argv[1]);
	if (stream.InitCheck() != B_OK) {
		printf("Couldn't open stream\n");
		return -1;
	}
	
	if (stream.OpenFile(argv[2]) != B_OK) {
		printf("Couldn't open file\n");
		return -1;
	}
	
	while (true) {
		char buf[1024];
		ssize_t amountRead = stream.Read(buf, 1024);
		if (amountRead < 0) {
			fprintf(stderr, "read error\n");
			return -1;
		}
		
		write(1, buf, amountRead);
		if (amountRead < 1024)
			break;
	}
	
	return 0;
}