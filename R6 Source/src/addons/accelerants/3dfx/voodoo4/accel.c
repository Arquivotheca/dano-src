/* :ts=8 bk=0
 *
 * accel.c:	Basic rendering acceleration.
 *
 * $Id:$
 *
 * Andrew Kimpton					1999.03.10
 *  Based on i740 driver code.
 */
 
#include <OS.h>
#include <image.h>
#include <opengl/GLDefines.h>
#include <add-ons/graphics/Accelerant.h>
#include <graphics_p/video_overlay.h>

#include <graphics_p/3dfx/voodoo4/voodoo4.h>
#include <graphics_p/3dfx/voodoo4/voodoo4_regs.h>
#include <graphics_p/3dfx/common/bena4.h>
#include <graphics_p/3dfx/common/debug.h>

#include "protos.h"
#include "fifo.h"

/*****************************************************************************
 * #defines
 */
#define	MONOSRCCTL	(FIELDDEF (BLT_MONOSRCCTL, MONOSRC, PATTERN) | \
			 FIELDDEF (BLT_MONOSRCCTL, MONOALIGN, 8BIT) | \
			 FIELDVAL (BLT_MONOSRCCTL, CHOPINITIAL, 0) | \
			 FIELDVAL (BLT_MONOSRCCTL, CLIPLEFT, 0) | \
			 FIELDVAL (BLT_MONOSRCCTL, CLIPRIGHT, 0))
#define	BLITCTL_RECT	(FIELDVAL (BLT_BLTCTL, USELOCALDEPTH, FALSE) | \
			 FIELDVAL (BLT_BLTCTL, PATVALIGN, 0) | \
			 FIELDDEF (BLT_BLTCTL, PATSRC, ZERO) | \
			 FIELDVAL (BLT_BLTCTL, PATISWRITEMASK, FALSE) | \
			 FIELDDEF (BLT_BLTCTL, PATTYPE, MONO) | \
			 FIELDDEF (BLT_BLTCTL, CKEYMODE, NONE) | \
			 FIELDVAL (BLT_BLTCTL, SRCISWRITEMASK, FALSE) | \
			 FIELDDEF (BLT_BLTCTL, SRCTYPE, COLOR) | \
			 FIELDDEF (BLT_BLTCTL, SRC, MEM) | \
			 FIELDDEF (BLT_BLTCTL, YINCDIR, DOWNWARD) | \
			 FIELDDEF (BLT_BLTCTL, XINCDIR, RIGHTWARD) | \
			 FIELDVAL (BLT_BLTCTL, MINTERM, MINTERM_PATCOPY))
#define	BLITCTL_BLIT	(FIELDVAL (BLT_BLTCTL, USELOCALDEPTH, FALSE) | \
			 FIELDVAL (BLT_BLTCTL, PATVALIGN, 0) | \
			 FIELDDEF (BLT_BLTCTL, PATSRC, ZERO) | \
			 FIELDVAL (BLT_BLTCTL, PATISWRITEMASK, FALSE) | \
			 FIELDDEF (BLT_BLTCTL, PATTYPE, MONO) | \
			 FIELDDEF (BLT_BLTCTL, CKEYMODE, NONE) | \
			 FIELDVAL (BLT_BLTCTL, SRCISWRITEMASK, FALSE) | \
			 FIELDDEF (BLT_BLTCTL, SRCTYPE, COLOR) | \
			 FIELDDEF (BLT_BLTCTL, SRC, MEM) | \
			 FIELDVAL (BLT_BLTCTL, MINTERM, MINTERM_SRCCOPY))

#if 1
#define WAIT_FOR_SLOTS(x) while (ci->Regs->regs_2d_u.regs_2d.status & SST_PCIFIFO_BUSY)
#else
#define WAIT_FOR_SLOTS(x)
#endif


/*****************************************************************************
 * Local prototypes.
 */
//static void writepacket (register uint32 *buf, register int32 size);
void WriteSerialNumber(uint32 serial_num);


/*****************************************************************************
 * Globals.
 */
extern thdfx_card_info	*ci;

static engine_token	enginetoken = {
	1, B_2D_ACCELERATION, NULL
};


/*****************************************************************************
 * Engine and sync_token management.
 */
uint32
getenginecount (void)
{
//	dprintf (("3dfx_v4_accel: getenginecount - ENTER\n"));
	return (1);
}

