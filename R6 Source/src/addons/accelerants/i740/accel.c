/* :ts=8 bk=0
 *
 * accel.c:	Basic rendering acceleration.
 *
 * $Id:$
 *
 * Leo L. Schwab					1998.11.05
 *  Based on NVidia and Matrox driver code.
 */
#include <graphics_p/i740/i740.h>
#include <graphics_p/i740/bena4.h>
#include <graphics_p/i740/debug.h>

#include "protos.h"


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


/*****************************************************************************
 * Local prototypes.
 */
//static void writepacket (register uint32 *buf, register int32 size);


/*****************************************************************************
 * Globals.
 */
extern i740_card_info	*ci;
extern i740_card_ctl	*cc;

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

	lockBena4 (&cc->cc_EngineLock);

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

	unlockBena4 (&cc->cc_EngineLock);
//dprintf ((">>> releaseengine() completes\n"));
	return (B_OK);
}

void
waitengineidle (void)
{
//dprintf((">>> waitengineidle()\n"));
	/*  FIXME:  Add something here...  */
	while (ci->ci_Regs->BLT_BlitCtl & FIELDVAL (BLT_BLTCTL, BUSY, TRUE))
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
		do {
			diff =
			 (ci->ci_Regs->NOPID & MASKEXPAND (I7INST_NOPID)) -
			 ((uint32) st->counter & MASKEXPAND (I7INST_NOPID));
			if (diff > MASKEXPAND (I7INST_NOPID) / 2)
				diff -= MASKEXPAND (I7INST_NOPID) + 1;
		} while (diff < 0  ||
		         (diff == 0  &&  (ci->ci_Regs->BLT_BlitCtl &
					  MASKEXPAND (BLT_BLTCTL_BUSY))));
	}

	cc->cc_PrimitivesCompleted = st->counter;
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
	register uint32		*src;
	register int32		w, h, pixelbytes, fboff;
	BltFullLoad		rect;

	if (et != &enginetoken)
		return;

	pixelbytes = (ci->ci_Depth + 7) >> 3;
	fboff = (uint8 *) ci->ci_FBBase - (uint8 *) ci->ci_BaseAddr0;

	/*
	 * Build instruction packet.
	 */
	rect.BLT_Instruction	= GFX2DOPREG_BLTER_FULL_LOAD;
	rect.BLT_Pitches	= FIELDVAL (BLT_PITCHES, DEST,
					    ci->ci_BytesPerRow) |
				   FIELDVAL (BLT_PITCHES, SRC,
					     ci->ci_BytesPerRow);
	rect.BLT_PatBGColor	=
	rect.BLT_PatFGColor	=
	rect.BLT_SrcBGColor	=
	rect.BLT_SrcFGColor	= color;
	rect.BLT_MonoSrcCtl	= MONOSRCCTL;
	rect.BLT_BlitCtl	= BLITCTL_RECT;
	rect.BLT_PatSrcAddr	= 0;

	while (count--) {
		w = (list->right - list->left + 1) * pixelbytes;
		h = list->bottom - list->top + 1;

		rect.BLT_SrcAddr =
		rect.BLT_DstAddr = list->top * ci->ci_BytesPerRow +
				   list->left * pixelbytes +
				   fboff;
		rect.BLT_DstSize = FIELDVAL (BLT_DSTSIZE, HEIGHT, h) |
				    FIELDVAL (BLT_DSTSIZE, BYTEWIDTH, w);

		writepacket ((uint32 *) &rect, sizeof (rect));

		list++;
	}
	cc->cc_PrimitivesIssued++;
	*ci->ci_Regs->LPFIFO =
	 GFX_NOP_ID (cc->cc_PrimitivesIssued & MASKEXPAND (I7INST_NOPID));
}


