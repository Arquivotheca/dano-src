/* :ts=8 bk=0
 *
 * render.c:	Basic rendering acceleration.
 *
 * $Id:$
 *
 * Leo L. Schwab					2000.07.14
 *  Based on I740 driver sources.
 *
 * Copyright 1999 Be Incorporated.
 */
#include <dinky/bena4.h>

#include <graphics_p/lynxem/lynxem.h>
#include <graphics_p/lynxem/debug.h>

#include "protos.h"


/*****************************************************************************
 * #defines
 */
#define	MINTERM_DESTINV	(MT_PSND | MT_PNSND | MT_NPSND | MT_NPNSND)

#define	_DRAWCTL_COMMON	(DEF2FIELD (DP_DRAWCTL, PATTERNSEL, MONO) \
			| VAL2FIELD (DP_DRAWCTL, DESTXUPDATEENABLE, FALSE) \
			| VAL2FIELD (DP_DRAWCTL, QUICKSTARTENABLE, FALSE) \
			| VAL2FIELD (DP_DRAWCTL, YSTRETCHENABLE, FALSE) \
			| DEF2FIELD (DP_DRAWCTL, SRCSEL, COLOR) \
			| VAL2FIELD (DP_DRAWCTL, CAPTUREENABLE, FALSE) \
			| DEF2FIELD (DP_DRAWCTL, ROPTYPE, ROP3) \
			| VAL2FIELD (DP_DRAWCTL, ROP2SRCISPATTERN, FALSE) \
			| DEF2FIELD (DP_DRAWCTL, MONOSRCALIGN, NONE) \
			| VAL2FIELD (DP_DRAWCTL, ROTATIONREPEATENABLE, FALSE) \
			| DEF2FIELD (DP_DRAWCTL, MATCHINGPIXEL, OPAQUE) \
			| DEF2FIELD (DP_DRAWCTL, TRANSPARENTCTL, SRC) \
			| VAL2FIELD (DP_DRAWCTL, TRANSPARENTENABLE, FALSE) \
			| VAL2FIELD (DP_DRAWCTL, ROP, MINTERM_PATCOPY))

#define	DRAWCTL_RECT	(_DRAWCTL_COMMON \
			| VAL2FIELD (DP_DRAWCTL, ROP, MINTERM_PATCOPY))
#define	DRAWCTL_RECTINV	(_DRAWCTL_COMMON \
			| VAL2FIELD (DP_DRAWCTL, ROP, MINTERM_DESTINV))
#define	DRAWCTL_BLTCOPY	(_DRAWCTL_COMMON \
			| VAL2FIELD (DP_DRAWCTL, ROP, MINTERM_SRCCOPY))

#define	DATAFMT_COMMON	(DEF2FIELD (DP_DATAFMT, PATXYOVERWRITESEL, NORMAL) \
			| VAL2FIELD (DP_DATAFMT, PATSTARTX, 0) \
			| VAL2FIELD (DP_DATAFMT, PATSTARTY, 0) \
			| DEF2FIELD (DP_DATAFMT, ENGINEMODE, XY) \
			| VAL2FIELD (DP_DATAFMT, STRETCHBLTSRCHEIGHT, 0))

#define	MAXENGINEWAIT	0xffffff /* timeout value for engine waits, ~6 secs */
#define	MAXFIFO		8

/* Wait until Command FIFO is empty */
#define WAITFIFOEMPTY()	waitfifoentries(MAXFIFO)

/* Wait until GP is idle and queue is empty */
#define	WAITIDLEEMPTY()							\
	do {								\
		WAITFIFOEMPTY();					\
		waitengineidle();					\
	} while(0)


/*****************************************************************************
 * Prototypes.
 */
static int readfifofree(void);
static void waitfifoentries(int entries);
static void resetengine(int waitfirst);


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
	register int	loop = 0;

