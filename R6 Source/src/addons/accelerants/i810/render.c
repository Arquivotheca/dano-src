/* :ts=8 bk=0
 *
 * render.c:	Basic rendering acceleration.
 *
 * $Id:$
 *
 * Leo L. Schwab					1999.10.30
 *  Based on I740 driver sources.
 *
 * Copyright 1999 Be Incorporated.
 */
#include <dinky/bena4.h>

#include <graphics_p/i810/i810.h>
#include <graphics_p/i810/debug.h>

#include "protos.h"


/*****************************************************************************
 * #defines
 */
#define	MINTERM_DESTINV	(MT_PSND | MT_PNSND | MT_NPSND | MT_NPNSND)

#define	BLITCTL_RECT	(DEF2FIELD (BLT_CTL, PATSRC, ALLZERO) | \
			 DEF2FIELD (BLT_CTL, DYNCOLOR, USEGLOBAL) | \
			 VAL2FIELD (BLT_CTL, ROP, MINTERM_PATCOPY))
#define	BLITCTL_RECTINV	(DEF2FIELD (BLT_CTL, PATSRC, ALLZERO) | \
			 DEF2FIELD (BLT_CTL, DYNCOLOR, USEGLOBAL) | \
			 VAL2FIELD (BLT_CTL, ROP, MINTERM_DESTINV))
#define	BLITCTL_BLIT	(VAL2FIELD (BLT_CTL, BLTDIR, 0) | \
			 DEF2FIELD (BLT_CTL, DYNCOLOR, USEGLOBAL) | \
			 VAL2FIELD (BLT_CTL, ROP, MINTERM_SRCCOPY))


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
	/*  FIXME:  Add something here...  */
	while (ci->ci_Regs->BLT_OpcodeCtl & MASKFIELD (BLT_OPCODECTL, BLTBUSY))
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
 * to drop to idle.  Happily, the I810 has a feature that makes this almost
 * trivial to implement.
 */
status_t
synctotoken (sync_token *st)
{
#define	ADJNOPIDMASK	FIELD2VAL (I8INST_PARSER, NOPID, \
				   MASKFIELD (I8INST_PARSER, NOPID))
#define	RANGEMASK	(~0UL >> 2)

	register int32	diff;

//dprintf ((">>> synctotoken(st: 0x%08x)\n", st));
//synclog (('sync', (uint32) st->counter));
//synclog (('issu', (uint32) cc->cc_PrimitivesIssued));
//synclog (('done', (uint32) cc->cc_PrimitivesCompleted));
#if 1
	if (st->counter > cc->cc_PrimitivesCompleted) {
		do {
//synclog (('cook', (uint32) ci->ci_HWStatusPage->PrimitiveCookie));
			diff = (  ci->ci_HWStatusPage->PrimitiveCookie
				& RANGEMASK)
			     - (  (uint32) st->counter & RANGEMASK);
			if (diff > RANGEMASK / 2)
				diff -= RANGEMASK + 1;
		} while (diff < 0);
		cc->cc_PrimitivesCompleted = st->counter;
	}
#else
	/*  What to do when I want to punt.  */
	waitengineidle ();
	cc->cc_PrimitivesCompleted = st->counter;
#endif

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
uint32				color,
uint32				bltctl,
register fill_rect_params	*list,
register uint32			count
)
{
	register int32	w, h, pixelbytes, fboff;
	Blt_Color	rect;
	GfxBuf		*gb;

	gb = &cc->cc_DispBuf;
	pixelbytes = gb->gb_PixelBytes;
	fboff = gb->gb_BaseOffset;

	/*
	 * Build instruction packet.
	 */
//synclog (('rect', count));
	rect.BLT_Instruction	= GFX2DOP_COLOR_BLT;
	rect.BLT_Ctl		= bltctl |
				  VAL2FIELD (BLT_CTL, DESTPITCH,
				             gb->gb_BytesPerRow);
	rect.BLT_PatBGColor	= color;
	while (count--) {
		w = (list->right - list->left + 1) * pixelbytes;
		h = list->bottom - list->top + 1;

		rect.BLT_DestDims	= VAL2FIELD (BLT_DESTDIMS, HEIGHT, h) |
					  VAL2FIELD (BLT_DESTDIMS, BYTEWIDTH, w);
		rect.BLT_DestAddr	= list->top * gb->gb_BytesPerRow +
					  list->left * pixelbytes +
					  fboff;

		writepacket (&rect, sizeof (rect), FALSE, 0);
		list++;
	}
	cc->cc_PrimitivesIssued++;

	flushdropcookie ((uint32) cc->cc_PrimitivesIssued);
}

