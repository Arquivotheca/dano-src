#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <OS.h>

int
main(int argc, char **argv)
{
	int		fd;
	uint16	coord[2];
	float	saveX[4], saveY[4];
	int		count = 0;
	bool	saveDown;
	float	xDelta, xScale, xMuly;
	float	yDelta, yScale, yMulx;
	int		i;

	fd = open("/dev/misc/cop8", O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "cannot open device\n");
		return 1;
	}
	saveDown = false;
	fprintf(stderr, "click in: top left corner\n");
	while (read(fd, &coord, sizeof(coord)) == sizeof(coord)) {
		bool down = false;
		bool up = false;
		bool move = false;

		if ((coord[0] == 0xFEFE) && (coord[1] == 0x0000)) {
			if (saveDown) {
				up = true;
				saveDown = false;
				count++;
				switch(count) {
				case 1:
					xDelta = -saveX[0];
					yDelta = -saveY[0];
					fprintf(stderr, "click in bottom left corner\n");
					break;
				case 2:
					yScale = 1 / (saveY[1] + yDelta);
					fprintf(stderr, "click in top right corner\n");
					break;
				case 3:
					xScale = 1 / (saveX[2] + xDelta);
					fprintf(stderr, "click in bottom right corner\n");
					break;
				case 4:
					xMuly = (1 / ((saveX[3] + xDelta) * xScale) - 1) / ((saveY[3] + yDelta) * yScale);
					yMulx = (1 / ((saveY[3] + yDelta) * yScale) - 1) / ((saveX[3] + xDelta) * xScale);
					printf("%f %f %f %f %f %f %f %f\n",
						xDelta, xScale, 0.0, xMuly,
						yDelta, yScale, 0.0, yMulx);
					fprintf(stderr, "validation:\n");
					for(i=0; i<4; i++)
						fprintf(stderr, " (%f,%f)\n",
							((saveX[i] + xDelta) * xScale) * (1 + ((saveY[i] + yDelta) * yScale) * xMuly),
							((saveY[i] + yDelta) * yScale) * (1 + ((saveX[i] + xDelta) * xScale) * yMulx));
					return 0;
				}
			}
		}
		else if ((coord[0] == 0xFEFE) && ((coord[1] & 0xff) == 0x01)) {
			uint8 buttons = coord[1] >> 8;
		}
		else {
			if (!saveDown) {
				down = true;
				saveDown = true;
			}
		
			float kXMin = 0x1900;
			float kXRange = 0xd700 - kXMin;
			float kYMin = 0x2a00;
			float kYRange = 0xd700 - kYMin;
			
			float x = ((float)coord[0] - kXMin) / kXRange;
			float y = ((float)coord[1] - kYMin) / kYRange;
			
			if ((x != saveX[count]) || (y != saveY[count])) {
				move = true;
				saveX[count] = x;
				saveY[count] = y;
			}
			
		}
	}
	return 1;
}
