#include <stdio.h>
#include <stdlib.h>

int binaryConvert(const char *s)
{
	int i = 0;
	while (*s)
	{
		i <<= 1;
		i += *s - '0';
		s++;
	}
	return i;
}

int main(int argc, char **argv)
{
	char *s;
	char buf[128];
	int val;
	int base = 0;
	int step = 1;

	if (argc > 1) base = atoi(argv[1]);
	if (argc > 2) step = atoi(argv[2]);
	
	while ((s = fgets(buf, sizeof(buf), stdin)))
	{
		size_t len = strlen(s) - 1;
		s[len] = '\0'; // lop off newline
		val = binaryConvert(s);
		printf("\t{ %2i, 0x%.2x, %4i }, // %s\n", len, val, base, s);
		base += step;
	}
}
