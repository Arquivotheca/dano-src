//////////////////////////////////////////////////////////////////////////////
// Set Video Modes
//    This file sets up the card in various modes, and does things in those
// modes.  It also does some maintainance things (setting clip rectangles,
// setting CLUTs...).
//
// Device Dependance: Lots.
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Includes //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include <sys/ioctl.h>
#include <Accelerant.h>
#include <Drivers.h>
#include <registers.h>
#include <cardid.h>
#include <private.h>

#include "defines.h"
#include "debugprint.h"
#include "standard_modes.h"
#include "mga_bios.h"
#include "accelerant_info.h"
#include "globals.h"
#include "mga_gx00.h"
#include "mga_millennium.h"
#include "mga_mystique.h"
#include "mga_util.h"
#include "hooks_mode.h"
#include "hooks_cursor.h"
#include "hooks_overlay.h"
#include "hooks_dpms.h"

//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Set the Clip Rectangle
//   Called by the mode set routines after frame buffer size changes.

void SetClipRect(uint32 cxl,
                 uint32 cyt,
                 uint32 cxr,
                 uint32 cyb,
                 uint32 v_width,
                 uint32 ydstorg) 
{
  STORM32W(STORM_CXBNDRY, ((cxr & 0x07ff)<<16) | (cxl & 0x07ff));
  STORM32W(STORM_YBOT, ydstorg + (cyb * v_width));
  STORM32W(STORM_YTOP, ydstorg + (cyt * v_width));

  ai->fifo_count += 3;  // update fifo count
}


//////////////////////////////////////////////////////////////////////////////
// Set Indexed Colors

void SetIndexedColors(uint count,
                      uint8 first,
                      uint8 *color_data,
                      uint32 flags)
{
  if(ai->dm.space == B_CMAP8)
    {
      // Copy color data into kernel shared area.
      memcpy(si->color_data + ((uint16)first * 3), color_data, count * 3);
      si->color_count = count;
      si->first_color = first;

      // Notify the kernel driver to program it for us.
      atomic_or((int32 *)&(si->flags), MKD_PROGRAM_CLUT);
    }
}


//////////////////////////////////////////////////////////////////////////////
// Program the CRTC

void programCRTC(uint32 flags, uchar *initCrtc);

void programCRTC(uint32 flags,
                 uchar *initCrtc)
{
  uchar scratch;

  ddprintf(("Programming CRTC registers.\n"));
        
  // Select access on 0x3d4 and 0x3d5, program clock select for pll.
  STORM8R(VGA_MISC_R, scratch);
  scratch &= 0x32;

  if(!(flags & B_POSITIVE_HSYNC)) scratch |= 0x40;
  if(!(flags & B_POSITIVE_VSYNC)) scratch |= 0x80;

  ddprintf(("  writing %lx to VGA_MISC.\n", (uint32) scratch | 0x09));
  STORM8W(VGA_MISC_W, (scratch | (uchar)0x09));
        
  // Unprotect CRTC registers 0-7.
  STORM8W(VGA_CRTC_INDEX, VGA_CRTC11);
  STORM8W(VGA_CRTC_DATA, 0x60);
        
  // Program the CRTC registers.
  for(scratch = 0; scratch <= 24; scratch++)
    {
      ddprintf(("  writing %lx to CRTC%ld.\n", (uint32) initCrtc[scratch], (uint32) scratch));
      STORM8W(VGA_CRTC_INDEX, scratch);
      STORM8W(VGA_CRTC_DATA, initCrtc[scratch]);
    }

  // Program the CRTCEXT registers.
  for(scratch = 25; scratch <= 30; scratch++)
    {
      ddprintf(("  writing %lx to CRTCEXT%ld.\n", (uint32) initCrtc[scratch], (uint32) scratch - 25));
      STORM8W(VGA_CRTCEXT_INDEX, (scratch - 25));
      STORM8W(VGA_CRTCEXT_DATA,  initCrtc[scratch]);
    }

  // Fix the G100.
  if((si->device_id == MGA_G100_AGP) ||
     (si->device_id == MGA_G100_PCI))
    {
      ddprintf(("  writing %lx to CRTCEXT6.\n", (uint32) ((ai->bios.pins31.MemParam & 0x38) >> 3) ));
      STORM8W(VGA_CRTCEXT_INDEX, 6);
      STORM8W(VGA_CRTCEXT_DATA, (ai->bios.pins31.MemParam & 0x38) >> 3);
    }
}


//////////////////////////////////////////////////////////////////////////////
// Calculate Bits Per Pixel

static uint32 calcBitsPerPixel(uint32 cs)
{
  switch(cs)
    {
    case B_RGB32_BIG:
    case B_RGBA32_BIG:
    case B_RGB32_LITTLE:
    case B_RGBA32_LITTLE:
      return 32;
      break;

    case B_RGB24_BIG:
    case B_RGB24_LITTLE:
      return 24;
      break;

    case B_RGB16_BIG:
    case B_RGB15_BIG:
    case B_RGBA15_BIG:
    case B_RGB16_LITTLE:
    case B_RGB15_LITTLE:
    case B_RGBA15_LITTLE:
      return 16;
      break;

    case B_CMAP8:
      return 8;
      break;
    }

  return 0;
}


//////////////////////////////////////////////////////////////////////////////
// Calculate CRTC Values

void calcCrtcRegs(display_mode *dm, uint32 start_addr, uchar *crtcTab, uint32 pixels_per_row);

