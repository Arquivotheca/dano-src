#include "private.h"
#include "generic.h"
#include "SetRegisters.h"
#include <sys/ioctl.h>

#if EMACHINE
#include <Debug.h>
#endif

#include "hooks_overlay.h"

/* YUCK */
#if 1
#define PANEL_WIDTH 1024
#define PANEL_HEIGHT 768
#define PANEL_SPEED (65000ul)
#else
#define PANEL_WIDTH 800
#define PANEL_HEIGHT 600
#define PANEL_SPEED (50000ul)
#endif


//#define xfprintf(a) ddprintf(a); snooze(1000)

#define MAX_OVERLAYS (sizeof(ai->ovl_tokens) / sizeof(ai->ovl_tokens[0]))

void SetDSP (int bitsperpixel, double ratio, uint32 pixelclock, uint32 XCLK, uint32 *dsp_config, uint32 *dsp_on_off);

///////////////////////////////////////////////////////////////////////////////
// Mode set header script.
///////////////////////////////////////////////////////////////////////////////

REGSET_STRUCT ModeSetHeaderTable[] =
{
	// Reset CRTC controller.
	{ CRTC_GEN_CNTL,		0x0,			~0x02000000ul,	REGSET_RMW },
	{ CRTC_GEN_CNTL,		0x02000000,		~0x02000000ul,		REGSET_RMW },

	// Init CRTC controller to known-stable state.
	// Modification: do not enable accessing through VGA aperature.
	{ CRTC_GEN_CNTL,		0x03010400,		0x0,			REGSET_WRITE },

	// Blank the screen.
	{ CRTC_GEN_CNTL,		0x40,			~0x00000040ul,	REGSET_RMW },

	// Don't map the A and B segments, as aren't using them.

	// Hold down CRTC reset.
	{ CRTC_GEN_CNTL,		0x0,			~0x02000000ul,	REGSET_RMW },


	// End of script.

	{ 0x0,					0x0,			0x0,		REGSET_FENCE }
};


///////////////////////////////////////////////////////////////////////////////
// Mode set footer script.
///////////////////////////////////////////////////////////////////////////////

REGSET_STRUCT ModeSetFooterTable[] =
{
	// Release CRTC reset.
	{ CRTC_GEN_CNTL,		0x02000000,		~0x02000000ul,		REGSET_RMW },

	// Reset CRTC controller.
	{ CRTC_GEN_CNTL,		0x0,			~0x02000000ul,	REGSET_RMW },
	{ CRTC_GEN_CNTL,		0x02000000,		~0x02000000ul,		REGSET_RMW },

	// Unblank the screen.
	{ CRTC_GEN_CNTL,		0x0,			~0x00000040ul,	REGSET_RMW },


	// End of script.

	{ 0x0,					0x0,			0x0,		REGSET_FENCE }
};

REGSET_STRUCT ModeSetHeaderTableLT[] =
{
	// Reset CRTC controllers.
	{ CRTC_GEN_CNTL,		0x0,			~0x02200000ul,	REGSET_RMW },
	{ CRTC_GEN_CNTL,		0x02200000,		~0x02200000ul,	REGSET_RMW },

	// Init CRTC controller to known-stable state.
	// Modification: do not enable accessing through VGA aperature.
	{ CRTC_GEN_CNTL,		0x03000400,		0x0,			REGSET_WRITE },

	// Blank the screen.
	{ CRTC_GEN_CNTL,		0x4c,			~0x0000004cul,	REGSET_RMW },

	// Don't map the A and B segments, as aren't using them.

	// Hold down CRTC resets.
	{ CRTC_GEN_CNTL,		0x0,			~0x02200000ul,	REGSET_RMW },


	// End of script.

	{ 0x0,					0x0,			0x0,		REGSET_FENCE }
};


///////////////////////////////////////////////////////////////////////////////
// Mode set footer script.
///////////////////////////////////////////////////////////////////////////////

REGSET_STRUCT ModeSetFooterTableLT[] =
{
	// Release CRTC resets.
	// Only allow the primary CRTC controller to be active.
	{ CRTC_GEN_CNTL,		0x02000000,		~0x02200000ul,		REGSET_RMW },

	// Reset CRTC controllers.
	{ CRTC_GEN_CNTL,		0x0,			~0x02200000ul,	REGSET_RMW },
	// Only allow the primary CRTC controller to be active.
	{ CRTC_GEN_CNTL,		0x02000000,		~0x02200000ul,		REGSET_RMW },

	// Reset the draw engine.
	{ GEN_TEST_CNTL,		0x0100,			~0x0100ul,		REGSET_RMW },
	{ GEN_TEST_CNTL,		0x0,			~0x0100ul,		REGSET_RMW },

	// Unblank the screen.
	// Ensure horizontal and vertical sync running.
	{ CRTC_GEN_CNTL,		0x0,			~0x0000004Cul,	REGSET_RMW },


	// End of script.

	{ 0x0,					0x0,			0x0,		REGSET_FENCE }
};

static void interrupt_enable(bool flag) {
	status_t result;
	ati_set_bool_state sbs;

	/* set the magic number so the driver knows we're for real */
	sbs.magic = ATI_PRIVATE_DATA_MAGIC;
	sbs.do_it = flag;
	/* contact driver and get a pointer to the registers and shared data */
	result = ioctl(fd, ATI_RUN_INTERRUPTS, &sbs, sizeof(sbs));
	ddprintf(("ioctl(ATI_RUN_INTERRUPTS) returns %ld\n", result));
}

static uint32 calcBitsPerPixel(uint32 cs) {
	uint32	bpp = 0;

	switch (cs) {
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB32_LITTLE:
		case B_RGBA32_LITTLE:
			bpp = 32; break;
		case B_RGB24_BIG:
		case B_RGB24_LITTLE:
			bpp = 24; break;
		case B_RGB16_BIG:
		case B_RGB16_LITTLE:
			bpp = 16; break;
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_RGB15_LITTLE:
		case B_RGBA15_LITTLE:
			bpp = 15; break;
		case B_CMAP8:
			bpp = 8; break;
	}
	ddprintf(("*** calcBitsPerPixel(0x%04lx) returning %ld\n", cs, bpp));
	return bpp;
}

// This function adjusts the CRTC_OFF_PITCH register to reflect the current
// frame buffer definition.
// RESTRICTIONS: The frame buffer must start on a qword boundary, and the
// offset within the frame buffer must be a qword multiple. The pitch of
// the frame buffer must be a multiple of 8 pixels.

