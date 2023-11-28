//////////////////////////////////////////////////////////////////////////////
// 2D Acceleration
//    These are the basic 2D acceleration functions which are currently
// supported by the BeOS R4.x driver model.  At the moment, that means
// blit, rect, dest-invert blit and span.
//
//    In time, I'd like to move this over to the driver and call through
// to it with something from the accelerant.  Ideally, I'd like to see as
// much of the device-dependant stuff as possible go over to the kernel
// driver.
//
// Device Dependance: Lots.
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//  Includes /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include <Accelerant.h>
#include <Drivers.h>
#include <registers.h>
#include <private.h>

#include "globals.h"
#include "mga_bios.h"
#include "accelerant_info.h"
#include "hooks_2d.h"


//////////////////////////////////////////////////////////////////////////////
// Defines ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#define SET_CMD(cmd) if (ai->last_command != cmd) { STORM32W(STORM_DWGCTL, ai->last_command = cmd) ; ai->fifo_count++; }

#define SET_COLOR(color) if (ai->last_fg_color != color) { STORM32W(STORM_FCOL, ai->last_fg_color = color) ; ai->fifo_count++; }

#if 1
#define WAIT_FOR_SLOTS(x) while ((STORM32(STORM_FIFOSTATUS) & 0x7f) < (x)) /* DO NOTHING */
#else
#define WAIT_FOR_SLOTS(x) (void)0
#endif

//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Screen to Screen Blit
//    This is the blit function.  I'll probably change the name of it at some
// point (it's a bit cumbersome right now...), but otherwise it's fine.  I've
// worked my usual brand of havoc on the function; it no longer checks to see
// if there are no entries in the list before proceeding to the main loop,
// which is a lose on the stupid case but a win otherwise.  It's still safe,
// since nothing touches the list until it's inside the while loop and the
// condition for while loop entry is the presence of at least one list
// member.
//    I've also inlined everything to speed up the math a bit.  The way
// things were before, there were a set of branches that the code passed
// through, and then it did the work based on some auto variables filled in
// on the way.  This way, we save some auto vars and we don't lose much on
// code size.  We also save some copy overhead.
//    Also, thought it's a small thing, I've tried to interleve the math
// and the register assignments.  If the compiler is cunning enough, it may
// be able to do the math while waiting for the bus I/O to complete.  I
// sincerely doubt it, but what the hell.  It's worth a shot on superscalar
// systems.
//
// For reference:
//
//   SGN     - Sign of the blit.
//   YDST    - Linear address >> 5.
//   LEN     - Length (in lines) of the blit.
//   FXBNDRY - Dest left & right boundary.
//   AR0     - Address: Last pixel of the first line.
//   AR3     - Address: First pixel of the first line.
//   AR5     - Source Y increment.
//   +GO     - Tacked on to a reg address, issues the "go" command.

