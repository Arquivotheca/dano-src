#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Debug.h>
#include <InterfaceDefs.h>
#include <Accelerant.h>
#include <stdio.h>

#include "screen_utils.h"

double rate_from_display_mode(const display_mode *dm)
{
	double dmRate = (double)dm->timing.pixel_clock * (double)1000.0;
	dmRate /= (double)dm->timing.h_total * (double)dm->timing.v_total;
	if (dm->timing.flags & B_TIMING_INTERLACED) dmRate /= 2;
	return dmRate;
}

bool modes_match(const display_mode *mode1, const display_mode *mode2)
{
	return memcmp(mode1, mode2, sizeof(display_mode)) == 0;
#if 0
	double rate1 = rate_from_display_mode(mode1);
	double rate2 = rate_from_display_mode(mode2);

	return (dm->timing.h_display == width) && (dm->timing.v_display == height) &&
			(dm->virtual_width == width) && (dm->virtual_height == height) &&
			fabs(rate1 - rate2) <= epsilon &&
			mode1->space == mode2->space &&
			mode1->flags == mode2->flags &&
			(mode1->timing.flags & ~(B_TIMING_INTERLACED)) ==
			(mode2->timing.flags & ~(B_TIMING_INTERLACED));
#endif
}

const char *spaceToMenuItem(uint32 /*cs*/)
{
	return 0;
};

const char *spaceToString(uint32 cs)
{
	const char *s;
	switch(cs)
	{
		case B_RGB32_LITTLE :
		case B_RGBA32_LITTLE :
		case B_RGB32_BIG :
		case B_RGBA32_BIG :
			s = "32 Bits/Pixel";
			break;

		case B_RGB16_LITTLE :
		case B_RGB16_BIG :
			s = "16 Bits/Pixel";
			break;

		case B_RGB15_LITTLE :
		case B_RGBA15_LITTLE :
		case B_RGB15_BIG :
		case B_RGBA15_BIG :
			s = "15 Bits/Pixel";
			break;

		case B_CMAP8 :
			s = "8 Bits/Pixel";
			break;

#define s2s(a) case a: s = #a ; break
		s2s(B_GRAY8);
		s2s(B_GRAY1);
		s2s(B_YUV422);
		s2s(B_YUV411);
		s2s(B_YUV9);
		s2s(B_YUV12);
		default:
			s = "unknown"; break;
#undef s2s
	}
	return s;
}

void dump_mode(const display_mode *dm)
{
	const display_timing *t = &(dm->timing);
	printf("  pixel_clock: %ldKHz\n", t->pixel_clock);
	printf("            H: %4d %4d %4d %4d\n", t->h_display, t->h_sync_start, t->h_sync_end, t->h_total);
	printf("            V: %4d %4d %4d %4d\n", t->v_display, t->v_sync_start, t->v_sync_end, t->v_total);
	printf(" timing flags:");
	if (t->flags & B_BLANK_PEDESTAL) printf(" B_BLANK_PEDESTAL");
	if (t->flags & B_TIMING_INTERLACED) printf(" B_TIMING_INTERLACED");
	if (t->flags & B_POSITIVE_HSYNC) printf(" B_POSITIVE_HSYNC");
	if (t->flags & B_POSITIVE_VSYNC) printf(" B_POSITIVE_VSYNC");
	if (t->flags & B_SYNC_ON_GREEN) printf(" B_SYNC_ON_GREEN");
	if (!t->flags) printf(" (none)\n");
	else printf("\n");
	printf(" refresh rate: %4.2f\n", rate_from_display_mode(dm));
	printf("  color space: %s\n", spaceToString(dm->space));
	printf(" virtual size: %dx%d\n", dm->virtual_width, dm->virtual_height);
	printf("dispaly start: %d,%d\n", dm->h_display_start, dm->v_display_start);

	printf("   mode flags:");
	if (dm->flags & B_SCROLL) printf(" B_SCROLL");
	if (dm->flags & B_8_BIT_DAC) printf(" B_8_BIT_DAC");
	if (dm->flags & B_HARDWARE_CURSOR) printf(" B_HARDWARE_CURSOR");
	if (dm->flags & B_PARALLEL_ACCESS) printf(" B_PARALLEL_ACCESS");
	if (dm->flags & B_IO_FB_NA) printf(" B_IO_FB_NA");
	if (!dm->flags) printf(" (none)\n");
	else printf("\n");
	printf("\n");
}
