//////////////////////////////////////////////////////////////////////////////
// Hardware Drawing
//
//    This file contains all hardware drawing functions required for the
// supported hardware.
//
// THIS FILE IS STILL UNDER DEVELOPMENT AND CONTAINS UNTESTED CODE
// COMMENTS STILL REQUIRE UPDATES. BrettHilder
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Includes //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <common_includes.h>
#include <accel_includes.h>
#include <GraphicsDefs.h>



// I have defined this macro to update ->engine.fifo_count
// during R128 command fifo command processes that are
// not specific to drawing routines. ( i.e. command queue management )
#define UPDATE_TICKS_QUE_MANAGE

/////////////////////////////////////////////////////
// PLL Register read macrox
// Macros to read and write indexed PLL registers
// in the ATI r128 register space.
#define PLL_ADDRESS(x)								\
	{	uint32 _ltmp;								\
		READ_REG(CLOCK_CNTL_INDEX, _ltmp);			\
		_ltmp = (_ltmp & 0x00000300) | ((x) & 0x1f);	\
		WRITE_REG(CLOCK_CNTL_INDEX, _ltmp);			\
	}
	
#define PLL_REGW(x,y)				\
	{	PLL_ADDRESS(x);				\
		WRITE_REG(CLOCK_CNTL_DATA,y);\
	}			

#define PLL_REGR(x,y)				\
	{	PLL_ADDRESS(x);				\
		READ_REG(CLOCK_CNTL_DATA,y);\
	}			
	

////////////////////////////////////////////////////////////
// HACK: FIFO timeouts.
// Be system_time() returns time in microseconds.
// Draw engine timeouts in ATI source are specified
// in DOS system timer clock ticks
// clock rate of 18.2 ticks per second
// 50 (ticks) * 0.055 seconds = 2.7 seconds. 
#define FIFO_TIMEOUT_MICROSEC 2700000


//////////////////////////////////////////////////////////////////////////////
// Prototypes /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void SCREEN_TO_SCREEN_BLIT(SHARED_INFO *si, engine_token *et, 
							blit_params *list, uint32 count);
void FILL_RECTANGLE(engine_token *et, uint32 colorIndex, 
							fill_rect_params *list, uint32 count);
void INVERT_RECTANGLE(engine_token *et, fill_rect_params *list, 
							uint32 count);
void FILL_SPAN(engine_token *et, uint32 colorIndex, uint16 *list, 
							uint32 count);
void DumpATI128StaticRegisters ( FILE * w, unsigned long int *AuxPtr, 
							int reg_count );
void AtiBitBlit(
			engine_token *et,
			SHARED_INFO *si,
			int16 src_x, int16 src_y,
			int16 src_width, int16 src_height,
			int16 dst_x, int16 dst_y );

//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Calculate Storage Bits Per Pixel
//    Utility function to calculate the number of storage bits per pixel
// given a colour space.
//    HACK - Move this into a utility file.

uint32 CalcBitsPerPixel(uint32 cs)
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


//////////////////////////////////////////////////////////
// ATI Engine Support Routines
// 
// Fifo synchronization, used by ENGINE_2D_ACTIVE()
// Code taken from ATI Prog Guide Section 4.2, and
// sample code CHAP3\initeng.c
//////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////
// void r128_WaitForIdle(void) 
// waits for the GUI command queue to become empty, 
// aswell as waits for the current GUI command to 
// complete.
void r128_WaitForIdle(void) 
{
	uint32	LTmp = 0;
	uchar	done = FALSE;			// end polling loop 
	uint32 	start_tick, 
			end_tick;	// timer time out vars
		
	ddprintf(("[R128 GFX]  r128_WaitForIdle() called.\n"));
		
	r128_WaitForFifo(64);	// wait until the command queue is empty
							// there are 64 command entries in queue.
	start_tick = end_tick = system_time();	// setup our timeout
	
	while(! done)
	{	READ_REG(GUI_STAT, LTmp);			// read status and test for complete
		done = (LTmp & GUI_ACTIVE) == ENGINE_IDLE;
	
		if ( !done )
		{	
			end_tick = system_time();		// get system time and 
											// compare against ATI timeout
			if( abs(end_tick - start_tick) > FIFO_TIMEOUT_MICROSEC )
			{	
				// give up and reset the gui engine.
				READ_REG(GUI_STAT, LTmp);	// dont know why they do this
				r128_ResetEngine();
				done = TRUE; // HACK break the loop if we reset.
			}
		}	
	}	
	// flush the pixel cache to ensure that all pending writes
	// to the frame buffer are complete.
	r128_FlushPixelCache();
}

