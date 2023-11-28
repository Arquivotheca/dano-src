/******************************************************************************
 * initeng.c                                                                  *
 *   GUI Engine initialization and related functions, including:              *
 *        void Radeon_InitEngine (void)                                       *
 *        void Radeon_WaitForIdle (void)                                      *
 *        void Radeon_WaitForFifo (uint32 entries)                             *
 *        void Radeon_ResetEngine (void)                                      *
 *        void Radeon_FlushPixelCache (void)                                  *
 *                                                                            *
 * Copyright (c) 2000 ATI Technologies Inc.  All rights reserved.             *
 ******************************************************************************/

#include <OS.h>
#include <KernelExport.h>

#include <graphics_p/video_overlay.h>

#include <graphics_p/radeon/defines.h>
#include <graphics_p/radeon/main.h>
#include <graphics_p/radeon/regdef.h>
#include <graphics_p/radeon/CardState.h>
#include <graphics_p/radeon/radeon_ioctls.h>

#include <add-ons/graphics/Accelerant.h>
#include "proto.h"

// Globals
uint32 term_count = 0;

/****************************************************************************
 * Radeon_GetBPPValue                                                         *
 *  Function: Get destination bpp format value based on bpp.                *
 *    Inputs: bpp - bit per pixel count                                     *
 *   Outputs: destination bpp format value                                  *
 ****************************************************************************/

uint32 Radeon_GetBPPValue (uint16 bpp)
{
    switch (bpp)
    {
        case 8:     return (DST_8BPP);
                    break;
        case 15:    return (DST_15BPP);
                    break;
        case 16:    return (DST_16BPP);
                    break;
        case 32:    return (DST_32BPP);
                    break;
        default:    return (0);
                    break;
    }

    return (0);
}   // Radeon_GetBPPValue

