//////////////////////////////////////////////////////////////////////////////
// Cursor Manipulation
//
// FILE : r128_cursor.c
// DESC :
//    This file contains all cursor manipulation functions required for the
// supported hardware.
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
// Update the Cursor Position
//    This function updates the actual cursor position registers.
// HACK - Revise this to pass physical position arguments here, for sanity's
// sake.
// HACK - test for interlaced lines in regard to setting the cursor image.

//void UpdateCursorPosition(void)
void UpdateCursorPosition(int32 image_x, int32 image_y)
{
//	int16 cur_x = image_x; 	
	int16 offset_x = 0;				// set default offset
	int16 offset_y = 0;
	
	int16 cur_x = image_x -  64;	// normalize coords down to 0
	int16 cur_y = image_y -  64;	

	int32 cursor_offset ;

	// find location of the cursor image in framebuffer
	// place it ten lines after the end of the frame buffer
	cursor_offset = si->display.fbc.bytes_per_row * 
		(si->display.dm.virtual_height + 20 );
	
	// round up to 128 byte boarder
	cursor_offset = cursor_offset + (128  - (cursor_offset % 128));				

	if (cur_x < 0 )
	{	offset_x = - cur_x;
		cur_x = 0;		
	}

	if (cur_y  < 0 )
	{	offset_y = - cur_y ;
		cur_y = 0;	
	}

	// set offset
	WRITE_REG(CUR_HORZ_VERT_OFF, ( offset_x << 16 ) | offset_y );

	// set cursor offset to cursor data region within frame buffer
	WRITE_REG(CUR_OFFSET, cursor_offset + (offset_y * 16));

	// update the position of the hardware cursor.
    WRITE_REG(CUR_HORZ_VERT_POSN, ((cur_x ) << 16) | (cur_y )   );
  	
}


//////////////////////////////////////////////////////////////////////////////
// Show the Cursor
//    This function either displays or hides the cursor, as indicated by the
// is_visible flag.

void ShowCursor(bool is_visible)
{
	uint32 LTmp;
  // Tattletale.
  ddprintf(("[R128 GFX]  ShowCursor(%s) called.\n", 
  				is_visible? "TRUE" : "FALSE"));
	
	READ_REG(CRTC_GEN_CNTL,LTmp);
	if (is_visible)
	{	LTmp |= 0x00010000 ; 
	}
	else
	{	LTmp &= ~(0x00010000);
	}	
	WRITE_REG(CRTC_GEN_CNTL,LTmp);
}


//////////////////////////////////////////////////////////////////////////////
// Set the Cursor Image
//    This function redefines the cursor shape to the 64x64 image indicated
// by the specified masks.


// set offset within cursor to '0', since 'Be' uses 64x64 pixel cursor
#define BE_CUR_HORZ_OFS 0	// horizontal offset of cursor	
#define BE_CUR_VERT_OFS 0	// vertical offset of cursor
#define FB_CUR_OFFSET 0 	// byte offset in framebuffer of the cursor.
#define FB_CUR_BASE ((uchar*)si->card.fb_base + FB_CUR_OFFSET)
	// base address of the cursor in memory.


void r128_UpdateCursor(SHARED_INFO *si);

void do_set_cursor_image(uint8 *cursor0, uint8 *cursor1)
{

// overview : the byte arrays cursor0 (XOR) and cursor1 (AND) are
// the XOR mask and the AND mask respectively.   They
// have been prepared prior to calling do_set_cursor_image

// the data are copied to the cursor location in 
// frame buffer memory, and location of the cursor
// in memory is set.

//#define CURSOR_TEMPLATE   // diagnostic


	uint8  *fp_cur_ptr;		// pointer to frame buffer
	uint16	iY;				// index counter
	uint32  cursor_offset;	// offset within the frame buffer
	
	uint32 LTmp;			// used to read visible status of cursor
	uint8   was_visible;	// to determine blanking requiremnt


  // Tattletale.
  ddprintf(("[R128 GFX]  do_set_cursor_image(%d,%d), ofs(%d,%d) called.\n", si->cursor.cursor_x, si->cursor.cursor_y, si->cursor.hot_x, si->cursor.hot_y));

	READ_REG(CRTC_GEN_CNTL,LTmp);
	was_visible = (LTmp & 0x00010000) != 0 ;


	if(was_visible)	
	{   ddprintf((" >>>> "));
		ShowCursor( FALSE ); // blank the cursor
	}
	// location of the cursor image in framebuffer
	// place it ten lines after the end of the frame buffer
	cursor_offset = si->display.fbc.bytes_per_row * 
		(si->display.dm.virtual_height + 20 );
	
	// round up to 128 byte boarder
	cursor_offset = cursor_offset + (128  - (cursor_offset % 128));		
	fp_cur_ptr = (uint8*)FB_CUR_BASE + cursor_offset;	// set porinter to cursor pixels
	

	for (iY = 0; iY < 64 ; iY ++)
	{
		memcpy( &fp_cur_ptr[iY*16], &cursor1[iY * 8], 8);	// first 8 bytes of a cursor pixel row
			// copy AND mask to framebuffer memory
						
		memcpy( &fp_cur_ptr[iY*16+8], &cursor0[iY * 8], 8);	// second 8 bytes of the cursor pizel row
			// copy XOR mask to framebuffer memory
			
		#ifdef CURSOR_TEMPLATE
		// diagnostic, adds alignment lines to cursor		
			fp_cur_ptr[iY*16 ]    |= 0x80;
			fp_cur_ptr[iY*16 + 8] |= 0x80;
			fp_cur_ptr[iY*16 + 7] |= 0x01;
			fp_cur_ptr[iY*16 +15] |= 0x01;			
		#endif			
	}

	// set the cursor colour
	WRITE_REG(CUR_CLR1, 0x0); 			// set foreground colour
	WRITE_REG(CUR_CLR0, 0xFFFFFFFF);    // set background colour
	
	// set cursor offset to cursor data region within frame buffer
	WRITE_REG(CUR_OFFSET, cursor_offset );

	WRITE_REG(CUR_HORZ_VERT_OFF, ( BE_CUR_HORZ_OFS << 16 ) | BE_CUR_VERT_OFS );

	if(was_visible)	
	{	   ddprintf((" <<< "));
			ShowCursor( TRUE ); // show the cursor	
	}			
}	


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////



