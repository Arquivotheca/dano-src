//////////////////////////////////////////////////////////////////////////////
// Set Video Modes - Generic Functions
//    This file contains the subset of routines dealing with mode setting
// and mode manipulation that aren't hardware-specific.
//
// Device Dependance: None.
//
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
// Set Indexed Colors
//    Sets the palette table at 8 bpp. Does nothing at other colour depths.

void SetIndexedColors(uint count,
                      uint8 first,
                      uint8 *color_data,
                      uint32 flags)
{
  // We need a critical section here, as we're messing with si.
  // HACK - We don't have one, yet. Put in proper si locking when si
  // has been cleaned up. In the meantime, if the user tries redefining
  // the palette twice at the same time, the user will get a garbled
  // palette.

  if(si->display.dm.space == B_CMAP8)
    {
      // Copy color data into kernel shared area.
      memcpy(si->display.color_data + ((uint16)first * 3), color_data, count * 3);

      // HACK - If we perform multiple palette writes before the VBI,
      // only the last write will take!
      si->display.color_count = count;
      si->display.first_color = first;

      // Notify the kernel driver to program it for us.
      atomic_or((int32 *)&(si->vbi.flags), GDS_PROGRAM_CLUT);
    }
}


//////////////////////////////////////////////////////////////////////////////
// Accelerant Mode Count
//    Return the number of 'built-in' display modes.

uint32 AccelerantModeCount(void)
{
  return ai.mode_count;
}


//////////////////////////////////////////////////////////////////////////////
// Get a Mode List
//    Copy the list of predefined modes to the buffer pointed at by *dm.

