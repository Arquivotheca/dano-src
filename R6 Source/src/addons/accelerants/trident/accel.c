/* :ts=8 bk=0
 *
 * accel.c:	Basic rendering acceleration.
 *
 * $Id:$
 *
 * Leo L. Schwab					1999.10.12
 *  Based on I740 driver code.
 */
#include <dinky/bena4.h>

#include <graphics_p/trident/trident.h>
#include <graphics_p/trident/debug.h>

#include "protos.h"


/*****************************************************************************
 * #defines
 */
#define	ENGINEBUSY(status)			\
		((status) & (FIELDDEF (DDDG_STATUS, BRESENHAM, BUSY) |	\
			     FIELDDEF (DDDG_STATUS, SETUP, BUSY) |	\
			     FIELDDEF (DDDG_STATUS, SP_DPE, BUSY) |	\
			     FIELDDEF (DDDG_STATUS, MEMORYIF, BUSY) |	\
			     FIELDDEF (DDDG_STATUS, CMDLIST, BUSY) |	\
			     FIELDDEF (DDDG_STATUS, CMDBUF, FULL) |	\
			     FIELDDEF (DDDG_STATUS, PCIWRITEBUF, NOTEMPTY)))

#define	REPLICATE(x)  				\
	if (ci->ci_Depth < 32) {		\
		x |= x << 16;			\
		if (ci->ci_Depth <= 8)		\
			x |= x << 8;		\
	}


/*****************************************************************************
 * Local prototypes.
 */


/*****************************************************************************
 * Globals.
 */
extern tri_card_info	*ci;
extern tri_card_ctl	*cc;

static engine_token	enginetoken = {
	1, B_2D_ACCELERATION, NULL
};


/*****************************************************************************
 * Engine and sync_token management.
 */
uint32
getenginecount (void)
{
//dprintf ((">>> getenginecount()\n"));
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
//dprintf ((">>> acquireengine(et:0x%08x, st:0x%08x)\n", et, st));
	if (caps & B_3D_ACCELERATION)
		/*  No 3D acceleration yet.  */
		return (B_ERROR);

	BLockBena4 (&cc->cc_EngineLock);

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
	int32	old;

//dprintf((">>> releaseengine(et:0x%08x, st:0x%08x)\n", et, st));
	if (!et) {
//dprintf((">>> ackthp! stub_release_engine() called with null engine_token!\n"));
		return (B_ERROR);
	}
	/*  Update the sync token, if any  */
	if (st) {
//dprintf((">>> updating sync token - id: %d, count %Ld\n",
// et->engine_id, cc->cc_PrimitivesIssued));
		st->engine_id	= et->engine_id;
		st->counter	= cc->cc_PrimitivesIssued;
	}

	BUnlockBena4 (&cc->cc_EngineLock);
//dprintf ((">>> releaseengine() completes\n"));
	return (B_OK);
}

void
waitengineidle (void)
{
//dprintf((">>> waitengineidle()\n"));
	/*  FIXME:  Add something here...  */
	DDDGraphicsEngineRegs	*dddg;
	int			count = 0, timeout = 0;
	int			busy;

	dddg = ci->ci_DDDG;
	while (1) {
		if (!ENGINEBUSY (dddg->DDDG_Status))
			return;
		if (++count == 10000000) {
			dprintf ((">>> Trident BitBLT engine time-out.\n"));
			count = 9990000;
			if (++timeout == 8) {
				/*  Reset BitBLT Engine  */
				dddg->DDDG_Status = 0;
				return;
			}
		}
	}
}

status_t
getsynctoken (engine_token *et, sync_token *st)
{
//dprintf ((">>> getsynctoken(et:0x%08x, st:0x%08x)\n", et, st));
	if (et) {
		st->engine_id	= et->engine_id;
		st->counter	= cc->cc_PrimitivesIssued;
		return (B_OK);
	} else
		return (B_ERROR);
}

/*
 * This does The Cool Thing (!) with the sync_token values, and waits for the
 * requested primitive to complete, rather than waiting for the whole engine
 * to drop to idle.  Happily, the I740 has a feature that makes this almost
 * trivial to implement.
 */