void UpdateCardFBuffer()
{
	unsigned long Offset;
	unsigned long BytesPerPixel = ai->bytes_per_pixel;
	unsigned long Pitch = ai->dm.virtual_width;

	Offset = ((char *)ai->fbc.frame_buffer - (char *)si->framebuffer);
	Offset += ai->dm.h_display_start * BytesPerPixel
		+ ai->dm.v_display_start * ai->fbc.bytes_per_row;

	// Convert to qword offset.
	Offset = Offset >> 3;

	// Convert to pixels * 8.
	Pitch = (Pitch >> 3);// / BytesPerPixel;

	// Reconfigure the relevant card register.

	WRITE_REG(CRTC_OFF_PITCH, ((Pitch & 0x03FF) << 22) | (Offset & 0x0FFFFF));

}

static status_t do_set_display_mode(display_mode *dm) {
	float QWordsPerPixel;
	float QWordRate;
	unsigned long HDisp, VDisp;
	unsigned long ActualHTotal, ActualVTotal;
	float PixelClock;
	float ActualPixelClock;
	float VClkDivIdeal;
	unsigned char VClkDiv;
	unsigned char VClkPost;
	int AltPost;
	float IdealXClksPerQW;
	float IdealOnThresh, IdealOffThresh;
	unsigned long Precision;
	unsigned long IntXClksPerQW;
	unsigned long IntOnThresh, IntOffThresh;
	unsigned long HWidth, VWidth;
	unsigned long HStart, VStart;
	unsigned long LTemp;
	uint32	Bpp;
	int LT = 0;
	int UseLCD;
	uint32 ulTempPOWER_MANAGEMENT = 0;
	double target_refresh;

#if 0
//EMACHINE
	/* The EMachine eOne integrated monitor is not tolerant at all, so
	   we will not tolerate any freedom for the sync settings. */
	if (dm->timing.h_display == 640) {
		dm->timing.h_sync_start = 816;
		dm->timing.h_sync_end = 1024;
		dm->timing.h_total = 1116;
		dm->timing.v_display = 480;
		dm->timing.v_sync_start = 486;
		dm->timing.v_sync_end = 490;
		dm->timing.v_total = 528;
		dm->timing.pixel_clock = 70709;
	}
	else if (dm->timing.h_display == 800) {
		dm->timing.h_sync_start = 820;
		dm->timing.h_sync_end = 846;
		dm->timing.h_total = 1192;
		dm->timing.v_display = 600;
		dm->timing.v_sync_start = 601;
		dm->timing.v_sync_end = 604;
		dm->timing.v_total = 628;
		dm->timing.pixel_clock = 67371;
	}
	else if (dm->timing.h_display == 1024) {
		dm->timing.h_sync_start = 1046;
		dm->timing.h_sync_end = 1082;
		dm->timing.h_total = 1190;
		dm->timing.v_display = 768;
		dm->timing.v_sync_start = 770;
		dm->timing.v_sync_end = 774;
		dm->timing.v_total = 800;
		dm->timing.pixel_clock = 71400;
	}
	
#if 0
	/* Use for scanning correct crt values for fixed sync monitor */
	static int32 offset = 0;

	dm->timing.h_display = 1024;
	dm->timing.h_sync_start = 1046;
	dm->timing.h_sync_end = 1082;
	dm->timing.h_total = 1190;
	dm->timing.v_display = 768;
	dm->timing.v_sync_start = 770;
	dm->timing.v_sync_end = 774;
	dm->timing.v_total = 800;
	dm->timing.pixel_clock = (75 * dm->timing.h_total * dm->timing.v_total)/1000;
	_sPrintf("H:%d-%d-%d-%d, V:%d-%d-%d-%d, Cl:%d\n",
	         dm->timing.h_display, 
			 dm->timing.h_sync_start,
			 dm->timing.h_sync_end,
			 dm->timing.h_total,
	         dm->timing.v_display, 
			 dm->timing.v_sync_start,
			 dm->timing.v_sync_end,
			 dm->timing.v_total,
			 dm->timing.pixel_clock);
	offset += 4;
#endif

#endif

#if EMACHINE
	//if (dm->timing.h_display == 640) {
		LT = 1;
		ai->PanelID = 1;
	//} else {
		//LT = 0;
		//ai->PanelID = 0;
	//}
#else
	switch (si->device_id & 0xff00) {
		case 0x4C00: LT = 1;
		break;
	}
#endif

	target_refresh = ((double)dm->timing.pixel_clock * 1000.0) / ((double)dm->timing.h_total * (double)dm->timing.v_total);

	ai->UsingLCD = UseLCD =
		(LT != 0) &&
		(dm->timing.h_display <= PANEL_WIDTH) &&
		(dm->timing.v_display <= PANEL_HEIGHT) &&
		((dm->timing.pixel_clock <= PANEL_SPEED) || (target_refresh < 61.0)) &&	// the "or target_refresh" compensates for sloppy mode selection in screen
		(ai->PanelID != 0);
	// No checking on screen size - this routine can set a mode of *any* resolution
	// (insert evil laughter here).

	ddprintf(("UseLCD is %d\n", UseLCD));

#if EMACHINE
	//UseLCD = 1;
#else
	// cut evil laughter short and compensate for Ratiometric Expansion wierdness
	if (UseLCD) {
		// tweak settings for particular RE modes
		if ((dm->timing.h_display == 640) && (dm->timing.v_display == 480)) {
			dm->timing.h_sync_start = 672;
			dm->timing.h_sync_end = 768;
			dm->timing.h_total = 864;
			dm->timing.v_sync_start = 517;
			dm->timing.v_sync_end = 523;
			dm->timing.v_total = 588;
		}
	}
#endif
	Bpp = calcBitsPerPixel(dm->space);

	// Interpret and sanity check the BPP specified.
	switch(Bpp)
	{
	case 8:
		QWordsPerPixel = 0.125;
		ai->bytes_per_pixel = 1;
		break;
	case 15:
	case 16:
		QWordsPerPixel = 0.25;
		ai->bytes_per_pixel = 2;
		break;
	case 32:
		QWordsPerPixel = 0.5;
		ai->bytes_per_pixel = 4;
		break;
	default:
		// Flakey BPP specified.
		return B_ERROR;
	}

	// keep a copy of this video mode
	ai->dm = *dm;
	// Update the row-bytes.
	ai->fbc.bytes_per_row = dm->virtual_width * ai->bytes_per_pixel;

	/* memtypes available? */
	if (memtypefd >= 0) {
		BMemSpec *ms = &(ai->fb_spec);
		
		/* release any previous frame buffer */
		if (ms->ms_MR.mr_Size) {
			ioctl(memtypefd, B_IOCTL_FREEBYMEMSPEC, ms, sizeof(*ms));
			ms->ms_MR.mr_Size = 0;
		}
		/* alloc frame buffer from genpool */
		memset(ms, 0, sizeof(*ms));
		ms->ms_PoolID = ai->poolid;
		
		/* Hmm, what *are* the frame buffer alignment constraints? */
		/* for now, make things 8 byte aligned */
		ms->ms_AddrCareBits = 7;
		ms->ms_AddrStateBits = 0;
		
		/* how much space for frame buffer */
		ms->ms_MR.mr_Size = ai->fbc.bytes_per_row * dm->virtual_height;
		if (ioctl(memtypefd, B_IOCTL_ALLOCBYMEMSPEC, ms, sizeof(*ms)) < 0) {
			/* failed */
			ms->ms_MR.mr_Size = 0;
			return B_NO_MEMORY;
		}
		/* determine actual start of frame buffer */
		ai->fbc.frame_buffer =
		 (void *) ((char *) si->framebuffer + ms->ms_MR.mr_Offset);
	} else {
		/* if not using memtypes, frame buffer starts after cursor */
		ai->fbc.frame_buffer = (void *)(((char *)si->framebuffer) + 1024);
	}
	/* in either case, the DMA location is calculated as follows */
	ai->fbc.frame_buffer_dma =
		(void *)(((char *)si->framebuffer_pci) +
		((char *)ai->fbc.frame_buffer - (char *)si->framebuffer));
	

	// Calculate the effective total dimensions of the screen as requested.
	HDisp = dm->timing.h_display >> 3;
	VDisp = dm->timing.v_display;
	ActualHTotal = dm->timing.h_total >> 3;
	ActualVTotal = dm->timing.v_total;
	// We now have the dimensions and valid requested refresh rate stored; now we
	// calculate the clock values.
	// Compute the pixel clock rate, feedback divisor and post divisor.

	// Get ideal pixel clock rate.
	PixelClock = UseLCD /* && (dm->timing.pixel_clock > PANEL_SPEED) */ ? PANEL_SPEED : dm->timing.pixel_clock;
	ActualPixelClock = PixelClock *= 1000.0;
	{
	si->refresh_period = ((bigtime_t)(dm->timing.v_total) * (bigtime_t)dm->timing.h_total * (bigtime_t)1000000LL) / (bigtime_t)PixelClock;
	si->blank_period = ((bigtime_t)(dm->timing.v_total - dm->timing.v_display) * (bigtime_t)dm->timing.h_total * (bigtime_t)1000000) / (bigtime_t)PixelClock;
	ddprintf(("refresh: %Ld\nblank: %Ld\n", si->refresh_period, si->blank_period));
	}
	// Set the ideal QWord rate here also.

	QWordRate = PixelClock * QWordsPerPixel;

	// Calculate the post divider to use.
	// Pick the largest one possible for best step resolution in PLLVCLK.
	// NOTE: The docs are contradictory as to whether or not special considerations
	// must be made for modes running at < 40 MHz. One page implies so, but another says
	// that you can still run in single clock VFC mode at any frequency. Taking the
	// simplest approach (no considerations), which seems to work.

	if (PixelClock < 1.66666667e7)
	{
		// VClk = PLLVCLK / 12;
		VClkPost = 3;
		AltPost = 1;
		PixelClock *= 12.0;
	}
	else if (PixelClock < 2.5e7)
	{
		// VClk = PLLVCLK / 8;
		VClkPost = 3;
		AltPost = 0;
		PixelClock *= 8.0;
	}
	else if (PixelClock < 3.3333333e7)
	{
		// VClk = PLLVCLK / 6.
		VClkPost = 2;
		AltPost = 1;
		PixelClock *= 6.0;
	}
	else if (PixelClock < 5.0e7)
	{
		// VClk = PLLVCLK / 4.
		VClkPost = 2;
		AltPost = 0;
		PixelClock *= 4.0;
	}
	else if (PixelClock < 6.6666666e7)
	{
		// VClk = PLLVCLK / 3.
		VClkPost = 0;
		AltPost = 1;
		PixelClock *= 3.0;
	}
	else if (PixelClock < 1.0e8)
	{
		// VClk = PLLVCLK / 2.
		VClkPost = 1;
		AltPost = 0;
		PixelClock *= 2.0;
	}
	else // PixelClock had better be less than 200 MHz.
	{
		// VClk = PLLVCLK / 1.
		VClkPost = 0;
		AltPost = 0;
	}

	// Compute the feedback divisor and actual data rate.
	VClkDivIdeal = PixelClock * 0.5 / ai->Card.RefClock;
	VClkDiv = (unsigned char) (0.5 + VClkDivIdeal);

	QWordRate *= ((float) VClkDiv) / VClkDivIdeal;

	// Update the actual refresh rate, too.
	//RefreshRate *= ((float) VClkDiv) / VClkDivIdeal;


	// Now, compute the settings for the DSP registers.
	// Fifo_Size fixed at 24 in ATI's sample code, and this seems to work,
	// so the value is subbed in wherever it occurs. Update these
	// equations if we need to write a driver for future cards.

	IdealXClksPerQW = ai->Card.XClock / QWordRate;

	// only do this if ratiometric expansion
	if (UseLCD && (dm->timing.h_display < PANEL_WIDTH)) {
		IdealXClksPerQW *= (float)(PANEL_HEIGHT) / (float)(dm->timing.v_display);
		if (dm->timing.h_display < 1024) IdealXClksPerQW /= 1.0146;
		if (dm->timing.h_display < 800) IdealXClksPerQW /= 1.0146;
		if (Bpp > 8) IdealXClksPerQW /= 1.0146;
		if ((Bpp > 16) && (dm->timing.h_display < 800)) {
			IdealXClksPerQW *= 1.00225; // Local Optima //1.0025; //1.0035; //1.006
		}
		ddprintf(("Tweaking IdealXClksPerQW to %f\n", IdealXClksPerQW));
	}

	if (IdealXClksPerQW >= ai->Card.PageSize)
		IdealOnThresh = 2.0 * ai->Card.PageSize + 1.0 + IdealXClksPerQW;
	else
		IdealOnThresh = 3.0 * ai->Card.PageSize;

	IdealOffThresh = IdealXClksPerQW * 23.0 + 1.0;

	// Find the precision value to use for converting these values to integers.
	Precision = 0;
	IdealXClksPerQW *= 2048.0;
	IdealOnThresh *= 64.0;
	IdealOffThresh *= 64.0;

	// NOTE: The maximum size of IdealOnThresh and IdealOffThresh should be 2047,
	// but in practice some padding is needed.
	while ((IdealXClksPerQW > 16383.0) || (IdealOnThresh > 1900.0)
		|| (IdealOffThresh > 1900.0))
	{
		Precision++;
		IdealXClksPerQW /= 2.0;
		IdealOnThresh /= 2.0;
		IdealOffThresh /= 2.0;
	}

	IntXClksPerQW = (unsigned long) (0.5 + IdealXClksPerQW);
	IntOnThresh = (unsigned long) (0.5 + IdealOnThresh);
	IntOffThresh = (unsigned long) (0.5 + IdealOffThresh);


	// We should have all of the clock information computed by now. Calculate
	// synchronization signals and we should be ready to do the mode set.

	HStart = dm->timing.h_sync_start >> 3;
	HWidth = (dm->timing.h_sync_end >> 3) - HStart;
	VStart = dm->timing.v_sync_start;
	VWidth = dm->timing.v_sync_end - VStart;

	// Adjust total/disp values, as stored as last element index, not number of elements.
	HDisp--;
	VDisp--;
	ActualHTotal--;
	ActualVTotal--;


	// TEST - Report calculated values to stdout.
#if DEBUG > 0
	ddprintf(("VClkDiv:   0x%x\n", VClkDiv));
	ddprintf(("Alt Post:  %d\n", AltPost));
	ddprintf(("VClk Post: %d\n", VClkPost));
	ddprintf(("XClocks per QWord: 0x%lx\n", IntXClksPerQW));
	ddprintf(("On Threshold:      0x%lx\n", IntOnThresh));
	ddprintf(("Off Threshold:     0x%lx\n", IntOffThresh));
	ddprintf(("Precision:         0x%lx\n", Precision));
	ddprintf(("H Displayed: 0x%lx\n", HDisp));
	ddprintf(("H Total:     0x%lx\n", ActualHTotal));
	ddprintf(("V Displayed: 0x%lx\n", VDisp));
	ddprintf(("V Total:     0x%lx\n", ActualVTotal));
	ddprintf(("H Start: 0x%lx\n", HStart));
	ddprintf(("H Width: 0x%lx\n", HWidth));
	ddprintf(("V Start: 0x%lx\n", VStart));
	ddprintf(("V Width: 0x%lx\n", VWidth));
	ddprintf(("Bpp: %lu\n", Bpp));
#endif


	// Everything has been calculated, and is presumably ok if we have reached this
	// point; set the registers.

	/* disable interrupts using the kernel driver */
	interrupt_enable(false);

	// NOTE: Not setting the OClk divisor. This should be set if overlays are to be
	// supported. As far as I can tell, it should be /1 for 1024x768 and lower resolutions
	// and /2 for 1152x900 and higher resolutions. However, there might be strange
	// restrictions on this.

	// Do pre-mode set register sets.
	SetRegisters(LT ? ModeSetHeaderTableLT : ModeSetHeaderTable);

	if (LT && (ai->PanelID && (dm->timing.h_display <= PANEL_WIDTH))) {
		// POWER_MGT_ON = 1, PWR_MGT_MODE = 01, AUTO_PWRUP_EN = 1, DIGON = 0
		READ_REG(LCD_INDEX, LTemp); // RMW, as index register is overloaded
		WRITE_REG(LCD_INDEX, ((LTemp & 0xfffffff0) | 0x08));
		READ_REG(LCD_DATA, LTemp);
// Added begin by hidean(Hitachi)
		ulTempPOWER_MANAGEMENT = LTemp;	// Save original value of POWER_MANAGEMENT
//		LTemp |= 0x04000000ul;
//		WRITE_REG(LCD_DATA, LTemp);		// DIGON at 1st.-> Oops! It's my misunderstanding.
		LTemp &= 0xfbfffff7ul;
		WRITE_REG(LCD_DATA, LTemp);		// Then, DIG=OFF and AUTO_PWRUP=DISABLE.
// Added end by hidean(Hitachi)
// Deleted(following 3 lines) begin by hidean(Hitachi)
//		LTemp &=  0xfbfffff0ul;
//		LTemp |=  0x0000000bul;
//		WRITE_REG(LCD_DATA, LTemp);
// Deleted end by hidean(Hitachi);
		snooze(100000);

		// turn the LCD off
		READ_REG(LCD_INDEX, LTemp); // RMW, as index register is overloaded
		WRITE_REG(LCD_INDEX, ((LTemp & 0xfffffff0) | 0x01));
		READ_REG(LCD_DATA, LTemp);
// Modified begin by hidean(Hitachi)
//		LTemp &=  0x00ff0000ul; // leave PLL selection bits alone, clear everything else
		LTemp &=  0x00ff0002ul; // Don't care LCD bit.
// Modified end by hidean(Hitachi)
		LTemp |=  0x400024d4ul; // switch to non-shadow everything
#if 1
//EMACHINE
		LTemp &= ~0x00000020;
		LTemp |=  0x00110088;
#endif
		WRITE_REG(LCD_DATA, LTemp);
		// Copy shadowed CRTC1 registers too.
		LTemp |=  0x80000000ul; // select shadow registers
		WRITE_REG(LCD_DATA, LTemp);

// Deleted(following 3 lines) begin by hidean(Hitachi)
//		snooze(300000); // this is the really important snooze
// Deleted end by hidean(Hitachi)
	}
	// Misc items.
	// Use VCLK3.
	READ_REG(CLOCK_CNTL, LTemp);
	LTemp &= ~0x0203;
	LTemp |= 0x03ul;
	WRITE_REG(CLOCK_CNTL, LTemp);
	// Set BPP.
	READ_REG(CRTC_GEN_CNTL, LTemp);
	LTemp &= ~0x0F00ul;
	LTemp &= ~0x0002ul;	// turn off interlaced video
	switch (Bpp)
	{
	case 8:
		LTemp |= 0x0200ul;
		break;
	case 15:
		LTemp |= 0x0300ul;
		break;
	case 16:
		LTemp |= 0x0400ul;
		break;
	default: // 32 bpp, as flakey bpps already caught.
		LTemp |= 0x0600ul;
	}
	WRITE_REG(CRTC_GEN_CNTL, LTemp);

	// PLL writes to set up the pixel clock.

	// Set VCLK3_FB_DIV.
	READ_REG(CLOCK_CNTL, LTemp);
	LTemp &= 0xFF0001FF;
	LTemp |= 0x0A << 10; // VCLK3_FB_DIV
	LTemp |= 0x00000200; // write mode
	LTemp |= ((unsigned long) VClkDiv) << 16; // value to write
	WRITE_REG(CLOCK_CNTL, LTemp);

	// Set XCLK = PLLMCLK/2, and std/alt post divider state.
	// get the contents of PLL_EXT_CNTL
	READ_REG(CLOCK_CNTL, LTemp);
	LTemp &= 0xFF0001FF;
	LTemp |= 0x0B << 10; // VCLK3_FB_DIV
	WRITE_REG(CLOCK_CNTL, LTemp);
	READ_REG(CLOCK_CNTL, LTemp);
#if 0
	LTemp &= 0xFF0001FF;
#else
	LTemp &= 0xFF0F01FF; // preserve state of XCLK
#endif
	LTemp |= 0x0B << 10; // PLL_EXT_CNTL
	LTemp |= 0x00000200; // write mode
#if 0
	if (AltPost)
		LTemp |= 0x81ul << 16; // value to write
	else
		LTemp |= 0x01ul << 16; // value to write
#else
	LTemp |= (AltPost ? (0x80ul << 16) : 0);
#endif
	WRITE_REG(CLOCK_CNTL, LTemp);

	// Set VCLK3 post divider.
	READ_REG(CLOCK_CNTL, LTemp);
	LTemp &= 0xFF0001FF;
	LTemp |= 0x06 << 10; // VCLK_POST_DIV
	LTemp |= 0x00000200; // write mode
	LTemp |= ((((unsigned long) VClkPost) << 6) | 0x3F) << 16; // value to write
	WRITE_REG(CLOCK_CNTL, LTemp);

	// Set to read PLL 0 (safe state).
	READ_REG(CLOCK_CNTL, LTemp);
	LTemp &= 0xFF0001FF;
	WRITE_REG(CLOCK_CNTL, LTemp);

	// Set the DSP registers.	
#if 0
	LTemp = (unsigned long) (0.5 + ai->Card.Latency);
	LTemp = (Precision << 20) | (LTemp << 16) | IntXClksPerQW;
	WRITE_REG(DSP_CONFIG, LTemp);

	LTemp = (IntOnThresh << 16) | IntOffThresh;
	WRITE_REG(DSP_ON_OFF, LTemp);
#else
	{
	uint32 dsp_config, dsp_on_off;
	SetDSP((Bpp+1)& ~1, UseLCD ? (double)(PANEL_WIDTH) / (double)dm->timing.h_display : 1.0, ActualPixelClock, ai->Card.XClock, &dsp_config, &dsp_on_off);
	ddprintf(("\n\nXClock: %f\n, PixelClock: %f\n", ai->Card.XClock, ActualPixelClock));
	ddprintf(("dsp_config: 0x%08lx\ndsp_on_off: 0x%08lx\n\n\n", dsp_config, dsp_on_off));
	READ_REG(DSP_CONFIG, LTemp);
	LTemp &= 0x00008000;
	LTemp |= dsp_config;
	WRITE_REG(DSP_CONFIG, LTemp);
	WRITE_REG(DSP2_CONFIG, LTemp);
	WRITE_REG(DSP_ON_OFF, dsp_on_off);
	WRITE_REG(DSP2_ON_OFF, dsp_on_off);
	}
#endif


	if (LT) {
		// FFB
		WRITE_REG(VGA_DSP_CONFIG, 0x056007f2ul);
		WRITE_REG(VGA_DSP_ON_OFF, 0x03f805b5ul);
		regs[DAC_CNTL] |= 0x0080ul;		// FFB
		// NOTE: For the LT series of chips, we have to do this twice; once for the normal
		// CRTC registers, and once for the shadowed versions used by the LCD.
	
		READ_REG(LCD_INDEX, LTemp); // RMW, as index register is overloaded
		WRITE_REG(LCD_INDEX, ((LTemp & 0xfffffff0) | 0x01));
	
		// Select non-shadowed CRTC1 registers.
		READ_REG(LCD_DATA, LTemp);
		//LTemp |= 0x08000000ul; // switch to crtc 2
		//LTemp &= ~0x80000000ul;	// switch to non-shadow
		LTemp &=  0x00ff0003ul; // leave PLL selection bits alone, clear everything else
		LTemp |=  0x400024d4ul; // switch to non-shadow everything
#if 1
//EMACHINE
		LTemp &= ~0x00000020;
		LTemp |=  0x00110088;
#endif
		WRITE_REG(LCD_DATA, LTemp);
	}

	// Set the CRTC registers. (the ones common to LT and non-LT chips)

	// Screen dimensions and synchronization.
	LTemp = (HDisp << 16) | ActualHTotal;
	WRITE_REG(CRTC_H_TOTAL_DISP, LTemp);
	LTemp = (HWidth << 16) | ((HStart & 0x0100ul) << 4) | 0x0200ul | (HStart & 0xFFul); // 0x0200ul was 0x0100ul
	// sync pulse is positive (0) by default, set to negative (1) if HSYNC flag not set
	if (!(dm->timing.flags & B_POSITIVE_HSYNC)) LTemp |= (1 << 21);
	WRITE_REG(CRTC_H_SYNC_STRT_WID, LTemp);
	LTemp = (VDisp << 16) | ActualVTotal;
	WRITE_REG(CRTC_V_TOTAL_DISP, LTemp);
	LTemp = (VWidth << 16) | VStart;
	// sync pulse is positive (0) by default, set to negative (1) if VSYNC flag not set
	if (!(dm->timing.flags & B_POSITIVE_VSYNC)) LTemp |= (1 << 21);
	WRITE_REG(CRTC_V_SYNC_STRT_WID, LTemp);

	// No overscan.
	WRITE_REG(OVR_WID_LEFT_RIGHT, 0x0);
	WRITE_REG(OVR_WID_TOP_BOTTOM, 0x0);
	WRITE_REG(OVR_CLR, 0x0);

	if (LT) {
		uint32 lcd_index_save;
		// Select shadow registers
		// LCD_INDEX still set to xxxxx1
		READ_REG(LCD_DATA, LTemp);
		LTemp |=  0x80000000ul; // select shadow registers
		WRITE_REG(LCD_DATA, LTemp);
	
		// Set the shadowed CRTC1 registers.
		// Screen dimensions and synchronization.
		LTemp = (HDisp << 16) | ActualHTotal;
		WRITE_REG(CRTC_H_TOTAL_DISP, LTemp);
		LTemp = (HWidth << 16) | ((HStart & 0x0100ul) << 4) | 0x0100ul | (HStart & 0xFFul);
		// sync pulse is positive (0) by default, set to negative (1) if HSYNC flag not set
		if (!(dm->timing.flags & B_POSITIVE_HSYNC)) LTemp |= (1 << 21);
		WRITE_REG(CRTC_H_SYNC_STRT_WID, LTemp);
		LTemp = (VDisp << 16) | ActualVTotal;
		WRITE_REG(CRTC_V_TOTAL_DISP, LTemp);
		LTemp = (VWidth << 16) | VStart;
		// sync pulse is positive (0) by default, set to negative (1) if HSYNC flag not set
		if (!(dm->timing.flags & B_POSITIVE_VSYNC)) LTemp |= (1 << 21);
		WRITE_REG(CRTC_V_SYNC_STRT_WID, LTemp);
	
		// No overscan.
		WRITE_REG(OVR_WID_LEFT_RIGHT, 0x0);
		WRITE_REG(OVR_WID_TOP_BOTTOM, 0x0);
		WRITE_REG(OVR_CLR, 0x0ul);
	
		// Select non-shadowed CRTC1 registers (again).
		READ_REG(LCD_DATA, LTemp);
		LTemp &=  0x00ff0003ul; // leave PLL selection bits alone, clear everything else
		LTemp |=  0x50002090ul; // switch to non-shadow with options
#if 1
//EMACHINE
		LTemp &= ~0x00000020;
		LTemp |=  0x00110088;
#endif
		WRITE_REG(LCD_DATA, LTemp);

		// enable ratiometric expansion for resolutions below the actual panel size
		// panel size currently hard coded to 1024x768
		// save the index register
		READ_REG(LCD_INDEX, lcd_index_save);
		//if (ai->PanelID && (dm->timing.h_display <= PANEL_WIDTH))
		if (UseLCD)
		{
			if (dm->timing.h_display < PANEL_WIDTH) {
				ddprintf(("RatExp 1a - PanelID %d, dm->timing.h_display: %d, PANEL_WIDTH: %d\n", ai->PanelID, dm->timing.h_display, PANEL_WIDTH));
				// 0 -> HSYNC_DELAY
				WRITE_REG(LCD_INDEX, 0x00);
				READ_REG(LCD_DATA, LTemp);
				LTemp &= ~0xf0000000ul;
				WRITE_REG(LCD_DATA, LTemp);
				// enable ratiometric expansion with blending
				LTemp = (dm->timing.h_display * 4096) / PANEL_WIDTH;
				LTemp |= 0xC0000000ul;
				WRITE_REG(LCD_INDEX, 0x04);
				WRITE_REG(LCD_DATA, LTemp);
			} else {
				ddprintf(("RatExp 1b - PanelID %d, dm->timing.h_display: %d, PANEL_WIDTH: %d\n", ai->PanelID, dm->timing.h_display, PANEL_WIDTH));
				// 1 -> HSYNC_DELAY
				WRITE_REG(LCD_INDEX, 0x00);
				READ_REG(LCD_DATA, LTemp);
				LTemp &= ~0xf0000000ul;
				LTemp |=  0x10000000ul;
				WRITE_REG(LCD_DATA, LTemp);
				// disable ratiometric expansion
				LTemp = 0;
				WRITE_REG(LCD_INDEX, 0x04);
				WRITE_REG(LCD_DATA, LTemp);
			}
			if (dm->timing.v_display < PANEL_HEIGHT) {
				ddprintf(("RatExp 2a - PanelID %d, dm->timing.v_display: %d, PANEL_HEIGHT: %d\n", ai->PanelID, dm->timing.v_display, PANEL_HEIGHT));
				// enable ratiometric expansion with blending
				LTemp = (dm->timing.v_display * 1024) / PANEL_HEIGHT;
				LTemp |= 0xC0000000ul;
				WRITE_REG(LCD_INDEX, 0x05);
				WRITE_REG(LCD_DATA, LTemp);
				// use vertical blending, not line replication
				LTemp = (1 << 10);
				WRITE_REG(LCD_INDEX, 0x06);
				WRITE_REG(LCD_DATA, LTemp);
				// strobe v-stretch (from SoftICEing the Windows driver)
				LTemp = (dm->timing.v_display * 1024) / PANEL_HEIGHT;
				LTemp |= 0x40000000ul;
				WRITE_REG(LCD_INDEX, 0x05);
				WRITE_REG(LCD_DATA, LTemp);
				snooze(1000); // Trey's wild-ass guess
				LTemp |= 0x80000000ul;
				WRITE_REG(LCD_DATA, LTemp);
			} else {
				ddprintf(("RatExp 2a - PanelID %d, dm->timing.v_display: %d, PANEL_HEIGHT: %d\n", ai->PanelID, dm->timing.v_display, PANEL_HEIGHT));
				// disable ratiometric expansion
				LTemp = 0;
				WRITE_REG(LCD_INDEX, 0x05);
				WRITE_REG(LCD_DATA, LTemp);
				WRITE_REG(LCD_INDEX, 0x06);
				WRITE_REG(LCD_DATA, LTemp);
			}
		}
		else
		{
			ddprintf(("No RatExp PanelID %d, dm->timing.h_display: %d, PANEL_WIDTH: %d\n", ai->PanelID, dm->timing.h_display, PANEL_WIDTH));
			// no panel attached, so disable ratiometric expansion
			LTemp = 0;
			WRITE_REG(LCD_INDEX, 0x04);
			WRITE_REG(LCD_DATA, LTemp);
			WRITE_REG(LCD_INDEX, 0x05);
			WRITE_REG(LCD_DATA, LTemp);
			WRITE_REG(LCD_INDEX, 0x06);
			WRITE_REG(LCD_DATA, LTemp);
		}
		// restore the index register
		WRITE_REG(LCD_INDEX, lcd_index_save);
	}

	// update framebuffer position and shape
	UpdateCardFBuffer();

	if (LT) {
		//if (ai->PanelID && (dm->timing.h_display <= PANEL_WIDTH))
		if (UseLCD)
		{
			snooze(200000);
		}
	
		// choose output device(s).
		READ_REG(LCD_INDEX, LTemp);
		WRITE_REG(LCD_INDEX, ((LTemp & 0xfffffff0) | 0x01));
		READ_REG(LCD_DATA, LTemp);
		//if (ai->PanelID && (dm->timing.h_display <= PANEL_WIDTH))
		if (UseLCD)
		{
			ddprintf(("LCD and CRT on - PanelID %d, dm->timing.h_display: %d, PANEL_WIDTH: %d\n", ai->PanelID, dm->timing.h_display, PANEL_WIDTH));
			// turn on LCD and CRT
			LTemp &= ~0x50000000ul; // don't shadow
			LTemp |=  0x00000040ul; // don't shadow
			LTemp |=  0x00000003ul;
		}
		else
		{
			ddprintf(("LCD off and CRT on - PanelID %d, dm->timing.h_display: %d, PANEL_WIDTH: %d\n", ai->PanelID, dm->timing.h_display, PANEL_WIDTH));
			// turn off LCD and turn on CRT
			LTemp &=  0xfffffffdul;
			LTemp |=  0x00000001ul;
		}
		WRITE_REG(LCD_DATA, LTemp);
		
		READ_REG(LCD_INDEX, LTemp);
		// enable/disable panel display
		//if (ai->PanelID && (dm->timing.h_display <= PANEL_WIDTH))
		if (UseLCD)
		{
			ddprintf(("panel enabled on - PanelID %d, dm->timing.h_display: %d, PANEL_WIDTH: %d\n", ai->PanelID, dm->timing.h_display, PANEL_WIDTH));
			// clear bit to enable
			LTemp &= ~0x00000100ul;
		}
		else
		{
			ddprintf(("panel DISabled on - PanelID %d, dm->timing.h_display: %d, PANEL_WIDTH: %d\n", ai->PanelID, dm->timing.h_display, PANEL_WIDTH));
			// set bit to disable
			LTemp |=  0x00000100ul;
		}
		WRITE_REG(LCD_INDEX, LTemp);
	
		if (ai->PanelID && (dm->timing.h_display <= PANEL_WIDTH)) 
		{
	// Added begin by hidean(Hitachi)
			READ_REG(LCD_INDEX, LTemp);
			WRITE_REG(LCD_INDEX, ((LTemp & 0xfffffff0) | 0x08));
			WRITE_REG(LCD_DATA, ulTempPOWER_MANAGEMENT); 
	// Added end by hidean(Hitachi)
		}
	}

	// Do post-mode set register sets.
	SetRegisters(LT ? ModeSetFooterTableLT : ModeSetFooterTable);

	// programm linear CLUT for non-indexed modes
	// the calling app must set the CLUT for indexed color modes, 'cause
	// we're not psychic
	if (Bpp > 8) {
		uint32 temp;
		volatile uint8 *foo = (vuint8 *)(&regs[DAC_REGS]);

		regs[DAC_CNTL] &= ~0x8000ul; // Make sure DAC is powered up.
		regs[DAC_CNTL] |= 0x0100ul; // Set DAC to 8-bit

		foo[2] = 0xFF; // DAC mask
		foo[0] = 0x0; // Start at colour 0
		for (temp = 0; temp < 256; temp++)
		{
			foo[1] = temp;
			foo[1] = temp;
			foo[1] = temp;
		}
	}
	/* enable interrupts using the kernel driver */
	interrupt_enable(true);
	dump_regs(("/boot/home/post-mode", false));
	return B_OK;
}

