//////////////////////////////////////////////////////////////////////////////
// Initialization Hooks
//
//    This file implements hardware-independent functions used to initialize
// the graphics card and otherwise perform accelerant initialization-related
// tasks.
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
// Initialize the Accelerant
//    Initialize all local accelerant variables by communicating with the
// kernel driver directly through the specified file descriptor.
//    HACK - We're performing hardware initialization here too. This should
// be moved to the kernel driver.

status_t Init(int the_fd)
{
  status_t result;
  gds_get_private_data gpd;

  // Memorize the file descriptor.
  ai.fd = the_fd;

  // Set the magic number so the driver knows we're for real.
  gpd.magic = GDS_IOCTL_MAGIC;

  // Contact driver and get a pointer to the shared data structure.
  result = ioctl(ai.fd, GDS_GET_PRIVATE_DATA, &gpd, sizeof(gpd));
  ddprintf(("ioctl(GDS_GET_PRIVATE_DATA) returns %d\n", result));
  // Fail if this doesn't work.
  if (result != B_OK) goto error0;
        
  // Memorize the pointer to the shared information structure.
  ai.si = gpd.si;
  // Store the SHARED_INFO pointer in another global variable to make life
  // easier elsewhere.
  si = ai.si;


  // Now, initialize the graphics card.
  // HACK - This should be done in the kernel driver, not here.


  // Hardware-specific routines are compartmentalized here.
  result = Init_Card();
  // Fail if initialization didn't work.
  if (result != B_OK) goto error0;


  // Figure out how much memory we have.
  si->card.mem_size = CountMemory(si->card.fb_base, si->card.fb_aperture_size);
  // Figure out what the maximum pixel clock is at various colour depths.
  calc_max_clocks(&(si->card.pix_clk_max8), &(si->card.pix_clk_max16),
    &(si->card.pix_clk_max32));
  
  // Initialize the engine benaphore.
  INIT_GDS_BENAPHORE(&(si->engine.engine_ben), "an engine semaphore (" DRIVER_NAME ")");
  // Initialize the count of register writes that pass though the card's FIFO.
  si->engine.last_idle_fifo = si->engine.fifo_count = 0;

  // NOTE: Cursor initialization is now assumed to be handled by the hardware-
  // -specific initialization routines.

  // Initialize locks for critical sections.


  // End of graphics card initialization.


  // Make sure that the cursor is hidden.
  // NOTE: I'm not sure if this should be here or not. If this is to be done once
  // and only once on startup, then it should be a part of the initialization
  // routines which will eventually be moved to the kernel. OTOH, that ends up
  // duplicating code. However, if it's here, then every time a new accelerant
  // instance is loaded, the cursor will be hidden. This may or may not be desired
  // behavior.

  // NOTE: The user is expected to acquire exclusive access to an engine before
  // calling drawing routines, and the cursor routines are theoretically called
  // only by the app_server, which serializes it's function calls to these
  // routines already. So, no critical section checking. Trey's call, not mine.
  ShowCursor(false);
  

  // Create a list of video modes that this device can generate.

  // For paranoia's sake, clear the old values before starting.
  ai.mode_count = 0;
  ai.predefined_mode_list_area = B_ERROR;
  ai.predefined_mode_list = NULL;
  // Generate the list.
  create_mode_list();

 error0:
  return result;
}


//////////////////////////////////////////////////////////////////////////////
// Uninitialize the Accelerant
//    Clean up any local data for this accelerant instance that needs
// cleaning up before it disappears. Shared data should be de-initialized
// by the kernel driver.

void UnInit(void)
{
  ddprintf(("GDS: UnInit() called\n"));

  // Get rid of the predefined mode table.
  delete_area(ai.predefined_mode_list_area);
}


//////////////////////////////////////////////////////////////////////////////
// Get the Size of Clone Information
//    This returns the size of the structure containing the information that
// needs to be passed when cloning an accelerant instance.

ssize_t CloneInfoSize(void)
{
  ddprintf(("GDS: CloneInfoSize() called\n"));

  return sizeof(ACCELERANT_INFO);
}


//////////////////////////////////////////////////////////////////////////////
// Get Clone Information
//    Extract all information needed from this accelerant instance to allow
// another instance to control the same graphics card that we are
// controlling.

void GetCloneInfo(ACCELERANT_INFO *data)
{
  ddprintf(("GDS: GetCloneInfo() called\n"));

  // NOTE: Among other things, we're passing a file descriptor in ai.
  // Hopefully this is still valid, because we'll need it in order to perform
  // ioctl calls.
  // NOTE: The predefined mode list will cease to be valid here, as it was
  // dynamically allocated by this instance of the accelerant and may vanish
  // when this accelerant does.
  *data = ai;
}


//////////////////////////////////////////////////////////////////////////////
// Initialize a Clone
//    Initialize the clone's ACCELERANT_INFO structure so that it is able to
// take over operations for the graphics card that its parent controlled.

status_t InitClone(ACCELERANT_INFO *data)
{
  ddprintf(("GDS: InitClone() called\n"));

  // NOTE: Among other things, we're passing a file descriptor in ai.
  // Hopefully this is still valid, because we'll need it in order to perform
  // ioctl calls.
  ai = *data;


  // Create the list of available modes all over again, as we may be in a
  // different address space, and the old mode list could certainly vanish
  // at any time even if we're in the same space.

  // For paranoia's sake, clear the old values before starting.
  ai.mode_count = 0;
  ai.predefined_mode_list_area = B_ERROR;
  ai.predefined_mode_list = NULL;
  // Regenerate the list.
  create_mode_list();


  // Initialize si here also.
  si = ai.si;

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Get Accelerant Device Information
//    This fills the specified structure with information about the card
// being managed.

status_t GetAccelerantDeviceInfo(accelerant_device_info *adi)
{
  ddprintf(("GDS: GetAccelerantDeviceInfo() called\n"));

  // The highest supported accelerant_device_info version number is stored in
  // B_ACCELERANT_VERSION. Just return version 1 information for now, though.


  // Note what version of the info we understand.
  adi->version = 1;


  // Get card name, chipset, and serial number information.
  // We expect _something_ to be set here, even if it's null strings.
  GetCardNameData(adi->name, sizeof(adi->name),
    adi->chipset, sizeof(adi->chipset),
    adi->serial_no, sizeof(adi->serial_no));

  // Terminate the strings, just in case.
  adi->name[sizeof(adi->name) - 1] = 0;
  adi->chipset[sizeof(adi->chipset) - 1] = 0;
  adi->serial_no[sizeof(adi->serial_no) - 1] = 0;


  // Get DAC speed from pixel clock maximum values. These are computed
  // by calc_max_clocks() upon initialization.
  if ((si->card.pix_clk_max8 >= si->card.pix_clk_max16)
    && (si->card.pix_clk_max8 >= si->card.pix_clk_max32))
  {
    adi->dac_speed = si->card.pix_clk_max8;
  }
  else if (si->card.pix_clk_max16 >= si->card.pix_clk_max32)
  {
    adi->dac_speed = si->card.pix_clk_max16;
  }
  else
    adi->dac_speed = si->card.pix_clk_max32;

  // Convert from kHz to MHz.
  adi->dac_speed /= 1000;


  // Get the amount of frame buffer memory.
  adi->memory = si->card.mem_size;


  // Report success.
  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Accelerant Retrace Semaphore
//    This returns the semaphore ID of the vertical retrace semaphore.

sem_id AccelerantRetraceSemaphore(void)
{
  return si->vbi.vblank;
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
