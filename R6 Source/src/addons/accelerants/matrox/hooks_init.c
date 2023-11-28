//////////////////////////////////////////////////////////////////////////////
// Initialization Hooks
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Includes //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include <Accelerant.h>
#include <string.h>
#include <sys/ioctl.h>
#include <Drivers.h>
#include <registers.h>
#include <cardid.h>
#include <private.h>
#include <errno.h>

#include "debugprint.h"
#include "mga_bios.h"
#include "accelerant_info.h"
#include "cardinit.h"
#include "hooks_cursor.h"
#include "hooks_mode.h"
#include "mga_util.h"
#include "hooks_init.h"


//////////////////////////////////////////////////////////////////////////////
// Globals ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

int fd;
int memtypefd;
int can_do_overlays;

SHARED_INFO     *si;
ACCELERANT_INFO *ai;

vuchar *regs;

area_id predefined_mode_list_area;

display_mode *predefined_mode_list;


//////////////////////////////////////////////////////////////////////////////
// Defines ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#define SET_AI_FROM_SI(si) ((ACCELERANT_INFO *)(si + 1))


//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

status_t InitMemTypes(void) {
	status_t result = B_OK;

	memtypefd = open("/dev/misc/genpool", O_RDWR);
	if (memtypefd < 0) return errno;
	/* create the pool if we're the primary accelerant */
	if (fd != -1) {
		BIoctl_CreatePool	icp;
		BPoolInfo		pi;

		memset (&pi, 0, sizeof (pi));
		pi.pi_Pool_AID	= si->fb_area;
		pi.pi_Pool	= (void *) si->framebuffer;
		pi.pi_Size	= ai->mem_size;
		strcpy (pi.pi_Name, "a matrox genpool");

		icp.icp_PoolInfo	= &pi;
		icp.icp_MaxAllocs	= ai->mem_size >> 13;
		icp.icp_UserDataSize	= 0;

		result = ioctl (memtypefd, B_IOCTL_CREATEPOOL, &icp, sizeof (icp));
		if (result >= 0) {
			ai->poolid = pi.pi_PoolID;
			result = B_OK;
		} else
			ai->poolid = -1;
	}

	can_do_overlays = 0;
	if ((memtypefd >= 0) && ((si->device_id == MGA_G200_AGP) || (si->device_id == MGA_G200_PCI)))
		can_do_overlays = 1;

	return result;
}

//////////////////////////////////////////////////////////////////////////////
// Initialize the Accelerant