status_t
synctotoken (sync_token *st)
{
	register int32	diff;

//dprintf ((">>> synctotoken(st: 0x%08x)\n", st));
	if (st->counter > cc->cc_PrimitivesCompleted) {
		/*  FIXME: Add something useful here.  */
		waitengineidle ();
		cc->cc_PrimitivesCompleted = st->counter;
	}

//dprintf ((">>> synctotoken() completes\n"));
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
	DDDGraphicsEngineRegs	*dddg;

	if (et != &enginetoken)
		return;

	dddg = ci->ci_DDDG;
	REPLICATE (color);
	
	dddg->DDDG_FGColor = color;
	dddg->DDDG_ROP = MINTERM_SRCCOPY;

	while (count--) {
//		while (dddg->DDDG_Status & (FIELDMASK (DDDG_STATUS, CMDBUF) |
//					    FIELDMASK (DDDG_STATUS, CMDLIST)))
//			/*  Wait for command buffer to be not full or busy.  */
//			;

		dddg->DDDG_DrawCmd = FIELDDEF (DDDG_DRAWCMD, OPCODE, LINE) |
				     FIELDDEF (DDDG_DRAWCMD, SRCCOLOR, OPAQUE) |
				     FIELDDEF (DDDG_DRAWCMD, ROP, ENABLE) |
				     FIELDDEF (DDDG_DRAWCMD, BLTSRC, CONST) |
				     FIELDVAL (DDDG_DRAWCMD, SRCSURFACE, 0) |
				     FIELDVAL (DDDG_DRAWCMD, DESTSURFACE, 0);

		dddg->DDDG_BltDestStart =
		 FIELDVAL (DDDG_BLTDESTSTART, X, list->left) |
		 FIELDVAL (DDDG_BLTDESTSTART, Y, list->top);
		dddg->DDDG_BltDestStop =
		 FIELDVAL (DDDG_BLTDESTSTOP, X, list->right) |
		 FIELDVAL (DDDG_BLTDESTSTOP, Y, list->bottom);

		waitengineidle ();

		list++;
	}
	cc->cc_PrimitivesIssued++;
}