void ScreenToScreenBlit(engine_token *et,
                        blit_params *list,
                        uint32 count)
{
  uint32 start; // linear start addresses.

  //   Stuff the drawing command into the register, since it's not going to
  // change over the lifetime of this function.
  WAIT_FOR_SLOTS(1);

  STORM32W(STORM_DWGCTL, (0x04004008         |
                          STORM_ATYPE_RPL    |
                          STORM_BOP_S        |
                          STORM_TRANSC_OPAQUE));

  ai->fifo_count++; // Update FIFO counter.

  //    Right, the startup and one-time stuff is done, and we can get to
  // the work at hand.  I've been a little bit eeeevil here, and have
  // abused the for() semantics slightly to foreshorten the code.  Note
  // that the third field in the for() is "things that happen after each
  // loop iteration"...

  for(; count-- ; list++, ai->fifo_count += 7)
    {
      start = ai->YDstOrg + list->src_left;  // Base linear address.
      WAIT_FOR_SLOTS(7);
      if(list->src_top < list->dest_top) // Bottom to Top
        {
          STORM32W(STORM_AR5,
                   (0x00040000 - ai->pixels_per_row) & 0x0003FFFF);

          start += (list->src_top + list->height) * ai->pixels_per_row;

          STORM32W(STORM_YDST,
                   ((list->dest_top + list->height)
                    * ai->pixels_per_row) >> 5);

          if(list->src_left < list->dest_left) // Right to Left
            {
              STORM32W(STORM_SGN, (STORM_SDY_NEG | STORM_SCANLEFT_ON));
              STORM32W(STORM_AR0, start);
              STORM32W(STORM_AR3, (start + list->width));
            }
          else // Left to Right
            {
              STORM32W(STORM_SGN, (STORM_SDY_NEG | STORM_SCANLEFT_OFF));
              STORM32W(STORM_AR0, (start + list->width));
              STORM32W(STORM_AR3, start);
            }
        }
      else // Top to Bottom
        {
          STORM32W(STORM_AR5, ai->pixels_per_row);

          start += (list->src_top * ai->pixels_per_row);

          STORM32W(STORM_YDST, 
                   ((list->dest_top * ai->pixels_per_row) >> 5));

          if(list->src_left < list->dest_left) // Right to Left
            {
              STORM32W(STORM_SGN, (STORM_SDY_POS | STORM_SCANLEFT_ON));
              STORM32W(STORM_AR0, start);
              STORM32W(STORM_AR3, (start + list->width));
            }
          else // Left to Right
            {
              STORM32W(STORM_SGN, (STORM_SDY_POS | STORM_SCANLEFT_OFF));
              STORM32W(STORM_AR0, (start + list->width));
              STORM32W(STORM_AR3, start);
            }
        }

      STORM32W(STORM_FXBNDRY, 
               (list->dest_left | ((list->dest_left + list->width) << 16)));

      STORM32W(STORM_LEN + STORM_GO,  list->height + 1);
    }
}

//////////////////////////////////////////////////////////////////////////////
// Screen to Screen Transparent Blit
//    This is the blit function.  I'll probably change the name of it at some
// point (it's a bit cumbersome right now...), but otherwise it's fine.  I've
// worked my usual brand of havoc on the function; it no longer checks to see
// if there are no entries in the list before proceeding to the main loop,
// which is a lose on the stupid case but a win otherwise.  It's still safe,
// since nothing touches the list until it's inside the while loop and the
// condition for while loop entry is the presence of at least one list
// member.
//    I've also inlined everything to speed up the math a bit.  The way
// things were before, there were a set of branches that the code passed
// through, and then it did the work based on some auto variables filled in
// on the way.  This way, we save some auto vars and we don't lose much on
// code size.  We also save some copy overhead.
//    Also, thought it's a small thing, I've tried to interleve the math
// and the register assignments.  If the compiler is cunning enough, it may
// be able to do the math while waiting for the bus I/O to complete.  I
// sincerely doubt it, but what the hell.  It's worth a shot on superscalar
// systems.
//
// For reference:
//
//   SGN     - Sign of the blit.
//   YDST    - Linear address >> 5.
//   LEN     - Length (in lines) of the blit.
//   FXBNDRY - Dest left & right boundary.
//   AR0     - Address: Last pixel of the first line.
//   AR3     - Address: First pixel of the first line.
//   AR5     - Source Y increment.
//   +GO     - Tacked on to a reg address, issues the "go" command.