status_t SET_DISPLAY_MODE(display_mode *mode_to_set) {
	display_mode bounds, target;

	/* ask for the specific mode */
	target = bounds = *mode_to_set;
	if (PROPOSE_DISPLAY_MODE(&target, &bounds, &bounds) == B_ERROR)
		return B_ERROR;
	return do_set_display_mode(&target);
}

status_t MOVE_DISPLAY(uint16 h_display_start, uint16 v_display_start) {
	// validate parameters
	// h must be multiple of 8
	if (h_display_start & 0x07)
		return B_ERROR;
	// must not run past end of display
	if ((ai->dm.timing.h_display + h_display_start) > ai->dm.virtual_width)
		return B_ERROR;
	if ((ai->dm.timing.v_display + v_display_start) > ai->dm.virtual_height)
		return B_ERROR;

	// everybody remember where we parked...
	ai->dm.h_display_start = h_display_start;
	ai->dm.v_display_start = v_display_start;
	// move it!	
	UpdateCardFBuffer();

	/* if the display moved, then we might need to update overlay windows */
	if (can_do_overlays) {
		int i;
		for (i = 0; i < MAX_OVERLAYS; i++) {
			if (ai->ovl_tokens[i].used) {
				ati_overlay_token *aot = &(ai->ovl_tokens[i]);
				CONFIGURE_OVERLAY((overlay_token *)aot, aot->ob, &(aot->ow), &(aot->ov));
			}
		}
	}

	return B_OK;
}

