/* 
 *
 * cursor.c:	Hardware cursor manipulation.
 */
#include <kernel/OS.h>
#include <support/Debug.h>
#include <add-ons/graphics/GraphicsCard.h>

#include <graphics_p/neomagic/neomagic.h>
#include <graphics_p/neomagic/bena4.h>
#include <graphics_p/neomagic/debug.h>

#include "protos.h"

extern pci_module_info *pci_bus;
extern neomagic_card_info	*ci;

/*****************************************************************************
 * #defines
 */
#define	CURS_MAXX			32
#define	CURS_MAXY			CURS_MAXX


/*****************************************************************************
 * Code.
 */

status_t
_set_cursor_shape (uint16	curswide, uint16	curshigh, uint16	hot_x, uint16	hot_y, uint8	*andbits,uint8	*xorbits)
{
	int     i, j, k,curswide0;
	bool    init;
	uchar   *buf;
	
	curswide0 = (curswide+7)>>3;

	ci->ci_MouseHotX = hot_x;
	ci->ci_MouseHotY = hot_y;

	for (k=0x7f0; k<0x800; k++)
	{
		buf = ((uchar*)ci->ci_BaseAddr0) + 1024 * k;
		/* Clear a 1K Block */
		for (i=0;i<1024;i++) 
		  buf[i] = 0x00;	
		
		for (i=0;i<curshigh;i++)
		{
		  for (j=0;j<curswide0;j++)
		  {
				buf[i*16+j] = xorbits[j+i*curswide0];
				buf[i*16+j+8] = (andbits[j+i*curswide0]^0xff);
		  }
		}
		buf += 1024;
	}

	// Cursor Start location in 1K Pages
	write_vga_grax(NEO_HWCURSORMEMLOC, 0x7f);
	write_vga_grax(NEO_HWICONMEMLOC, 0x00);

	// BeOS Cursors are White on Black
	write_vga_grax(NEO_HWCURSORBKGNDRED, 0x00);
	write_vga_grax(NEO_HWCURSORBKGNDGREEN, 0x00);
	write_vga_grax(NEO_HWCURSORBKGNDBLUE, 0x00);
	write_vga_grax(NEO_HWCURSORFGNDRED, 0xff);
	write_vga_grax(NEO_HWCURSORFGNDGREEN, 0xff);
	write_vga_grax(NEO_HWCURSORFGNDBLUE, 0xff);
	return (B_OK);
}

void
_show_cursor (bool on)
{
	if (on)
		write_vga_grax(NEO_HWCURSORCNTL, 1);
	else
		write_vga_grax(NEO_HWCURSORCNTL, 0);

}

void
_move_cursor (int16 x, int16 y)
{
	uint32 tmp;
	
	ci->ci_MousePosX = x - ci->ci_MouseHotX;
	ci->ci_MousePosY = y - ci->ci_MouseHotY;

	if (ci->ci_MousePosX < 0)
		ci->ci_MousePosX = 0;

	if (ci->ci_MousePosY < 0)
		ci->ci_MousePosY = 0;

 	write_vga_grax(NEO_CURSORXLOW, (ci->ci_MousePosX & 0xff));
 	write_vga_grax(NEO_CURSORXHIGH, (ci->ci_MousePosX >> 8));
 	write_vga_grax(NEO_CURSORYLOW, (ci->ci_MousePosY & 0xff));
 	write_vga_grax(NEO_CURSORYHIGH, (ci->ci_MousePosY >> 8));
}