status_t GetModeList(display_mode *dm)
{
  memcpy(dm, ai.predefined_mode_list, ai.mode_count * sizeof(display_mode));

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Create the Mode List
//    Create a list of predefined modes. Called once after initialization.

void create_mode_list(void)
{
  size_t max_size;
  int res_count, refresh_count, cspace_count;
  uint32
    pix_clk_range;
  display_mode
    standard_mode,
    *dst,
    low,
    high;
  int diag_mode_count, vesa_mode_count, algorithmic_mode_count;
  int index;

  // Allocate space for the predefined mode list.

  // Count the number of diagnostics modes that we'll be adding.
  // The list is terminated by an entry filled with zero values. Test the pixel
  // clock, as this should never be zero.
  for
    (
      diag_mode_count = 0;
      diagnostic_display_modes[diag_mode_count].timing.pixel_clock;
      diag_mode_count++
    );

  // Count the number of VESA modes that we'll be adding.
  // The list is terminated by an entry filled with zero values. Test the pixel
  // clock, as this should never be zero.
  for
    (
      vesa_mode_count = 0;
      vesa_standard_modes[vesa_mode_count].timing.pixel_clock;
      vesa_mode_count++
    );
  // Multiply this by the number of colour spaces that each mode may use.
  vesa_mode_count *= STANDARD_COLOUR_SPACE_COUNT;

  // Calculate the number of algorithmic modes that we'd be adding in the worst case
  // (all possible modes are supported by the card).
  algorithmic_mode_count = STANDARD_RESOLUTION_COUNT * STANDARD_REFRESH_RATE_COUNT
    * STANDARD_COLOUR_SPACE_COUNT;
    
  // Calculate the total number of display_mode entries that we need.
  max_size = 0;

  if (USE_DIAGNOSTICS_MODES)
    max_size += diag_mode_count;

  if (USE_VESA_MODES)
    max_size += vesa_mode_count;
  else
    max_size += algorithmic_mode_count;

  max_size++; // Don't forget the end-of-list marker.

  // Calculate the amount of memory needed. Adjust up to nearest multiple of B_PAGE_SIZE.
  max_size = ((max_size * sizeof(display_mode)) + (B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);

  // Create an area to hold the info.
  ai.predefined_mode_list_area = create_area("predefined mode info", (void **)&(ai.predefined_mode_list), B_ANY_ADDRESS, max_size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
  if (ai.predefined_mode_list_area < B_OK) return;


  // Initialize counters and pointers.
  dst = ai.predefined_mode_list;
  ai.mode_count = 0;


  // If diagnostics modes are to be used, add all supported diagnostics modes to
  // the list.

  if (USE_DIAGNOSTICS_MODES)
  {
    // Walk through the diagnostics mode list, adding supported modes to the list.
    for (index = 0; diagnostic_display_modes[index].timing.pixel_clock; index++)
    {
      // Store the mode parameters.
      standard_mode = diagnostic_display_modes[index];

      // See if this mode is supported.

      // Initialize low and high range boundaries.
      low = high = standard_mode;

      // Allow all possible flags, subject to ProposeVideoMode's restrictions.
      low.flags = 0;
      high.flags = ~low.flags; // Make sure that we don't have to worry about integer types.

      // Set low and high range boundaries for the pixel clock.
      // Range is arbitrarily picked as 6.25% of the desired pixel clock.
      pix_clk_range = low.timing.pixel_clock >> 5;
      low.timing.pixel_clock -= pix_clk_range;
      high.timing.pixel_clock += pix_clk_range;

      // Bump up the maximum virtual width so that any wierd frame buffer pitch
      // restrictions can be satisfied.
      high.virtual_width = 1920;

      // Put this mode in the list and test it.
      *dst = standard_mode;

      // Use ProposeVideoMode to see if the mode is supported.
      if(ProposeVideoMode(dst, &low, &high) != B_ERROR)
      {
        // Diagnostics.
//        ddprintf(("%dx%d 0x%04x\n", dst->timing.h_display, dst->timing.v_display, dst->space));

        // The mode is supported; increment pointers appropriately.
        dst++;
        ai.mode_count++;
      }
    }
  }


  // Add normal modes to the list. Either read these from the table of VESA
  // standard modes or construct them from the standard mode templates, as
  // indicated.

  if (USE_VESA_MODES)
  {
    // Walk through all VESA modes, adding supported modes to the list.
    for (index = 0; vesa_standard_modes[index].timing.pixel_clock; index++)
    {
      // Store the mode parameters.
      standard_mode = vesa_standard_modes[index];

      // Step through the standard colour spaces, testing the mode with each.
      for (cspace_count = 0; cspace_count < STANDARD_COLOUR_SPACE_COUNT; cspace_count++)
      {
        // Specify the colour space.
        standard_mode.space = standard_colour_spaces[cspace_count];


        // See if this mode is supported.

        // Initialize low and high range boundaries.
        low = high = standard_mode;

        // Allow all possible flags, subject to ProposeVideoMode's restrictions.
        low.flags = 0;
        high.flags = ~low.flags; // Make sure that we don't have to worry about integer types.

        // Set low and high range boundaries for the pixel clock.
        // Range is arbitrarily picked as 6.25% of the desired pixel clock.
        pix_clk_range = low.timing.pixel_clock >> 5;
        low.timing.pixel_clock -= pix_clk_range;
        high.timing.pixel_clock += pix_clk_range;

        // Bump up the maximum virtual width so that any wierd frame buffer pitch
        // restrictions can be satisfied.
        high.virtual_width = 1920;


        // Put this mode in the list and test it.
        *dst = standard_mode;

        // Use ProposeVideoMode to see if the mode is supported.
        if(ProposeVideoMode(dst, &low, &high) != B_ERROR)
        {
          // Diagnostics.
//          ddprintf(("%dx%d 0x%04x\n", dst->timing.h_display, dst->timing.v_display, dst->space));

          // The mode is supported; increment pointers appropriately.
          dst++;
          ai.mode_count++;
        }
      }
    }
  }
  else // Construct the standard modes from the standard mode templates.
  {
    // Walk through all standard modes, adding supported modes to the list.
    for (res_count = 0; res_count < STANDARD_RESOLUTION_COUNT; res_count++)
    {
      // Construct the mode.
      // Store most of the mode parameters.
      standard_mode = standard_display_modes[res_count];

      for (refresh_count = 0; refresh_count < STANDARD_REFRESH_RATE_COUNT; refresh_count++)
      {
        // Continue constructing the mode.
        // Calculate the pixel clock from the refresh rate and the total pixel count.
        // Remember, this is in kHz.
        standard_mode.timing.pixel_clock =
          ( standard_refresh_rates[refresh_count]
          * ((uint32) standard_mode.timing.h_total)
          * ((uint32) standard_mode.timing.v_total) )
          / 1000;

        for (cspace_count = 0; cspace_count < STANDARD_COLOUR_SPACE_COUNT; cspace_count++)
        {
          // Continue constructing the mode.
          // Specify the colour space.
          standard_mode.space = standard_colour_spaces[cspace_count];


          // See if this mode is supported.

          // Initialize low and high range boundaries.
          low = high = standard_mode;

          // Allow all possible flags, subject to ProposeVideoMode's restrictions.
          low.flags = 0;
          high.flags = ~low.flags; // Make sure that we don't have to worry about integer types.

          // Set low and high range boundaries for the pixel clock.
          // Range is arbitrarily picked as 6.25% of the desired pixel clock.
          pix_clk_range = low.timing.pixel_clock >> 5;
          low.timing.pixel_clock -= pix_clk_range;
          high.timing.pixel_clock += pix_clk_range;

          // Bump up the maximum virtual width so that any wierd frame buffer pitch
          // restrictions can be satisfied.
          high.virtual_width = 1920;


          // Put this mode in the list and test it.
          *dst = standard_mode;

          // Use ProposeVideoMode to see if the mode is supported.
          if(ProposeVideoMode(dst, &low, &high) != B_ERROR)
          {
            // Diagnostics.
//            ddprintf(("%dx%d 0x%04x\n", dst->timing.h_display, dst->timing.v_display, dst->space));

            // The mode is supported; increment pointers appropriately.
            dst++;
            ai.mode_count++;
          }
        }
      }
    }
  }


  // Mark the end of the supported mode list.
  dst->timing.pixel_clock = 0;
}


//////////////////////////////////////////////////////////////////////////////
// Set the Video Mode

status_t SetVideoMode(display_mode *dm)
{
  display_mode low = *dm;
  display_mode high = *dm;
  display_mode target = *dm;

  // Allow all possible flags, subject to ProposeVideoMode's restrictions.
  low.flags = 0;
  high.flags = ~low.flags; // Make sure that we don't have to worry about integer types.

  if(ProposeVideoMode(&target, &low, &high) != B_OK)
    return B_ERROR;

  // Acquire the engine benaphore. This is Trey's solution to solving the locking
  // problem for mode sets.
  // HACK - If the user doesn't realize that mode sets require the benaphore, they
  // could deadlock themselves quite nicely here.
  ACQUIRE_GDS_BENAPHORE(&(si->engine.engine_ben));

  // Set the display mode.
  do_set_display_mode_crt(&target, &(si->display.dm), &(si->display.fbc));
  do_set_display_mode_fp(&target, &(si->display.dm), &(si->display.fbc));

  // Recalculate the frame buffer DMA pointer as a sanity check.
  si->display.fbc.frame_buffer_dma
    = (void *)((uint8*)si->card.fb_base_pci
      + ((uint8*)si->display.fbc.frame_buffer - (uint8*)si->card.fb_base));

  // Release the engine benaphore.
  RELEASE_GDS_BENAPHORE(&(si->engine.engine_ben));

  // hide the hardware cursor if it's not being used
  if (!(target.flags & B_HARDWARE_CURSOR)) ShowCursor(false);

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Get Video Mode

status_t GetVideoMode(display_mode *dm)
{
  // we should probably check that a mode has been set

  *dm = si->display.dm;  // report the mode

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Get Framebuffer Configuration

status_t GetFramebufferConfig(frame_buffer_config *fbc)
{
  // Copy over the current frame buffer configuration as recorded in si.
  *fbc = si->display.fbc;

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Get the Pixel Clock Limits

status_t GetPixelClockLimits(display_mode *dm,
                             uint32 *low,
                             uint32 *high)
{
  double tmpdbl;
  uint32 limit_clock;

  if(!dm) return B_ERROR;

  /* min-out at 45Hz */
  tmpdbl
    = (double)dm->timing.h_total * (double)dm->timing.v_total * (double)45.0;

  *low = (uint32) (tmpdbl / 1000.0);

  /* max-out at 150Hz */
  tmpdbl
    = (double)dm->timing.h_total * (double)dm->timing.v_total * (double)150.0;

  *high = (uint32) (tmpdbl / 1000.0);

  /* adjust limits based on mode and card type */

  switch(dm->space & 0x0fff)
    {
    case B_CMAP8:
      limit_clock = si->card.pix_clk_max8;
      break;

    case B_RGB15:
    case B_RGB16:
      limit_clock = si->card.pix_clk_max16;
      break;

    case B_RGB32:
      limit_clock = si->card.pix_clk_max32;
      break;

    default:
      return B_ERROR;
    }

  if(*high > limit_clock) *high = limit_clock;
  if(*low > *high) *low = *high;

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
