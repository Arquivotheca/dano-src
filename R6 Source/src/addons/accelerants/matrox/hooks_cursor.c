//////////////////////////////////////////////////////////////////////////////
// Cursor Functions
//    This is where we store the cursor hooks.  It's a dank, drafty little
// corner that we don't visit much, but it's nice to have here.
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Includes //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include <Accelerant.h>
#include <Drivers.h>
#include <registers.h>
#include <private.h>
#include <cardid.h>

#include <unistd.h>

#include "globals.h"
#include "debugprint.h"
#include "mga_bios.h"
#include "accelerant_info.h"
#include "globals.h"
#include "hooks_mode.h"
#include "hooks_cursor.h"


//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Actually set the cursor position bits
//    Cards with the TI DAC need to move the cursor in the vertical retrace
// period.  Cards with integrated DACs need to move the cursor *outside* the
// retrace.

static void UpdateCursorPosition(void)
{
  switch(si->device_id)
    {
    case MGA_2064W:
    case MGA_2164W:
    case MGA_2164W_AGP:
      atomic_or((int32 *)&(si->flags), MKD_MOVE_CURSOR);
      break;

    case MGA_1064S:
    case MGA_G100_AGP:
    case MGA_G200_AGP:
    case MGA_G100_PCI:
    case MGA_G200_PCI:
      {
        uint16 cx = si->cursor_x;
        uint16 cy = si->cursor_y;

        // make pretty sure we don't update during the blanking period
        // wait at most one second for vertical blank
        //acquire_sem_etc(si->vblank, 1, B_TIMEOUT, 1 * 1000 * 1000);
        // wait until we're NOT in a vertical retrace
        while (STORM32(STORM_STATUS) & 0x08) /* spin */;
                
        // now update the regs
        DAC8W(TVP3026_CUR_XHI, (cx >> 8) & 0xff);
        DAC8W(TVP3026_CUR_XLOW, cx & 0xff);
        DAC8W(TVP3026_CUR_YHI, (cy >> 8) & 0xff);
        DAC8W(TVP3026_CUR_YLOW, cy & 0xff);

      }
      break;
    }
}


//////////////////////////////////////////////////////////////////////////////
// Show/Hide the Cursor
//    Not much to be said here.  Depending on the flag setting, we either
// show or hide the cursor.

void ShowCursor(bool is_visible)
{
  uchar tmpByte;
        
  if (si->device_id != MGA_1064S) {

    DAC8W(TVP3026_INDEX, TVP3026_CURSOR_CTL);
    DAC8R(TVP3026_DATA, tmpByte);

    if(is_visible) tmpByte |= 0x02;
    else           tmpByte &= 0xfc;

    switch (si->device_id)
      {
        case MGA_1064S: // two bits of cursor control - other bits should be zero on write
          tmpByte &= 0x03;
          break;
        case MGA_G100_AGP: // three bits of cursor control - other bits should be zero on write
        case MGA_G200_AGP:
        case MGA_G100_PCI:
        case MGA_G200_PCI:
          tmpByte &= 0x07;
          break;
        default: // cards with the TI dac only
          if (ai->dm.timing.h_display >= 1200) tmpByte |= 0x10;
          else tmpByte &= 0xef;
          tmpByte &= 0xdf;
          break;
      }

    DAC8W(TVP3026_DATA, tmpByte);
  } else {
    // hide mystique cursor by moving off screen
    if (is_visible) UpdateCursorPosition();
    else {
      DAC8W(TVP3026_CUR_XHI, 0);
      DAC8W(TVP3026_CUR_XLOW, 0);
    }
  }
  ai->cursor_is_visible = is_visible;
}


//////////////////////////////////////////////////////////////////////////////
// Set Cursor Shape

