
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <SupportDefs.h>


int
main(
	int argc, 
	char * argv[])
{
	char * end;
	int32 value;
	char * val;
	int base = 10;

	if ((argc != 2) || ((argv[1][0] == '-') && !isdigit(argv[1][1]))) {
		fprintf(stderr, "usage: error error-no\n");
		fprintf(stderr, "Prints clear-text name of error code given. Error can be in decimal, hex or octal.\n");
		return 1;
	}
	val = argv[1];
	if (!strncmp(val, "0x", 2)) {
		base = 16;
		val += 2;
	}
	else if (!strncmp(val, "0", 1)) {
		base = 8;
		val += 1;
	}
	if (base != 10) {
		uint32 ul = strtoul(val, &end, base);
		value = *(int32 *)&ul;
	}
	else {
		value = strtol(val, &end, base);
	}
	if (end-val != strlen(val)) {
		fprintf(stderr, "warning: not all digits were used\n");
	}
	printf("0x%x: %s\n", value, strerror(value));
	return 0;
}