status_t
acquireengine (
uint32		caps,
uint32		max_wait,
sync_token	*st,
engine_token	**et
)
{
	(void) max_wait;

//	dprintf (("3dfx_v4_accel: acquireengine(et:0x%08x, st:0x%08x) - ENTER\n", et, st));
	if (caps & B_3D_ACCELERATION)
		/*  No 3D acceleration yet.  */
		return (B_ERROR);

	lockBena4 (&ci->EngineLock);

	/*  Sync if required  */
	if (st)
		synctotoken (st);

	/*  Return an engine_token  */
	*et = &enginetoken;
	return (B_OK);
}

status_t
releaseengine (engine_token *et, sync_token *st)
{
//dprintf(("3dfx_v4_accel: releaseengine(et:0x%08x, st:0x%08x) - ENTER\n", et, st));
	if (!et) {
//dprintf(("3dfx_v4_accel: releaseengine - EXIT called with null engine_token!\n"));
		return (B_ERROR);
	}

	/*  Update the sync token, if any  */
	if (st) {
//dprintf(("3dfx_v4_accel: updating sync token - id: %d, count %Ld\n", et->engine_id, ci->PrimitivesIssued));
		st->engine_id	= et->engine_id;
		st->counter	= ci->PrimitivesIssued;
	}

	unlockBena4 (&ci->EngineLock);
//dprintf (("3dfx_v4_accel: releaseengine - EXIT\n"));
	return (B_OK);
}

void
waitengineidle (void)
{
	uint32 stat, i, cnt;
	bigtime_t start_time, end_time;
	
	// We need to wait for the Fifo to drain
  // 1) Send a NOP, which waits for any commands in progress
  // 2) Read the status register for 3 consecutive non-busy
  FifoAllocateSlots(1, 2);
  SET_PKT2_HEADER(SSTCP_COMMAND);
  FifoWriteLong(1, SST_2D_COMMAND, SSTG_NOP|SSTG_GO);

  i=0;
  cnt=0;
  start_time=0;

  do
  {
		stat = _V5_ReadReg( ci, V5_2D_STATUS );
    if (stat & SST_GUI_BUSY)
			i=0;
		else
			i++;
    cnt++;
    if (cnt==1000) {
      if (!start_time) {
				start_time = system_time();
      } else {
				end_time = system_time();
				if (end_time-start_time > (3 * 1000000))
				{
// If we've been waiting for longer than 3 seconds then something bad has
// happened - reset the FIFO
//				  TDFXResetFifo(pTDFX);
//dprintf(("waitengineidle - Need To Reset Fifo !!\n"));
				  start_time=0;
        }
      }
      cnt=0;
    }
  } while (i<3);
}

status_t
getsynctoken (engine_token *et, sync_token *st)
{
//dprintf (("3dfx_v4_accel: getsynctoken(et:0x%08x, st:0x%08x) - ENTER\n", et, st));

	if (et) {
		st->engine_id	= et->engine_id;
		st->counter	= ci->PrimitivesIssued;
		return (B_OK);
	} else
		return (B_ERROR);
}

/*
 * This does The Cool Thing (!) with the sync_token values, and waits for the
 * requested primitive to complete, rather than waiting for the whole engine
 * to drop to idle.  BEWARE - Do not actually write to the FIFO in this
 * routine - the engine lock is not currently held and you'll corrupt the FIFO
 * command stream.
 */
status_t
synctotoken (sync_token *st)
{
	uint32 serial;
	
//dprintf (("3dfx_v4_accel: synctotoken(st->counter = %Ld) - ENTER\n", st->counter));

	serial = *(vuint32*)(ci->AllocSentinal->address);
	serial = *(vuint32*)(ci->AllocSentinal->address+32);
	serial = *(vuint32*)(ci->AllocSentinal->address);
	
	while (serial < st->counter)
	{
		// Wait a little if we need to re-check the counter, no need to bang the PCI bus to death
		snooze(10);

//dprintf(("3dfx_v4_accel: synctotoken() waiting - st->counter = %Ld, serial = %ld\n", st->counter, serial));

		serial = *(vuint32*)(ci->AllocSentinal->address);
		serial = *(vuint32*)(ci->AllocSentinal->address+32);
		serial = *(vuint32*)(ci->AllocSentinal->address);
	}
//	snooze(250);
	ci->PrimitivesCompleted = serial;

//dprintf (("3dfx_v4_accel: synctotoken - EXIT\n"));
	return (B_OK);
}