//dprintf((">>> waitengineidle()\n"));
	while ((IDXREG_R (ci->ci_VGARegs, VGA_CR, SR_DEVPSTAT)
	        & MASKFIELD (SR_DEVPSTAT, DRAWBUSY))  &&
	       ++loop < MAXENGINEWAIT)
		/*  FIXME:  Be nicer to the system than this...  */
		;
	if (loop >= MAXENGINEWAIT)
		resetengine (FALSE);
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
	if (cc->cc_PrimitivesCompleted < st->counter) {
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
uint32				color,
uint32				drawctl,
register fill_rect_params	*list,
register uint32			count
)
{
	register DPRegs	*dpr;
	register uint32	val;

	dpr = ci->ci_DPRegs;

	waitfifoentries (7);
	dpr->FGColor	=
	dpr->BGColor	= color;
	dpr->SrcAddr	=
	dpr->DestAddr	= ci->ci_FBBase >> 3;
	dpr->Pitches	= VAL2FIELD (DP_PITCHES, DEST,
			             ci->ci_CurDispMode.virtual_width)
			| VAL2FIELD (DP_PITCHES, SRC,
			             ci->ci_CurDispMode.virtual_width);
	dpr->WinWidth	= VAL2FIELD (DP_WINWIDTH, DEST,
			             ci->ci_CurDispMode.virtual_width)
			| VAL2FIELD (DP_WINWIDTH, SRC,
			             ci->ci_CurDispMode.virtual_width);
	switch (ci->ci_Depth) {
	case 8:
		val = DEF2FIELD (DP_DATAFMT, FORMAT, 8BPP);
		break;
	case 15:
	case 16:
		val = DEF2FIELD (DP_DATAFMT, FORMAT, 16BPP);
		break;
	case 32:
		val = DEF2FIELD (DP_DATAFMT, FORMAT, 32BPP);
		break;
	}
	dpr->DataFmt = DATAFMT_COMMON | val;


	/*
	 * Build instruction packet.
	 */
	while (count--) {
		waitfifoentries (4);
		dpr->SrcXY	=
		dpr->DestXY	= VAL2FIELD (DP_SRCDESTXY, Y, list->top)
				| VAL2FIELD (DP_SRCDESTXY, X, list->left);
		dpr->Size	= VAL2FIELD (DP_SIZE, HEIGHT,
				             list->bottom - list->top + 1)
				| VAL2FIELD (DP_SIZE, WIDTH,
				             list->right - list->left + 1);
		dpr->DrawCtl	= drawctl
				| VAL2FIELD (DP_DRAWCTL, START, TRUE)
				| DEF2FIELD (DP_DRAWCTL, BLTDIR, XINC)
				| DEF2FIELD (DP_DRAWCTL, CMD, BLT);

		list++;
	}
	cc->cc_PrimitivesIssued++;
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
	
	rectfill_gen (color, DRAWCTL_RECT, list, count);
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

	rectfill_gen (0, DRAWCTL_RECTINV, list, count);
}


void
blit (engine_token *et, register blit_params *list, register uint32 count)
{
	register DPRegs	*dpr;
	register uint32	val;

	if (et != &enginetoken)
		return;

	dpr = ci->ci_DPRegs;

	waitfifoentries (7);
	dpr->FGColor	=
	dpr->BGColor	= 0;
	dpr->SrcAddr	=
	dpr->DestAddr	= ci->ci_FBBase >> 3;
	dpr->Pitches	= VAL2FIELD (DP_PITCHES, DEST,
			             ci->ci_CurDispMode.virtual_width)
			| VAL2FIELD (DP_PITCHES, SRC,
			             ci->ci_CurDispMode.virtual_width);
	dpr->WinWidth	= VAL2FIELD (DP_WINWIDTH, DEST,
			             ci->ci_CurDispMode.virtual_width)
			| VAL2FIELD (DP_WINWIDTH, SRC,
			             ci->ci_CurDispMode.virtual_width);
	switch (ci->ci_Depth) {
	case 8:
		val = DEF2FIELD (DP_DATAFMT, FORMAT, 8BPP);
		break;
	case 15:
	case 16:
		val = DEF2FIELD (DP_DATAFMT, FORMAT, 16BPP);
		break;
	case 32:
		val = DEF2FIELD (DP_DATAFMT, FORMAT, 32BPP);
		break;
	}
	dpr->DataFmt = DATAFMT_COMMON | val;

	/*
	 * Build instruction packet.
	 */
	while (count--) {
		uint16	xs, ys, xd, yd;

		if (list->dest_top > list->src_top  ||
		    (list->dest_top == list->src_top  &&
		     list->dest_left > list->src_left))
		{
			/*
			 * Copy backwards.
			 */
			val = DEF2FIELD (DP_DRAWCTL, BLTDIR, XDEC);
			xs = list->src_left + list->width;
			ys = list->src_top + list->height;
			xd = list->dest_left + list->width;
			yd = list->dest_top + list->height;
		} else {
			/*
			 * Copy from top to bottom, left to right.
			 */
			val = DEF2FIELD (DP_DRAWCTL, BLTDIR, XINC);
			xs = list->src_left;
			ys = list->src_top;
			xd = list->dest_left;
			yd = list->dest_top;
		}

		waitfifoentries (4);
		dpr->SrcXY	= VAL2FIELD (DP_SRCDESTXY, Y, ys)
				| VAL2FIELD (DP_SRCDESTXY, X, xs);
		dpr->DestXY	= VAL2FIELD (DP_SRCDESTXY, Y, yd)
				| VAL2FIELD (DP_SRCDESTXY, X, xd);
		dpr->Size	= VAL2FIELD (DP_SIZE, HEIGHT, list->height + 1)
				| VAL2FIELD (DP_SIZE, WIDTH, list->width + 1);
		dpr->DrawCtl	= DRAWCTL_BLTCOPY
				| val	/*  Blit direction  */
				| VAL2FIELD (DP_DRAWCTL, START, TRUE)
				| DEF2FIELD (DP_DRAWCTL, CMD, BLT);

		list++;
	}
	cc->cc_PrimitivesIssued++;
}


