#include "accelerant.h"
#include "driver_ioctl.h"
#include "sis620defs.h"

/*
 * ******** Hardware Cursor routines ********
 *
 * - Specific to SiS 630 chipset 
 * - Uses MMIO8500 registers
 *
 */
status_t	sis630_Set_Cursor_Shape(uint16 width, uint16 height, uint16 hotX, uint16 hotY,uchar *andMask, uchar *xorMask);
void 		sis630_Move_Cursor(int16 screenX,int16 screenY);
void		sis630_Show_Cursor(bool on);

static uint32 cursor1_state = 0 ;


status_t sis630_Set_Cursor_Shape(uint16 width, uint16 height, uint16 hotX, uint16 hotY,uchar *andMask, uchar *xorMask) {
	uchar t;
	uint32 i,j,k,cursor_address;
	uint8 *p ;
	 
	vddprintf(("sis630: SetCursorShape()\n"));

	cursor1_state &= ~0xffff ; // clear address
	cursor1_state |= ( ci->ci_Cursor_offset >> 10 ) ;
	
	// Cursor size is 64*64, 2 bits per pixel
	// so this means 1 kb
	
	// Data format is different from original SiS :
	// First half of the row describes xor mask ( inversion )
	// Second half of the row describes and mask ( color )
	
	p = (uint8*) ci->ci_Cursor_offset ;		// location in video memory
	p +=(uint32)ci->ci_BaseAddr0;			// mapped memory address
	p += 16*1024 - 1024 ;					// cursor address at the end of the reserved 16 kbytes
  
	// Draw the required cursor
	for(j=0;j<64;j++) {
		if (j==height) width=0;
		for(i=0;i<width/8;i++) *p++ = *andMask++ ;
		for(;i<64/8;i++)       *p++ = 0xff ;
		for(i=0;i<width/8;i++) *p++ = *xorMask++ ;
		for(;i<64/8;i++)       *p++ = 0x00 ;
		}
  	
	cursor1_state |= 0xf << 24 ; // pattern
 
	outl(0x8500, cursor1_state );
	outl(0x8504, 0x00ffffff ) ; // Color 0 RGB
	outl(0x8508, 0x00000000 ) ; // Color 1 RGB
	
	ci->ci_MouseHotX= hotX;
	ci->ci_MouseHotY= hotY;
  
	return(B_NO_ERROR);
	}

void sis630_Move_Cursor(int16 screenX,int16 screenY) {
	ci->ci_MousePosX = screenX - ci->ci_MouseHotX ;
	if ( ci->ci_MousePosX < 0 ) ci->ci_MousePosX = 0 ;

	ci->ci_MousePosY = screenY - ci->ci_MouseHotY ;
	if ( ci->ci_MousePosY < 0 ) ci->ci_MousePosY = 0;
	
	outl(0x850c, ci->ci_MousePosX & 0x0fff);
	outl(0x8510, ci->ci_MousePosY & 0x07ff);
	}

void sis630_Show_Cursor(bool on) {
	
	vddprintf(("sis630: CRT1 Cursor register (0x8500) was 0x%08x ",inl(0x8500)));

	if (on) {
		cursor1_state |= ( 1 << 30 ) ;
		}
	else {
		cursor1_state &= ~( 1 << 30 ) ;
		}
	
 cursor1_state |= 0xf << 24 ;

	outl( 0x8500, cursor1_state ) ;
	vddprintf(("and now is 0x%08x\n",inl(0x8500)));
	
	}
  