status_t SetCursorShape(uint16 width,
                        uint16 height,
                        uint16 hot_x,
                        uint16 hot_y,
                        uint8 *andMask,
                        uint8 *xorMask)
{
  int i;
  int j;

  uchar tmpByte;
  uchar *cursor0 = si->cursor0; // [512]
  uchar *cursor1 = si->cursor1; // [512]
  uchar c0[512];
  uchar c1[512];
  bool changed = false;

  // init the cursor pixmaps

  for(i = 0; i < 512; i++)
    {
      // remember the shape of the old cursor
      c0[i] = cursor0[i];
      c1[i] = cursor1[i];
      // clear the new cursor to be transparent
      cursor0[i] = 0x00;
      cursor1[i] = 0xff;
    }
        
  for (i = 0; i < height; i++) // One row at a time...
    {
      for (j = 0; j < width >> 3; j++) // 8 pixels at a time...
        {
          int k;
          k = i * 8 + j;
          // note if the cursor actually changed shape
          if (c0[k] != *xorMask) changed = true;
          if (c1[k] != *andMask) changed = true;
          // set the new cursor bits
          cursor0[k] = *xorMask++;
          cursor1[k] = *andMask++; // sense of bits reversed for TVP3026
        }
    }

  // progam the cursor only if it changed shape

  if(changed)
    {
      switch(si->device_id)
        {
        case MGA_2064W:
        case MGA_2164W:
        case MGA_2164W_AGP:
          {
            uint32 cursor_was_visible = ai->cursor_is_visible;
                
            if(ai->set_cursor_colors)
              {
                DAC8W(TVP3026_CUR_COL_ADDR, 0x01); // Start with color 0.
                DAC8W(TVP3026_CUR_COL_DATA, 0xff); // Make it white.
                DAC8W(TVP3026_CUR_COL_DATA, 0xff);
                DAC8W(TVP3026_CUR_COL_DATA, 0xff);

                // Auto-advance to cursor color 1.

                DAC8W(TVP3026_CUR_COL_DATA, 0x00); // Make it black.
                DAC8W(TVP3026_CUR_COL_DATA, 0x00);
                DAC8W(TVP3026_CUR_COL_DATA, 0x00);
                ai->set_cursor_colors = 0;
              }

            if(cursor_was_visible) ShowCursor(false);

            // wait at most one second for vertical blank
            // avoid changing the cursor shape until we _know_ it's hidden

            acquire_sem_etc(si->vblank, 1, B_TIMEOUT, 1 * 1000 * 1000);

            // now re-program the cursor

            DAC8W(TVP3026_INDEX, TVP3026_CURSOR_CTL);
            DAC8R(TVP3026_DATA, tmpByte);
            tmpByte &= 0xf3; // cursor ram address top two bits == 0
            DAC8W(TVP3026_DATA, tmpByte);
            DAC8W(TVP3026_INDEX, 0x00); // start with cursor color 0

            for (i = 0; i < 512; i++) DAC8W(TVP3026_CUR_RAM, cursor0[i]);
            for (i = 0; i < 512; i++) DAC8W(TVP3026_CUR_RAM, cursor1[i]);

            if(cursor_was_visible) ShowCursor(true);
          }
          break;

        case MGA_1064S:
        case MGA_G100_AGP:
        case MGA_G200_AGP:
        case MGA_G100_PCI:
        case MGA_G200_PCI:
          {
            // cursor is stored at end of frame buffer minus 2KB
            // the buffer only needs 1KB, but putting it there causes
            // cursor glitches on the G200, so we back off another 1KB

            uchar *cursorData = ai->cursorData;
            ulong cursorAddr = (uchar *)cursorData - (uchar *)si->framebuffer;

#if defined(__POWERPC__)
            ulong tmpUlong;
#endif

            ddprintf(("cursorAddr: 0x%08x, cursorData: 0x%08x\n",
                      cursorAddr,
                      cursorData));

            if (ai->set_cursor_colors)
              {
                if (si->device_id == MGA_1064S) {
                  // the Mystique cursor is always shown
                  // it's hidden by moving it off screen
                  // This is because reading/writing the contents of
                  // this register under heavy frame buffer access on
                  // multi-processor hardware locks the PCI bus!
                  DAC8W(TVP3026_INDEX, TVP3026_CURSOR_CTL);
                  DAC8W(TVP3026_DATA, 0x02);
                }
                DAC8W(MID_INDEX, MID_XCURCOL0RED);
                DAC8W(MID_X_DATAREG, 0xff);
                DAC8W(MID_INDEX, MID_XCURCOL0GREEN);
                DAC8W(MID_X_DATAREG, 0xff);
                DAC8W(MID_INDEX, MID_XCURCOL0BLUE);
                DAC8W(MID_X_DATAREG, 0xff);
                DAC8W(MID_INDEX, MID_XCURCOL1RED);
                DAC8W(MID_X_DATAREG, 0x00);
                DAC8W(MID_INDEX, MID_XCURCOL1GREEN);
                DAC8W(MID_X_DATAREG, 0x00);
                DAC8W(MID_INDEX, MID_XCURCOL1BLUE);
                DAC8W(MID_X_DATAREG, 0x00);

                // make sure cursor address is in control registers

                DAC8W(MID_INDEX, MID_XCURADDH);
                DAC8W(MID_X_DATAREG, (cursorAddr >> 18) & 0x1f);
                DAC8W(MID_INDEX, MID_XCURADDL);
                DAC8W(MID_X_DATAREG, (cursorAddr >> 10) & 0xff);
                ai->set_cursor_colors = 0;
              }

            // wait at most one second for vertical blank
            //acquire_sem_etc(si->vblank, 1, B_TIMEOUT, 1 * 1000 * 1000);

#if defined(__POWERPC__)
			// We need to do this on PPC because of byte-swapping issues
			// It's not required with little-endian framebuffers
			
            // save the MACCESS value
            STORM32R(STORM_OPMODE, tmpUlong);

            // make writes 8bits wide
            STORM32W(STORM_OPMODE, 0);
#endif

            // write cursor bitmaps

            for(i = 0; i < 64; i++)
              {
                for(j = 0; j < 8; j++)
				  {
				    *cursorData++ = cursor0[i * 8 + (7 - j)];
				  }
		
		        for(j = 0; j < 8; j++)
				  {
				    *cursorData++ = cursor1[i * 8 + (7 - j)];
				  }
              }
			// Fill up the next 1KB with "transparent" so that the buffer overrun at
			// the top of the display with G200 cards isn't as visible.
			// The cursor still "blinks", but I haven't found a way to stop that yet.
			// UPDATE: ChrisT says Matrox acknowledged the bug, and says the workaround
			// is to reprogram the cursor shape when the cursor move near the top of the
			// display.  Needs to be implemented after R4.1
			for(i = 0; i < 64; i++)
				{
					for(j = 0; j < 8; j++) *cursorData++ = 0x00;
					for(j = 0; j < 8; j++) *cursorData++ = 0xff;
				}

#if defined(__POWERPC__)
            // restore writes
            STORM32W(STORM_OPMODE, tmpUlong);
#endif

          }
        }
    }

  // update the translated cursor coords.
  // Remember, if the hot spot moves one way, the cursor moves the other.

  si->cursor_x += (int)ai->hot_x - (int)hot_x;
  si->cursor_y += (int)ai->hot_y - (int)hot_y;

  // remember new hot spot

  ai->hot_x = hot_x;
  ai->hot_y = hot_y;

  // Reposition the cursor in case the hot-spot changed

  UpdateCursorPosition();

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Move the Cursor

void MoveCursor(uint16 x,
                uint16 y)
{
  bool move_screen = false;
  uint16 hds = ai->dm.h_display_start;
  uint16 vds = ai->dm.v_display_start;
  uint16 h_adjust = 7; // some power of 2 - 1

  // clamp cursor to virtual display
  if(x >= ai->dm.virtual_width)  x = ai->dm.virtual_width - 1;
  if(y >= ai->dm.virtual_height) y = ai->dm.virtual_height - 1;

  // adjust h/v_display_start to move cursor onto screen
  if(x >= (ai->dm.timing.h_display + hds))
    {
      hds = ((x - ai->dm.timing.h_display) + 1 + h_adjust) & ~h_adjust;
      move_screen = true;
    }
  else if(x < hds)
    {
      hds = x & ~h_adjust;
      move_screen = true;
    }

  if(y >= (ai->dm.timing.v_display + vds))
    {
      vds = y - ai->dm.timing.v_display + 1;
      move_screen = true;
    }
  else if(y < vds)
    {
      vds = y;
      move_screen = true;
    }

  if(move_screen) MoveDisplayArea(hds,vds);

  // put cursor in correct physical position
  x -= hds;
  y -= vds;

  // move the cursor to the requested location
  // save the location
  si->cursor_x = 64 - ai->hot_x + x;
  si->cursor_y = 64 - ai->hot_y + y;

  // HACK: part of the Mystique MP machine fix
  if ((ai->cursor_is_visible) || (si->device_id != MGA_1064S))
    UpdateCursorPosition();
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