///////////////////////////////////////////////////////////////////
// r128_WaitForFifo(int32 entries)
// waits for 'entries' number of entries to become available 
// in the GUI drawing queue, if there is a timeout, then
// the engine is reset.
void r128_WaitForFifo(uint32 entries) 
{
	uchar	done = FALSE;			// end polling loop
	uint32	LTmp = 0 ; 

	uint32 	start_tick, end_tick;	// timer time out vars
	start_tick = end_tick = system_time();

//	ddprintf(("[R128 GFX]  r128_WaitForFifo(%d) called : ", entries));
	
	while (!done)
	{
		READ_REG(GUI_STAT, LTmp);
		if((done = (LTmp & 0x00000FFF ) >= entries))
		{
		//	ddprintf(("Success\n"));
		} 
		else
		if ( !done )
		{			
			end_tick = system_time();		// get system time and 
											// compare against ATI timeout
			ddprintf(("."));				

			if( abs(end_tick - start_tick) > FIFO_TIMEOUT_MICROSEC )
			{	// give up and reset the gui engine.
				READ_REG( GUI_STAT, LTmp );
				ddprintf(("WaitForFifo Fail, GUI_STAT=%08x\n", LTmp));				
				r128_ResetEngine();
				done = TRUE ; // HACK break out of loop				
			}
		}						
	}
}

//////////////////////////////////////////////////////////////////////////////
// r128_FlushPixelCache(void)
// required when reading back video memory after an engine
// operation to insure all data was written to the video memory
// from the pixel cache
void r128_FlushPixelCache(void)
{
	uint32 i;
	uint32 LTmp = 0;
	uchar done = FALSE ;
//	ddprintf(("[R128 GFX]  r128_FlushPixelCache called.\n"));
	
	READ_REG(PC_NGUI_CTLSTAT, LTmp);		// initiate the flush
	LTmp |= 0x000000ff;
	WRITE_REG(PC_NGUI_CTLSTAT, LTmp );
	
	i = 0;
	while ( ! done  && i < 16384)		// reasonable timeout 
	{
		READ_REG(PC_NGUI_CTLSTAT, LTmp)
		done = (LTmp & PC_BUSY) != PC_BUSY;	// set done condition
		i++;
	}
	
}

/////////////////////////////////////////////////////////////////
// r128_ResetEngine(void)
// When the gui becomes locked or hung, we must reset the engine
// to continue.  This involves seeting the engine clocks, and
// the engine itself.
void r128_ResetEngine(void)
{
	uint32 save_genresetcntl = 0; 
	uint32 save_clockcntlindex = 0; 
	uint32 save_mclkcntl = 0; 
	uint32 LTmp = 0;
	
//  ddprintf(("[R128 GFX]  r128_resetEngine() called.\n"));
	
	r128_FlushPixelCache();		// flush the cache
	
	READ_REG(CLOCK_CNTL_INDEX, save_clockcntlindex);
	PLL_REGR(MCLK_CNTL, save_mclkcntl);
	
	// we must now force the engine clocks to active before
	// performing the engine reset. We must turn them back on later..
	
	PLL_REGW ( MCLK_CNTL, save_mclkcntl | 0x00030000);
	
	// save GEN_RESET_CNTL register
	READ_REG(GEN_RESET_CNTL, save_genresetcntl);
	
	// reset by setting bit, add read delay, then clear bit, add read delay
	WRITE_REG(GEN_RESET_CNTL, save_genresetcntl | SOFT_RESET_GUI);
	READ_REG( GEN_RESET_CNTL, LTmp);
	WRITE_REG(GEN_RESET_CNTL, save_genresetcntl & (~SOFT_RESET_GUI));
	READ_REG(GEN_RESET_CNTL, LTmp);
	
	// restore  engine clocks
	PLL_REGW( MCLK_CNTL, save_mclkcntl);
	
	// restore the two register we changed
	WRITE_REG(CLOCK_CNTL_INDEX, save_clockcntlindex);
	WRITE_REG(GEN_RESET_CNTL, save_genresetcntl);
	
}

///////////////////////////////////////////////////////////
// r128_GetBBPValue( uint32 bppvalue )
// Translates numeric bits per pixels, into ATi's 
// bits per pixels value.
uint32 r128_GetBBPValue ( uint32 bppvalue )
{
	switch(bppvalue)
	{
	case 8: 
		return (DST_8BPP); 
		break;
	case 15: 
		return (DST_15BPP); 
		break;
	case 16: 
		return (DST_16BPP); 
		break;
	case 24: 
		return (DST_24BPP); 
		break;
	case 32: 
		return (DST_32BPP); 
		break;
	default:
		return (0);		
	}
}