void
rectfill (
engine_token			*et,
uint32				color,
register fill_rect_params	*list,
register uint32			count
)
{
	if (et != &enginetoken)
		return;
	
	rectfill_gen (color, BLITCTL_RECT, list, count);
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

	rectfill_gen (0, BLITCTL_RECTINV, list, count);
}


void
blit (engine_token *et, register blit_params *list, register uint32 count)
{
	register int32	pixelbytes, fboff, pitch;
	Blt_SrcCopy	blit;
	GfxBuf		*gb;
	uint32		bltctl;

	if (et != &enginetoken)
		return;

	gb = &cc->cc_DispBuf;
	pixelbytes = gb->gb_PixelBytes;
	fboff = gb->gb_BaseOffset;

	/*
	 * Build instruction packet.
	 */
//synclog (('blit', count));
	blit.BLT_Instruction	= GFX2DOP_SRC_COPY_BLT;
	while (count--) {
		/*
		 * I810 expects start addresses to be the actual start
		 * address, which means it has to be adjusted depending on
		 * which directions you're blitting.
		 */
		bltctl = BLITCTL_BLIT;

		if (list->dest_top > list->src_top) {
			/*
			 * Copy from bottom to top.
			 */
			bltctl |= DEF2FIELD (BLT_CTL, BLTDIR, XINC);
			blit.BLT_SrcAddr =
			 (list->src_top + list->height) * gb->gb_BytesPerRow +
			 list->src_left * pixelbytes +
			 fboff;
			blit.BLT_DestAddr =
			 (list->dest_top + list->height) * gb->gb_BytesPerRow +
			 list->dest_left * pixelbytes +
			 fboff;
			pitch = -gb->gb_BytesPerRow;
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
			bltctl |= DEF2FIELD (BLT_CTL, BLTDIR, XDEC);
			blit.BLT_SrcAddr =
			 list->src_top * gb->gb_BytesPerRow +
			 (list->src_left + list->width) * pixelbytes +
			 fboff + pixelbytes - 1;
			blit.BLT_DestAddr =
			 list->dest_top * gb->gb_BytesPerRow +
			 (list->dest_left + list->width) * pixelbytes +
			 fboff + pixelbytes - 1;
			pitch = gb->gb_BytesPerRow;
		} else {
			/*
			 * Copy from top to bottom, left to right.
			 */
			bltctl |= DEF2FIELD (BLT_CTL, BLTDIR, XINC);
			blit.BLT_SrcAddr =
			 list->src_top * gb->gb_BytesPerRow +
			 list->src_left * pixelbytes +
			 fboff;
			blit.BLT_DestAddr =
			 list->dest_top * gb->gb_BytesPerRow +
			 list->dest_left * pixelbytes +
			 fboff;
			pitch = gb->gb_BytesPerRow;
		}

		blit.BLT_Ctl		= bltctl
					| VAL2MASKD (BLT_CTL, DESTPITCH, pitch);
		blit.BLT_SrcPitchStatus	= VAL2MASKD (BLT_SRCPITCHSTATUS, PITCH,
						    pitch);
		blit.BLT_DestDims	= VAL2FIELD (BLT_DESTDIMS, HEIGHT,
						    list->height + 1)
					| VAL2FIELD (BLT_DESTDIMS, BYTEWIDTH,
						    (list->width + 1) *
						     pixelbytes);

		writepacket (&blit, sizeof (blit), FALSE, 0);
		list++;
	}
	cc->cc_PrimitivesIssued++;

	flushdropcookie ((uint32) cc->cc_PrimitivesIssued);
}


