//////////////////////////////////////////////////////////////////////////////
// Hardware-Specific Accelerant Functions
//    This file contains prototypes for all hardware-specific functions that
// the generic driver skeleton requires that the developer implement for the
// accelerant.
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Function Prototypes ///////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Initialization Functions


// Initialize Graphics Card
//    This function should perform all necessary hardware initialization for
// the card controlled by the current instance of the accelerant.
status_t Init_Card(void);

// Get Card Name, Chipset, and Serial Number.
//    This function returns the name, chipset name, and serial number for the
// card controlled by the current instance of the accelerant. These must
// include a terminating null character and must be no longer than the length
// specified (including the terminating null character).
void GetCardNameData(char *name, int name_size, char *chipset,
  int chipset_size, char *serial_number, int serial_size);


//////////////////////////////////////////////////////////////////////////////
// Mode Set Functions


// Set the Display Mode
//    ProposeDisplayMode will always be called before this function. Put as
// much of the error checking in there as possible, as opposed to here.
//    This is expected to update *final_dm and *final_fbc to reflect the
// display mode and frame buffer configuration after the mode set.
//    final_fbc.frame_buffer_dma will be automatically calculated, so this
// function doesn't have to set it.
void do_set_display_mode(display_mode *dm, display_mode *final_dm,
  frame_buffer_config *final_fbc);

// Calculate Maximum Clocks
//    This calculates the maximum pixel clock rates at 8, 16, and 32 bpp
// colour depths (in kHz).
void calc_max_clocks(uint32 *max_pclk_8bpp, uint32 *max_pclk_16bpp,
  uint32 *max_pclk_32bpp);

// Propose a Video Mode
//    Adjust target video mode to be vaild for this device within the ranges
// specified by low and high.  Return B_ERROR if we can't accomodate.
status_t ProposeVideoMode(display_mode *target,
                          const display_mode *low,
                          const display_mode *high);

// Move the Display Area
status_t MoveDisplayArea(uint16 h_display_start,
                               uint16 v_display_start);


//////////////////////////////////////////////////////////////////////////////
// Cursor Functions

// Actually set the cursor position registers.
//    This function updates the actual cursor position registers. The upper
// left corner of the cursor image should be moved to the position specified.
//    This is called by MoveCursor and SetCursorShape, so pay careful
// attention to locking to avoid deadlock.
void UpdateCursorPosition(int32 image_x, int32 image_y);


// Show/Hide the Cursor
//    This function either displays or hides the cursor, as indicated by the
// is_visible flag.
void ShowCursor(bool is_visible);


// Actually set the cursor shape
//    This function should set the cursor shape given the masks provided.
// cursor0 corresponds to the xor mask and cursor1 corresponds to the and
// mask, following BeOS conventions. Each array is 512 bytes long, so that a
// 64x64 cursor is defined. This corresponds to what most graphics hardware
// can actually handle, though BeOS presently supports only 16x16 cursors.
void do_set_cursor_image(uint8 *cursor0, uint8 *cursor1);


//////////////////////////////////////////////////////////////////////////////
// 2D Acceleration Functions


// This is called directly as the appropriate hook function. Make sure that
// it updates si->engine.fifo_count appropriately.
void ScreenToScreenBlit(engine_token *et,
                        blit_params *list,
                        uint32 count);

// This is called directly as the appropriate hook function. Make sure that
// it updates si->engine.fifo_countfifo_count appropriately.
void RectangleFill(engine_token *et,
                   uint32 color,
                   fill_rect_params *list,
                   uint32 count);

// This is called directly as the appropriate hook function. Make sure that
// it updates si->engine.fifo_countfifo_count appropriately.
void RectangleInvert(engine_token *et,
                     fill_rect_params *list,
                     uint32 count);

// This is called directly as the appropriate hook function. Make sure that
// it updates si->engine.fifo_countfifo_count appropriately.
void SpanFill(engine_token *et,
              uint32 color,
              uint16 *list,
              uint32 count);


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
