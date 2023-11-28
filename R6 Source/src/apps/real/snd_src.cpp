#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <OS.h>
#include "snd_src.h"


//---------------------------------------------------


	SndSrc::SndSrc(char *path)
{
	long	ref;

	ref = open(path, O_RDWR);
	length = lseek(ref, 0, SEEK_END);

	length -= 0x40;
	lseek(ref, 0x40, SEEK_SET);

	buffer = (short *)malloc(length);
	read(ref, buffer, length);

	length = length / 4;
	close(ref);
}

//---------------------------------------------------

	SndSrc::~SndSrc()
{
	free((char *)buffer);
}

//---------------------------------------------------

float	SndSrc::GetValue(float i, float dt)
{
	long	pos;
	long	dp;
	float	v;
	short	vv;
	long	pi;
	short	vmin;
	short	vmax;
	long	step;

	dp = (int)(dt * length + 0.5);
	pos = (int)(i * length + 0.5);

	step = 1 + (dp >> 3);			//was 4

	if ((pos + dp) >= length)
		dp = 1;

	vmin = 32767;
	vmax = -32767;

	for (pi = pos; pi < (pos + dp); pi += step) {
		vv = buffer[pi];
		if (vv < vmin) vmin = vv;
		if (vv > vmax) vmax = vv;
	}

	vmin = abs(vmin);
	vmax = abs(vmax);	
	
	if (vmin > vmax) vv = vmin; else vv = vmax;

	v = vv / 16384.0;
	
	if (v > 1.0) v = 1.0;
	return v;
}

//---------------------------------------------------
