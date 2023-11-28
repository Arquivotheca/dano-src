#include <stdio.h>

void	main()
{
	char	c;


	while(!feof(stdin)) {
		c = getchar();
		if (c != 0x0d)
			putchar(c);
	}
}