void SET_INDEXED_COLORS(uint count, uint8 first, uint8 *color_data, uint32 flags) {
	vuint8 *dacregs  = (vuint8 *)&regs[DAC_REGS];
	
	// HACK - make this do nothing in >8bpp, as the palette table doubles as a
	// gamma table and so this will mess it up. If Be requires that the palette
	// be preserved through mode switches, make our own array to hold the palette
	// and copy it over at mode switch time.
	
	if (ai->dm.space != B_CMAP8) return;

	// paranoia, as it should be set elsewhere, but it doesn't cost much
	regs[DAC_CNTL] &= ~0x8000ul; // Make sure DAC is powered up.
	regs[DAC_CNTL] |= 0x0100ul; // Set DAC to 8-bit

	//   Due to the nature of the ATI card, we need to do bytewise writes
	// to the dac regs.  Stupid, but there you go.  So, we do it like we're
	// supposed to.
	
	// Little-endian, apparently.
	dacregs[2] = 0xFF; // Write mask.
	dacregs[0] = first;
	while (count--) {
		dacregs[1] = *color_data++;
		dacregs[1] = *color_data++;
		dacregs[1] = *color_data++;
	}
}

status_t SET_DPMS_MODE(uint32 dpms_flags) {
	uint32 LTemp;
	status_t err = B_OK;

	READ_REG(CRTC_GEN_CNTL, LTemp);
	LTemp &= ~0x4C;	// clear all disable bits (including display disable)
	switch(dpms_flags) {
		case B_DPMS_ON:	// H: on, V: on
			// do nothing, bits already clear
			WRITE_REG(CRTC_GEN_CNTL, LTemp);
			break;
		case B_DPMS_STAND_BY: // H: off, V: on, display off
			LTemp |= 0x44;
			WRITE_REG(CRTC_GEN_CNTL, LTemp);
			break;
		case B_DPMS_SUSPEND: // H: on, V: off, display off
			LTemp |= 0x48;
			WRITE_REG(CRTC_GEN_CNTL, LTemp);
			break;
		case B_DPMS_OFF: // H: off, V: off, display off
			LTemp |= 0x4C;
			WRITE_REG(CRTC_GEN_CNTL, LTemp);
			break;
		default:
			err = B_ERROR;
			break;
	}
	if (ai->UsingLCD) {
		// choose output device(s).
		READ_REG(LCD_INDEX, LTemp);
		WRITE_REG(LCD_INDEX, ((LTemp & 0xfffffff0) | 0x01));
		READ_REG(LCD_DATA, LTemp);
		if (dpms_flags == B_DPMS_ON)
		{
			// turn on LCD and CRT
			LTemp &= ~0x50000000ul; // don't shadow
			LTemp |=  0x00000040ul; // don't shadow
			LTemp |=  0x00000003ul;
		}
		else
		{
			// turn off LCD and turn on CRT
			LTemp &=  0xfffffffdul;
			LTemp |=  0x00000001ul;
		}
		WRITE_REG(LCD_DATA, LTemp);
		READ_REG(LCD_INDEX, LTemp);
		if (dpms_flags == B_DPMS_ON) {
			// turn on the LCD
			// clear bit to enable
			LTemp &= ~0x00000100ul;
		} else {
			// turn off the LCD
			// set bit to disable
			LTemp |=  0x00000100ul;
		}
		WRITE_REG(LCD_INDEX, LTemp);
	}
	return err;
}