status_t Init(int the_fd)
{
  status_t result;
  mga_get_private_data gpd;
  MGA_SET_BOOL_STATE mmr;

  /* memorise the file descriptor */
  fd = the_fd;

  /* set the magic number so the driver knows we're for real */
  gpd.magic = MGA_PRIVATE_DATA_MAGIC;

  /* contact driver and get a pointer to the registers and shared data */
  result = ioctl(fd, MGA_GET_PRIVATE_DATA, &gpd, sizeof(gpd));
  ddprintf(("ioctl(MGA_GET_PRIVATE_DATA) returns %d\n", result));
  if (result != B_OK) goto error0;
        
  /* transfer the info to our globals */
  si = gpd.si;
  ai = SET_AI_FROM_SI(si);
  regs = si->regs;
  ddprintf(("original regs is 0x%08x\n", regs));

  /* map the BIOS into the frame buffer so we can hunt down the PInS */
  mmr.magic = MGA_PRIVATE_DATA_MAGIC;
  mmr.do_it = TRUE;
  result = ioctl(fd, MGA_MAP_ROM, &mmr, sizeof(mmr));
  ddprintf(("ioctl(MGA_MAP_ROM, TRUE) returns %d\n", result));
  if (result != B_OK) goto error0;

  /* find the BIOS */
  result = readBIOS();
        
  /* un-map the BIOS */
  mmr.do_it = FALSE;
  result |= ioctl(fd, MGA_MAP_ROM, &mmr, sizeof(mmr));

  ddprintf(("ioctl(MGA_MAP_ROM, FALSE) returns %d\n", result));

  if(result != B_OK) goto error0;

  // Call the appropriate initialization routine.

  switch (si->device_id)
    {
    case MGA_1064S:     // Mystique.
      Init_1064();
      break;

    case MGA_2064W:     // Millennium I.
      Init_2064();
      break;

    case MGA_2164W:     // Millennium II.
    case MGA_2164W_AGP: // Millennium II AGP.
      Init_2164();
      break;

    case MGA_G100_AGP:  // G100
    case MGA_G100_PCI:
      Init_G100();
      break;

    case MGA_G200_AGP:  // G200
    case MGA_G200_PCI:
      Init_G200();
      break;
    }

  /* and frame buffer position info */
  ai->fbc.frame_buffer = ai->fbc.frame_buffer_dma = 0;
  memset(&(ai->fb_spec), 0, sizeof(ai->fb_spec));
  memset(&(ai->ovl_buffer_specs[0]), 0, sizeof(ai->ovl_buffer_specs));
  memset(&(ai->ovl_buffers[0]), 0, sizeof(ai->ovl_buffers));
  memset(&(ai->ovl_tokens[0]), 0, sizeof(ai->ovl_tokens));
  /* init the mem-types driver.  Ignore errors for now. */
  (void)InitMemTypes();

  switch (si->device_id)
    {
    case MGA_2064W:
    case MGA_2164W:
    case MGA_2164W_AGP:
		/* we use DAC memory for the cursor on these cards */
		ai->cursorData = 0;
      break;

    case MGA_1064S:
    case MGA_G100_AGP:
    case MGA_G200_AGP:
    case MGA_G100_PCI:
    case MGA_G200_PCI:
		/* we use frame buffer memory for the cursor on these cards */
		ai->cursorData = 0;
		if (memtypefd >= 0) {
			/* alloc cursor from genpool */
			BMemSpec ms;
	
			// init the memspec
			memset(&ms, 0, sizeof(ms));
			ms.ms_PoolID = ai->poolid;
			// cursor data takes 2KB
			ms.ms_MR.mr_Size = 2048;
			// aligned on 1KB boundary
			ms.ms_AddrCareBits = (1024 - 1);
			ms.ms_AddrStateBits = 0;
			if (ioctl(memtypefd, B_IOCTL_ALLOCBYMEMSPEC, &ms, sizeof(ms)) == 0)
				ai->cursorData = (uchar *)si->framebuffer + ms.ms_MR.mr_Offset;
		}
		/* still no allocation? */
		if (!ai->cursorData)
			/* take the last 2KB of video memory */
			ai->cursorData = (uchar *)si->framebuffer + (ai->mem_size - 2048);
		break;
    }

  /* init fifo slot counting */
  switch (si->device_id)
    {
    case MGA_1064S:
    case MGA_2064W:
      /* 32 fifo slots */
      ai->fifo_limit = 32;
      break;

    case MGA_2164W:
    case MGA_2164W_AGP:
    case MGA_G100_AGP:
    case MGA_G200_AGP:
    case MGA_G100_PCI:
    case MGA_G200_PCI:
      /* 64 fifo slots */
      ai->fifo_limit = 64;
      break;
    }

  /* create mask from fifo_limit */
  ai->fifo_mask = (ai->fifo_limit << 1) - 1;
  
  /* how fast can we go? */
  calc_max_clocks();
  /* create a list of video modes that this device can generate */
  create_mode_list();
  
  /* init the shared semaphore */
  ai->engine_sem = create_sem(0, "a matrox engine sem");
  ai->engine_ben = 0;
  /* count of issued parameters or commands */
  ai->last_idle_fifo = ai->fifo_count = 0;
  /* init last color and command to bogus values
     so they'll be set the first time they're used */
  ai->last_fg_color = 0x12345678;
  ai->last_command = 0;

  /* notify cursor routines to set the color */
  ai->set_cursor_colors = 1;
  /* ensure cursor state */
  ShowCursor(false);
 error0:
  return result;
}


//////////////////////////////////////////////////////////////////////////////
// Uninitialize the Accelerant
//    This should probably do something...

void UnInit(void)
{
  ddprintf(("uninit()\n"));
  if (memtypefd >= 0) {
	if (fd >= 0) {
		// whack the pool
		BPoolInfo	pi;
	
		memset (&pi, 0, sizeof (pi));
		pi.pi_PoolID = ai->poolid;
		ioctl (memtypefd, B_IOCTL_DELETEPOOL, &pi, sizeof (pi));

		// nuke the pool id
		ai->poolid = -1;
	}
	// close the driver
	close(memtypefd);
	memtypefd = -1;
  }
}


//////////////////////////////////////////////////////////////////////////////
// Get the Size of Clone Information
//    This is about what you'd expect; it returns the sizeof() the clone
// data structure.  If it wasn't for the "all hooks must be function
// pointers" restriction, we could return that number in place of the
// hook, though I suppose this is more flexible.

ssize_t CloneInfoSize(void)
{
  return sizeof(SHARED_INFO *);
}


