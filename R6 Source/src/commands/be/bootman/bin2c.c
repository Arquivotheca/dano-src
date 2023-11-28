#include <stdio.h>

main()
{
	int c, count = 0;

	while ((c = getchar()) != EOF) {
		if ((count & 0xf) == 0)
			printf("\t\"\\x%2.2x", c);
		else if ((count & 0xf) == 0xf)
			printf("\\x%2.2x\"\n", c);
		else
			printf("\\x%2.2x", c);
		count++;
	}

	if (count & 0xf) printf("\"\n");
}