//////////////////////////////////////////////////////////////////////////////////
// r128_CalcPitch()
// this function calculates pitch for the current display mode
// 'si' is the current shared info,
// 'RoundTo' is the memory alignment requirement per scan line
//   16 in most cases other than block-write operations, where
//   it needs to be set to 128.
//
// NOTE : PITCH in ATI terminology is used to measure the size of memory for 
// representing a scan line of pixels.  
// 1. must be integer multiple of 8 pixels and padded up to next multiple of 8.
// 2. the memory size of must be a multiple of 16 ( in the case of BLOCK write
//    it must be multiples of 128 ).
//
uint32 r128_CalcPitch(SHARED_INFO * si, uint32 RoundTo)
{
	uint32 bppvalue;

	bppvalue = CalcBitsPerPixel(si->display.dm.space);	

	
	if( bppvalue == 24)
	{	return si->display.dm.virtual_width / 8 * 3;
	}
	else
	{	return si->display.dm.virtual_width / 8 ;
	}
	
#if 0
// NOTE: the following logic was required as per 
// ATi's documenation, but in reality it is not.
//  
	uint32 pitch, padded_pixels, calced_pixels;
	// round up the number of pixels on a line to a multiple of 8
	padded_pixels = si->display.dm.virtual_width + (8 - (si->display.dm.virtual_width % 8 )) %8;	
//	ddprintf(("   r128_CalcPitch(%ld) pad=%ld ",RoundTo, padded_pixels ));
	
	// round up the number of bytes per line to 16 or 128.
	calced_pixels = CalcBitsPerPixel(si->display.dm.space);
	pitch  =  calced_pixels * ( padded_pixels );
//	ddprintf(("calced_pixels = %ld,  pitch = %ld", calced_pixels, pitch));
	
	pitch  = pitch + (RoundTo - (pitch % RoundTo)) % RoundTo;
//	ddprintf((" pitch<rounded> = %ld", pitch));
	return pitch;
#endif

}	


//////////////////////////////////////////////////////////////////////////////
// r128_GetColourCode(color)
// returns ATi colour code, this function requires analysis
// 
uint32 r128_GetColourCode(uint32 color)
{
	return color;
}



///////////////////////////////////////////////////////
// r128_InitEngine()
// Resets and initializes the hardware drawing engine
// on the Rage128.  This logic was sourced from ati
// rage128  guide.
void r128_InitEngine(SHARED_INFO *si )
{
	uint32 temp, bppvalue, bpp_code;

	ddprintf(("[R128] r128_InitEngine\n"));


	// code fomr ATI initeng.c
	WRITE_REG(SCALE_3D_CNTL, 0x00000000); // insure 3d is disabled

	WRITE_REG(PC_NGUI_CTLSTAT, 0xff); // force immediate updates
	
	r128_ResetEngine(); // do an engine reset, just in case it is hung

	r128_WaitForFifo(1);				// setup engine offset registers
	WRITE_REG(DEFAULT_OFFSET,0x00000000);
//	WRITE_REG(DEFAULT_OFFSET,0x00000400);

	// calc bits per pixel and ATi's pitch ( bytes per line )
	bppvalue = CalcBitsPerPixel(si->display.dm.space);	

// this calculation needs to be qualified.	
	temp = r128_CalcPitch(si, 16);	
	
	
	if(bppvalue == 24)
	{	temp = temp * 3;
	}

	WRITE_REG(DEFAULT_PITCH,temp);		// setup engine pitch register
	
	// setup scissor registers to maximum dimensions
	// BAH confirm scissor usage and values with Chris
	r128_WaitForFifo(2);
	WRITE_REG(SC_TOP_LEFT, 0x00000000);
	WRITE_REG(DEFAULT_SC_BOTTOM_RIGHT, (0x1FFF << 16 | 0x1FFF));
	
#ifdef UPDATE_TICKS_QUE_MANAGE
	si->engine.fifo_count += 4;
#endif	


	bpp_code = r128_GetBBPValue (bppvalue );


	ddprintf(("   -bits per pixesl = %ld, temp(DefaultPitch) = %ld,  bpp_code = %04lx\n", bppvalue, temp, bpp_code));

	
	r128_WaitForFifo(1);
//	bppvalue = r128_GetBBPValue ( bppvalue );   // set the drawing control registers
	temp =
	    GMC_SRC_PITCH_OFFSET_DEFAULT |
	    GMC_DST_PITCH_OFFSET_DEFAULT |
		GMC_SRC_CLIP_DEFAULT 		|
		GMC_DST_CLIP_DEFAULT 		|
		GMC_BRUSH_SOLIDCOLOR 		|
		(bpp_code << 8) 			|
		GMC_SRC_DSTCOLOR			|
		GMC_BYTE_ORDER_MSB_TO_LSB	|
		GMC_DP_CONVERSION_TEMP_6500	|
		ROP3_PATCOPY				|
		GMC_DP_SRC_RECT				|
		GMC_3D_FCN_EN_CLR			|
		GMC_DST_CLR_CMP_FCN_CLEAR	|
		GMC_AUX_CLIP_CLEAR			|
		GMC_WRITE_MASK_SET ;		
	WRITE_REG( DP_GUI_MASTER_CNTL, temp);
	
	r128_WaitForFifo(8);			
	
	WRITE_REG( DST_BRES_ERR, 0 );	// clear the line drawing registers
	WRITE_REG( DST_BRES_INC, 0 );
	WRITE_REG( DST_BRES_DEC, 0 );

	WRITE_REG( DP_BRUSH_FRGD_CLR, 0xffffffff); // set brush colour regs
	WRITE_REG( DP_BRUSH_BKGD_CLR, 0x0);
		
	WRITE_REG( DP_SRC_FRGD_CLR, 0xffffffff);	// set source colour regs
	WRITE_REG( DP_SRC_BKGD_CLR, 0x0 );
	
	WRITE_REG( DP_WRITE_MASK, 0xffffffff);		// default write mask

#ifdef UPDATE_TICKS_QUE_MANAGE
	si->engine.fifo_count += 7;
#endif	

	r128_WaitForIdle();	// wait for the engine to become idle

	ddprintf(("  -Init engine done\n"));
	
}