uint32 DPMS_CAPABILITIES(void) {
	return 	B_DPMS_ON | B_DPMS_STAND_BY  | B_DPMS_SUSPEND | B_DPMS_OFF;
}

uint32 DPMS_MODE(void) {
	uint32 LTemp;
	uint32 mode = B_DPMS_ON;

	READ_REG(CRTC_GEN_CNTL, LTemp);
	// what modes are set?
	switch ((LTemp & 0x0C) >> 2) {
		case 0:	// H: on, V: on
			mode = B_DPMS_ON;
			break;
		case 1: // H: off, V: on
			mode = B_DPMS_STAND_BY;
			break;
		case 2: // H: on, V: off
			mode = B_DPMS_SUSPEND;
			break;
		case 3: // H: off, V: off
			mode = B_DPMS_OFF;
			break;
	}
	return mode;
}

// Prototypes
int mbit (uint32 x);

// Macros
#define MAX(x, y) ( ( (x) > (y) ) ? (x) : (y) )
#define MIN(x, y) ( ( (x) > (y) ) ? (y) : (x) )

// 0 - DRAM   1 - EDO    2 - SDRAM/SGRAM
short Ram_Types[] = { 0, 0, 1, 1, 2, 2, 3, 3 };

// Loop Latency Table
short l_table [2][2] = { { 8,  6 },    // DRAM
						 { 9,  8 } };  // SDRAM

