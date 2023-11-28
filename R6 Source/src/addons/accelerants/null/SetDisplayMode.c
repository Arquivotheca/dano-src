#include "GlobalData.h"
#include "generic.h"
#include <sys/ioctl.h>
#include <GraphicsDefs.h>
/*
	Calculates the number of bits for a given color_space.
	Usefull for mode setup routines, etc.
*/
static uint32 calcBitsPerPixel(uint32 cs) {
	uint32	bpp = 0;

	switch (cs) {
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB32_LITTLE:
		case B_RGBA32_LITTLE:
			bpp = 32; break;
		case B_RGB24_BIG:
		case B_RGB24_LITTLE:
			bpp = 24; break;
		case B_RGB16_BIG:
		case B_RGB16_LITTLE:
			bpp = 16; break;
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_RGB15_LITTLE:
		case B_RGBA15_LITTLE:
			bpp = 15; break;
		case B_CMAP8:
			bpp = 8; break;
	}
	return bpp;
}

/*
	The code to actually configure the display.  Unfortunately, there's not much
	that can be provided in the way of sample code.  If you're lucky, you're writing
	a driver for a device that has all (or at least most) of the bits for a particular
	configuration value in the same register, rather than spread out over the standard
	VGA registers + a zillion expansion bits.  In any case, I've found that the way
	to simplify this routine is to do all of the error checking in PROPOSE_DISPLAY_MODE(),
	and just assume that the values I get here are acceptable.
*/
static void do_set_display_mode(display_mode *dm) {
	/* save the display mode */
	si->dm = *dm;
	si->fbc.frame_buffer = si->framebuffer;
	si->fbc.frame_buffer_dma = si->framebuffer_pci;
	si->fbc.bytes_per_row = dm->virtual_width * (calcBitsPerPixel(dm->space) >> 3);
}

/*
	The exported mode setting routine.  First validate the mode, then call our
	private routine to hammer the registers.
*/
status_t SET_DISPLAY_MODE(display_mode *mode_to_set) {
	(void)mode_to_set;
	do_set_display_mode(my_mode_list);
	return B_OK;
}

/*
	Set which pixel of the virtual frame buffer will show up in the
	top left corner of the display device.  Used for page-flipping
	games and virtual desktops.
*/
status_t MOVE_DISPLAY(uint16 h_display_start, uint16 v_display_start) {
	(void)h_display_start;(void)v_display_start;
	return B_OK;
}

/*
	Set the indexed color palette.
*/
void SET_INDEXED_COLORS(uint count, uint8 first, uint8 *color_data, uint32 flags) {
(void)count;(void)first;(void)color_data;(void)flags;
}