//////////////////////////////////////////////////////////////////////////////
// Get Clone Information
//    Point the pointer we are handed at the data structure of the currently
// active clone.  Not too tough, really.  This may run into fun later,
// though, depending on how memory sharing is handled.

void GetCloneInfo(SHARED_INFO **data)
{
  *data = si;
}


//////////////////////////////////////////////////////////////////////////////
// Initialize a Clone
//    There's a little magic going on below.

status_t InitClone(SHARED_INFO **data)
{
  fd = -1;
  si = *data;
  ai = SET_AI_FROM_SI(si);
  regs = si->regs;

  /* Init the mem-types driver.  Ignore errors for now. */
  (void)InitMemTypes();

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Get Accelerant Device Information

status_t GetAccelerantDeviceInfo(accelerant_device_info *adi)
{
  // [Chris:] I can't find any references in the documentation about
  // requiring the version number to be specified when calling. So, the
  // version number will be set to B_ACCELERANT_VERSION at the very
  // beginning of this function, and the if() blocks will now make sense.

  // note what version of the info we understand
  adi->version = B_ACCELERANT_VERSION;

  // return the version 1 info if applicable.
  if(adi->version >= 1)
    {
      char *scratch1;
      char *scratch2;

      // Get the device name and chipset name.
      switch(si->device_id)
        {
        case MGA_2064W:
          scratch1 = "Matrox Millennium (original)";
          scratch2 = "MGA 2064";
          break;

        case MGA_2164W:
          scratch1 = "Matrox Millennium-II PCI";
          scratch2 = "MGA 2164 PCI";
          break;

        case MGA_2164W_AGP:
          scratch1 = "Matrox Millennium-II AGP";
          scratch2 = "MGA 2164 AGP";
          break;

        case MGA_1064S:
          if(si->revision <= 2) scratch1 = "Matrox Mystique";
          else                  scratch1 = "Matrox Mystique 220";
          scratch2 = "MGA 1064";
          break;

        case MGA_G100_AGP:
          scratch1 = "Matrox Productiva G100 AGP";
          scratch2 = "MGA G100 AGP";
          break;

        case MGA_G200_AGP:
          scratch1 = "Matrox G200 AGP based card";
          scratch2 = "MGA G200 AGP";
          break;

        case MGA_G100_PCI:
          scratch1 = "Matrox Productiva G100 PCI";
          scratch2 = "MGA G100 PCI";
          break;

        case MGA_G200_PCI:
          scratch1 = "Matrox G200 PCI based card";
          scratch2 = "MGA G200 PCI";
          break;

        default:
          scratch1 = "Some other card";
          scratch2 = "UNKOWN";
          break;
        }

      strncpy(adi->name, scratch1, sizeof(adi->name));
      adi->name[sizeof(adi->name)-1] = '\0';

      strncpy(adi->chipset, scratch2, sizeof(adi->chipset));
      adi->chipset[sizeof(adi->chipset)-1] = '\0';

      // Get the device serial number.
      switch(si->device_id)
        {
        case MGA_2064W:
          scratch1 = (char *)ai->bios.orig.SerNo;
          break;

        case MGA_2164W:
        case MGA_2164W_AGP:
        case MGA_1064S:
          scratch1 = (char *)ai->bios.pins.SerNo;
          break;

        case MGA_G100_AGP:
        case MGA_G200_AGP:
        case MGA_G100_PCI:
        case MGA_G200_PCI:
          scratch1 = (char *)ai->bios.pins31.SerNo; break;
          break;

        default:
          scratch1 = "UNKOWN"; break;
          break;
        }

      strncpy(adi->serial_no, scratch1, sizeof(adi->serial_no));
      adi->serial_no[sizeof(adi->serial_no)-1] = '\0';

      // Get DAC speed from pixel clock maximum values. These are computed
      // by calc_max_clocks() upon initialization.
      if ((ai->pix_clk_max8 >= ai->pix_clk_max16) && (ai->pix_clk_max8 >= ai->pix_clk_max32))
        adi->dac_speed = ai->pix_clk_max8;
      else if (ai->pix_clk_max16 >= ai->pix_clk_max32)
        adi->dac_speed = ai->pix_clk_max16;
      else
        adi->dac_speed = ai->pix_clk_max32;
      // Convert from kHz to MHz.
      adi->dac_speed /= 1000;

      // Get the amount of frame buffer memory.
      adi->memory = ai->mem_size;
    }

  // return the version 2 info if applicable.
  // none, yet

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Accelerant Retrace Semaphore

sem_id AccelerantRetraceSemaphore(void)
{
  return si->vblank;
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
