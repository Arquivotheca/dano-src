#include <kernel/OS.h>
#include <support/Debug.h>
#include <add-ons/graphics/GraphicsCard.h>

#include <graphics_p/3dfx/banshee/banshee.h>
#include <graphics_p/3dfx/banshee/banshee_regs.h>
#include <graphics_p/3dfx/banshee/banshee_defs.h>
#include <graphics_p/3dfx/common/bena4.h>

#include "protos.h"

extern uint32 io_base_offset;
extern pci_module_info *pci_bus;
extern thdfx_card_info	*ci;

/*****************************************************************************
 * #defines
 */
#define	CURS_MAXX			32
#define	CURS_MAXY			CURS_MAXX


/*****************************************************************************
 * Code.
 */

status_t
_set_cursor_shape (
uint16	curswide,
uint16	curshigh,
uint16	hot_x,
uint16	hot_y,
uint8	*andbits,
uint8	*xorbits
)
{
	register int		x, y;
	register uint8		*curs;
	uint32				dstoff;
	int					sbpr;
	
	curs = ci->CursorBase;

	/*
	 * Layout of 64x64 2-bit AND/XOR cursors has been empirically
	 * discovered as follows:
	 *
	 * Memory
	 * Offsets	Function
	 * -----------	--------
	 * 0x00 - 0x07	Line 0, plane 0 (AND)
	 * 0x08 - 0x0f	Line 0, plane 1 (XOR)
	 * 0x10 - 0x17	Line 1, plane 0 (AND)
	 * 0x18 - 0x1f	Line 1, plane 1 (XOR)
	 * 0x20 - 0x27	Line 2, plane 0 (AND)
	 * 0x28 - 0x2f	Line 2, plane 1 (XOR)
	 * 0x30 - 0x37	Line 3, plane 0 (AND)
	 * 0x38 - 0x3f	Line 3, plane 1 (XOR)
	 *  etc.
	 *
	 * Pixels within the cursor area appear as follows:
	 *
	 * AND  XOR	Appearance
	 * ---  ---	----------
	 *  0    0	Solid color; hwCurC0
	 *  0    1	Solid color; hwCurC1
	 *  1    0	Transparent (current Screen Color)
	 *  1    1	Invert underlying framebuffer pixel (NOT Screen Color)
	 */

//	dprintf("3dfx: setcursorshape - ENTER\n");
//	dprintf("3dfx: setcursorshape - curs = 0x%x, curswide = %d, curshigh = %d, hot_x = %d, hot_y = %d\n", curs, curswide, curshigh, hot_x, hot_y);

	sbpr = 0;	
	for(y=0; y < 64; y++)
	{
		for(x=0; x < 8; x++)
		{
			if(x < ((curswide + 7) / 8) && y < curshigh)
			{
				curs[y * 16 + x    ] = *(andbits + sbpr);
				curs[y * 16 + x + 8] = *(xorbits + sbpr);
				sbpr++;
			}
			else
			{
				curs[y * 16 + x    ] = 0xff;
				curs[y * 16 + x + 8] = 0x00;
			}
		}
	}

	/*  Write white and black to extended palette hwCurC0 & hwCurC1  */
	_V3_WriteReg_NC( ci, V3_VID_CUR_C_0, (0xff << SST_CURSOR_RED_SHIFT) | (0xff << SST_CURSOR_GREEN_SHIFT) | (0xff << SST_CURSOR_BLUE_SHIFT));
	_V3_WriteReg_NC( ci, V3_VID_CUR_C_1, (0x00 << SST_CURSOR_RED_SHIFT) | (0x00 << SST_CURSOR_GREEN_SHIFT) | (0x00 << SST_CURSOR_BLUE_SHIFT));

	/*  Set cursor image address  */
	dstoff = (uint32) ci->CursorBase - (uint32) ci->BaseAddr1;
	_V3_WriteReg_NC( ci, V3_VID_CUR_PAT_ADDR, dstoff);

	ci->MouseHotX = hot_x;
	ci->MouseHotY = hot_y;

	/*  New hotpoint may move actual upper-left corner; reposition  */
	// Banshee Cursor locations refer to the bottom right of the cursor
	_V3_WriteReg_NC( ci, V3_VID_CUR_LOC, ((ci->MousePosX - ci->MouseHotX + 64) << SST_CURSOR_X_SHIFT) | ((ci->MousePosY - ci->MouseHotY + 64) << SST_CURSOR_Y_SHIFT));

//	dprintf("3dfx_accel: setcursorshape - EXIT\n");
	return (B_OK);
}

void _show_cursor (bool on)
{
	register uint32		tmp;

	tmp = ci->reg.r_V3_VID_PROC_CFG;
	
	if (on)
		tmp |= SST_CURSOR_EN;
	else
		tmp &= ~SST_CURSOR_EN;

	_V3_WriteReg_NC( ci, V3_VID_PROC_CFG, tmp );
}

void _move_cursor (int16 x, int16 y)
{
	uint32 tmp;
	
	ci->MousePosX = x-1;
	ci->MousePosY = y-1;

	atomic_or (&ci->IRQFlags, IRQF_MOVECURSOR);
	if (!(ci->IRQFlags & IRQF__ENABLED))
		_V3_WriteReg_NC( ci, V3_VID_CUR_LOC, ((ci->MousePosX - ci->MouseHotX + 64) << SST_CURSOR_X_SHIFT) | ((ci->MousePosY - ci->MouseHotY + 64) << SST_CURSOR_Y_SHIFT));
}