/*****************************************************************************
 * Rendering code.
 */
/*
 * Handles 8-, 15-, 16-, and 32-bit modes.
 */
void
rectfill_gen (
engine_token			*et,
register uint32			color,
register fill_rect_params	*list,
register uint32			count
)
{
	uint32 srcColorFormat;
	
	if (et != &enginetoken)
	{
//		dprintf(("3dfx_v4_accel: blit - EXIT - no enginetoken\n"));
		return;
	}
	switch(ci->Depth)
	{
		case 8:
			srcColorFormat = SSTG_PIXFMT_8BPP;
			break;
		case 16:
			srcColorFormat = SSTG_PIXFMT_16BPP;
			break;
		case 32:
			srcColorFormat  = SSTG_PIXFMT_32BPP;
			break;
		default:
			dprintf(("3dfx_v4_accel: blit - Unsupported Color Format !\n"));
			return;
	}

	while (count--)
	{
	  FifoMakeRoom(1, 8);
	  SET_PKT2_HEADER(SSTCP_CLIP0MIN | SSTCP_CLIP0MAX | SSTCP_DSTBASEADDR | SSTCP_DSTFORMAT | SSTCP_COLORFORE | SSTCP_DSTSIZE | SSTCP_DSTXY | SSTCP_COMMAND); 
		FifoWriteLong(1, SST_2D_CLIP0MIN, 0);
		FifoWriteLong(1, SST_2D_CLIP0MAX, (list->right+1) | ((list->bottom+1) << 16));
		FifoWriteLong(1, SST_2D_DSTBASEADDR, ((uint32) (ci->FBBase) - (uint32) (ci->BaseAddr1)));
		FifoWriteLong(1, SST_2D_DSTFORMAT, (ci->BytesPerRow << SSTG_DST_STRIDE_SHIFT) | (srcColorFormat));
		FifoWriteLong(1, SST_2D_COLORFORE, color);
		FifoWriteLong(1, SST_2D_DSTSIZE, (list->right - list->left + 1) | ((list->bottom - list->top + 1) << 16));
		FifoWriteLong(1, SST_2D_DSTXY, (list->left) | ((list->top) << 16));
		FifoWriteLong(1, SST_2D_COMMAND,  SSTG_RECTFILL | SSTG_GO | SSTG_ROP_SRCCOPY);
		list++;
	}	
	ci->PrimitivesIssued++;
	WriteSerialNumber(ci->PrimitivesIssued);
}

void
rectangle_invert (
engine_token			*et,
register fill_rect_params	*list,
register uint32			count
)
{
	uint32 srcColorFormat;
	
	if (et != &enginetoken)
	{
//		dprintf(("3dfx_v4_accel: rectangle_invert - EXIT - no enginetoken\n"));
		return;
	}
	switch(ci->Depth)
	{
		case 8:
			srcColorFormat = SSTG_PIXFMT_8BPP;
			break;
		case 16:
			srcColorFormat = SSTG_PIXFMT_16BPP;
			break;
		case 32:
			srcColorFormat  = SSTG_PIXFMT_32BPP;
			break;
		default:
			dprintf(("3dfx_v4_accel: rectangle_invert - Unsupported Color Format !\n"));
			return;
	}
	while (count--)
	{
	  FifoMakeRoom(1, 7);
	  SET_PKT2_HEADER(SSTCP_CLIP0MIN | SSTCP_CLIP0MAX | SSTCP_DSTBASEADDR | SSTCP_DSTFORMAT | SSTCP_DSTSIZE | SSTCP_DSTXY | SSTCP_COMMAND);
		FifoWriteLong(1, SST_2D_CLIP0MIN, 0);
		FifoWriteLong(1, SST_2D_CLIP0MAX, (list->right+1) | ((list->bottom+1) << 16));
		FifoWriteLong(1, SST_2D_DSTBASEADDR, ((uint32) (ci->FBBase) - (uint32) (ci->BaseAddr1)));
		FifoWriteLong(1, SST_2D_DSTFORMAT, (ci->BytesPerRow << SSTG_DST_STRIDE_SHIFT) | (srcColorFormat));
		FifoWriteLong(1, SST_2D_DSTSIZE, (list->right - list->left + 1) | ((list->bottom - list->top + 1) << 16));
		FifoWriteLong(1, SST_2D_DSTXY, (list->left) | ((list->top) << 16));
		FifoWriteLong(1, SST_2D_COMMAND,  (SSTG_RECTFILL | SSTG_GO | (SSTG_ROP_NDST << SSTG_ROP0_SHIFT)));
		list++;
	}	
	ci->PrimitivesIssued++;
	WriteSerialNumber(ci->PrimitivesIssued);
}