void calcCrtcRegs(display_mode *dm,
                  uint32 start_addr,
                  uchar *crtcTab,
                  uint32 pixels_per_row)
{
  ulong h_front_p, h_sync, h_back_p;
  ulong h_vis, v_vis, v_front_p, v_sync, v_back_p;
  ulong v_blank;
  uchar v_blank_e, hvidmid;
  ulong hTotal, hBlankStart, calcHorizBlankEnd, hSyncStart, vTotal;
  ulong vSyncStart, vDisplayEnable, offset, vBlankStart;
  int is_interlaced;
  int vclkhtotal, vclkhvis, vclkhsync, vclkhbackp;
  int vclkhblank, vclkhfrontp;
  int temp;
  uint32 bits_per_pixel = calcBitsPerPixel(dm->space);
  int interleave = (ai->mem_size > VIDRAM2MB) | (si->device_id == MGA_1064S);

  h_front_p  = (ulong)dm->timing.h_sync_start - dm->timing.h_display;
  h_sync     = (ulong)dm->timing.h_sync_end - dm->timing.h_sync_start;
  h_back_p   = (ulong)dm->timing.h_total - dm->timing.h_sync_end;
  h_vis      = (ulong)dm->timing.h_display;

  v_front_p  = (ulong)dm->timing.v_sync_start - dm->timing.v_display;
  v_sync     = (ulong)dm->timing.v_sync_end - dm->timing.v_sync_start;
  v_back_p   = (ulong)dm->timing.v_total - dm->timing.v_sync_end;
  v_vis      = (ulong)dm->timing.v_display;

  is_interlaced      = (ulong)dm->timing.flags & B_TIMING_INTERLACED;
  ddprintf(("Interlaced?:  %s\n", (is_interlaced ? "YES" : "NO")));

  /*---------------- calculation of reg htotal --------------------*/
  temp = h_vis + h_front_p + h_sync + h_back_p;
  vclkhtotal = temp >> 3;
  hTotal = vclkhtotal - 5;
  ddprintf(("hTotal: %d 0x%02x\n", hTotal, hTotal));

  crtcTab[0] = (uchar)(hTotal & 0xff);

  /*---- calculation of the horizontal display enable end reg ----*/
  vclkhvis = h_vis >> 3;
  crtcTab[1] = (uchar)vclkhvis-1;


  /*----- calculation of the horizontal blanking start register ----*/
  hBlankStart = vclkhvis - 1;
  ddprintf(("hBlankStart: %d 0x%02x\n", hBlankStart, hBlankStart));
  crtcTab[2] = (uchar)(hBlankStart & 0xff);

  /*----- calculation of horizontal blanking end register -------*/
  vclkhfrontp = h_front_p >> 3;
  vclkhsync   = h_sync >> 3;
  vclkhbackp  = h_back_p >> 3;
  vclkhblank  = vclkhfrontp + vclkhbackp + vclkhsync;
  calcHorizBlankEnd = (vclkhvis + vclkhblank) - 1;
  ddprintf(("calcHorizBlankEnd: %d 0x%02x\n", calcHorizBlankEnd, calcHorizBlankEnd));
  calcHorizBlankEnd &= 0x7f;
  crtcTab[3] = (uchar)(calcHorizBlankEnd & 0x1f);

  /*----- calculation of the horizontal retrace start register -----*/
  hSyncStart = (vclkhvis + vclkhfrontp) - 1;
  ddprintf(("hSyncStart: %d 0x%02x\n", hSyncStart, hSyncStart));
  crtcTab[4]     = (uchar)(hSyncStart & 0xff);

  /*----- calculation of the horizontal retrace end register -------*/
  temp = ((vclkhvis + vclkhfrontp + vclkhsync) - 1);
  ddprintf(("retrace end: %d 0x%02x\n", temp, temp));
  temp &= 0x1f;
  temp |= (calcHorizBlankEnd & 0x20) << 2;
  crtcTab[5] = (uchar)(temp & 0xff);

  /*----- calculation of the vertical total register -------------*/
  vTotal = (v_vis + v_front_p + v_sync + v_back_p) - 2;
  crtcTab[6] = (uchar)(vTotal & 0xff);

  /*------ preset row scan register ------------------------------*/
  crtcTab[8]=0;

  /*------ cursor row start --------------*/
  crtcTab[10]=0x10;  /* cursor off */

  /*------ cursor row end ----------------*/
  crtcTab[11]=0;
   
  /*------ start address low ------------*/
  /*------ start address high -----------*/

  crtcTab[12] = (uchar)((start_addr & 0xff00) >> 8);
  crtcTab[13] = (uchar)(start_addr & 0xff);

  /*------ cursor position high --------------------------*/
  /*------ cursor position low  --------------------------*/
  crtcTab[14]=0;
  crtcTab[15]=0;

  /*------ calculation of the vertical retrace start register -----*/
  vSyncStart = (v_vis + v_front_p) - 1;
  crtcTab[16] = (uchar)(vSyncStart & 0xff);

  /*------ calculation of the vertical retrace end register ------*/
  temp = (v_vis + v_front_p + v_sync - 1) & 0x0f;
  temp |= 0x20;
  crtcTab[17] = (uchar)temp;

  /*----- calculation of the vertical display enable register ---*/
  vDisplayEnable = v_vis - 1;
  crtcTab[18]=(uchar)(vDisplayEnable & 0xff);

  /*----- calculation of the offset register ------------------*/
  ddprintf(("dm->virtual_width: %d\n", dm->virtual_width));
  ddprintf(("pixels_per_row: %d\n", pixels_per_row));
  /* offset = (uint32)dm->virtual_width / (64 / bits_per_pixel); */
  offset = (pixels_per_row * bits_per_pixel) / 64;
  ddprintf(("offset 1: %d\n", offset));
  /*** In interlace offset must be multiplied by 2 ***/
  if (is_interlaced) offset <<= 1;
  ddprintf(("offset 2: %d\n", offset));
  if (interleave) offset >>= 1;
  ddprintf(("offset 3: %d\n", offset));
  crtcTab[19] = (uchar)(offset & 0xff);
  ddprintf(("offset 4: %d\n", offset));

  /*----- under line location register ---------*/
  crtcTab[20]=0;

  /*-----------calculation of the vertical blanking start register ----------*/
  /*FFB OK */
  vBlankStart = v_vis - 1;
  crtcTab[21] = (uchar)(vBlankStart & 0xff);

  /*-----------calculation of the vertical blanking end register ------------*/
  /*FFB OK */
  v_blank = v_back_p + v_sync + v_front_p;
  v_blank_e= (uchar)(v_vis + v_blank) - 1;
  crtcTab[22] = v_blank_e;

  /*---- mode control register ---------*/
  /*FFB OK */
  crtcTab[23] = 0xc3; /*0xc3 */

  /*---- line compare register ---------*/
  crtcTab[24] = 0xff; /* linecomp (the 0x0ff from 0x3ff) */
  crtcTab[24] = (uchar)(vBlankStart & 0xff);
  /*---- calculation of the maximum scan line register --------*/
  /*FFB OK */
  temp = (vBlankStart & 0x200) >> 4;
  temp |= (vBlankStart & 0x200) >> 3;
  crtcTab[9] = (uchar)(temp & 0xff);
        
  /*---- overflow register -------------------*/
  /*FFB OK */
  temp  = (uchar)((vTotal & 0x100) >> 8);
  temp |= (uchar)((vDisplayEnable & 0x100) >> 7);
  temp |= (uchar)((vSyncStart & 0x100) >> 6);
  temp |= (uchar)((vBlankStart & 0x100) >> 5);
  temp |= (uchar)((vBlankStart & 0x100) >> 4);
  temp |= (uchar)((vTotal & 0x200) >> 4);
  temp |= (uchar)((vDisplayEnable & 0x200) >> 3);
  temp |= (uchar)((vSyncStart & 0x200) >> 2);
  crtcTab[7] = (uchar)(temp & 0xff);

  /*----------- CRTCEXT0 register -----------------------------------------*/
  temp = 0;
  temp |= (start_addr & 0x0f0000) >> 16;
  temp |= (offset & 0x300) >> 4;
  if (is_interlaced) temp |= 0x80;
  crtcTab[25] = (uchar)(temp & 0xff);

  /*----------- CRTCEXT1 register -----------------------------------------*/
  temp = 0;
  temp |= (hTotal & 0x100) >> 8;
  temp |= (hBlankStart & 0x100) >> 7;
  temp |= (hSyncStart & 0x100) >>  6;
  temp |= (calcHorizBlankEnd & 0x40);
  crtcTab[26] = (uchar)(temp & 0xff);

  /*----------- CRTCEXT2 register -----------------------------------------*/
  temp = (vBlankStart & 0x400) >> 3;
  temp |= (vTotal & 0xc00) >> 10;
  temp |= (vDisplayEnable & 0x400) >> 8;
  temp |= (vBlankStart & 0xc00) >> 7;
  temp |= (vSyncStart & 0xc00) >> 5;
  crtcTab[27] = (uchar)(temp & 0xff);

  /*----------- CRTCEXT3 register -----------------------------------------*/
  switch(bits_per_pixel)
    {
    case  8: temp = 2; break;
    case 16: temp = 4; break;
    case 32: temp = 8; break;
    default:           break;
    }

  if(interleave) temp /= 2;

  temp -= 1; 
  temp |= 0x80; /* mgamode = 1 */
  /* BL spec p 5-47 */

  switch(ai->mem_size)
    {
    case VIDRAM2MB: temp |= 0x08; break;
    case VIDRAM4MB:               break;
    case VIDRAM8MB:
    default:        temp |= 0x10; break;
    }

  switch(si->device_id)
    {
    case MGA_1064S:
    case MGA_G100_AGP:
    case MGA_G100_PCI:
    case MGA_G200_AGP:
    case MGA_G200_PCI:
      temp &= 0xe7;
      break;
    }

  crtcTab[28] = (uchar)(temp & 0xff);

  /*----------- CRTCEXT4 register -----------------------------------------*/
  crtcTab[29] = 0;

  /*----------- CRTCEXT5 register -----------------------------------------*/
  /* If interlace enable */
  if(is_interlaced)
    {
      hvidmid =
        ((vclkhvis + vclkhfrontp) +
         (vclkhvis + vclkhfrontp + vclkhsync) - 
         vclkhtotal) / 2 - 1;
    }
  else
    {
      hvidmid = 0;    /* default: no-interlace */
    }

  crtcTab[30] = hvidmid;     
}