#if 0
void
spanfill (
engine_token	*et,
uint32		color,
register uint16	*list,
register uint32	count
)
{
	register Blt_Scanline	*sl;
	Blt_MonoPatternSL	*setup;

	if (et != &enginetoken)
		return;

	/*  Compose setup packet.  */
	setup = (Blt_MonoPatternSL *) getpacketspace (sizeof (*setup));

	setup->BLT_Instruction	= GFX2DOP_MONO_PATTERN_SL_BLT;
	setup->BLT_SetupCtl	= DEF2FIELD (BLT_CTL, PATSRC, ALLZERO) |
				  DEF2FIELD (BLT_CTL, MONOPAT, NORMAL) |
				  DEF2FIELD (BLT_CTL, DYNCOLOR, USEGLOBAL) |
				  VAL2FIELD (BLT_CTL, ROP, MINTERM_PATCOPY) |
				  VAL2FIELD (BLT_CTL, DESTPITCH,
					     ci->ci_BytesPerRow);
	setup->BLT_ClipY1	= 0;
	setup->BLT_ClipY2	= ci->ci_CurDispMode.virtual_height - 1;
	setup->BLT_ClipX1X2	= VAL2FIELD (BLT_DESTX1X2, X1, 0) |
				  VAL2FIELD
				   (BLT_DESTX1X2, X2,
				    ci->ci_CurDispMode.virtual_width - 1);
	setup->BLT_SetupBGColor	=
	setup->BLT_SetupFGColor	= color;
	setup->BLT_PatternData0	=
	setup->BLT_PatternData1	= 0;

	/*
	 * Build scanline packets.
	 */
	while (count--) {
		sl = (Blt_Scanline *) getpacketspace (sizeof (*sl));

		sl->BLT_Instruction	= GFX2DOP_SCANLINE_BLT(0);
		sl->BLT_DestX1X2	= VAL2FIELD (BLT_DESTX1X2, X1,
						     list[1]) |
					  VAL2FIELD (BLT_DESTX1X2, X2,
						     list[2]);
		sl->BLT_DestY1		= list[0];

		list += 3;	/*  Geez, this is lame...  */
	}
	cc->cc_PrimitivesIssued++;

	flushdropcookie ((uint32) cc->cc_PrimitivesIssued);
	executepackets ();
}
#endif



/*  FIXME:  Write something useful here...  */
status_t
AccelInit (struct gfx_card_info *ci)
{
	register DPRegs	*dpr;
	uint8		val;

	dpr = ci->ci_DPRegs;

	resetengine (FALSE);

	waitfifoentries (7);
	dpr->CmpColor	= 0;
	dpr->CmpMask	= ~0;
	dpr->MaskEnable	= VAL2MASKD (DP_MASKENABLE, BYTELANEENABLE, ~0)
			| VAL2MASKD (DP_MASKENABLE, BITMASK, ~0);
	dpr->ScisTopLeftCtl
			= VAL2FIELD (DP_SCISTOPLEFTCTL, TOP, 0)
			| VAL2FIELD (DP_SCISTOPLEFTCTL, ENABLE, FALSE)
			| DEF2FIELD (DP_SCISTOPLEFTCTL, DISABLEWRITES, OUTSIDE)
			| VAL2FIELD (DP_SCISTOPLEFTCTL, LEFT, 0);
	dpr->ScisBotRight = 0;
	dpr->MonoPat[0]	=
	dpr->MonoPat[1]	= 0;

	return (B_OK);
}

/*****************************************************************************
 * Engine management code.
 */
static int
readfifofree (void)
{
	/*
	 * FIXME: WARNING!!  This does not work with the Lynx3DM; SR_DEVPSTAT
	 * does not store the FIFO free count.
	 */
	register uint8	val;
	
	val = IDXREG_R (ci->ci_VGARegs, VGA_SR, SR_DEVPSTAT);
	if (val & MASKFIELD (SR_DEVPSTAT, DRAWFIFO))
		/*  FIFO is draining  */
		return (FIELD2VAL (SR_DEVPSTAT, FIFOFREESPACE, val) + 1);
	else
		return (MAXFIFO);
}

static void
waitfifoentries (int nentries)
{
	int	loop = 0;

	if (nentries > MAXFIFO)	nentries = MAXFIFO;

	while (readfifofree () < nentries  &&  ++loop < MAXENGINEWAIT)
		/*  FIXME:  Should really be nicer than this...  */
		;
	if (loop >= MAXENGINEWAIT)
		resetengine (FALSE);
}

static void
resetengine (int waitfirst)
{
	vgaregs	*vregs;
	uint8	tmp;

	if (waitfirst)
		WAITIDLEEMPTY ();
	
	vregs = ci->ci_VGARegs;

	tmp = IDXREG_R (vregs, VGA_SR, SR_PCIMISCCTL);
	IDXREG_W (vregs, VGA_SR, SR_PCIMISCCTL,
	          tmp
	          | VAL2FIELD (SR_PCIMISCCTL, SWDRAWABORTENABLE, TRUE)
	          | VAL2FIELD (SR_PCIMISCCTL, DRAWABORTENABLE, TRUE));

	WAITIDLEEMPTY ();

	IDXREG_W (vregs, VGA_SR, SR_PCIMISCCTL, tmp);
}
