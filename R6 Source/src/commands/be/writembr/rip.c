#include <stdio.h>

int main(void)
{
	int pos = 0;
	int c;

	printf("unsigned char data[] = {\n\t");

	while ((c = getchar()) != EOF) {
		printf("0x%2.2x, ", c);
		if ((pos++ % 8) == 7)
			printf("\n\t");
	}

	printf("\n};\n");
}