//////////////////////////////////////////////////////////////////////////////
// Pitch for Display Mode

static uint32 pitchForDisplayMode(display_mode *target, uint32 *row_pixels)
{
  static int pix_adj[] = {0, 64, 32, 64, 32};
  static int pix_adj_i[] = {0, 128, 64, 128, 32};

  uint32 pixel_adjust;
  uint32 bytes_per_pixel;
  uint32 pixels_per_row;

  bytes_per_pixel = calcBitsPerPixel(target->space) >> 3;

  // NOTE: I'm not sure if this is accurate for the Gx00 cards. Even if it
  // isn't, the more conservative values are being used, so it should still
  // work.
  // UPDATE: (FFB) Actually, they're not the *most* conservative numbers,
  // just the most likely.  This needs to be revisited for G200 cards with 16MB of frame buffer.
  pixel_adjust = ((ai->mem_size == 2*1024*1024) ||
                  (si->device_id == MGA_1064S) 
                  ? pix_adj[bytes_per_pixel] : pix_adj_i[bytes_per_pixel]);

  // UPDATE: Since we're not using the auto-linearizer, we don't need to
  // restrict frame buffer pitches for the xx64 cards. Just use the same
  // formula based on granularity for everything.

  //   Adjust the requested width to satisfy granularity requirements.
  // Round up to the next multiple of the granularity.
  pixels_per_row
    = (target->virtual_width + pixel_adjust - 1) & ~(pixel_adjust - 1);

  if(row_pixels) *row_pixels = pixels_per_row;

  return pixels_per_row * bytes_per_pixel;
}


//////////////////////////////////////////////////////////////////////////////
// Set the Display Mode
//    "set the display mode, damn it!  No whimpy error checking here"
//    Hmmm.  Looks like Trey was debugging late... :)
//    I don't recall.  However, this function may now fail, so we're returning
//    a status_t result code.