void
blit (engine_token *et, register blit_params *list, register uint32 count)
{
	register uint32		*src;
	register int32		pixelbytes, fboff, w;
	BltFullLoad		blit;

	if (et != &enginetoken)
		return;

	pixelbytes = (ci->ci_Depth + 7) >> 3;
	fboff = (uint8 *) ci->ci_FBBase - (uint8 *) ci->ci_BaseAddr0;

	/*
	 * Build instruction packet.
	 */
	blit.BLT_Instruction	= GFX2DOPREG_BLTER_FULL_LOAD;
	blit.BLT_Pitches	= FIELDVAL (BLT_PITCHES, DEST,
					    ci->ci_BytesPerRow) |
				   FIELDVAL (BLT_PITCHES, SRC,
					     ci->ci_BytesPerRow);
	blit.BLT_PatBGColor	=
	blit.BLT_PatFGColor	=
	blit.BLT_SrcBGColor	=
	blit.BLT_SrcFGColor	=
	blit.BLT_PatSrcAddr	= 0;
	blit.BLT_MonoSrcCtl	= MONOSRCCTL;
	blit.BLT_BlitCtl	= BLITCTL_BLIT;

	while (count--) {
		/*
		 * I740 expects start addresses to be the actual start
		 * address, which means it has to be adjusted depending on
		 * which directions you're blitting.
		 */
		if (list->dest_top > list->src_top) {
			/*
			 * Copy from bottom to top.
			 */
			blit.BLT_BlitCtl |= FIELDDEF
					     (BLT_BLTCTL, YINCDIR, UPWARD);
			blit.BLT_SrcAddr =
			 (list->src_top + list->height) * ci->ci_BytesPerRow +
			 list->src_left * pixelbytes +
			 fboff;
			blit.BLT_DstAddr =
			 (list->dest_top + list->height) * ci->ci_BytesPerRow +
			 list->dest_left * pixelbytes +
			 fboff;
		} else if (list->dest_top == list->src_top  &&
			   list->dest_left > list->src_left)
		{
			/*
			 * Copy from right to left.
			 ***
			 * The blitter knows nothing about pixel sizes; it
			 * just copies bytes.  So we need to point at the
			 * very last byte of each row, rather than the last
			 * pixel.
			 */
			blit.BLT_BlitCtl |= FIELDDEF
					     (BLT_BLTCTL, XINCDIR, LEFTWARD);
			blit.BLT_SrcAddr =
			 list->src_top * ci->ci_BytesPerRow +
			 (list->src_left + list->width) * pixelbytes +
			 fboff + pixelbytes - 1;
			blit.BLT_DstAddr =
			 list->dest_top * ci->ci_BytesPerRow +
			 (list->dest_left + list->width) * pixelbytes +
			 fboff + pixelbytes - 1;
		} else {
			/*
			 * Copy from top to bottom, left to right.
			 */
			blit.BLT_SrcAddr = list->src_top * ci->ci_BytesPerRow +
					   list->src_left * pixelbytes +
					   fboff;
			blit.BLT_DstAddr = list->dest_top * ci->ci_BytesPerRow +
					   list->dest_left * pixelbytes +
					   fboff;
		}

		blit.BLT_DstSize = FIELDVAL (BLT_DSTSIZE, HEIGHT,
					     list->height + 1) |
				    FIELDVAL (BLT_DSTSIZE, BYTEWIDTH,
					      (list->width + 1) * pixelbytes);

		writepacket ((uint32 *) &blit, sizeof (blit));

		list++;
	}
	cc->cc_PrimitivesIssued++;
	*ci->ci_Regs->LPFIFO =
	 GFX_NOP_ID (cc->cc_PrimitivesIssued & MASKEXPAND (I7INST_NOPID));
}


void
writepacket (register uint32 *buf, register int32 size)
{
#define	SIZEOF_FIFO	16

	register vuint32	*fifo;
	register int		fill;

	fifo = ci->ci_Regs->LPFIFO;
	size /= sizeof (uint32);

	/*
	 * Wait for instruction FIFO to be empty.  (This is a guess on my
	 * part; the HW may stall the CPU until free space is available.  I'm
	 * still waiting for an answer from Intel on this one.)
	while (ci->ci_Regs->INPOINT)
		;
	 */

	/*
	 * Write instruction packet to the low-priority FIFO.
	 */
	fill = SIZEOF_FIFO;
	while (--size >= 0) {
		if (fill >= SIZEOF_FIFO) {
			while ((fill = GetBF (ci->ci_Regs->INPOINT,
					      INPOINT_LPFIFO)) >= SIZEOF_FIFO)
				;
		}
		*fifo = *buf++;
		fill++;
	}
}


/*  FIXME:  Write something useful here...  */
status_t
AccelInit (register struct i740_card_info *ci)
{
	return (B_OK);
}