void ScreenToScreenTransBlit(engine_token *et,
						uint32 transparent_color,
                        blit_params *list,
                        uint32 count)
{
  uint32 start; // linear start addresses.

  //   Stuff the drawing command into the register, since it's not going to
  // change over the lifetime of this function.
  WAIT_FOR_SLOTS(3);
  STORM32W(STORM_DWGCTL, (0x44004008         |
                          STORM_ATYPE_RPL    |
                          STORM_BOP_S        |
                          STORM_TRANSC_OPAQUE));

  // all bits participate in the mask
  STORM32W(STORM_BCOL, 0xffffffff);

  // adjust trans_color for bit depth
  switch (ai->dm.space) {
    case B_CMAP8:
      transparent_color &= 0x000000ff;
      transparent_color |= (transparent_color <<  8);
      transparent_color |= (transparent_color << 16);
      break;
    case B_RGB16_BIG:
    case B_RGB15_BIG:
    case B_RGBA15_BIG:
    case B_RGB16_LITTLE:
    case B_RGB15_LITTLE:
    case B_RGBA15_LITTLE:
      transparent_color &= 0x0000ffff;
      transparent_color |= (transparent_color << 16);
      break;
  }
  STORM32W(STORM_FCOL, transparent_color);

  ai->fifo_count += 3; // Update FIFO counter.

  //    Right, the startup and one-time stuff is done, and we can get to
  // the work at hand.  I've been a little bit eeeevil here, and have
  // abused the for() semantics slightly to foreshorten the code.  Note
  // that the third field in the for() is "things that happen after each
  // loop iteration"...

  for(; count-- ; list++, ai->fifo_count += 7)
    {
      start = ai->YDstOrg + list->src_left;  // Base linear address.
      WAIT_FOR_SLOTS(7);
      if(list->src_top < list->dest_top) // Bottom to Top
        {
          STORM32W(STORM_AR5,
                   (0x00040000 - ai->pixels_per_row) & 0x0003FFFF);

          start += (list->src_top + list->height) * ai->pixels_per_row;

          STORM32W(STORM_YDST,
                   ((list->dest_top + list->height)
                    * ai->pixels_per_row) >> 5);

          if(list->src_left < list->dest_left) // Right to Left
            {
              STORM32W(STORM_SGN, (STORM_SDY_NEG | STORM_SCANLEFT_ON));
              STORM32W(STORM_AR0, start);
              STORM32W(STORM_AR3, (start + list->width));
            }
          else // Left to Right
            {
              STORM32W(STORM_SGN, (STORM_SDY_NEG | STORM_SCANLEFT_OFF));
              STORM32W(STORM_AR0, (start + list->width));
              STORM32W(STORM_AR3, start);
            }
        }
      else // Top to Bottom
        {
          STORM32W(STORM_AR5, ai->pixels_per_row);

          start += (list->src_top * ai->pixels_per_row);

          STORM32W(STORM_YDST, 
                   ((list->dest_top * ai->pixels_per_row) >> 5));

          if(list->src_left < list->dest_left) // Right to Left
            {
              STORM32W(STORM_SGN, (STORM_SDY_POS | STORM_SCANLEFT_ON));
              STORM32W(STORM_AR0, start);
              STORM32W(STORM_AR3, (start + list->width));
            }
          else // Left to Right
            {
              STORM32W(STORM_SGN, (STORM_SDY_POS | STORM_SCANLEFT_OFF));
              STORM32W(STORM_AR0, (start + list->width));
              STORM32W(STORM_AR3, start);
            }
        }

      STORM32W(STORM_FXBNDRY, 
               (list->dest_left | ((list->dest_left + list->width) << 16)));

      STORM32W(STORM_LEN + STORM_GO,  list->height + 1);
    }
}


//////////////////////////////////////////////////////////////////////////////
// Fill a Rectangle
//    We used to check the degenerate case here; if some fool called us
// with no rectangles and a count of zero, we had a quick idiot check
// that would catch it.  The cost of this, of course, was an extra
// branch.  Worse, it was a duplicate of the condition of this hook's
// main loop.  So I figured "screw 'em".  You're fool enough to call
// this hook with no work, and you get two registers set and can eat
// the math and I/O overhead.  The exchange for this is one
// less branch on the intro and moderately cleaner code.

void RectangleFill(engine_token *et,
                   uint32 color,
                   fill_rect_params *list,
                   uint32 count)
{
  // Set the drawing control register.
  WAIT_FOR_SLOTS(1);
  if(ai->block_mode_ok) // SGRAM or WRAM is present; use block fill.
    {
      STORM32W(STORM_DWGCTL,
                (0x00007804         |
                 STORM_ATYPE_BLK    | 
                 STORM_BOP_S        |
                 STORM_TRANSC_OPAQUE));
    }
  else // SDRAM is present; use replace fill.
    {
      STORM32W(STORM_DWGCTL,
                (0x00007804         |
                 STORM_ATYPE_RPL    | 
                 STORM_BOP_S        |
                 STORM_TRANSC_OPAQUE));
    }

  ai->fifo_count++; // Update FIFO counter.

  SET_COLOR(color); // Set the drawing colour.

  //    Right, the startup and one-time stuff is done, and we can get to
  // the work at hand.  I've been a little bit eeeevil here, and have
  // abused the for() semantics slightly to foreshorten the code.  Note
  // that the third field in the for() is "things that happen after each
  // loop iteration"...

  for(; count-- ; list++, ai->fifo_count += 3)
    {
      WAIT_FOR_SLOTS(3);
      STORM32W(STORM_FXBNDRY,
               ((uint32)list->left | (uint32)((list->right + 1) << 16)));

      STORM32W(STORM_YDST,
               (((uint32)list->top) * ai->pixels_per_row) >> 5);

      STORM32W(STORM_LEN + STORM_GO,
               ((uint32)list->bottom - (uint32)list->top + 1));
    }
}