status_t do_set_display_mode(display_mode *dm);

status_t do_set_display_mode(display_mode *dm)
{
  uchar crtcRegs[30];
  uint32        bits_per_pixel = calcBitsPerPixel(dm->space);
  uint32        bytes_per_row;
  uint32        tmpulong;
  MGA_SET_BOOL_STATE ri;

  // save it for later
  ai->bytes_per_pixel = bits_per_pixel >> 3;
  ai->dm = *dm;
  ddprintf(("bytes_per_pixel: %d\n", ai->bytes_per_pixel));

  ai->fbc.bytes_per_row
    = bytes_per_row
    = pitchForDisplayMode(dm, &(ai->pixels_per_row));

  ddprintf(("bytes_per_row: %d\npixels_per_row: %d\n",
            bytes_per_row,
            ai->pixels_per_row));

 
  ai->fbc.frame_buffer = 0;

  /* memtypes available? */
  if (memtypefd >= 0) {
	BMemSpec *ms = &(ai->fb_spec);

	/* release any previous frame buffer */
	if (ms->ms_MR.mr_Size) {
		ioctl(memtypefd, B_IOCTL_FREEBYMEMSPEC, ms, sizeof(*ms));
	}
	/* alloc frame buffer from genpool */
	memset(ms, 0, sizeof(*ms));
	ms->ms_PoolID = ai->poolid;
	
	/* Hmm, what *are* the frame buffer alignment constraints? */
	/* for now, make things 8 byte aligned */
	ms->ms_AddrCareBits = 7;
	ms->ms_AddrStateBits = 0;

	/* how much space for frame buffer */
	ms->ms_MR.mr_Size = bytes_per_row * dm->virtual_height;
	switch (si->device_id) {
		case MGA_2064W:
		case MGA_2164W:
	    case MGA_2164W_AGP:
			/* for cards with an external 64 bit DAC, a line can't cross a 4MB boundry */
			/* allocate a little more memory so we can adjust the framebuffer start */
			/* we always have to do this, as the pool allocator may place the fb anywhere */
			ms->ms_MR.mr_Size += bytes_per_row;
			break;
		default:
			break;
	}
	if (ioctl(memtypefd, B_IOCTL_ALLOCBYMEMSPEC, ms, sizeof(*ms)) < 0) {
		/* failed */
		ms->ms_MR.mr_Size = 0;
		goto use_old_method;
	}
	/* determine actual start of frame buffer */
	switch (si->device_id) {
	case MGA_2064W:
	case MGA_2164W:
	case MGA_2164W_AGP:
		ai->fbc.frame_buffer =
		 (void *) (((VIDRAM4MB - ms->ms_MR.mr_Offset) % bytes_per_row)
		           + ((uint32) si->framebuffer + ms->ms_MR.mr_Offset));
		break;
	default:
		ai->fbc.frame_buffer =
		 (void *) ((uchar *) si->framebuffer + ms->ms_MR.mr_Offset);
		break;
	}
	ai->YDstOrg = ((uint8 *)ai->fbc.frame_buffer - (uint8 *)si->framebuffer) / ai->bytes_per_pixel;
  }

use_old_method:
  if (!ai->fbc.frame_buffer) {
	/* adjust offset of display so any line doesn't cross a 4MB boundry */
	/*
	FIXME: this code is for the 2x64 cards, at least, and probably the 1064 cards.
	It's definitely needs updating for the G200 cards, as they support more than 8MB
	of usable frame buffer.  Also, ProposeDisplayMode() needs to take this adjustment
	into account for the appropriate cards.     
	*/
	if(bytes_per_row * dm->virtual_height > VIDRAM4MB)
		{
		ai->YDstOrg = (VIDRAM4MB % bytes_per_row) / ai->bytes_per_pixel;
		}
	else
		{
		ai->YDstOrg = 0;
		}
	ddprintf(("ai->YDstOrg: %d\n", ai->YDstOrg));
	
	ai->fbc.frame_buffer
		= (uchar *)si->framebuffer + (ai->YDstOrg * ai->bytes_per_pixel);
  }

  ai->start_addr
    = (bytes_per_row * dm->v_display_start)
    + ((dm->h_display_start + ai->YDstOrg) * ai->bytes_per_pixel);

  ddprintf(("start_addr: %d\n", ai->start_addr));

  /* may need to constrain h_display_start to get valid start_addr */
  ai->interleave = (ai->mem_size > VIDRAM2MB) || (si->device_id == MGA_1064S);

  /* start_addr in 32bit words */
  ai->start_addr /= 4;

  /* 64bit words in interleave mode */
  if (ai->interleave) ai->start_addr /= 2;

  calcCrtcRegs(dm, ai->start_addr, crtcRegs, ai->pixels_per_row);

  /* don't hammer the monitor :-) */
  SCREEN_OFF;

  /* disable interrupts */
  ri.magic = MGA_PRIVATE_DATA_MAGIC;
  ri.do_it = FALSE;
  ioctl(fd, MGA_RUN_INTERRUPTS, &ri, sizeof(ri));
        
  programCRTC(dm->timing.flags, crtcRegs);
        
  switch(si->device_id)
    {
    case MGA_1064S:
      /* program the DAC */
      calcMidRegs(dm->space, dm->timing.flags);
      /* set pixel clock */
      dm->timing.pixel_clock = midSetPixClock(dm->timing.pixel_clock);
      break;

    case MGA_G100_AGP:
    case MGA_G100_PCI:
    case MGA_G200_AGP:
    case MGA_G200_PCI:
      /* program the DAC */
      calcMidRegs(dm->space, dm->timing.flags);
      /* set pixel clock */
      dm->timing.pixel_clock = Gx00SetPixClock(dm->timing.pixel_clock);
      break;

    case MGA_2064W:
    case MGA_2164W:
    case MGA_2164W_AGP:
      /* enforce interleave mode */
      tmpulong = get_pci(STORM_OPTION, 4);

      if(ai->interleave) tmpulong |= 0x00001000;
      else               tmpulong &= 0xffffefff;

      set_pci(STORM_OPTION, 4, tmpulong);
      /* program the DAC */
      calcDacRegs(dm->space, ai->interleave, dm->timing.flags);
      /* set pixel clock */
      dm->timing.pixel_clock =
        tiSetPixClock(dm->timing.pixel_clock, ai->interleave, bits_per_pixel);
      break;
    }

  switch(dm->space)
    {
    case B_RGB32_BIG:
    case B_RGBA32_BIG:
      tmpulong = 0x00020000;
      break;

    case B_RGB16_BIG:
    case B_RGB15_BIG:
    case B_RGBA15_BIG:
      tmpulong = 0x00010000;
      break;

    case B_RGB32_LITTLE:
    case B_RGBA32_LITTLE:
    case B_RGB24_LITTLE:
    case B_RGB24_BIG:     /* 24 BIG not really allowed */
    case B_RGB16_LITTLE:
    case B_RGB15_LITTLE:
    case B_RGBA15_LITTLE:
    case B_CMAP8:
    default:
      tmpulong = 0;
      break;
    }

  STORM32W(STORM_OPMODE, tmpulong);

  switch(dm->space)
    {
    case B_RGB32_BIG:
    case B_RGBA32_BIG:
    case B_RGB32_LITTLE:
    case B_RGBA32_LITTLE:
      tmpulong = 2;
      break;

    case B_RGB24_BIG:
    case B_RGB24_LITTLE:
      tmpulong = 3;
      break;

    case B_RGB16_BIG:
    case B_RGB16_LITTLE:
      tmpulong = 1;
      break;

    case B_RGB15_BIG:
    case B_RGBA15_BIG:
    case B_RGB15_LITTLE:
    case B_RGBA15_LITTLE:
      tmpulong = 0x80000001;
      break;

    case B_CMAP8:
    default:
      tmpulong = 0;
      break;
    }

  switch(si->device_id)
    {
    case MGA_G100_AGP:
    case MGA_G100_PCI:
    case MGA_G200_AGP:
    case MGA_G200_PCI:
      tmpulong |= 0x00004000;
      break;
    }

  STORM32W(STORM_MACCESS, tmpulong);

  /* common regs */
  STORM32W(STORM_PITCH, (ai->pixels_per_row & 0x0fff) | 0x8000);
  //  STORM32W(STORM_PLNWT, 0xffffffff);
  STORM32W(STORM_YDSTORG, ai->YDstOrg);

  // HARDWARE HACK - The PLNWT register should *not* be set on a G100 using
  // SDRAM, as this causes a partial memory reset which causes subsequent
  // memory access to fail. This is an undocumented hardware bug/restriction
  // on the G100.

  switch(si->device_id)
    {
    case MGA_G100_PCI:
    case MGA_G100_AGP:
      if (ai->block_mode_ok)
      {
        // The card is using SGRAM, so setting this register is ok.
        STORM32W(STORM_PLNWT, 0xffffffff);
      }
      else
        ; // Do nothing, as the card uses SDRAM.
      break;

    default: // No restrictions, so set the PLNWT register as usual.
      STORM32W(STORM_PLNWT, 0xffffffff);
    }

  /* resume interrupts */
  ri.do_it = TRUE;
  ioctl(fd, MGA_RUN_INTERRUPTS, &ri, sizeof(ri));

  // Nail the clip rect to the screen size.

  SetClipRect(0,
              0,
              ai->pixels_per_row - 1,
              ai->dm.virtual_height - 1,
              ai->pixels_per_row,
              ai->YDstOrg);


  // Save updated versions of display mode parameters that have changed.
  // Just the pixel clock, for now.
  ai->dm.timing.pixel_clock = dm->timing.pixel_clock;

  // Start displaying stuff.
  SetDPMS_Mode(B_DPMS_ON);
  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Calculate Maximum Clocks

void calc_max_clocks(void)
{
  uint32 dac_max;

  switch(si->device_id) 
    {
    case MGA_2064W:
      switch (ai->bios.orig.RamdacType & 0xff)
        {
        case 0x002: dac_max = 250000; break;
        case 0x001: dac_max = 220000; break;
        default:        dac_max = 192000;
        }
      break;

    case MGA_2164W:
    case MGA_2164W_AGP:
    case MGA_1064S:
      if (ai->bios.pins.RamdacSpeed != 0xff) 
        {
          dac_max = (ai->bios.pins.PclkMax != 0xff) ?
            (int)ai->bios.pins.PclkMax : (int)ai->bios.pins.RamdacSpeed;
          dac_max += 100;
          dac_max *= 1000;
        }
      else switch(si->device_id)
        {
        case MGA_2164W:
        case MGA_2164W_AGP:
          dac_max = 220000; break;
        case MGA_1064S:
          dac_max = si->revision > 2 ? 220000 : 135000; break;
        default:
          dac_max = 135000;
        }
      break;

    case MGA_G100_AGP:
    case MGA_G100_PCI:
    case MGA_G200_AGP:
    case MGA_G200_PCI:
      ai->pix_clk_max8 = ((uint32)ai->bios.pins31.PclkMax8 + 100) * 1000;
      ai->pix_clk_max16 = ((uint32)ai->bios.pins31.PclkMax16 + 100) * 1000;
      ai->pix_clk_max32 = ((uint32)ai->bios.pins31.PclkMax32 + 100) * 1000;
      /* bail out now */
      return;

    default:
      ddprintf(("Yeek!  Bogus si->device_id in calc_max_clocks()\n"));
      dac_max = 0;
      break;
    }

  ai->pix_clk_max8 = ai->pix_clk_max16 = dac_max;

  if(si->device_id == MGA_1064S)
    ai->pix_clk_max32 = (dac_max < 131000 ? dac_max : 131000);
  else
    ai->pix_clk_max32 = (dac_max < 230000 ? dac_max : 230000);
}


//////////////////////////////////////////////////////////////////////////////
// Accelerant Mode Count
//    Return the number of 'built-in' display modes.

uint32 AccelerantModeCount(void)
{
  return ai->mode_count;
}


//////////////////////////////////////////////////////////////////////////////
// Get a Mode List
//    Copy them to the buffer pointed at by *dm.

status_t GetModeList(display_mode *dm)
{
  memcpy(dm, predefined_mode_list, ai->mode_count * sizeof(display_mode));

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Propose a Video Mode
//    Adjust target video mode to be vaild for this device within the ranges
// specified by low and high.  Return B_ERROR if we can't accomodate.

status_t ProposeVideoMode(display_mode *target,
                          const display_mode *low,
                          const display_mode *high)
{
  status_t result = B_OK;

  uint32 row_bytes;
  uint32 row_pixels;
  uint32 limit_clock;

  double target_refresh
    = ((double)target->timing.pixel_clock * 1000.0) 
    / ((double)target->timing.h_total * (double)target->timing.v_total);

  bool want_same_width = target->timing.h_display == target->virtual_width;
  bool want_same_height = target->timing.v_display == target->virtual_height;

  /* validate horizontal timings */
  {
    /* horizontal must be multiples of 8 */
    uint16 h_display = target->timing.h_display >> 3;
    uint16 h_sync_start = target->timing.h_sync_start >> 3;
    uint16 h_sync_end = target->timing.h_sync_end >> 3;
    uint16 h_total = target->timing.h_total >> 3;

    /* ensure reasonable minium display and sequential order of parms */
    if (h_display < (320 >> 3)) h_display = 320 >> 3;
    if (h_display > (2048 >> 3)) h_display = 2048 >> 3;
    if (h_sync_start < (h_display + 2)) h_sync_start = h_display + 2;
    if (h_sync_end < (h_sync_start + 3)) h_sync_end = h_sync_start + 3; /*(0x001f >> 2);*/
    if (h_total < (h_sync_end + 1)) h_total = h_sync_end + 1;
    /* adjust for register limitations */
    if ((h_total - h_display) > 0x007f) {
      h_total = h_display + 0x007f;
      if (h_sync_end > (h_total - 1)) h_sync_end = h_total - 1;
      if (h_sync_start > h_sync_end) h_sync_start = h_sync_end - 0x001f;
    }
    if ((h_sync_end - h_sync_start) > 0x001f) h_sync_end = h_sync_start + 0x001f;

    // Due to a hardware restriction in Matrox cards, the total number of horizontal
    // characters must *not* be of the form 8k + 7 (for integer k).
    if ((h_total & 0x07) == 0x07)
    {
      // If register restrictions allow, increment the total. Otherwise, decrement
      // and hope that this doesn't cause problems (e.g. overlapping the display area
      // or the sync pulse).
      if ((h_total - h_display) < 0x007f)
        h_total++;
      else
        h_total--;
    }

    target->timing.h_display = h_display << 3;
    target->timing.h_sync_start = h_sync_start << 3;
    target->timing.h_sync_end = h_sync_end << 3;
    target->timing.h_total = h_total << 3;
  }

  if((target->timing.h_display < low->timing.h_display) ||
     (target->timing.h_display > high->timing.h_display) ||
     (target->timing.h_sync_start < low->timing.h_sync_start) ||
     (target->timing.h_sync_start > high->timing.h_sync_start) ||
     (target->timing.h_sync_end < low->timing.h_sync_end) ||
     (target->timing.h_sync_end > high->timing.h_sync_end) ||
     (target->timing.h_total < low->timing.h_total) ||
     (target->timing.h_total > high->timing.h_total))
    {
      result = B_BAD_VALUE;
    }

  /* validate vertical timings */
  {
    uint16 v_display = target->timing.v_display;
    uint16 v_sync_start = target->timing.v_sync_start;
    uint16 v_sync_end = target->timing.v_sync_end;
    uint16 v_total = target->timing.v_total;

    /* ensure reasonable minium display and sequential order of parms */

    if(v_display < 200) v_display = 200;
    if(v_display > 2048) v_display = 2048; /* ha! */
    if(v_sync_start < (v_display + 1)) v_sync_start = v_display + 1;
    if(v_sync_end < v_sync_start) v_sync_end = v_sync_start + (0x000f >> 2);
    if(v_total < (v_sync_end + 1)) v_total = v_sync_end + 1;

    /* adjust for register limitations */

    if((v_total - v_display) > 0x00ff)
      {
        v_total = v_display + 0x00ff;
        if(v_sync_end > (v_total - 1)) v_sync_end = v_total - 1;
        if(v_sync_start > v_sync_end) v_sync_start = v_sync_end - 0x000f;
      }

    if((v_sync_end - v_sync_start) > 0x000f)
      {
        v_sync_end = v_sync_start + 0x000f;
      }

    target->timing.v_display = v_display;
    target->timing.v_sync_start = v_sync_start;
    target->timing.v_sync_end = v_sync_end;
    target->timing.v_total = v_total;
  }

  if((target->timing.v_display < low->timing.v_display) ||
     (target->timing.v_display > high->timing.v_display) ||
     (target->timing.v_sync_start < low->timing.v_sync_start) ||
     (target->timing.v_sync_start > high->timing.h_sync_start) ||
     (target->timing.v_sync_end < low->timing.v_sync_end) ||
     (target->timing.v_sync_end > high->timing.v_sync_end) ||
     (target->timing.v_total < low->timing.v_total) ||
     (target->timing.v_total > high->timing.v_total))
    {
      result = B_BAD_VALUE;
    }

  /* adjust pixel clock for DAC limits and target refresh rate */
  target->timing.pixel_clock
    = target_refresh * ((double)target->timing.h_total)
    * ((double)target->timing.v_total) / 1000.0;

  switch(target->space & 0x0fff)
    {
    case B_CMAP8:
      limit_clock = ai->pix_clk_max8;
      break;

    case B_RGB15:
    case B_RGB16:
      limit_clock = ai->pix_clk_max16;
      break;

    case B_RGB32:
      limit_clock = ai->pix_clk_max32;
      break;

    default:
      limit_clock = 0;
    }

  if(target->timing.pixel_clock > limit_clock)
    {
      target->timing.pixel_clock = limit_clock;
    }

  if((target->timing.pixel_clock < low->timing.pixel_clock) ||
     (target->timing.pixel_clock > high->timing.pixel_clock))
    {
      result = B_BAD_VALUE;
    }

  /* validate display vs. virtual */
  if((target->timing.h_display > target->virtual_width) || want_same_width)
    {
      target->virtual_width = target->timing.h_display;
    }

  if((target->timing.v_display > target->virtual_height) || want_same_height)
    {
      target->virtual_height = target->timing.v_display;
    }

  if(target->virtual_width > 2048) target->virtual_width = 2048;

  row_bytes = pitchForDisplayMode(target, &row_pixels);

  if(!want_same_width) target->virtual_width = row_pixels;

  /* adjust virtual width for engine limitations */
  if((target->virtual_width < low->virtual_width) ||
     (target->virtual_width > high->virtual_width))
    {
      result = B_BAD_VALUE;
    }

  /* memory requirement for frame buffer */
  /* NOTE: (FFB) This needs more work, at the verly least for dealing with
     xx64 cards where the frame buffer cannot cross a 4MB frame buffer */
  {
  uint32 fb_mem_size = ai->mem_size;
  switch (si->device_id) {
    case MGA_1064S:
    case MGA_G100_AGP:
    case MGA_G100_PCI:
    case MGA_G200_AGP:
    case MGA_G200_PCI:
      fb_mem_size -= 2048; /* subtract out cursor buffer */
      break;
  }
  if((row_bytes * target->virtual_height) > fb_mem_size)
    {
      target->virtual_height = fb_mem_size / row_bytes;
    }
  }

  if(target->virtual_height > 2048) target->virtual_height = 2048;

  if (target->virtual_height < target->timing.v_display)
    {
      // not enough frame buffer memory for the mode
      result = B_ERROR;
    }
  else if((target->virtual_height < low->virtual_height) ||
          (target->virtual_height > high->virtual_height))
    {
      result = B_BAD_VALUE;
    }

  /* flags? */
  // we *HAVE* to use hardware cursor, until the app_server is fixed to let us switch
  if (((low->flags & B_HARDWARE_CURSOR) == (high->flags & B_HARDWARE_CURSOR)) && !(high->flags & B_HARDWARE_CURSOR))
    // fail miserably
    return B_ERROR;
  // set the bit
  target->flags |= B_HARDWARE_CURSOR;
    
  // only allow overlays on supported devices
  switch (si->device_id) {
    case MGA_G200_AGP:
    case MGA_G200_PCI:
      break;
	default:
	  // clear flag on all unsupported devices
	  target->flags &= ~B_SUPPORTS_OVERLAYS;
      break;
  }

  // if overlays required and not supported, fail
  // if overlays not required and suggested, fail
  if (((low->flags & B_SUPPORTS_OVERLAYS) == (high->flags & B_SUPPORTS_OVERLAYS)) &&
      ((high->flags & B_SUPPORTS_OVERLAYS) != (target->flags & B_SUPPORTS_OVERLAYS)))
    return B_ERROR;

  return result;
}


//////////////////////////////////////////////////////////////////////////////

void create_mode_list(void)
{
  size_t max_size;
  int res_count, /* refresh_count, */ cspace_count;
  uint32
    pix_clk_range;
  display_mode
    standard_mode,
    *dst,
    low,
    high;
  int diag_mode_count, standard_mode_count;
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

  // Calculate the number of standard modes that we'd be adding in the worst case
  // (all possible modes are supported by the card).
  standard_mode_count = STANDARD_RESOLUTION_COUNT // (not doing this for now) * STANDARD_REFRESH_RATE_COUNT
    * STANDARD_COLOUR_SPACE_COUNT;
    
  // Calculate the total number of display_mode entries that we need.
  max_size = diag_mode_count + standard_mode_count;

  // Calculate the amount of memory needed. Adjust up to nearest multiple of B_PAGE_SIZE.
  max_size = ((max_size * sizeof(display_mode)) + (B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);

  // Create an area to hold the info.
  predefined_mode_list_area = create_area("MGA accelerant mode info", (void **)&predefined_mode_list, B_ANY_ADDRESS, max_size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
  if (predefined_mode_list_area < B_OK) return;


  // Initialize counters and pointers.
  dst = predefined_mode_list;
  ai->mode_count = 0;

  // Walk through the diagnostics mode list, adding supported modes to the list.
  for (index = 0; diagnostic_display_modes[index].timing.pixel_clock; index++)
  {
    // Store the mode parameters.
    standard_mode = diagnostic_display_modes[index];

    // See if this mode is supported.

    // Initialize low and high range boundaries.
    low = high = standard_mode;
    // let the flag values float as appropriate for the card or mode
    low.flags = 0;
    high.flags = ~0;

    // Set low and high range boundaries for the pixel clock.
    // Range is arbitrarily picked as 6.25% of the desired pixel clock.
    pix_clk_range = low.timing.pixel_clock >> 5;
    low.timing.pixel_clock -= pix_clk_range;
    high.timing.pixel_clock += pix_clk_range;

    // Bump up the maximum virtual width so that any wierd frame buffer pitch
    // restrictions can be satisfied.
    high.virtual_width = 2048;

    // Put this mode in the list and test it.
    *dst = standard_mode;

    // Use ProposeVideoMode to see if the mode is supported.
    if(ProposeVideoMode(dst, &low, &high) != B_ERROR)
    {
      // Diagnostics.
//      ddprintf(("%dx%d 0x%04x\n", dst->timing.h_display, dst->timing.v_display, dst->space));

      // The mode is supported; increment pointers appropriately.
      dst++;
      ai->mode_count++;
    }
  }

  // Walk through all standard modes, adding supported modes to the list.
  for (cspace_count = 0; cspace_count < STANDARD_COLOUR_SPACE_COUNT; cspace_count++)
  {

      for (res_count = 0; res_count < STANDARD_RESOLUTION_COUNT; res_count++)
      {
        // Construct the mode.
        // Store most of the mode parameters.
        standard_mode = standard_display_modes[res_count];
        // Continue constructing the mode.
        // Specify the colour space.
        standard_mode.space = standard_colour_spaces[cspace_count];


        // See if this mode is supported.

        // Initialize low and high range boundaries.
        low = high = standard_mode;
    	// let the flag values float as appropriate for the card or mode
        low.flags = 0;
        high.flags = ~0;

        // Set low and high range boundaries for the pixel clock.
        // Range is arbitrarily picked as 6.25% of the desired pixel clock.
        pix_clk_range = low.timing.pixel_clock >> 5;
        low.timing.pixel_clock -= pix_clk_range;
        high.timing.pixel_clock += pix_clk_range;

        // Bump up the maximum virtual width so that any wierd frame buffer pitch
        // restrictions can be satisfied.
        // FIXME: Gx00 cards support up to 2048 pixel wide buffers
        high.virtual_width = 1920;


        // Put this mode in the list and test it.
        *dst = standard_mode;

        // Use ProposeVideoMode to see if the mode is supported.
        if(ProposeVideoMode(dst, &low, &high) != B_ERROR)
        {
          // Diagnostics.
//          ddprintf(("%dx%d 0x%04x\n", dst->timing.h_display, dst->timing.v_display, dst->space));

          // The mode is supported
          // Find out if it's unique
          if (!ai->mode_count || (ai->mode_count && memcmp(dst, dst-1, sizeof(*dst)))) {
            // increment pointers appropriately.
            dst++;
            ai->mode_count++;
          }
        }
      }
  }
}


//////////////////////////////////////////////////////////////////////////////
// Set the Video Mode

status_t SetVideoMode(display_mode *dm)
{
  display_mode low = *dm;
  display_mode high = *dm;
  display_mode target = *dm;
  status_t result;

  // let the flags values float as appropriate
  low.flags = 0;
  high.flags = ~0;

  if(ProposeVideoMode(&target, &low, &high) == B_ERROR) return B_ERROR;

  // acquire the shared benaphore

  if((atomic_add(&(ai->engine_ben), 1)) >= 1)
    {
      acquire_sem(ai->engine_sem);
    }

  result = do_set_display_mode(&target);

  // release the shared benaphore

  if((atomic_add(&(ai->engine_ben), -1)) > 1)
    {
      release_sem(ai->engine_sem);
    }

  // hide the hardware cursor if it's not being used
  if (!(target.flags & B_HARDWARE_CURSOR)) ShowCursor(false);

  return result;
}


//////////////////////////////////////////////////////////////////////////////
// Get Video Mode

status_t GetVideoMode(display_mode *dm)
{
  // we should probably check that a mode has been set

  *dm = ai->dm;  // report the mode

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Get Framebuffer Configuration

status_t GetFramebufferConfig(frame_buffer_config *fbc)
{
  fbc->frame_buffer = ai->fbc.frame_buffer;

  fbc->frame_buffer_dma
    = (void *)((uint8*)si->framebuffer_pci
               + ((uint8*)ai->fbc.frame_buffer - (uint8*)si->framebuffer));

  fbc->bytes_per_row = ai->fbc.bytes_per_row;

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

  *low = tmpdbl / 1000.0;

  /* max-out at 150Hz */
  tmpdbl
    = (double)dm->timing.h_total * (double)dm->timing.v_total * (double)150.0;

  *high = tmpdbl / 1000.0;

  /* adjust limits based on mode and card type */

  switch(dm->space & 0x0fff)
    {
    case B_CMAP8:
      limit_clock = ai->pix_clk_max8;
      break;

    case B_RGB15:
    case B_RGB16:
      limit_clock = ai->pix_clk_max16;
      break;

    case B_RGB32:
      limit_clock = ai->pix_clk_max32;
      break;

    default:
      return B_ERROR;
    }

  if(*high > limit_clock) *high = limit_clock;
  if(*low > *high) *low = *high;

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Move the Display Area
#define MAX_OVERLAYS (sizeof(ai->ovl_tokens) / sizeof(ai->ovl_tokens[0]))

status_t MoveDisplayArea(uint16 h_display_start,
                         uint16 v_display_start)
{
  ai->dm.h_display_start = h_display_start;
  ai->dm.v_display_start = v_display_start;

  /* calculate the new start_addr */
  si->start_addr
    = (ai->fbc.bytes_per_row * ai->dm.v_display_start)
    + ((ai->dm.h_display_start + ai->YDstOrg) * ai->bytes_per_pixel);

  /* may need to constrain h_display_start to get valid start_addr */
  /* start_addr in 32bit words */
  si->start_addr /= 4;

  /* 64bit words in interleave mode */
  if(ai->interleave) si->start_addr /= 2;

  ddprintf(("B_MOVE_DISPLAY_AREA: 0x%08x\n", si->start_addr));

  /* notify the interrupt handler */
  atomic_or((int32 *)&(si->flags), MKD_SET_START_ADDR);

	/* if the display moved, then we might need to update overlay windows */
	if (can_do_overlays) {
		int i;
		for (i = 0; i < MAX_OVERLAYS; i++) {
			if (ai->ovl_tokens[i].used) {
				mga_overlay_token *mot = &(ai->ovl_tokens[i]);
				CONFIGURE_OVERLAY((overlay_token *)mot, mot->ob, &(mot->ow), &(mot->ov));
			}
		}
	}
  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
