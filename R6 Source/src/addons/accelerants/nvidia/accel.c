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

#include <graphics_p/nvidia/nvidia.h>
#include <graphics_p/nvidia/debug.h>

#include "protos.h"


/*****************************************************************************
 * #defines
 */
#define	MINTERM_DESTINV	(MT_PSND | MT_PNSND | MT_NPSND | MT_NPNSND)


/*****************************************************************************
 * Local prototypes.
 */


/*****************************************************************************
 * Globals.
 */
extern gfx_card_info	*ci;
extern gfx_card_ctl	*cc;

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
	while (ci->ci_HWInst.Rop->FifoFree < ci->ci_HWInst.FifoEmptyCount  ||
	       (*ci->ci_HWInst.BUSYREG & ci->ci_HWInst.BusyBit))
		;
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

status_t
synctotoken (sync_token *st)
{
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
static void
setcliprect (uint16 x1, uint16 y1, uint16 x2, uint16 y2)
{
	int height = y2-y1 + 1;
	int width  = x2-x1 + 1;
	
	RIVA_FIFO_FREE (ci->ci_HWInst, Clip, 2);
	ci->ci_HWInst.Clip->TopLeft     = (y1     << 16) | (x1 & 0xffff);
	ci->ci_HWInst.Clip->WidthHeight = (height << 16) | width;
}

static void
setpattern (uint32 clr0, uint32 clr1, uint32 pat0, uint32 pat1)
{
	RIVA_FIFO_FREE (ci->ci_HWInst, Patt, 5);
	ci->ci_HWInst.Patt->Shape         = 0; /* 0 = 8X8, 1 = 64X1, 2 = 1X64 */
	ci->ci_HWInst.Patt->Color0        = clr0;
	ci->ci_HWInst.Patt->Color1        = clr1;
	ci->ci_HWInst.Patt->Monochrome[0] = pat0;
	ci->ci_HWInst.Patt->Monochrome[1] = pat1;
}

static void
setrop (uint8 rop)
{
	if (cc->cc_CurROP != rop) {
		cc->cc_CurROP = rop;
		RIVA_FIFO_FREE (ci->ci_HWInst, Rop, 1);
		ci->ci_HWInst.Rop->Rop3 = rop;
	}
}



/*
 * Handles 8-, 15-, 16-, and 32-bit modes.
 */
static void
rectfill_gen (
uint32				color,
uint32				minterm,
register fill_rect_params	*list,
register uint32			count
)
{
	register uint16	w, h;

	setrop (minterm);
	RIVA_FIFO_FREE (ci->ci_HWInst, Bitmap, 1);
	ci->ci_HWInst.Bitmap->Color1A = color;

	while (count--) {
		w = list->right - list->left + 1;
		h = list->bottom - list->top + 1;
		
		RIVA_FIFO_FREE (ci->ci_HWInst, Bitmap, 2);
		ci->ci_HWInst.Bitmap->UnclippedRectangle[0].TopLeft     =
		 (list->left << 16) | list->top;
		ci->ci_HWInst.Bitmap->UnclippedRectangle[0].WidthHeight =
		 (w << 16) | h;

		list++;
	}
	cc->cc_PrimitivesIssued++;
}

void
rectfill (
engine_token			*et,
register uint32			color,
register fill_rect_params	*list,
register uint32			count
)
{
	if (et != &enginetoken)
		return;

	rectfill_gen (color, MINTERM_SRCCOPY, list, count);
}

void
rectinvert (
engine_token			*et,
register fill_rect_params	*list,
register uint32			count
)
{
	if (et != &enginetoken)
		return;

	rectfill_gen (0, MINTERM_DESTINV, list, count);
}


void
blit (engine_token *et, register blit_params *list, register uint32 count)
{
	if (et != &enginetoken)
		return;

	setrop (MINTERM_SRCCOPY);

	while (count--) {
		RIVA_FIFO_FREE (ci->ci_HWInst, Blt, 3);
		ci->ci_HWInst.Blt->TopLeftSrc  =
		 (list->src_top      << 16) | list->src_left;
		ci->ci_HWInst.Blt->TopLeftDst  =
		 (list->dest_top     << 16) | list->dest_left;
		ci->ci_HWInst.Blt->WidthHeight =
		 ((list->height + 1) << 16) | (list->width + 1);

		list++;
	}
	cc->cc_PrimitivesIssued++;
}


status_t
AccelInit (register struct gfx_card_info *ci)
{
	(void) ci;

	cc->cc_CurROP = ~0;
	setrop (MINTERM_SRCCOPY);
	setcliprect (0, 0, 0x7fff, 0x7fff);
	setpattern (0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
	return (B_OK);
}
