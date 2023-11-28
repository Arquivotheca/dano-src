/* :ts=8 bk=0
 *
 * mode.c:	Routine(s) to deal with the display mode.
 *
 * $Id:$
 *
 * Leo L. Schwab					2000.08.24
 *
 * Copyright 2000 Be Incorporated.
 */
#include <kernel/OS.h>
#include <graphics_p/i810/i810.h>
#include <graphics_p/i810/debug.h>
#include <unistd.h>
#include <errno.h>

#include "protos.h"


/****************************************************************************
 * Globals
 */
extern gfx_card_info	*ci;
extern gfx_card_ctl	*cc;
extern int		devfd;


/****************************************************************************
 * This "thunk" routine is remarkable in that it has to reset the
 * acceleration state after changing modes.
 */
status_t
setdisplaymode (display_mode *dm)
{
	GfxBuf		gb;
	status_t	retval;

	/*
	 * We grab the rendering lock so confusion doesn't result.
	 */
	BLockBena4 (&cc->cc_EngineLock);
	acquire_sem (ci->ci_DispBufLock);

	memset (&gb, 0, sizeof (gb));
	gb.gb_ColorSpace	= dm->space;
	gb.gb_Width		= dm->virtual_width;
	gb.gb_Height		= dm->virtual_height;
	gb.gb_Layout		= GFXBUF_LAYOUT_PACKEDLINEAR;
	gb.gb_Usage		= GFXBUF_USAGE_DISPLAY
				| GFXBUF_USAGE_2DSRC
				| GFXBUF_USAGE_2DDEST;
	gb.gb_NMips		= 0;
	gb.gb_ExtraData		= NULL;

	/*  Free existing framebuffer.  */
	if ((retval = ioctl (devfd, GFX_IOCTL_FREEGFXBUF,
			     &cc->cc_DispBuf, sizeof (GfxBuf))) < 0)
	{
dprintf ((">>> FREE OF OLD FRAMEBUFFER FAILED!\n"));
		retval = errno;
		goto err;	/*  Look down  */
	}

	/*  Get the new one.  */
	if ((retval = ioctl (devfd, GFX_IOCTL_ALLOCGFXBUF,
			     &gb, sizeof (GfxBuf))) < 0)
	{
dprintf ((">>>!!! Display buffer allocation failed: %d (0x%08lx)\n",
 retval, retval));
		/*  CRAP!  Try and restore previous allocation.  */
		if ((retval = ioctl (devfd, GFX_IOCTL_ALLOCGFXBUF,
				     &cc->cc_DispBuf, sizeof (GfxBuf))) < 0)
		{
			/*  Well, I don't know what else I can do...  */
dprintf ((">>>!!! YIKES!  Can't restore framebuffer allocation!!  retval = %d\n",
 retval));
		}
		retval = errno;
		goto err;	/*  Look down  */
	}
	memcpy (&cc->cc_DispBuf, &gb, sizeof (GfxBuf));
dprintf ((">>> Display buffer allocated at offset 0x%08lx\n",
 gb.gb_BaseOffset));


	if ((retval = ioctl (devfd, GDRV_IOCTL_SET_DISPMODE,
			     dm, sizeof (*dm))) < 0)
	{
		retval = errno;
		goto err;	/*  Look down  */
	}
	if ((retval = ioctl (devfd, GDRV_IOCTL_SET_FBBASE,
	                     &gb.gb_BaseOffset, sizeof (uint32))) < 0)
	{
		retval = errno;
		goto err;	/*  Look down  */
	}

	retval = AccelInit (ci);

	/*  Now we're really done; release the locks.  */
err:	release_sem (ci->ci_DispBufLock);
	BUnlockBena4 (&cc->cc_EngineLock);
	return (retval);
}

status_t
setfbbase (uint32 fbbase)
{
	GCmd_FrontBufferInfo	fbi;
	uint32			wait;

	/*
	 * Consider moving this into the kernel; we need to keep
	 * openerinfo.oi_FBBase up to date.
	 */
	wait = GFXCMDPARSER_WAIT_FOR_EVENT
	     | VAL2FIELD (GFXCMDPARSER_WAIT_FOR_EVENT, PAGEFLIP, TRUE);

	fbi.gcmd_Instruction	=
	 GFXCMDPARSER_FRONT_BUFFER_INFO
	 | VAL2FIELD (GFXCMDPARSER_FRONT_BUFFER_INFO, PITCH,
	              cc->cc_DispBuf.gb_BytesPerRow >> 3)
	 | DEF2FIELD (GFXCMDPARSER_FRONT_BUFFER_INFO, TYPE, SYNC);
	fbi.gcmd_BaseAddr	= fbbase;

#if 0
	/*
	 * Update parallel settings in the CRTC registers.
	 */
	mr->mr_ATTR[AR_HPAN] = (fbbase & 3) << 1;	// Pixel panning

//	fbbase >>= 2;	// 32-bit quantization
	mr->mr_CRTC[CR_FBBASE_9_2]	= SetBitField (fbbase, 9:2, 7:0);
	mr->mr_CRTC[CR_FBBASE_17_10]	= SetBitField (fbbase, 17:10, 7:0);
	mr->mr_CRTC[CR_FBBASE_23_18]	= SetBitField (fbbase, 23:18, 5:0)
					| MASKFIELD (CR_FBBASE_23_18, LOADADDR);
	mr->mr_CRTC[CR_FBBASE_31_24]	= SetBitField (fbbase, 31:24, 7:0);
#endif

	BLockBena4 (&cc->cc_EngineLock);
	writepacket (&wait, sizeof (wait), FALSE, 1);
	writepacket (&fbi, sizeof (fbi), TRUE, 1);  /*  Post to IRQ ring.  */
	BUnlockBena4 (&cc->cc_EngineLock);
	
	return (B_OK);
}

status_t
getframebufferconfig (frame_buffer_config *fbuf)
{
	fbuf->frame_buffer	= (void *) cc->cc_DispBuf.gb_BaseAddr;
	fbuf->frame_buffer_dma	= (void *) cc->cc_DispBuf.gb_BaseAddr_PCI;
	fbuf->bytes_per_row	= cc->cc_DispBuf.gb_BytesPerRow;

	return (B_OK);
}
