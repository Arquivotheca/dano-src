//////////////////////////////////////////////////////////////////////////////
// Cursor Functions
//    This file implements the generic, hardware-independent functions
// required for cursor manipulation.
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Includes //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include <common_includes.h>
#include <accel_includes.h>


//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Set the Cursor Shape

status_t SetCursorShape(uint16 width,
                        uint16 height,
                        uint16 hot_x,
                        uint16 hot_y,
                        uint8 *andMask,
                        uint8 *xorMask)
{
  int i;
  int j;

  uchar *cursor0; // [512]
  uchar *cursor1; // [512]
  uchar c0[512];
  uchar c1[512];
  bool changed = false;

  // NOTE: This is assumed to be serialized by the app server, which is
  // theoretically the only thing that touches the cursor under normal
  // conditions. So, no critical section checking. Trey's call, not mine.

  // Initialize pointers.
  cursor0 = si->cursor.cursor0; // [512]
  cursor1 = si->cursor.cursor1; // [512]

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
          cursor1[k] = *andMask++;
        }
    }

  // program the cursor if it changed shape

  if(changed)
    {
      // NOTE: Put locking on the cursor images here, so that the cursor
      // image isn't changed as we're storing it. These are local pointers
      // to variables in the SHARED_INFO structure.
      do_set_cursor_image(cursor0, cursor1);
    }

  // update the translated cursor coords.
  // Remember, if the hot spot moves one way, the cursor moves the other.

  si->cursor.cursor_x += (int)si->cursor.hot_x - (int)hot_x;
  si->cursor.cursor_y += (int)si->cursor.hot_y - (int)hot_y;

  // remember new hot spot

  si->cursor.hot_x = hot_x;
  si->cursor.hot_y = hot_y;

  // Reposition the cursor in case the hot-spot changed

  UpdateCursorPosition(si->cursor.cursor_x, si->cursor.cursor_y);

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Move the Cursor

void MoveCursor(uint16 x,
                uint16 y)
{
  bool move_screen = false;
  uint16 hds = si->display.dm.h_display_start;
  uint16 vds = si->display.dm.v_display_start;
  uint16 h_adjust = 7; // some power of 2 - 1

  // NOTE: This is assumed to be serialized by the app server, which is
  // theoretically the only thing that touches the cursor under normal
  // conditions. So, no critical section checking. Trey's call, not mine.

  // clamp cursor to virtual display
  if(x >= si->display.dm.virtual_width)  x = si->display.dm.virtual_width - 1;
  if(y >= si->display.dm.virtual_height) y = si->display.dm.virtual_height - 1;

  // adjust h/v_display_start to move cursor onto screen
  if(x >= (si->display.dm.timing.h_display + hds))
    {
      hds = ((x - si->display.dm.timing.h_display) + 1 + h_adjust) & ~h_adjust;
      move_screen = true;
    }
  else if(x < hds)
    {
      hds = x & ~h_adjust;
      move_screen = true;
    }

  if(y >= (si->display.dm.timing.v_display + vds))
    {
      vds = y - si->display.dm.timing.v_display + 1;
      move_screen = true;
    }
  else if(y < vds)
    {
      vds = y;
      move_screen = true;
    }

  if (move_screen)
    MoveDisplayArea(hds,vds);

  // put cursor in correct physical position
  x -= hds;
  y -= vds;

  // move the cursor to the requested location
  // save the location
  si->cursor.cursor_x = 64 - si->cursor.hot_x + x;
  si->cursor.cursor_y = 64 - si->cursor.hot_y + y;

  // Update the cursor position.
  UpdateCursorPosition(si->cursor.cursor_x, si->cursor.cursor_y);
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
