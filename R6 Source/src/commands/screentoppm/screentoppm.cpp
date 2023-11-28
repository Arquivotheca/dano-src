/*****************************************************************

 program: screentoppm
 purpose: convert a be screendump to ppm format
 author : Gertjan van Ratingen
 date   : 8-feb-1997

 usage  : screentoppm <dumpfile>
 output will be sent to stdout
*****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <InterfaceDefs.h>
#include <Application.h>
#include <Screen.h>

int
main(int argc, char **argv)
{
	FILE *f;
	unsigned long x0, y0, x1, y1, colorspace, datasize;
	int i;
	unsigned char buf[4], rgb[3];
	unsigned char c;
	const color_map *colmap;
	const rgb_color *sys_colors;

	// need to contruct a BApplication, or system_colors() will not work
	new BApplication("application/x-vnd.Be-cmd-SPPM");

	if (argc < 2 || strcmp(argv[1], "-h") == 0)
	{
		printf("Usage: screentoppm <screendumpfile>\n");
		printf("Reads Be screendumpfile, writes ppm data to stdout\n");
		return 0;
	}

	if (f = fopen(argv[1], "r"))
	{
		fread(&x0, 4, 1, f);
		fread(&y0, 4, 1, f);
		fread(&x1, 4, 1, f);
		fread(&y1, 4, 1, f);
		fread(&colorspace, 4, 1, f);
		fread(&datasize, 4, 1, f);

		x1++;
		y1++;

//		fprintf(stderr, "Screendump %d x %d, colorspace %d\n",
//				x1, y1, bytesperpixel);

		BScreen screen( B_MAIN_SCREEN_ID );

		switch(colorspace)
		{
			case B_COLOR_8_BIT:
				/* 8-bit screen capture */
				colmap = screen.ColorMap();
				if (colmap == NULL)
				{
					fprintf(stderr, "Cannot find system colors\n");
				}
				else
				{
					sys_colors = colmap->color_list;
					printf("P6\n%d %d\n255\n", x1, y1);
					for(i=x1*y1; i; i--)
					{
						if (fread(&c, 1, 1, f) != 1) break;
						rgb[0] = sys_colors[c].red;
						rgb[1] = sys_colors[c].green;
						rgb[2] = sys_colors[c].blue;
						fwrite(rgb, 3, 1, stdout); /* only 3 bytes rgb */
					}
				}
				break;
			case B_RGB_32_BIT:
				/* 32-bit screen capture */
				printf("P6\n%d %d\n255\n", x1, y1);
				for(i=x1*y1; i; i--)
				{
					if (fread(buf, 4, 1, f) != 1) break;
					rgb[0] = buf[2];
					rgb[1] = buf[1];
					rgb[2] = buf[0];
					fwrite(rgb, 3, 1, stdout); /* only 3 bytes rgb */
				}
				break;
			default:
				fprintf(stderr, "Sorry, unsupported screenmode\n");
				break;
		}
		fclose(f);
	}

	delete be_app;
	return 0;
}