//////////////////////////////////////////////////////////////////////
// FILL_RECTANGLE, simple implementation from ATI source code, nothing
// special here,  must test colour codes values, unsure of impact
//
void fill_a_rectangle(engine_token *et, uint32 colorIndex, 
		fill_rect_params *list, uint32 count, uint32 rop3_code)
{
	int32 height, width, bppvalue, bits_per_pixel;
	uint32 save_dp_mix = 0;
	uint32 save_dp_datatype = 0;
	uint32 tmp, tmp2, tmp3;
	int32 pitch;

//	bppvalue = CalcBitsPerPixel(si->display.dm.space);

	bits_per_pixel = CalcBitsPerPixel(si->display.dm.space);// get bits per pixel from Be	
	bppvalue = r128_GetBBPValue(bits_per_pixel);			// convert to ATi Bpp Code
	pitch = r128_CalcPitch(si, 8);

	
//	ddprintf(("[R128] fill_a_rectangle(%ld) sp=%04x  bpp=%04x  bpV=%04x pitch=%08lx \n",
//	     count, si->display.dm.space, bits_per_pixel, bppvalue, pitch));		
	
	r128_WaitForFifo(2);					// set for SRC, or !DST
	READ_REG(DP_MIX, save_dp_mix);			// save default DP_MIX
	READ_REG(DP_DATATYPE, save_dp_datatype);
	
	si->engine.fifo_count +=2;	// update engine count

	
	
//	r128_WaitForFifo(1);					// set for SRC, or !DST
///////
// danger zone
//	READ_REG(SCALE_3D_CNTL, save_scale_3d_cntl);
//	WRITE_REG(SCALE_3D_CNTL, 0x0);	
//	WRITE_REG(DP_MIX,  rop3_code | DP_SRC_RECT);
///////

	r128_WaitForFifo( 5);
	WRITE_REG(DST_OFFSET, 0 );
	WRITE_REG(SRC_OFFSET, 0 );
	WRITE_REG(DEFAULT_OFFSET, 0);
	
	WRITE_REG(DST_PITCH, pitch);
	WRITE_REG(SRC_PITCH, pitch );
	si->engine.fifo_count +=5;	// update engine count

//
// TO DO : program DP_GUI_MASTER_CNTL
//
	si->engine.fifo_count +=1;
	while (count --)
	{
		if (list->right < list->left)
		{
			tmp = list->right;
			list->right = list->left;
			list->left = tmp;
		}
		width = list->right - list->left +1;						// width, height
		height = list->bottom - list->top +1;

//		ddprintf(("  l:%d  t:%d  w:%d  h:%d\n", list->left, list->top, width, height ) );

		r128_WaitForFifo(1);		
//		bppvalue = r128_GetBBPValue (bppvalue );			// setup DP_DATATYPE for a solid colur brush, and to proper
		tmp = BRUSH_SOLIDCOLOR | bppvalue | rop3_code;
		WRITE_REG(DP_DATATYPE, tmp ); // colour depth.
		si->engine.fifo_count +=1;

	// set gemoetry of fill area.	
		r128_WaitForFifo(4);
		WRITE_REG(DP_BRUSH_FRGD_CLR, colorIndex); //colorIndex);
		WRITE_REG(DP_BRUSH_BKGD_CLR, colorIndex); //colorIndex);

		tmp2 = ((list->left << 16 ) | list->top) ;
		WRITE_REG(DST_X_Y, tmp2 );		// draw a filled pattern

		tmp3 = ((height << 16 ) |  width);
		WRITE_REG(DST_HEIGHT_WIDTH, tmp3);

//		ddprintf(("   rectangle XY:%08lx HW:%08lx  r:%04x l:%04x t:%04x b:%04x (w%08lx) (h%08lx) \n", 
//				tmp2, tmp3, list->right, list->left, list->top, list->bottom, width, height));

		si->engine.fifo_count +=4;
		
		list++;						// process next rectangle
	}

	r128_WaitForFifo(2);			// restore DP_MIX value
	WRITE_REG(DP_MIX, save_dp_mix);
	WRITE_REG(DP_DATATYPE, save_dp_datatype);
	
	si->engine.fifo_count+=2;
}