void Radeon_InitEngine (CardInfo *ci)
{
    uint32 temp;
    int bpp;

//dprintf(( "Radeon_accel Radeon_InitEngine ci=%p \n", ci ));
    // ensure 3D is disabled
    WRITE_REG_32(RB3D_CNTL, 0);

    // do an Engine Reset, just in case it's hung right now
//dprintf(( "Radeon_accel Radeon_InitEngine 1\n" ));
    Radeon_ResetEngine (ci);

    // setup engine offset registers
//dprintf(( "Radeon_accel Radeon_InitEngine 2\n" ));
    Radeon_WaitForFifo (ci,1);

//dprintf(( "Radeon_accel Radeon_InitEngine 3\n" ));
    // setup engine pitch registers
    temp = READ_REG_32(DEFAULT_PITCH_OFFSET);
    WRITE_REG_32 (DEFAULT_PITCH_OFFSET,
         (temp & DEFAULT_PITCH_OFFSET__DEFAULT_TILE_MASK
       | (ci->FBStride << DEFAULT_PITCH_OFFSET__DEFAULT_PITCH__SHIFT)));
// recalculate crtc pitch (actually only required for 800x600x8)
    if (ci->bitpp == 15)
    	bpp = 16;
    else
        bpp = ci->bitpp;
       
//dprintf(( "Radeon_accel Radeon_InitEngine 4\n" ));
    temp = READ_REG_32(CRTC_PITCH) & 0x800;	//init crtc pitch value;
    temp = temp | (((ci->xres * ci->bytepp + 0x3f)
         & ~(0x3f)) / bpp);
    WRITE_REG_32(CRTC_PITCH, temp);

//dprintf(( "Radeon_accel Radeon_InitEngine 5\n" ));
    // set scissors to maximum size
    Radeon_WaitForFifo (ci,1);
//dprintf(( "Radeon_accel Radeon_InitEngine 6\n" ));
    WRITE_REG_32 (DEFAULT_SC_BOTTOM_RIGHT,
         (0x1FFF << DEFAULT_SC_BOTTOM_RIGHT__DEFAULT_SC_BOTTOM__SHIFT) | 0x1FFF);

    // Set the drawing controls registers.
//dprintf(( "Radeon_accel Radeon_InitEngine 7\n" ));
    Radeon_WaitForFifo (ci,1);
//dprintf(( "Radeon_accel Radeon_InitEngine 8\n" ));
    temp = Radeon_GetBPPValue (ci->bitpp);

    WRITE_REG_32 (DP_GUI_MASTER_CNTL, GMC_SRC_PITCH_OFFSET_DEFAULT |
                              GMC_DST_PITCH_OFFSET_DEFAULT |
                              GMC_SRC_CLIP_DEFAULT         |
                              GMC_DST_CLIP_DEFAULT         |
                              GMC_BRUSH_SOLIDCOLOR         |
                              (temp << DP_GUI_MASTER_CNTL__GMC_DST_DATATYPE__SHIFT) |
                              GMC_SRC_DSTCOLOR             |
                              GMC_BYTE_ORDER_MSB_TO_LSB    |
//                              GMC_DP_CONVERSION_TEMP_6500  |
                              ROP3_PATCOPY                 |
                              GMC_DP_SRC_RECT              |
                              GMC_DST_CLR_CMP_FCN_CLEAR    |
                              GMC_WRITE_MASK_SET);

//dprintf(( "Radeon_accel Radeon_InitEngine 9\n" ));
    Radeon_WaitForFifo (ci,7);

    // Clear the line drawing registers.
    WRITE_REG_32 (DST_LINE_START,0);
    WRITE_REG_32 (DST_LINE_END,0);

    // set brush color registers
    WRITE_REG_32 (DP_BRUSH_FRGD_CLR, 0xFFFFFFFF);
    WRITE_REG_32 (DP_BRUSH_BKGD_CLR, 0x00000000);

    // set source color registers
    WRITE_REG_32 (DP_SRC_FRGD_CLR, 0xFFFFFFFF);
    WRITE_REG_32 (DP_SRC_BKGD_CLR, 0x00000000);

    // default write mask
    WRITE_REG_32 (DP_WRITE_MSK, 0xFFFFFFFF);

//dprintf(( "Radeon_accel Radeon_InitEngine 10\n" ));
    // Wait for all the writes to be completed before returning
    Radeon_WaitForIdle (ci);

//dprintf(( "Radeon_accel Radeon_InitEngine 11\n" ));
    return;
} // Radeon_InitEngine ()


/******************************************************************************
 * Radeon_WaitForIdle - wait until engine active bit is idle.                   *
 *  Function: This function uses the DOS tick counter to serve as a           *
 *            timeout clock. If the engine is in a lockup condition,          *
 *            the busy bit may stay set. In this case, a timeout will         *
 *            occur, an error message will occur, and the program will        *
 *            terminate.                                                      *
 *    Inputs: NONE                                                            *
 *   Outputs: NONE                                                            *
 ******************************************************************************/
void Radeon_WaitForIdle (CardInfo *ci)
{
    bigtime_t starttick, endtick;

//dprintf(( "Radeon_accel Radeon_WaitForIdle ci=%p \n", ci ));
    // Insure FIFO is empty before waiting for engine idle.
    Radeon_WaitForFifo (ci,64);

    starttick = system_time();
    endtick = starttick;
    while ((READ_REG_32 (RBBM_STATUS) & GUI_ACTIVE) != ENGINE_IDLE)
    {
        endtick = system_time();
        if ( abs (endtick - starttick) > 1000000 )
        {
            // We should reset the engine at this point.
dprintf(( "Radeon_accel Radeon_WaitForIdle Engine reset \n" ));
            Radeon_ResetEngine (ci);
exit(-1);
        } // if
    } // while

    // flush the pixel cache
    Radeon_FlushPixelCache (ci);

    return;

} // Radeon_WaitForIdle ()


/******************************************************************************
 * Radeon_WaitForFifo - wait n empty FIFO entries.                            *
 *  Function: The FIFO contains upto 64 empty entries. The 'entries'          *
 *            value must be 1 to 64. This function implements the same        *
 *            timeout mechanism as the Radeon_WaitForIdle() function.         *
 *    Inputs: entries - number of entries spaces to wait for. Max - 64        *
 *   Outputs: NONE                                                            *
 ******************************************************************************/
