#include "GlobalData.h"
#include "generic.h"

status_t SET_CURSOR_SHAPE(uint16 width, uint16 height, uint16 hot_x, uint16 hot_y, uint8 *andMask, uint8 *xorMask) {
	(void)width;(void)height;(void)hot_x;(void)hot_y;(void)andMask;(void)xorMask;
	return B_OK;
}

/*
Move the cursor to the specified position on the desktop.  If we're
using some kind of virtual desktop, adjust the display start position
accordingly and position the cursor in the proper "virtual" location.
*/
void MOVE_CURSOR(uint16 x, uint16 y) {
	(void)x;(void)y;
}

void SHOW_CURSOR(bool is_visible) {
	(void)is_visible;
}