////////////////////////////////////////////////////////////////////////////
/// Accelerant Functions called from Accelerant Hooks
////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// This is called directly as the appropriate hook function. Make sure that
// it updates si->engine.fifo_count appropriately.
void ScreenToScreenBlit(engine_token *et, blit_params *list, uint32 count)
{

  uint32 bpp_code;				// bit per pixel value
  uint32 save_dp_datatype = 0;	// saved r128 registers
  uint32 save_dp_cntl  = 0;
  uint32 save_scale_3d_cntl = 0;
  uint32 save_dp_mix = 0;

  blit_params *bp = list;		// pointer to current blit retangle, & dest

  // localized blit coordinants
  int16 s_left  ;	 			// Source origin
  int16 s_top ; 
  int16 d_left ;				// Destination origin 
  int16 d_top ; 
  int16 width ;					// size of blit rectangle
  int16 height ; 
  int16 xdir ;   				// direction of blit
  int16 ydir ; 

  //  debug_ptr_go = TRUE ; 	// set debug control flag to on. 
    
  // Tattletale.
  ddprintf(("[R128 GFX]  ScreenToScreenBlit( %ld) called.\n",count));

	// save registers
	r128_WaitForFifo(4);
	
	READ_REG(SCALE_3D_CNTL, save_scale_3d_cntl);
	READ_REG(DP_DATATYPE, save_dp_datatype);
	READ_REG(DP_CNTL, save_dp_cntl);
	READ_REG(DP_MIX, save_dp_mix);

	si->engine.fifo_count +=4;


	// pixel density
	bpp_code = CalcBitsPerPixel(si->display.dm.space);// get bits per pixel from Be	
	bpp_code = r128_GetBBPValue(bpp_code);			// convert to ATi Bpp Code

	while (count--)  		// loop for all Blits
	{
		s_left = bp->src_left;	// set source origin
		s_top = bp->src_top;
		d_left = bp->dest_left;	// set destination
		d_top = bp->dest_top;
		width = bp->width + 1 ;	// set size
		height = bp->height + 1;
		xdir = 1;	// default movement : left to right
		ydir = 1;
	
		// adjust direction of blitting, and set respective offsets 
		if(s_left < d_left )
		{	xdir = 0;		// right to left
			s_left += (width - 1);
			d_left += (width -1);		
		}
		if(s_top < d_top)
		{	ydir = 0;
			s_top += (height -1);
			d_top += (height -1);
		}
					
	// set DP_DATATYPE for SRCCOPY, current pixel depth, src=dst
	// brush setting does not matter
	
		r128_WaitForFifo(6);

		WRITE_REG(DP_DATATYPE, (save_dp_datatype & ~0x30F0F) | bpp_code | SRC_DSTCOLOR | BRUSH_SOLIDCOLOR  );
	
		// set DP_MIX to SRCCOPY, rectangular source
		WRITE_REG(DP_MIX, ROP3_SRCCOPY | DP_SRC_RECT);
	
		// set DP_CNTL for left to right, top to bottom respective of the blit dir
		WRITE_REG(DP_CNTL, (save_dp_cntl & (~0x03)) |  ydir << 1 | xdir );
	
		// set the source and destination x and y values
		WRITE_REG(SRC_Y_X, (s_top << 16) | s_left);
		WRITE_REG(DST_Y_X, (d_top << 16) | d_left);
	
		// perform the blit
		WRITE_REG(DST_HEIGHT_WIDTH, (height << 16) | width);
		
		si->engine.fifo_count +=6;	// update engine count

		bp ++;	// point to next blit
	
		// HACK : wait for card to complete, crashes without this.
		// 
		r128_WaitForIdle() ;				
	}

	// restore registers
	r128_WaitForFifo(4);
	WRITE_REG(SCALE_3D_CNTL, save_scale_3d_cntl);
	WRITE_REG(DP_DATATYPE, save_dp_datatype);
	WRITE_REG(DP_CNTL, save_dp_cntl);
	WRITE_REG(DP_MIX, save_dp_mix);

	si->engine.fifo_count +=4;	// update engine count


}