//////////////////////////////////////////////////////////////////////////////
// Invert a Rectangle
//    We used to check the degenerate case here; if some fool called us
// with no rectangles and a count of zero, we had a quick idiot check
// that would catch it.  The cost of this, of course, was an extra
// branch.  Worse, it was a duplicate of the condition of this hook's
// main loop.  So I figured "screw 'em".  You're fool enough to call
// this hook with no work, and you get two registers set and can eat
// the math and I/O overhead.  The exchange for this is one
// less branch on the intro and moderately cleaner code.

void RectangleInvert(engine_token *et,
                     fill_rect_params *list,
                     uint32 count)
{
  WAIT_FOR_SLOTS(1);
  STORM32W(STORM_DWGCTL,           // Set the drawing control register to:
           (0x00007804         |   // - rect command
            STORM_ATYPE_RSTR   |   // -
            STORM_BOP_NOTD     |   // - invert destination
            STORM_TRANSC_OPAQUE)); // - opaque draw

  ai->fifo_count++; // Update FIFO counter.

  //    Right, the startup and one-time stuff is done, and we can get to
  // the work at hand.  I've been a little bit eeeevil here, and have
  // abused the for() semantics slightly to foreshorten the code.  Note
  // that the third field in the for() is "things that happen after each
  // loop iteration"...

  for(; count-- ; list++, ai->fifo_count += 3)
    {
      WAIT_FOR_SLOTS(3);
      STORM32W(STORM_FXBNDRY, 
                ((uint32)list->left | (uint32)((list->right+1) << 16)));

      STORM32W(STORM_YDST, 
                (((uint32)list->top) * ai->pixels_per_row) >> 5);

      STORM32W(STORM_LEN + STORM_GO,
                (uint32)list->bottom - (uint32)list->top + 1);
    }
}


//////////////////////////////////////////////////////////////////////////////
// Fill a Span
//    This does about what you'd expect.  A cursory glance at the
// manual didn't enlighten me on the subject of spans, but I assume
// the scheme below (single line rects) will serve as well as any.
//    Once again I've blown the degenerate case test out of a hook
// hook function.  It doesn't save much, and it's only really a win
// if most of the calls to the hook are of zero length.  Otherwise,
// it's a fairly big lose.


void SpanFill(engine_token *et,
              uint32 color,
              uint16 *list,
              uint32 count)
{
  WAIT_FOR_SLOTS(1);
  if(ai->block_mode_ok) // SGRAM or WRAM is present; use block fill.
    {
      STORM32W(STORM_DWGCTL,           // Set draw control reg to:
               (0x00007804         |   //  - rect opcode
                STORM_ATYPE_BLK    |   //  - block mode
                STORM_BOP_S        |   //  - block color fill
                STORM_TRANSC_OPAQUE)); //  - opaque draw
    }
  else // SDRAM is present; use replace fill.
    {
      STORM32W(STORM_DWGCTL,           // Set draw control reg to:
               (0x00007804         |   //  - rect opcode
                STORM_ATYPE_RPL    |   //  - replace mode
                STORM_BOP_S        |   //  - block color fill
                STORM_TRANSC_OPAQUE)); //  - opaque draw
    }

  SET_COLOR(color);       // Set the drawing colour.

  ai->fifo_count += 1;    // Update FIFO counter.

  //    Right, the startup and one-time stuff is done, and we can get to
  // the work at hand.  I've been a little bit eeeevil here, and have
  // abused the for() semantics slightly to foreshorten the code.  Note
  // that the third field in the for() is "things that happen after each
  // loop iteration"...

  for(; count-- ; list += 3, ai->fifo_count += 3)
    {
      WAIT_FOR_SLOTS(3);
      STORM32W(STORM_LEN, 1);

      STORM32W(STORM_FXBNDRY,
                ((uint32)list[1] | (uint32)((list[2]+1) << 16)));

      STORM32W(STORM_YDST + STORM_GO,
                (((uint32)list[0]) * ai->pixels_per_row) >> 5);
    }
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