void
blit (engine_token *et, register blit_params *list, register uint32 count)
{
	DDDGraphicsEngineRegs	*dddg;

	if (et != &enginetoken)
		return;

	dddg = ci->ci_DDDG;
	
	dddg->DDDG_ROP = MINTERM_SRCCOPY;

	while (count--) {
//		while (dddg->DDDG_Status & (FIELDMASK (DDDG_STATUS, CMDBUF) |
//					    FIELDMASK (DDDG_STATUS, CMDLIST)))
//			/*  Wait for command buffer to be not full or busy.  */
//			;

		if (list->dest_top > list->src_top  ||
		    (list->dest_top == list->src_top  &&
		     list->dest_left > list->src_left))
		{
			/*
			 * Copy backwards; bottom to top, right to left.
			 */
			dddg->DDDG_DrawCmd =
			 FIELDDEF (DDDG_DRAWCMD, OPCODE, TRAPEZOID) |
			 FIELDDEF (DDDG_DRAWCMD, SRCCOLOR, OPAQUE) |
			 FIELDDEF (DDDG_DRAWCMD, ROP, ENABLE) |
			 FIELDDEF (DDDG_DRAWCMD, BLTSRC, FB) |
			 FIELDVAL (DDDG_DRAWCMD, SRCSURFACE, 0) |
			 FIELDVAL (DDDG_DRAWCMD, DESTSURFACE, 0) |
			 FIELDDEF (DDDG_DRAWCMD, BLTDIR, DEC);

			dddg->DDDG_BltSrcStart =
			 FIELDVAL (DDDG_BLTSRCSTART, X,
				   list->src_left + list->width) |
			 FIELDVAL (DDDG_BLTSRCSTART, Y,
				   list->src_top + list->height);
			dddg->DDDG_BltSrcStop =
			 FIELDVAL (DDDG_BLTSRCSTOP, X, list->src_left) |
			 FIELDVAL (DDDG_BLTSRCSTOP, Y, list->src_top);
			dddg->DDDG_BltDestStart =
			 FIELDVAL (DDDG_BLTDESTSTART, X,
				   list->dest_left + list->width) |
			 FIELDVAL (DDDG_BLTDESTSTART, Y,
				   list->dest_top + list->height);
			dddg->DDDG_BltDestStop =
			 FIELDVAL (DDDG_BLTDESTSTOP, X, list->dest_left) |
			 FIELDVAL (DDDG_BLTDESTSTOP, Y, list->dest_top);
		} else {
			/*
			 * Copy forwards; top to bottom, left to right.
			 */
			dddg->DDDG_DrawCmd =
			 FIELDDEF (DDDG_DRAWCMD, OPCODE, TRAPEZOID) |
			 FIELDDEF (DDDG_DRAWCMD, SRCCOLOR, OPAQUE) |
			 FIELDDEF (DDDG_DRAWCMD, ROP, ENABLE) |
			 FIELDDEF (DDDG_DRAWCMD, BLTSRC, FB) |
			 FIELDVAL (DDDG_DRAWCMD, SRCSURFACE, 0) |
			 FIELDVAL (DDDG_DRAWCMD, DESTSURFACE, 0) |
			 FIELDDEF (DDDG_DRAWCMD, BLTDIR, INC);

			dddg->DDDG_BltSrcStart =
			 FIELDVAL (DDDG_BLTSRCSTART, X, list->src_left) |
			 FIELDVAL (DDDG_BLTSRCSTART, Y, list->src_top);
			dddg->DDDG_BltSrcStop =
			 FIELDVAL (DDDG_BLTSRCSTOP, X, list->src_left + list->width) |
			 FIELDVAL (DDDG_BLTSRCSTOP, Y, list->src_top + list->height);
			dddg->DDDG_BltDestStart =
			 FIELDVAL (DDDG_BLTDESTSTART, X, list->dest_left) |
			 FIELDVAL (DDDG_BLTDESTSTART, Y, list->dest_top);
			dddg->DDDG_BltDestStop =
			 FIELDVAL (DDDG_BLTDESTSTOP, X, list->dest_left + list->width) |
			 FIELDVAL (DDDG_BLTDESTSTOP, Y, list->dest_top + list->height);
		}

		waitengineidle ();

		list++;
	}
	cc->cc_PrimitivesIssued++;
}



status_t
AccelInit (register struct tri_card_info *ci)
{
	DDDGraphicsEngineRegs	*dddg;
	uint32			pitch;

	dddg = ci->ci_DDDG;

	pitch = FIELDVAL (DDDG_SRCDESTSTRIDEBUF, PITCH,
			  ci->ci_CurDispMode.virtual_width >> 3) |
		FIELDVAL (DDDG_SRCDESTSTRIDEBUF, BASEADDR,
			  ci->ci_FBBase >> 3);

	switch (ci->ci_Depth) {
	case 8:
		pitch |= FIELDDEF (DDDG_SRCDESTSTRIDEBUF, BPP, CMAP8);
		break;
	case 15:
		pitch |= FIELDDEF (DDDG_SRCDESTSTRIDEBUF, BPP, RGB555);
		break;
	case 16:
		pitch |= FIELDDEF (DDDG_SRCDESTSTRIDEBUF, BPP, RGB565);
		break;
	case 32:
		pitch |= FIELDDEF (DDDG_SRCDESTSTRIDEBUF, BPP, RGBx8888);
		break;
	}
	dddg->DDDG_SrcStrideBuf0 = pitch;
	dddg->DDDG_SrcStrideBuf1 = pitch;
	dddg->DDDG_SrcStrideBuf2 = pitch;
	dddg->DDDG_SrcStrideBuf3 = pitch;
	dddg->DDDG_DestStrideBuf0 = pitch;
	dddg->DDDG_DestStrideBuf1 = pitch;
	dddg->DDDG_DestStrideBuf2 = pitch;
	dddg->DDDG_DestStrideBuf3 = pitch;

	return (B_OK);
}