//////////////////////////////////////////////////////////////////////////////
// This is called directly as the appropriate hook function. Make sure that
// it updates si->engine.fifo_count appropriately.

void RectangleFill(engine_token *et, uint32 color, fill_rect_params *list,
  uint32 count)
{

  // Tattletale.
//  ddprintf(("[R128 GFX]  RectangleFill() called.\n"));
  
  fill_a_rectangle(et, color, list, count, ROP3_SRCCOPY);
  
}

// debug global
// 
uint32 rop_count =  0;

uint32 rop_codes[] = {
 ROP3_BLACKNESS,
 ROP3_NOTSRCERASE,
 ROP3_NOTSRCCOPY,
 ROP3_SRCERASE,
 ROP3_DSTINVERT,
 ROP3_PATINVERT,
 ROP3_SRCINVERT,
 ROP3_SRCAND,
 ROP3_DSTCOPY,
 ROP3_MERGEPAINT,
 ROP3_MERGECOPY,
 ROP3_SRCCOPY,
 ROP3_SRCPAINT,
 ROP3_PATCOPY,
 ROP3_PATPAINT,
 ROP3_WHITENESS
};

//////////////////////////////////////////////////////////////////////////////
// This is called directly as the appropriate hook function. Make sure that
// it updates si->engine.fifo_count appropriately.
void RectangleInvert(engine_token *et, fill_rect_params *list, uint32 count)
{
	//debug FILE * w;
	uint32 rop_code = 0;	
	
  // Tattletale.
    ddprintf(("[R128 GFX]  RectangleInvert(%ld = %08lx) called.\n", rop_count, rop_code));


	// HACK : Try all ROP codes.
	rop_count ++;
	if(rop_count > (sizeof(rop_codes) / sizeof (rop_codes[0])) )
	{	rop_count = 0;
	}
	rop_code = rop_codes [rop_count];


  // fill the rectangles with the destination (ALU) inverted 

// last experimental  
//  fill_a_rectangle(et, 0x55005500, list, count, ROP3_DSTINVERT); // ROP = !DST
//  fill_a_rectangle(et,   0xFFFFFFFF, list, count, ROP3_DSTINVERT); // ROP = !DST

  fill_a_rectangle(et,   0xFFFFFFFF, list, count, rop_code); // ROP = !DST

//    RectangleFill(et, 0x222, list, 1);

#if 0
	w = fopen ("/boot/home/wdump.txt","w");
	if(w) 
	{	DumpATI128StaticRegisters ( w, (uint32*)si->card.regs, 0 );
		fclose(w);
	}
#endif

}


//////////////////////////////////////////////////////////////////////////////
// This is called directly as the appropriate hook function. Make sure that
// it updates si->engine.fifo_count appropriately.

void SpanFill(engine_token *et, uint32 color, uint16 *list, uint32 count)
{

	fill_rect_params dlist;	// make a span into a rectangle one pixel high

	// for each span line, draw a single rectangle with 
	// height of one.
	while (count--) 
	{	dlist.top = dlist.bottom = *list++;
		dlist.left = *list++;
		dlist.right = *list++;

		fill_a_rectangle(et, color, &dlist, 1, ROP3_SRCCOPY); 
	}
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////



