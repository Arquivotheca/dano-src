//////////////////////////////////////////////////////////////////////////////
// Shared Info Structure Definition
//
//    This file defines the contents of information structures shared between
// the kernel driver and accelerant instances.
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Typedefs //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Shared Info
//    Information for one graphics device that is shared between the kernel
// driver and accelerant instances.

typedef struct
{
  // Card-related parameters.
  //    These fields should be set during initialization.
  struct
  {
    uint16 vendor_id;         // PCI vendor ID for the card.
    uint16 device_id;         // PCI device ID for the card.
    uint8 revision;           // PCI device revision number for the card.

    uint8 interrupt_line;     // Interrupt line for the vertical blanking
                              // interrupt.

    area_id regs_area;        // Register aperture area identifier.
    vuchar *regs;             // Register aperture pointer.
    uint32 reg_aperture_size; // Register aperture size (amount of memory
                              // mapped).

    area_id fb_area;          // Frame buffer aperture area identifier.
    void *fb_base;            // Frame buffer aperture base pointer (the start
                              // of card memory).
    void *fb_base_pci;        // Frame buffer aperture base pointer (physical
                              // address).
    uint32 fb_aperture_size;  // Frame buffer aperture size (amount of memory
                              // mapped).

    uint32 mem_size;          // Amount of usable frame buffer memory on the
                              // card.

    uint32 pix_clk_max8;      // Maximum pixel clock at 8 bpp, in kHz.
    uint32 pix_clk_max16;     // Maximum pixel clock at 15/16 bpp, in kHz.
    uint32 pix_clk_max32;     // Maximum pixel clock at 32 bpp, in kHz.
  } card;


  // Variables related to 2D drawing engine management.
  struct
  {
    uint64 last_idle_fifo;    // Last fifo slot we *know* the engine was idle
                              // after.
    uint64 fifo_count;        // Last fifo slot used.
    uint64 fifo_limit;        // Slot age after which command guaranteed
                              // complete.

    gds_benaphore engine_ben; // Benaphore for engine ownership.
  } engine;


  // Variables related to vertical blanking interrupt handling.
  struct
  {
    sem_id vblank;            // Semaphore released during the VBI.
    int32 flags;              // Flags specifying operations to be performed
                              // during the VBI.
  } vbi;


  // Variables related to hardware cursor management.
  struct
  {
    uint8 cursor0[512]; // Cursor image, BeOS format.
    uint8 cursor1[512];

    uint16 cursor_x;    // Cursor location (hot spot position on the screen).
    uint16 cursor_y;

    uint16 hot_x;   // Cursor hot spot, relative to the upper left corner of
                    // the cursor image.
    uint16 hot_y;
  } cursor;


  // Variables related to CRT display mode and frame buffer configuration.
  struct
  {
    display_mode dm;            // Current display mode configuration.
    frame_buffer_config fbc;    // Current bytes_per_row and start of frame
                                // buffer.

    // NOTE: The list of predefined display modes is stored in the accelerant
    // in a local information structure.

    // NOTE: This is presently not used correctly. Lock it and rig it to
    // expand to encompass new writes and all previous writes following the
    // most recent VBI.
    uint16 first_color;         // The first colour needing to be modified.
    uint16 color_count;         // The number of CLUT entries needing to be
                                // modified.
    uint8 color_data[3 * 256];  // Modified palette data.
  } display;


// Incorporate this structure only if it is non-empty.
#if HARDWARE_SPECIFIC_SI_EXISTS

  // Additional variables and parameters that are hardware-specific.
  HARDWARE_SPECIFIC_SHARED_INFO hw_specific;

#endif
} SHARED_INFO;


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
