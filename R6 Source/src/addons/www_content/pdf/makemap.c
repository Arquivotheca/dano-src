#include <stdio.h>

void makemap(const unsigned char *us)
{
	unsigned long int map[8];
	int i;

	for (i = 0; i < 8; i++) map[i] = 0;
	while (*us)
	{
		unsigned char x = *us++;
		map[x / 32] |= (1 << (x & 31));
	}
	for (i = 0; i < 8; i++) printf(" 0x%.8lx%s%s", map[i], i != 7 ? "," : "", (i & 7) == 7 ? "\n" : "");
	printf("\n");
}

int main(int argc, char **argv)
{
	printf("whitemap "); makemap(" \t\f\r\n");
	printf("eolmap "); makemap("\r\n");
	printf("nummap "); makemap("0123456789-+.");
	printf("wordmap "); makemap("<>{}[]()/% \t\f\r\n");
	return 0;
}
