#include "private.h"
#include "generic.h"
#include "SetRegisters.h"

status_t SET_CURSOR_SHAPE(uint16 width, uint16 height, uint16 hot_x, uint16 hot_y, uint8 *andMask, uint8 *xorMask) {
	uchar CursorPic[1024];
	int InputByteOset;
	int InputBitOset;
	int OutputByteOset;
	int OutputBitOset;
	int iX;
	int iY;
	uchar AndBit;
	uchar XorBit;

	// NOTE: For this implementation (and for the 3D Rage Pro in general), cursor width
	// and height _must_ be <= 64.
	// NOTE: For BeOS, cursor width and height must be equal to 16.

	if ((width != 16) || (height != 16))
	{
		ddprintf(("ERROR - Invalid cursor dimensions specified to define_cursor."));
		return B_ERROR;
	}
	else if ((hot_x >= width) || (hot_y >= height))
	{
		ddprintf(("ERROR - Hot spot position in define_cursor is not within cursor."));
		return B_ERROR;
	}
	else
	{
		ddprintf(("Define_Cursor call ok. Init and draw cursor."));

		// Init cursor image
		for (iX = 0; iX < 1024; iX++)
			CursorPic[iX] = 0;

		// Construct cursor image from masks.
		InputByteOset = 0;
		OutputByteOset = 0; // Keep in upper right corner of 64x64 region.
		for (iY = 0; iY < height; iY++)
		{
			InputBitOset = 0;
			OutputBitOset = (64 - width) << 1;

			for (iX = 0; iX < width; iX++)
			{
				// See the BeOS and 3D Rage Pro documentation for the cursor pixel
				// representations under each system.

				// Assume that mask data has MS..LS pixel order within bytes.
				AndBit = andMask[InputByteOset + (InputBitOset >> 3)];
				AndBit = (AndBit >> (7 - (InputBitOset & 7))) & 1;
				XorBit = xorMask[InputByteOset + (InputBitOset >> 3)];
				XorBit = (XorBit >> (7 - (InputBitOset & 7))) & 1;

				// Cursor data has reverse pixel order within bytes (LS..MS).

//				CursorPic[OutputByteOset + (OutputBitOset >> 3)] |=
//					(((AndBit ^ 1) << 1) | (XorBit ^ AndBit)) << (OutputBitOset & 7);

				// Be's documentation lies; and bit should be inverse of stated.

				CursorPic[OutputByteOset + (OutputBitOset >> 3)] |=
					((AndBit << 1) | (XorBit ^ (AndBit ^ 1))) << (OutputBitOset & 7);

				InputBitOset++;
				OutputBitOset += 2;
			}

			InputByteOset += (width + 7) >> 3;
			OutputByteOset += 16;
		}


		// Critical section - accessing graphics card memory and registers.
		//lock_card();


		// Copy the cursor image to the cursor buffer.
		memcpy(ai->Cursor.pCursorBuffer, CursorPic, 1024);

		// Set the cursor buffer and size.
		// Buffer offset is in qwords from the base of the primary aperature.
		WRITE_REG(CUR_OFFSET, ((uchar *) (ai->Cursor.pCursorBuffer) - (uchar *) (si->framebuffer)) >> 3);
		WRITE_REG(CUR_HORZ_VERT_OFF, ((64 - height) << 16) | (64 - width));

		// Set the cursor colours.
		// NOTE: The Rage Pro wants an 8-bit colour index even though it doesn't use it. Give it
		// an index that will be meaningful under most palettes.
		WRITE_REG(CUR_CLR0, 0x00000000); // (0,0,0), 8-bit colour index 0
		WRITE_REG(CUR_CLR1, 0xFFFFFF0F); // (255,255,255), 8-bit colour index 15

		// Update cursor variables appropriately.
		ai->Cursor.CursorDefined = 1;
		ai->Cursor.Width = (int) width;
		ai->Cursor.Height = (int) height;
		ai->Cursor.HotX = (int) hot_x;
		ai->Cursor.HotY = (int) hot_y;


		// End critical section.
		//unlock_card();


		// Set the cursor position.
		// THIS ISNT'T NECESSARY UNDER THE BEOS SPEC!
//		if ((move_cursor(0, 0) != B_OK)  // Set location to (0,0) on the display.
//			|| (show_cursor(bYES) != B_OK))  // Display the cursor.
//			return B_ERROR;
	}

	return B_OK;
}

void MOVE_CURSOR(uint16 x, uint16 y) {
	int CurX, CurY;
	int XDelta, YDelta;

	bool move_screen = false;
	uint16 hds = ai->dm.h_display_start;
	uint16 vds = ai->dm.v_display_start;
	uint16 h_adjust = 7; // some power of 2 - 1
	/* clamp cursor to virtual display */
	if (x >= ai->dm.virtual_width) x = ai->dm.virtual_width - 1;
	if (y >= ai->dm.virtual_height) y = ai->dm.virtual_height - 1;
	/* adjust h/v_display_start to move cursor onto screen */
	if (x >= (ai->dm.timing.h_display + hds)) {
		hds = ((x - ai->dm.timing.h_display) + 1 + h_adjust) & ~h_adjust;
		move_screen = true;
	} else if (x < hds) {
		hds = x & ~h_adjust;
		move_screen = true;
	}
	if (y >= (ai->dm.timing.v_display + vds)) {
		vds = y - ai->dm.timing.v_display + 1;
		move_screen = true;
	} else if (y < vds) {
		vds = y;
		move_screen = true;
	}
	// checking for visible cursor should keep Dominoes from bouncing around
	if (move_screen && ai->cursor_is_visible) MOVE_DISPLAY(hds,vds);

	/* put cursor in correct physical position */
	x -= hds;
	y -= vds;

	// Find cursor position and size change needed (if any).
	CurX = x;
	CurX -= ai->Cursor.HotX;
	CurY = y;
	CurY -= ai->Cursor.HotY;

	if (CurX < 0)
	{
		XDelta = -CurX;
		CurX = 0;
	}
	else
		XDelta = 0;

	if (CurY < 0)
	{
		YDelta = -CurY;
		CurY = 0;
	}
	else
		YDelta = 0;

	// Adjust cursor position and size and cursor offset appropriately.
	WRITE_REG(CUR_HORZ_VERT_OFF, ((64 - ai->Cursor.Height + YDelta) << 16) | (64 - ai->Cursor.Width + XDelta));
	WRITE_REG(CUR_OFFSET, (((uchar *) (ai->Cursor.pCursorBuffer) - (uchar *) (si->framebuffer)) >> 3) + (2 * YDelta));
	WRITE_REG(CUR_HORZ_VERT_POSN, (CurY << 16) | CurX);

}

void SHOW_CURSOR(bool is_visible) {
	uint32 ControlFlags;

	if (is_visible)
	{
		// Adjust control flags.
		READ_REG(GEN_TEST_CNTL, ControlFlags);
		ControlFlags = (ControlFlags & 0x00203000) | 0x80;
		WRITE_REG(GEN_TEST_CNTL, ControlFlags);
	
		ddprintf(("Cursor made visible."));
	}
	else
	{
		// Adjust control flags.
		READ_REG(GEN_TEST_CNTL, ControlFlags);
		ControlFlags = ControlFlags & 0x00203000;
		WRITE_REG(GEN_TEST_CNTL, ControlFlags);
	
		ddprintf(("Cursor hidden."));
	}
	ai->cursor_is_visible = is_visible;
}