void
blit (engine_token *et, register blit_params *list, register uint32 count)
{
	uint32 srcColorFormat, blitDirection, blitOffsetX, blitOffsetY;
	
	if (et != &enginetoken)
	{
//		dprintf(("3dfx_v4_accel: blit - EXIT - no enginetoken\n"));
		return;
	}
	switch(ci->Depth)
	{
		case 8:
			srcColorFormat = SSTG_PIXFMT_8BPP;
			break;
		case 16:
			srcColorFormat = SSTG_PIXFMT_16BPP;
			break;
		case 32:
			srcColorFormat  = SSTG_PIXFMT_32BPP;
			break;
		default:
			dprintf(("3dfx_v4_accel: blit - Unsupported Color Format !\n"));
			return;
	}
	while (count--)
	{
		blitDirection = blitOffsetX = blitOffsetY = 0;
		if (list->src_left < list->dest_left)
		{
			blitDirection |= 0x01;	/* Right to left */
			blitOffsetX = list->width;
		}
		if (list->src_top < list->dest_top)
		{
			blitDirection |= 0x02;	/* Bottom to top */
			blitOffsetY = list->height;
		}

		FifoMakeRoom(1, 11);
		SET_PKT2_HEADER(SSTCP_DSTBASEADDR | SSTCP_SRCBASEADDR | SSTCP_CLIP0MIN | SSTCP_DSTFORMAT | SSTCP_SRCFORMAT | SSTCP_SRCSIZE | SSTCP_SRCXY | SSTCP_DSTXY | SSTCP_CLIP0MAX | SSTCP_DSTSIZE | SSTCP_COMMAND);
		FifoWriteLong(1, SST_2D_CLIP0MIN, 0);
		/* The Clipping Region is specified in screen co-ordinates */
		FifoWriteLong(1, SST_2D_CLIP0MAX, (list->dest_left + list->width+1) | ((list->dest_top + list->height+1) << 16));
		FifoWriteLong(1, SST_2D_DSTBASEADDR, ((uint32) (ci->FBBase) - (uint32) (ci->BaseAddr1)));
		FifoWriteLong(1, SST_2D_DSTFORMAT, (ci->BytesPerRow << SSTG_DST_STRIDE_SHIFT) | (srcColorFormat));
		FifoWriteLong(1, SST_2D_SRCBASEADDR, ((uint32) (ci->FBBase) - (uint32) (ci->BaseAddr1)));
		FifoWriteLong(1, SST_2D_SRCFORMAT, (ci->BytesPerRow << SSTG_DST_STRIDE_SHIFT) | (srcColorFormat));
		FifoWriteLong(1, SST_2D_SRCSIZE, (list->width+1) | ((list->height+1) << 16));
		FifoWriteLong(1, SST_2D_SRCXY, (list->src_left) + blitOffsetX | ((list->src_top + blitOffsetY) << 16));
		FifoWriteLong(1, SST_2D_DSTSIZE, (list->width+1) | ((list->height+1) << 16));
		FifoWriteLong(1, SST_2D_DSTXY, (list->dest_left) + blitOffsetX | ((list->dest_top + blitOffsetY) << 16));
		// Write the command register
		FifoWriteLong(1, SST_2D_COMMAND, (SSTG_BLT | SSTG_GO | SSTG_ROP_SRCCOPY | (blitDirection << 14)));

		list++;
	}	
	ci->PrimitivesIssued++;
	WriteSerialNumber(ci->PrimitivesIssued);
}