void
spanfill (
engine_token	*et,
uint32		color,
register uint16	*list,
register uint32	count
)
{
	Blt_MonoPatternSL	setup;
	Blt_Scanline		sl;
	GfxBuf			*gb;
	int32			pixelbytes;

	if (et != &enginetoken)
		return;

	gb = &cc->cc_DispBuf;
	pixelbytes = gb->gb_PixelBytes;

	/*  Compose setup packet.  */
//synclog (('span', count));
	setup.BLT_Instruction	= GFX2DOP_MONO_PATTERN_SL_BLT;
	setup.BLT_SetupCtl	= DEF2FIELD (BLT_CTL, PATSRC, ALLZERO)
				| DEF2FIELD (BLT_CTL, MONOPAT, NORMAL)
				| DEF2FIELD (BLT_CTL, DYNCOLOR, USEGLOBAL)
				| VAL2FIELD (BLT_CTL, ROP, MINTERM_PATCOPY)
				| VAL2FIELD (BLT_CTL, DESTPITCH,
					     gb->gb_BytesPerRow);
	setup.BLT_ClipY1	= gb->gb_BaseOffset;
	setup.BLT_ClipY2	= gb->gb_BaseOffset + gb->gb_Size - 1;
	setup.BLT_ClipX1X2	= VAL2FIELD (BLT_DESTX1X2, X1, 0)
				| VAL2FIELD (BLT_DESTX1X2, X2,
				             gb->gb_Width * pixelbytes - 1);
	setup.BLT_SetupBGColor	=
	setup.BLT_SetupFGColor	= color;
	setup.BLT_PatternData0	=
	setup.BLT_PatternData1	= 0;
	writepacket (&setup, sizeof (setup), FALSE, 0);

	/*
	 * Build scanline packets.
	 */
	sl.BLT_Instruction	= GFX2DOP_SCANLINE_BLT
				| VAL2FIELD (GFX2DOP_SCANLINE_BLT,
				             PATVALIGN, 0);
	while (count--) {
		sl.BLT_DestX1X2	= VAL2FIELD (BLT_DESTX1X2, X1,
				             list[1] * pixelbytes)
				| VAL2FIELD (BLT_DESTX1X2, X2,
				             (list[2] + 1) * pixelbytes - 1);
		sl.BLT_DestY1	= list[0] * gb->gb_BytesPerRow
				+ gb->gb_BaseOffset;

		writepacket (&sl, sizeof (sl), FALSE, 0);
		list += 3;	/*  Geez, this is lame...  */
	}
	cc->cc_PrimitivesIssued++;

	flushdropcookie ((uint32) cc->cc_PrimitivesIssued);
}



/*  FIXME:  Write something useful here...  */
status_t
AccelInit (register struct gfx_card_info *ci)
{
	(void) ci;

	return (B_OK);
}



/*****************************************************************************
 * Flush internal write buffers and write a cookie to the status page.
 * This also forces execution of the batch.
 */
void
flushdropcookie (uint32 cookie)
{
	GCmd_Store32	*gcmd;
	uint32		buf[(sizeof (uint32) + sizeof (*gcmd))
			    / sizeof (uint32)];

	buf[0] = GFXCMDPARSER_FLUSH
	       | VAL2FIELD (GFXCMDPARSER_FLUSH, AGP, FALSE)
	       | VAL2FIELD (GFXCMDPARSER_FLUSH, INVALIDATECACHE, FALSE);

	gcmd = (GCmd_Store32 *) &buf[1];
	gcmd->gcmd_Instruction	= GFXCMDPARSER_STORE_DWORD_INDEX;
	gcmd->gcmd_PhysAddr	= offsetof (HWStatusPage, PrimitiveCookie);
	gcmd->gcmd_Value	= cookie;

	writepacket (buf, sizeof (buf), TRUE, 0);
//synclog (('drop', cookie));
}


/*****************************************************************************
 * Command packet management.
 */
/*
 * A size of zero is valid, and can be used to begin execution of a
 * previously-written batch of packets.
 */