// 1M  2M
short n_table [2][2] = { { 3,  2 },   // DRAM
						 { 2,  1 } }; // SDRAM

// Minium number of bits
int mbit (uint32 x)
{
    int i ;

    for (i = 31 ; i >= 0 ; --i)
    {
        if (x & (0x1 << i))
        {
            return (i + 1);
        }
    }

    return 1 ;
} // mbit ()...



/****************************************************************************
 * SetDSP (void)                                                            *
 *  Function: Calculates and sets the DSP_CONFIG and DSP_ON_OFF registers   *
 *    Inputs: NONE                                                          *
 *   Outputs: NONE                                                          *
 ****************************************************************************/
void SetDSP (int bitsperpixel, double ratio, uint32 pixelclock, uint32 XCLK, uint32 *dsp_config, uint32 *dsp_on_off)
{
    int     r_loop ;
    uint8    Mem_Size = (ai->mem_size > (1024 * 1024 * 2)) & 1;
    uint8    Ram_Type = (Ram_Types[ai->Card.ram_type] > 1) & 1;
    uint32   temp32;
    uint8    temp8;
    double  pw; //, ratio;
//    int     Pix1Size[] = { 0, 4, 8, 16, 16, 24, 32, 0};  // Pixel Width conversion
    int     bpp1;         // Pixel Size
    double  xclk;       // XCLK
    double  x1;         // XCLK / Qword
    double  vclk1;      // Dot clock for Display
    int     bx1;        // number of bits for X
    double  t1;         // Max FIFO size
    int     bt1;        // number of bits for integer portion of t
    int     p1;         // Useable precision
    int     f1;         // Actual FIFO size
    double  r1_off;     // FIFO off point
    double  r1_on;      // FIFO on point
    int     n;          // number of cycles/qw
    int     pfc;        // Page fault clock
    int     l;          // latency value
    int     rcc;        // Max random access cycle clocks

    // Get Video Clock frequency
    vclk1 = 0;
    vclk1 = pixelclock; //GetPixelClock ();

    // Get Pixel size
    bpp1 = 0;
    bpp1 = bitsperpixel; //Pix1Size[ (regr8(CRTC_GEN_CNTL+1) & 0x07) >> 0];

    // Numbers of XCLKS in a Qword
    x1 = 0;
    xclk = (double) XCLK; //PLL_BLOCK.XCLK;
    x1 = (xclk * 64.0) / (vclk1 * bpp1);

    // Stretch Ratio CRT1 Only
#if 0
    ratio = 1;
    temp32 = LCD_regr (HORZ_STRETCHING);          // (source/dest) * 4096
    if (temp32 & (1<<31))   // Horizontal Stretch enable
    {
        ratio = 4096.0/ (float) (temp32 & 0xffff);       // (dest/source)
        x1 *= ratio ;
    }
#else
	ddprintf(("ratio: %lf\n", ratio));
	x1 *= ratio;
#endif

    bx1 = 0;
    bx1 = mbit ((uint32)x1);

    t1 = 0;
    t1 = x1 * 32.0;

    bt1 = 0;
    bt1 = mbit ((uint32)(t1));

    p1 = 0;
    p1 = MAX((bt1 - 5), (bx1 - 3)) ; p1 = MIN(7, p1); p1 = MAX(0, p1);

    f1 = 0;
    f1 = MIN((int)( (float) (1<<(5+p1)) / x1 ), 32);

    r1_off = 0;
    r1_off = (double)( (int)(x1 * (f1 - 1) - 0.99999));

    l = l_table[Ram_Type][Mem_Size];
	ddprintf(("l_table[%d][%d] = %d\n", Ram_Type, Mem_Size, l));
    r_loop = l + 2;

    pfc = ai->Card.mem_cycle.trp  + 1 +
          ai->Card.mem_cycle.trcd + 1 +
          ai->Card.mem_cycle.tcrd ;

    n   = n_table[Ram_Type][Mem_Size] ;
	ddprintf(("n_table[%d][%d] = %d\n", Ram_Type, Mem_Size, n));

    rcc = MAX( (ai->Card.mem_cycle.trp + 1 + ai->Card.mem_cycle.tras + 1), (pfc + n));

    r1_on = 3.0 * rcc + (int)(x1 + 0.99999 + 2.0);// * (ratio > 0);
    //r1_on = 2.0 * rcc + pfc + n;

    // DSP Config
    pw = (double) (0x2 << (11-p1)) ;
    pw /= 2.0;
    temp32  = (int)(x1 *pw ) ;
    temp32 |= (r_loop  & 0xf ) << 16 ;
    temp32 |= (p1      & 0x7 ) << 20 ;

	*dsp_config = temp32;
#if 0
    // We will program both DSP register sets, as we will be using
    // the same pixel clock for both displays (if present)
    regw (DSP_CONFIG, regr (DSP_CONFIG) & (1<<15) | temp32);
    regw (DSP2_CONFIG, regr (DSP2_CONFIG) & (1<<15) | temp32);
#endif

    // DSP_ON_OFF
    pw = (double) (0x2 << (6-p1));
    pw /= 2.0;
    temp32  = (int)(r1_on *pw ) << 16;
    temp32 |= (int)(r1_off*pw );

	*dsp_on_off = temp32;
	
#if 0
    regw (DSP_ON_OFF, temp32);
    regw (DSP2_ON_OFF, temp32);
#endif

} // SetDSP ()...