void span_fill(engine_token *et,
              uint32 color,
              uint16 *list,
              uint32 count)
{
	uint32 srcColorFormat;
	
	if (et != &enginetoken)
	{
//		dprintf(("3dfx_v4_accel: span_fill - EXIT - no enginetoken\n"));
		return;
	}
	switch(ci->Depth)
	{
		case 8:
			srcColorFormat = SSTG_PIXFMT_8BPP;
			break;
		case 16:
			srcColorFormat = SSTG_PIXFMT_16BPP;
			break;
		case 32:
			srcColorFormat  = SSTG_PIXFMT_32BPP;
			break;
		default:
			dprintf(("3dfx_v4_accel: span_fill - Unsupported Color Format !\n"));
			return;
	}

	while (count--)
	{
		FifoMakeRoom(1, 8);
		SET_PKT2_HEADER(SSTCP_DSTBASEADDR | SSTCP_DSTFORMAT | SSTCP_CLIP0MIN | SSTCP_COLORFORE | SSTCP_DSTXY | SSTCP_DSTSIZE | SSTCP_CLIP0MAX | SSTCP_COMMAND);
		FifoWriteLong(1, SST_2D_CLIP0MIN, 0);
		/* The Clipping Region is specified in screen co-ordinates */
		FifoWriteLong(1, SST_2D_CLIP0MAX, (list[2]+1) | ((list[0]+1) << 16));
		FifoWriteLong(1, SST_2D_DSTBASEADDR, ((uint32) (ci->FBBase) - (uint32) (ci->BaseAddr1)));
		FifoWriteLong(1, SST_2D_DSTFORMAT, (ci->BytesPerRow << SSTG_DST_STRIDE_SHIFT) | (srcColorFormat));
		FifoWriteLong(1, SST_2D_COLORFORE, color);
		FifoWriteLong(1, SST_2D_DSTSIZE, (list[2] + 1) | ( 1 << 16));
		FifoWriteLong(1, SST_2D_DSTXY, (list[1]) | ((list[0]) << 16));
		FifoWriteLong(1, SST_2D_COMMAND, (SSTG_RECTFILL | SSTG_GO | SSTG_ROP_SRCCOPY));

		list += 3;
	}	
	ci->PrimitivesIssued++;
	WriteSerialNumber(ci->PrimitivesIssued);
}

void WriteSerialNumber(uint32 serial_num)
{
	uint32 packet_buffer[32];

	// Write a 3D NOP Command
	packet_buffer[0] = (0x1 << SSTCP_PKT4_MASK_SHIFT) | 
										 	((((uint32)offsetof(regs_3d, nopCMD)) << 1)) | 
                      SSTCP_PKT4;
	packet_buffer[1] = 0;
  FifoAllocateSlots(1, 2);
	FifoWriteLong(1, 0 /* unused */, packet_buffer[0]);
	FifoWriteLong(1, 0 /* unused */, packet_buffer[1]);

	// Write a 2D NOP command to ensure that everything is flushed before the sentinal
	// area is written.
  FifoAllocateSlots(1, 2);
  SET_PKT2_HEADER(SSTCP_COMMAND);
  FifoWriteLong(1, SST_2D_COMMAND, SSTG_NOP|SSTG_GO);

	// Write the PKT5 command to write the serial number
  packet_buffer[0] = (SSTCP_PKT5_LFB | 
                      (1 /*32 bit word */ << SSTCP_PKT5_NWORDS_SHIFT) | 
                      SSTCP_PKT5); 
  packet_buffer[1] = ((uint32)((uint32) (ci->AllocSentinal->address) - (uint32)(ci->BaseAddr1))) & SSTCP_PKT5_BASEADDR; 
	packet_buffer[2] = serial_num;
	
	FifoAllocateSlots(1, 3);
	FifoWriteLong(1, 0 /* unused */, packet_buffer[0]);
	FifoWriteLong(1, 0 /* unused */, packet_buffer[1]);
	FifoWriteLong(1, 0 /* unused */, packet_buffer[2]);
}

/*  FIXME:  Write something useful here...  */
status_t
AccelInit (register struct thdfx_card_info *ci)
{
	return (B_OK);
}

static image_id accelerator_image;
static int32 initCount;

status_t Init3DEngine( char *device_path, void *context )
{
	status_t (*init)( void *gc, char *dp ) = 0;

dprintf(( "3dfx_accel:  Init3DEngine \n" ));
	if( !initCount )
	{
		accelerator_image = load_add_on( "_CPU_accelerants/libVoodoo4" );
		if( accelerator_image < B_OK )
		{
			return B_ERROR;
		}
		else
		{
			if( get_image_symbol( accelerator_image, "_AcceleratorInit", B_SYMBOL_TYPE_TEXT, (void **)&init ) <  B_OK )
			{
				unload_add_on( accelerator_image );
				return B_ERROR;
			}
		}
		initCount++;
	}

	if( (*init)( device_path, context ) >= B_OK )
	{
		return B_OK;
	}

	initCount--;
	if( !initCount )
		unload_add_on( accelerator_image );

	return B_ERROR;
}