void
writepacket (void *cl_buf, register int32 size, int execute, int which)
{
	register uint32	*dest;
	RingBuffer	*ring;
	uint32		nop;
	uint32		base, *cmdtail;

	if (which) {
		/*  IRQ ring.  */
		base = ci->ci_IRQCmdBase;
		cmdtail = &cc->cc_IRQCmdTail;
		ring = &ci->ci_Regs->IRQRING;
	} else {
		/*  Low-priority ring.  */
		base = ci->ci_CmdBase;
		cmdtail = &cc->cc_CmdTail;
		ring = &ci->ci_Regs->LPRING;
	}
	dest = (uint32 *) (ci->ci_BaseAddr0 + base + *cmdtail);

//dropnop:
	if (size) {
		register uint32	*buf;
		int32		tail, diff;

		/*
		 * Wait for enough room in the command buffer.
		 * We add the size of the client buffer to the tail pointer
		 * and see if it leads the head pointer by *less* than the
		 * size of the client buffer.  If it does, that means we're
		 * about to cross over the head pointer, overflowing the
		 * command buffer.
		 */
		if ((tail = *cmdtail + size) >= ci->ci_CmdSize)
			tail -= ci->ci_CmdSize;
//synclog (('tail', tail));
		do {
			/*  (Man, I hate this wraparound compensation stuff)  */
			diff = tail
			     - (ring->RINGBUF1
			        & MASKFIELD (RINGBUF1, HEADOFFSET64));
			if (diff > (int32) ci->ci_CmdSize / 2)
				diff -= ci->ci_CmdSize;
			else if (-diff > (int32) ci->ci_CmdSize / 2)
				diff += ci->ci_CmdSize;
//synclog (('hddf', (ring->RINGBUF1 << 16) | (diff & 0xffff)));
		} while (diff >= 0  &&  diff < size);

		/*  Copy supplied buffer into ring buffer.  */
		buf = cl_buf;
		if ((*cmdtail += size) >= ci->ci_CmdSize) {
			/*  Wrap around end of buffer.  */
			uint32	*bufend;

			bufend = (uint32 *) (ci->ci_BaseAddr0
			                     + base
			                     + ci->ci_CmdSize);

			size >>= 2;
			while (--size >= 0) {
				*dest++ = *buf++;
				if (dest >= bufend)
					dest = (uint32 *) (ci->ci_BaseAddr0
					                   + base);
			}
			*cmdtail -= ci->ci_CmdSize;
		} else {
			/*  Simple copy  */
			size >>= 2;
			while (--size >= 0)
				*dest++ = *buf++;
		}
	}
	if (execute) {
		/*  64-bit alignment required.  */
		if (*cmdtail & 0x7) {
			/*
			 * Drop a NOP to pad out to 64 bits.
			 * There are those who would consider this approach
			 * *evil*, and I'm inclined to agree.  But *damn*,
			 * it works nice.
			 */
			nop = GFXCMDPARSER_NOP;
			writepacket (&nop, sizeof (nop), FALSE, which);

//			nop = GFXCMDPARSER_NOP;
//			cl_buf = &nop;
//			size = sizeof (nop);
//			goto dropnop;	/*  Look up  */
		}
		/*  Move the HW tail pointer; execute new commands.  */
		ring->RINGBUF0 = ring->RINGBUF0
		               & ~MASKFIELD (RINGBUF0, TAILOFFSET)
		               | *cmdtail;
	}
}


/*****************************************************************************
 * A very handy debugging aid (why didn't I write this earlier?).
 */
#if DEBUG_SYNCLOG
void
_synclog_ (uint32 code, uint32 val)
{
	register int	idx, prev;

	idx = cc->cc_SyncLogIdx;
	if ((prev = idx - 1) < 0)
		prev += NSYNCLOGENTRIES;

	/*  Don't record duplicates  */
	if (cc->cc_SyncLog[prev][0] != code  ||
	    cc->cc_SyncLog[prev][1] != val)
	{
		cc->cc_SyncLog[idx][0] = code;
		cc->cc_SyncLog[idx][1] = val;
		if (++idx >= NSYNCLOGENTRIES)
			idx = 0;
		cc->cc_SyncLogIdx = idx;
	}
}
#endif
