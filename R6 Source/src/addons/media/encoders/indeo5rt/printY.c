#include <stdio.h>

#include "encode.cpk/convtabs.c"

void
printY(const uint32 *src, char *name, int step)
{
	int i;
	printf("const uint8 %s[] = {\n", name);
	for(i=0; i<256; i+=8*step) {
		int j;
		if(i != 0)
			printf(",\n");
		printf("\t");
		for(j=0; j<7*step; j+=step)
			printf("0x%02x, ", src[i+j]>>16);
		printf("0x%02x", src[i+j]>>16);
	}
	printf("\n};\n\n");
}

void
printYUV(const uint32 *src, char *name, int step)
{
	int i;
	printf("const uint32 %s[] = {\n", name);
	for(i=0; i<256; i+=4*step) {
		int j;
		if(i != 0)
			printf(",\n");
		printf("\t");
		for(j=0; j<3*step; j+=step)
			printf("0x%06x, ", src[i+j]);
		printf("0x%06x", src[i+j]);
	}
	printf("\n};\n\n");
}

main()
{
	printY(RtoYUV, "RtoY", 1);
	printY(GtoYUV, "GtoY", 1);
	printY(BtoYUV, "BtoY", 1);

	printY(RtoYUV, "R5toY", 8);
	printY(GtoYUV, "G5toY", 8);
	printY(BtoYUV, "B5toY", 8);

	printYUV(RtoYUV, "R5toYUV", 8);
	printYUV(GtoYUV, "G5toYUV", 8);
	printYUV(BtoYUV, "B5toYUV", 8);
}