void Radeon_WaitForFifo (CardInfo *ci, uint32 entries)
{
	bigtime_t starttick, endtick;

//dprintf(( "Radeon_accel Radeon_WaitForFifo ci=%p \n", ci ));
    starttick = system_time();

    while ((READ_REG_32 (RBBM_STATUS) & RBBM_STATUS__CMDFIFO_AVAIL_MASK) < entries)
    {
        endtick = system_time();
        if (abs (endtick - starttick) > 1000000 )
        {
            // we should reset the engine at this point.
dprintf(( "Radeon_accel Radeon_WaitForFifo Engine reset \n" ));
            Radeon_ResetEngine (ci);
exit(-1);
        } // if
    } // while
} // Radeon_WaitForFifo ()


/******************************************************************************
 * Radeon_ResetEngine - reset the Radeon GUI.                                 *
 *  Function: When the GUI becomes locked or hung, we must reset the engine   *
 *            to continue.  This involves resetting the engine clocks, and    *
 *            the engine itself.                                              *
 *    Inputs: NONE                                                            *
 *   Outputs: NONE                                                            *
 ******************************************************************************/
void Radeon_ResetEngine (CardInfo *ci)
{
    uint32 save_genresetcntl, save_clockcntlindex, save_mclkcntl;

    // Flush the pixel cache
    Radeon_FlushPixelCache (ci);

    save_clockcntlindex = READ_REG_32 (CLOCK_CNTL_INDEX);
    PLL_REGR (MCLK_CNTL, save_mclkcntl);

    // we must now force the engine clocks to active before
    // performing the engine reset.  We must turn them back on later...
    if( ci->isMobility )
    {
    	PLL_REGW (MCLK_CNTL, save_mclkcntl | 0x00060000);
	}
    else
    {
		PLL_REGW (MCLK_CNTL, save_mclkcntl | 0x000f0000);
	}

    // save GEN_RESET_CNTL register
    save_genresetcntl = READ_REG_32 (DISP_MISC_CNTL);

    // reset by setting bit, add read delay, then clear bit, add read delay
    WRITE_REG_32 (DISP_MISC_CNTL, save_genresetcntl |
          DISP_MISC_CNTL__SOFT_RESET_GRPH_PP);
    READ_REG_32 (DISP_MISC_CNTL);

    WRITE_REG_32 (DISP_MISC_CNTL, save_genresetcntl & 
        ~(DISP_MISC_CNTL__SOFT_RESET_GRPH_PP));
    READ_REG_32 (DISP_MISC_CNTL);

    // restore engine clocks
    PLL_REGW (MCLK_CNTL, save_mclkcntl);

    // restore the two registers we changed
    WRITE_REG_32 (CLOCK_CNTL_INDEX, save_clockcntlindex);
    WRITE_REG_32 (DISP_MISC_CNTL, save_genresetcntl);

    term_count++; // for monitoring engine hangs
    return;

} // Radeon_ResetEngine

/**********************************************************************************
 *  Radeon_FlushPixelCache ()                                                       *
 *                                                                                *
 *  This function is required when reading back video memory after an engine      *
 *  operation to insure all data was written to the video memory from the pixel   *
 *  cache.                                                                        *
 *                                                                                *
 **********************************************************************************/
void Radeon_FlushPixelCache (CardInfo *ci)
{
    uint16 i;

    // initiate flush
    WRITE_REG_32 (RB2D_DSTCACHE_CTLSTAT, READ_REG_32 (RB2D_DSTCACHE_CTLSTAT) | 0xf); 

    // check for completion but limit looping to 16384 reads
    i = 0;
    while (((READ_REG_32 (RB2D_DSTCACHE_CTLSTAT) & DC_BUSY) == DC_BUSY)
    && (i < 16384))
    {
        i++;
    }

    return;

} // Radeon_FlushPixelCache ()