status_t Shutdown3DEngine( char *device_path, void *context )
{
	dprintf(( "3dfx_accel:  Shutdown3DEngine \n" ));
	
	initCount--;
	if( !initCount )
		unload_add_on( accelerator_image );
	return B_OK;
}

void get_device_info( char *device_path, const char **name, uint8 *depth, uint8 *stencil, uint8 *accum )
{
	*name = "3dfx Voodoo 4";
	*depth = 1;
	*stencil = 0;
	*accum = 0;
}

void get_3d_modes( char *device_path, void (* callback)(video_mode *modes), uint32 min_color, uint32 min_depth, uint32 min_stencil, uint32 min_accum )
{
	static int32 widths[5] = {640, 800, 1024, 1152, 1280};
	static int32 heights[5] = {480, 600, 768, 864, 1024};
	int32 ct;
	video_mode m;

	if( !((min_stencil < BGL_8_BIT) || (min_stencil == BGL_8_BIT) ))
		return;
	if( min_accum > BGL_NONE )
		return;
	if( !(((min_color & BGL_BIT_DEPTH_MASK) < BGL_8_BIT) ||
		((min_color & BGL_BIT_DEPTH_MASK) == BGL_16_BIT) ||
		((min_color & BGL_BIT_DEPTH_MASK) == BGL_32_BIT)) )
		return;
	if( !((min_depth < BGL_8_BIT) || (min_depth == BGL_16_BIT) || (min_depth == BGL_24_BIT)) )
		return;
	if( ((min_color & BGL_BIT_DEPTH_MASK) == BGL_32_BIT ) && (min_depth == BGL_16_BIT) )
		return;
	if( ((min_color & BGL_BIT_DEPTH_MASK) == BGL_16_BIT ) && (min_stencil != BGL_NONE) )
		return;
	
	//The user asked for a color and depth mode we can do.
	
	if( min_stencil ||
		((min_color & BGL_BIT_DEPTH_MASK) == BGL_32_BIT) ||
		((min_color & BGL_BIT_DEPTH_MASK) == BGL_HIGH_QUALITY) ||
		((min_color & BGL_BIT_DEPTH_MASK) == BGL_ANY) ||
		(min_depth == BGL_24_BIT) ||
		(min_depth == BGL_HIGH_QUALITY) ||
		(min_depth == BGL_ANY) )
	{
		m.color = (min_color & BGL_COLOR_BUFFER_MASK) | BGL_32_BIT;
		if( (min_depth == BGL_NONE) && (min_stencil == BGL_NONE))
		{
			m.depth = BGL_NONE;
			m.stencil = BGL_NONE;
		}
		else
		{
			m.depth = BGL_24_BIT;
			m.stencil = BGL_8_BIT;
		}
	
		m.width = 0;
		m.height = 0;
		m.refresh = 0;
		m.accum = BGL_NONE;
		callback( &m );
		for( ct=0; ct<5; ct++ )
		{
			m.width = widths[ct];
			m.height = heights[ct];
			m.refresh = 60;
			callback( &m );
			m.refresh = 70;
			callback( &m );
			m.refresh = 75;
			callback( &m );
			m.refresh = 85;
			callback( &m );
		}
	}
	
	if( (min_stencil==BGL_NONE) && (
		((min_color & BGL_BIT_DEPTH_MASK) == BGL_16_BIT) ||
		((min_color & BGL_BIT_DEPTH_MASK) == BGL_FASTEST) ||
		((min_color & BGL_BIT_DEPTH_MASK) == BGL_ANY) ||
		(min_depth == BGL_16_BIT) ||
		(min_depth == BGL_FASTEST) ||
		(min_depth == BGL_ANY) ))
	{
		m.color = (min_color & BGL_COLOR_BUFFER_MASK) | BGL_16_BIT;
		if( min_depth == BGL_NONE )
		{
			m.depth = BGL_NONE;
		}
		else
		{
			m.depth = BGL_16_BIT;
		}
	
		m.width = 0;
		m.height = 0;
		m.refresh = 0;
		m.accum = BGL_NONE;
		m.stencil = BGL_NONE;
		callback( &m );
		for( ct=0; ct<5; ct++ )
		{
			m.width = widths[ct];
			m.height = heights[ct];
			m.refresh = 60;
			callback( &m );
			m.refresh = 70;
			callback( &m );
			m.refresh = 75;
			callback( &m );
			m.refresh = 85;
			callback( &m );
		}
	}
	
}



