#include <stdio.h>
#include <support2/SupportDefs.h>

#define SHIFT_BITS 8

static void
generate_lut(char *name, float f, int32 offset)
{
	int32 i;
	printf("int32 %s[0x100] = {\n", name);
	for (i=0;i<0x100;i++) {
		printf("0x%8.8x", (int32)(f * (i - offset) * (1 << SHIFT_BITS) + 0.5));
		if (i != 0xff)
			printf(", ");
		if ((i & 3) == 3)
			printf("\n");
	}
	printf("};\n\n");
}

main()
{
	printf("#include <SupportDefs.h>\n\n");
	generate_lut("LUT_1_164", 1.164, 16);
	generate_lut("LUT_1_596", 1.596, 128);
	generate_lut("LUT_0_813", 0.813, 128);
	generate_lut("LUT_0_392", 0.392, 128);
	generate_lut("LUT_2_017", 2.017, 128);
}

