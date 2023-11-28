#include "private.h"
#include "generic.h"
#include "SetRegisters.h"
#include "CardStartScript.h"
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <errno.h>

typedef struct
{
    uint32 min_freq;	/* min/max pixel clock */
    uint32 max_freq;
    uint16 XCLK;
	// uint16 MCLK_vram; /* XCLK uses the value in this slot of the freq. table */
	uint16 SCLK;
	uint16 MCLK_lowpower;
    uint16 MCLK_dram;
    uint16 ref_freq;
    uint16 ref_divider;
	uint8	SCLK_entry;
	uint8	MCLK_entry;
} _PLL_BLOCK;

_PLL_BLOCK clockinfo;

#if DEBUG > 0
#include <string.h>
#include <errno.h>

void ddump_regs(const char *name, bool serial) {
	int i;
	FILE *f = fopen(name, "w+");
	for (i = 0; i < 0x200; i++) {
		if (f) fprintf(f, "%03x 0x%08lx\n", i, regs[i]);
		if (serial) ddprintf(("%03x 0x%08lx\n", i, regs[i]));
	}
	if (f) fprintf(f, "\nPLL REGS\n");
	if (serial) ddprintf(("\nPLL REGS\n"));
	for (i = 0; i < 0x40; i++) {
		uint32 ltmp;
		ltmp = regs[CLOCK_CNTL];
		ltmp &= ~0x0000fe00;
		ltmp |= (i << 10);
		regs[CLOCK_CNTL] = ltmp;
		ltmp = regs[CLOCK_CNTL];
		if (f) fprintf(f, "%02d 0x%02lx\n", i, (ltmp >> 16) & 0xff);
		if (serial) ddprintf(("%02d 0x%02lx\n", i, (ltmp >> 16) & 0xff));
	}
	if (f) fprintf(f, "\nLCD REGS\n");
	if (serial) ddprintf(("\nLCD REGS\n"));
	for (i = 0; i < 0x40; i++) {
		uint32 ltmp;
		ltmp = regs[LCD_INDEX];
		ltmp &= ~0x0000003f;
		ltmp |= i;
		regs[LCD_INDEX] = ltmp;
		ltmp = regs[LCD_DATA];
		if (f) fprintf(f, "0x%02x 0x%08lx\n", i, ltmp);
		if (serial) ddprintf(("0x%02x 0x%08lx\n", i, ltmp));
	}
	if (f) fclose(f);
	if (serial) ddprintf(("\n"));
}
#endif


#define SET_AI_FROM_SI(si) ((accelerant_info *)(si + 1))

status_t InitMemTypes(void) {
	status_t result = B_ERROR;

	if (accelerantIsClone) {
		if (ai->poolid >= 0) {
			memtypefd = open("/dev/misc/genpool", O_RDWR);
			result = B_OK;
		}
	} else {
		/* create the pool if we're the primary accelerant */
		memtypefd = open("/dev/misc/genpool", O_RDWR);
		if (memtypefd >= 0) {
			BIoctl_CreatePool	icp;
			BPoolInfo		pi;

			memset (&pi, 0, sizeof (pi));
			pi.pi_Pool_AID	= si->fb_area;
			pi.pi_Pool	= (void *) si->framebuffer;
			pi.pi_Size	= ai->mem_size;
			strcpy (pi.pi_Name, "an ati genpool");
			
			icp.icp_PoolInfo	= &pi;
			icp.icp_MaxAllocs	= ai->mem_size >> 13;
			icp.icp_UserDataSize	= 0;

			result = ioctl (memtypefd, B_IOCTL_CREATEPOOL, &icp, sizeof (icp));
			if (result >= 0) {
				ai->poolid = pi.pi_PoolID;
				result = B_OK;
			} else
				ai->poolid = -1;
		}
	}

	can_do_overlays = 0;
	if (memtypefd >= 0) {
		// TODO: figure out which chipsets can do overlays
		can_do_overlays = 1;
	}

	return result;
}

void UninitMemTypes(void) {
	/* whack the pool */
	BPoolInfo	pi;

	memset (&pi, 0, sizeof (pi));
	pi.pi_PoolID = ai->poolid;
	ioctl (memtypefd, B_IOCTL_DELETEPOOL, &pi, sizeof (pi));

	/* nuke the pool id */
	ai->poolid = -1;
	/* close the memtypes driver */
	close(memtypefd);
	memtypefd = -1;
	can_do_overlays = 0;
}

///////////////////////////////////////////////////////////////////////////////
// Verify the Type of Card
//   This function is designed to check the device ID and vendor ID of the
// card we're going to use, and return a subtype.  The theory behind this is
// that we can handle several similar cards as long as we know what subtype
// they are.

static uint32 VerifyCardType(void)
{
	uint32 Count;
	int Found;
	
	//   The following loop walks through our little structure array of
	// supported VendorID/DeviceID combinations.  If it terminates, we
	// don't support the card we have been asked to.
	
	// End of table is indicated by an entry with all zero values. Test HitXClks,
	// which should never be zero.
	Found = 0;
	for (Count = 0; (SupportedDevices[Count].VendorID || SupportedDevices[Count].DeviceID)
		&& (!Found); Count++) {
		//   We're checking both VendorID and DeviceID here, since it is
		// possible that someday we'll have to deal with an OEM version of
		// a card that is the same DID but a different VID or something
		// henna like that.  It doesn't cost us much execution overhead,
		// it happens once in the init code.  Might as well waste a couple
		// of nanoseconds and do this correctly.
		
		Found = (SupportedDevices[Count].VendorID == si->vendor_id) &&
			(SupportedDevices[Count].DeviceID == si->device_id);

		if (Found) {
			// Get copies of the "standard" device limits.
			ai->Card.MaxXClk = SupportedDevices[Count].MaxXClk;
			ai->Card.MinXPerQ = SupportedDevices[Count].HitXClks;
			// FFB Check RAM type and adjust.  A better solution is required.
			// if it's not a plain 3D Rage (aka GT)
			if ((si->device_id != 0x4754) &&
#if EMACHINE
				(si->device_id != 0x474d) &&
#endif
				(si->device_id != 0x4755) &&
				(si->device_id != 0x4756)) {
				// adjust the MaxXClk and MinXPerQ according to RAM type
				uint32 MemType;
				// Determine the type of memory being used.
				READ_REG(CONFIG_STAT0, MemType);
				MemType &= 0x07ul;
			
				ddprintf(("RAM type 0x%02lx.\n", MemType));
				switch(MemType)
				{
				case 1: // Basic DRAM.
				case 2: // EDO DRAM.
				case 3: // Hyper page DRAM/EDO DRAM.
					ai->Card.MaxXClk = 66.0;
					ai->Card.MinXPerQ = 2.0;
					break;
				case 4: // SDRAM.
				case 5: // SGRAM.
					ai->Card.MaxXClk = 100.0;
					ai->Card.MinXPerQ =  1.3;
					break;
				default:
					// Unknown RAM type; bail out.
					return 0;
				}
				
			}
		}
	}

	return Found;
}

// Determines the crystal frequency being used in the card and the maximum card
// bus frequency, and sets the reference PLL, MCLK, and XCLK appropriately.
void Config_Clocks(bool slowMCLK)
{
	float CrystalFreq;
	float BusFreq;
	float Temp;
	unsigned char RefDiv;
	unsigned char MDiv, SDiv;
	unsigned long LTemp;
	unsigned char *CrystalPtr;

    uint8 *bios_ptr;
    uint16 rom_table_offset;
    uint16 freq_table_offset;

	// Autodetect the crystal frequency.

	// ATI says:
	// Bios segment, offset 48h -> word offset of ROM header.
	// ROM header offset 10h -> word pointer to frequency table.
	// Frequency table offset 8h -> reference frequency in tens of kHz (word).

	CrystalPtr = (unsigned char *) si->rom;
	LTemp = *((uint16 *) (CrystalPtr + 0x48));
	LTemp = *((uint16 *) (CrystalPtr + LTemp + 0x10));
	LTemp = *((uint16 *) (CrystalPtr + LTemp + 0x08));

    // First, let's get the pointer to the BIOS header.
    bios_ptr = (uint8 *)si->rom;

    rom_table_offset = (uint16)(bios_ptr[0x48] | (bios_ptr[0x49] << 8));

    freq_table_offset = bios_ptr[rom_table_offset + 16] |
                        (bios_ptr[rom_table_offset + 17] << 8);

	// offset 18: MCLK freq in normal mode for VRAM boards
    clockinfo.MCLK_lowpower = bios_ptr[freq_table_offset + 14] |
                     (bios_ptr[freq_table_offset + 15] << 8);

    clockinfo.MCLK_dram = bios_ptr[freq_table_offset + 16] |
                     (bios_ptr[freq_table_offset + 17] << 8);

    clockinfo.XCLK = bios_ptr[freq_table_offset + 18] |
                     (bios_ptr[freq_table_offset + 19] << 8);

    clockinfo.ref_freq = bios_ptr[freq_table_offset + 8] |
                     (bios_ptr[freq_table_offset + 9] << 8);

    clockinfo.ref_divider = bios_ptr[freq_table_offset + 10] |
                     (bios_ptr[freq_table_offset + 11] << 8);

    clockinfo.min_freq = bios_ptr[freq_table_offset + 2] |
                     (bios_ptr[freq_table_offset + 3] << 8);

    clockinfo.max_freq = bios_ptr[freq_table_offset + 4] |
                     (bios_ptr[freq_table_offset + 5] << 8);

    clockinfo.SCLK = bios_ptr[freq_table_offset + 20] |
                     (bios_ptr[freq_table_offset + 21] << 8);

    clockinfo.MCLK_entry = bios_ptr[freq_table_offset + 22];

    clockinfo.SCLK_entry = bios_ptr[freq_table_offset + 23];

	ddprintf(("\n\n       XCLK: %d\n", clockinfo.XCLK));
	ddprintf(("MCLK_lowpow: %d\n", clockinfo.MCLK_lowpower));
	ddprintf(("  MCLK_dram: %d\n", clockinfo.MCLK_dram));
	ddprintf(("       SCLK: %d\n", clockinfo.SCLK));
	ddprintf((" MCLK_entry: %d\n", clockinfo.MCLK_entry));
	ddprintf((" SCLK_entry: %d\n", clockinfo.SCLK_entry));
	ddprintf(("   ref_freq: %d\n", clockinfo.ref_freq));
	ddprintf(("ref_divider: %d\n", clockinfo.ref_divider));
	ddprintf(("   min_freq: %ld\n", clockinfo.min_freq));
	ddprintf(("   max_freq: %ld\n\n\n", clockinfo.max_freq));
	
#if 0
	CrystalFreq = 29.498928 * 1.0e6;
#else
	// According to ATI, there are only three values that will show up here.
	// If we recognize these, store them at full precision. Otherwise, take
	// the 3-4 digits we're given.
	switch (LTemp)
	{
	case 1432:
		CrystalFreq = 14.31818 * 1.0e6;
		break;
	case 2860:
		CrystalFreq = 28.636363 * 1.0e6;
		break;
	case 2950:
		CrystalFreq = 29.498928 * 1.0e6;
		break;
	default:
		CrystalFreq = LTemp;
		CrystalFreq *= 1.0e4;
	}
#endif
	// Diagnostics.
	ddprintf(("Crystal frequency autodetected as %1.4f MHz.\n", CrystalFreq / 1.0e6));


	// Determine maximum bus frequency.
	// This depends only on device ID, and so is stored in the device table.
#if 0
	BusFreq = ai->Card.MaxXClk * 1.0e6;
#else
	BusFreq = clockinfo.XCLK * 10000.0;
#endif


	// Determine the maximum supported pixel clock frequency.

	// Should work for any Rage Pro and maybe any Mach64.
//	ai->Card.MaxVClock = 110.0 * 1.0e6; // Conservative.
//	ai->Card.MaxVClock = 200.0 * 1.0e6; // Insanely aggressive.
#if 0
	ai->Card.MaxVClock = 202.5 * 1.0e3; // Insanely aggressive (but at least measured in the right units).
#else
	ai->Card.MaxVClock = clockinfo.max_freq * 10;
#endif
	/* these are probably a function of memory bandwidth, but I don't have the formula */
	ai->pix_clk_max8 = ai->pix_clk_max16 = (uint32)ai->Card.MaxVClock;
	ai->pix_clk_max32 = (uint32)(ai->Card.MaxVClock * 0.80);

	// Determine the maximum supported data rate as a function of xclock while we're
	// at it.
	// Update: the device table now has float values with safety factors already
	// included. Just copy this directly. Safety factor is typically about 1.3.

	// NOTE: for cards with <2 megs of memory, double this as they only have a 32-bit
	// data width!
	// Check this in memory setup routine.
	// Set in VerifyCardType
	//ai->.Card.MinXPerQ = SupportedDevices[ai->Type].HitXClks;


	// Calculate the divider for the reference clock.

	// Go for about 450 kHz, as with the @play card.
#if 0
#if EMACHINE
	Temp = CrystalFreq / (460.0 * 1.0e3);
#else
	Temp = CrystalFreq / (450.0 * 1.0e3);
#endif
	RefDiv = (unsigned char) (0.5 + Temp);
#else
	RefDiv = clockinfo.ref_divider;
#endif
	// Store the real reference clock freqency.
	Temp = RefDiv;
	ai->Card.RefClock = CrystalFreq / Temp;

	ddprintf(("BusFreq: %f\nRefClock: %f\n", BusFreq, ai->Card.RefClock));
	
	// Calculate the feedback divider value for PLLMCLK, which drives
	// XCLK and MCLK.

	Temp = BusFreq / ai->Card.RefClock;
	MDiv = (unsigned char) (0.5 + Temp); // PLLMCLK = REF * 2 * MDiv, or 2x BusFreq.

	// Store the real frequency of XCLK.
#if 0
	Temp = MDiv;
	ai->Card.XClock = ai->Card.RefClock * Temp;
#else
	ai->Card.XClock = clockinfo.XCLK * 10000.0;
#endif

#if 0
#if 1
	if (slowMCLK) {
		// Calculate the feedback divider value for PLLSCLK, which drives
		// MCLK.
		Temp = BusFreq / ai->Card.RefClock;
		// Slow down MClk a bit. This uses the same ratio found in the 4c42 Rage LT Pro.
#if EMACHINE
		Temp = (ai->Card.RefClock * 2) / clockinfo.SCLK;
		Temp *= 0.79;
		//Temp *= clockinfo.SCLK;
		//Temp /= clockinfo.XCLK;
#else
		Temp *= 0.75;
#endif
		//Temp *= 0.50;
		SDiv = (unsigned char) (0.5 + Temp); // PLLSCLK = REF * 2 * SDiv.
		ddprintf(("SDiv: %d (0x%02x)\n", SDiv, SDiv));
	}
#else
	if (clockinfo.XCLK != clockinfo.SCLK) {
		Temp = (clockinfo.SCLK * 2) / clockinfo.ref_freq;
		SDiv = (unsigned char) (0.5 + Temp); // PLLSCLK = REF * 2 * SDiv.
	}
#endif
#endif

	// Set the appropriate PLL values.
#if 0
	// Critical section - messing with card registers.
	//lock_card();

	// Write reference divider.
	LTemp = regs[CLOCK_CNTL];
	LTemp &= 0xFF0001FF;
	LTemp |= 0x02 << 10; // PLL_REF_DIV
	LTemp |= 0x00000200; // write mode
	LTemp |= ((unsigned long) RefDiv) << 16; // value to write
	regs[CLOCK_CNTL] = LTemp;

	// Write PLLMCLK divider.
	LTemp = regs[CLOCK_CNTL];
	LTemp &= 0xFF0001FF;
	LTemp |= 0x04 << 10; // MCLK_FB_DIV
	LTemp |= 0x00000200; // write mode
	LTemp |= ((unsigned long) MDiv) << 16; // value to write
	regs[CLOCK_CNTL] = LTemp;

	if (slowMCLK) {
		// Write PLLSCLK divider.
		LTemp = regs[CLOCK_CNTL];
		LTemp &= 0xFF0001FF;
		LTemp |= 0x15 << 10; // SPLL_FB_DIV
		LTemp |= 0x00000200; // write mode
		LTemp |= ((unsigned long) SDiv) << 16; // value to write
		regs[CLOCK_CNTL] = LTemp;
	}
	// Set to read PLL 0 (safe state).
	LTemp = regs[CLOCK_CNTL];
	LTemp &= 0xFF0001FF;
	regs[CLOCK_CNTL] = LTemp;
#endif
	// End critical section.
	//unlock_card();
}


// Determines the amount of card memory available by seeing how far up the frame buffer
// data can be written and read back reliably. Does a paranoia check to make sure that
// It isn't just wrapping, either.
unsigned long Get_Card_Mem_Size()
{
	// Allowed sizes actually go up to 16 megs, but clip at the register window for now.
	const unsigned long AllowedSizes[] =
	{ 0x00080000, 0x00100000, 0x00180000, 0x00200000,
	  0x00280000, 0x00300000, 0x00380000, 0x00400000,
	  0x00500000, 0x00600000, 0x00700000, 0x007FF800,
	  0x0 };

	unsigned long MaxMem;
	unsigned long RWIndex;
	int iMaxIndex, iTestIndex, iX;
	unsigned long LTemp;
	int IsOk;
	uint32	*VramBase;
	
	VramBase = (uint32 *)si->framebuffer;
	MaxMem = 0; // Default.
	IsOk = 1;
	// Step through ever-larger memory sizes, recording size if passes test and
	// ignoring otherwise.
	for (iMaxIndex = 0; (AllowedSizes[iMaxIndex] != 0) && IsOk; iMaxIndex++)
	{
		// Write test values to the linear aperature.
		// Only need to do this for the farthest location, as previous locations
		// already have been written to in previous passes.
		RWIndex = AllowedSizes[iMaxIndex];
		RWIndex = (RWIndex - 16384) >> 2;
		for (iX = 0; iX < 4096; iX++)
		{
			LTemp = RWIndex;
			// Hash LTemp. As the parameters for the hash are prime, it should
			// be extremely unlikely to get these values through a glitch, and
			// the pattern only repeats at prime intervals, so aliasing shouldn't
			// fool the test either.
			LTemp = (263 * (LTemp % 65521) + 29) % 65521;
			// Extend this to 32 bits.
			LTemp |= (LTemp ^ 0x0000FFFFul) << 16;
			VramBase[RWIndex] = LTemp;
			RWIndex++;
		}

		// Verify that all test patterns are still intact. If values written past the
		// end of memory drop off the face of the frame buffer, the farthest pattern(s)
		// will not be what they should be. If values written past the end of memory
		// wrap, then previous patterns will be overwritten (or partly overwritten,
		// as the test location at 8 megs is actually at 8 megs - 2k).
		// As soon as an invalid value is detected, IsOk is set to 0, which should
		// quickly terminate the test loops.
		for (iTestIndex = 0; (iTestIndex <= iMaxIndex) && IsOk; iTestIndex++)
		{
			RWIndex = AllowedSizes[iTestIndex];
			RWIndex = (RWIndex - 16384) >> 2;
			for (iX = 0; (iX < 4096) && IsOk; iX++)
			{
				LTemp = RWIndex;
				// Hash LTemp. As the parameters for the hash are prime, it should
				// be extremely unlikely to get these values through a glitch, and
				// the pattern only repeats at prime intervals, so aliasing shouldn't
				// fool the test either.
				LTemp = (263 * (LTemp % 65521) + 29) % 65521;
				// Extend this to 32 bits.
				LTemp |= (LTemp ^ 0x0000FFFFul) << 16;
				// Test against the value read from the frame buffer.
				if (VramBase[RWIndex] != LTemp)
					IsOk = 0;
				RWIndex++;
			}
		}

		// If the test patterns check out, update MaxMem accordingly.
		if (IsOk)
			MaxMem = AllowedSizes[iMaxIndex];
	}

	// TEST
	ddprintf(("\nAvailable card memory: %lu bytes.\n", MaxMem));

	return MaxMem;
}


// Determines the memory latency and page size information needed for setting
// the DSP regs, determines the total amount of on-board memory, and performs
// register sets required for proper memory access.
// Returns 1 if successful and 0 if an error occurs.
int Config_Memory()
{
	unsigned long MemAvailable;
	unsigned long NominalMemAvailable;
	unsigned long MemType;
	unsigned long LTemp;
	static float xclk_limits[16] = {
		50.0e6,	66.0e6,	75.0e6,	83.0e6,	90.0e6,	95.0e6,	100.0e6, 105.0e6,
		110.0e6, 115.0e6, 120.0e6, 125.0e6, 133.0e6, 143.0e6, 166.0e6,
		9999.0e6
	};
	// By good fortune, memory configuration registers are set to strap to their
	// correct settings. We just have to read them and tweak one or two parameters.

	// This above statment is incorrect.  The BIOS configures the MEM_CNTL register
	// at POST-time.  If the BIOS doesn't run, we're hosed :-(

	// Lock access to the card registers before messing with them.
	//lock_card();


	// Tweak the memory refresh rate to match our card bus frequency.
	// Note: setting this too low does no harm (it just drains bandwidth). Too high
	// and memory isn't refreshed often enough (very bad).
	READ_REG(MEM_CNTL, LTemp);
	ddprintf(("MEM_CNTL refresh: 0x%08lx\n", LTemp & 0x00f00000));
#if EMACHINE
	LTemp &= ~0x00f00000ul;

	{ uint32 x = 0;
	ddprintf(("ai->Card.XClock: %f\n", ai->Card.XClock));
	while (ai->Card.XClock >= xclk_limits[x]) 
	{
		ddprintf(("%d -> %f\n", x, xclk_limits[x]));
		x++;
	}
	LTemp |= (x << 20);
	}
#else
	LTemp &= ~0x00700000ul;

	if (ai->Card.XClock < 4.4e7)
		LTemp |= 0x00000000ul;
	else if (ai->Card.XClock < 5.0e7)
		LTemp |= 0x00100000ul;
	else if (ai->Card.XClock < 5.5e7)
		LTemp |= 0x00200000ul;
	else if (ai->Card.XClock < 6.6e7)
		LTemp |= 0x00300000ul;
	else if (ai->Card.XClock < 7.5e7)
		LTemp |= 0x00400000ul;
	else if (ai->Card.XClock < 8.0e7)
		LTemp |= 0x00500000ul;
	else if (ai->Card.XClock < 1.0e8)
		LTemp |= 0x00600000ul;
	else
		LTemp |= 0x00700000ul;
#endif
	ddprintf(("MEM_CNTL refresh: 0x%08lx\n", LTemp & 0x00f00000));

	WRITE_REG(MEM_CNTL, LTemp);


	// Get the nominal amount of memory on the card, as this influences a few settings
	// and we can't check it directly yet.

	READ_REG(MEM_CNTL, LTemp);

	{
	uint8 temp8 = (LTemp >> 8) & 0xff;

    ai->Card.mem_cycle.trp = temp8 & 0x03;
    ai->Card.mem_cycle.trcd = (temp8 >> 2) & 0x03;
    ai->Card.mem_cycle.tcrd = (temp8 >> 4) & 0x01;

    temp8 = (LTemp >> 16) & 0xff;

    ai->Card.mem_cycle.tras = temp8 & 0x07;
	}

	LTemp &= 0x0Ful;
	if (LTemp < 8)
		NominalMemAvailable = (LTemp + 1) * 0x00080000;
	else if (LTemp < 12)
		NominalMemAvailable = (LTemp - 3) * 0x00100000;
	else
		NominalMemAvailable = (LTemp - 7) * 0x00200000;


	// Determine the type of memory being used.
	READ_REG(CONFIG_STAT0, LTemp);
	ai->Card.ram_type = MemType = LTemp & 0x07ul;



	// Unlock access to the card registers, as we might return here.
	//unlock_card();


	// Calculate latency and page size based on type of memory.
	// Check for cards with < 2 megabytes of memory, also, as these have different
	// timing parameters.

	ddprintf(("RAM type 0x%02lx.\n", MemType));
	switch(MemType)
	{
	case 1: // Basic DRAM.
	case 2: // EDO DRAM.
	case 3: // Hyper page DRAM/EDO DRAM.
		if (NominalMemAvailable <= 0x00200000)
		{
			ai->Card.Latency = 8;
			ai->Card.PageSize = 10;
		}
		else // 2 megabytes or more.
		{
			ai->Card.Latency = 6;
			ai->Card.PageSize = 9;
		}
		break;
	case 4: // SDRAM.
	case 5: // SGRAM.
		if (NominalMemAvailable <= 0x00200000)
		{
			ai->Card.Latency = 9;
			ai->Card.PageSize = 10;
		}
		else // 2 megabytes or more.
		{
			ai->Card.Latency = 8;
			ai->Card.PageSize = 8;
		}
		break;
	default:
		// Unknown RAM type; bail out.
		return 0;
	}


	// While we're here, check for <=2 megs and adjust data bandwidth specifications
	// appropriately. Cards with <=2 megs were designed with a 32-bit memory bus instead
	// of a 64-bit one.

	if (NominalMemAvailable <= 0x00200000)
	{
		ai->Card.MinXPerQ *= 2.0;
	}


	// Lock access to the card registers again.
	//lock_card();


	// Disable the VGA aperature, while we're poking at the memory registers.
	// Keep the ROM in its normal location.
	READ_REG(CONFIG_STAT0, LTemp);
	LTemp &= ~0x18ul;
	LTemp &= ~0x80ul;	// FFB - LT-specific?
	//LTemp &= ~0x20ul;	// GUI clock controled by GUI activity
	WRITE_REG(CONFIG_STAT0, LTemp);


	// Unlock access to the card registers; no more modifications needed.
	//unlock_card();


	// Wait for a few ms to allow the card to stabilize before testing memory.
	// Not sure if this is needed, but it doesn't hurt.
	snooze(10000);


	// Set a pointer to just past the end of useable video memory.

	ai->mem_size = MemAvailable = Get_Card_Mem_Size();
#if 0
	// HACK - clip to 8 megs minus 2k, so that don't overwrite the register aperature.
	// I.e., assume aperature enabled and <= 8 megs always.
	if (MemAvailable > 0x007FF800)
		MemAvailable = 0x007FF800;

	ai->VramEnd = (unsigned long *) (((char *) ai->VramBase) + MemAvailable);
#endif

	return (MemAvailable != 0);
}

/* initialize the accelerant */
status_t INIT_ACCELERANT(int the_fd) {
	status_t result;
	ati_get_private_data gpd;
	/* memorize the file descriptor */
	fd = the_fd;
	/* note that we're the primary accelerant */
	accelerantIsClone = 0;
	/* set the magic number so the driver knows we're for real */
	gpd.magic = ATI_PRIVATE_DATA_MAGIC;
	/* contact driver and get a pointer to the registers and shared data */
	ddprintf(("prior to ioctl(ATI_GET_PRIVATE_DATA)\n"));
	result = ioctl(fd, ATI_GET_PRIVATE_DATA, &gpd, sizeof(gpd));
	ddprintf(("ioctl(ATI_GET_PRIVATE_DATA) returns %ld\n", result));
	if (result != B_OK) goto error0;
	
	/* transfer the info to our globals */
	si = gpd.si;
	ai = SET_AI_FROM_SI(si);
	regs = si->regs;
	ddprintf(("original regs is 0x%08lx\n", (uint32)regs));

	dump_regs(("/boot/home/pre-dump", false));

	/* fail by default */
	result = B_ERROR;
	
	/* bail if we can't support this card */
	if (!VerifyCardType()) goto error0;

	/* bail if we can't find an appropriate ROM */
	/* perhaps this won't be required in the future? */
	/* this is now done in the kernel driver to deal with mapping motherboard implementations */
	// if (!IsOurROM(si->rom)) goto error0;

	// Do the register sets required to initialize the card, configure it for
	// proper memory access, and set it up with clock frequencies that our mode
	// set routines like.
	{
		REGSET_STRUCT *start, *end;
		bool isLT = false;
		bool slowMCLK = false;
#if EMACHINE
		isLT = true;
		slowMCLK = true;
		start = SetupStartLT;
		end = SetupEndLT;
#else
		switch (si->device_id & 0xff00) {
		case 0x4C00:
			start = SetupStartLT;
			end = SetupEndLT;
			isLT = true;
			slowMCLK = true;
			break;
		default:
			start = SetupStart;
			end = SetupEnd;
			break;
		}
#endif

		// HACK ALERT - doing this to slow down the graphics engine clock
		switch (si->device_id) {
		case 0x4749:
#if 0
EMACHINE
		case 0x474d:
#endif
		case 0x4750:
		case 0x4751:
			start = SetupStartLT;
			end = SetupEnd;
			slowMCLK = true;
			break;
		}
		SetRegisters(start);
		Config_Clocks(slowMCLK);
		// Check to see that this is successful.
		if (!Config_Memory())
		 goto error0; // Bail if not successful.
		SetRegisters(end);

		if (isLT) {
			// Is there an LCD panel attached?
			// FFB: this is a cheesey HACK, and we should be doing DDC2+EDID, but hey...
			// Reading CONFIG_STAT0 at least gives us what the BIOS thinks is there
			READ_REG(CONFIG_STAT0, ai->PanelID);
			ai->PanelID = (ai->PanelID & 0x001f0000) >> 16;
		}
		ai->UsingLCD = 0;
	}
	ddprintf(("start/end regs programmed\n"));

#if 0
	// Initialize the DAC to greyscale for gamma correction.
	regs[DAC_CNTL] &= ~0x8000ul;	// Make sure DAC is powered up.
	regs[DAC_CNTL] |= 0x0100ul;		// Set DAC to 8-bit
	
	{
	uint32 temp;
	volatile uint8 *foo = (vuint8 *)(&regs[DAC_REGS]);
	
	foo[2] = 0xFF; // DAC mask
	foo[0] = 0x0; // Start at colour 0
	for (temp = 0; temp < 256; temp++)
		{
		foo[1] = temp;
		foo[1] = temp;
		foo[1] = temp;
		}
	}
#endif
	
	// Initialize the frame buffer pointer. This is fixed, whatever happens to the
	// frame buffer's dimensions or the screen position.
	// Frame buffer starts 1k after the start of card memory, to leave room for the
	// cursor buffer.
	//ai->FrameBase = (void *) (((char *) (ai->VramBase)) + 1024);
	//ai->fbc.frame_buffer = (void *)(((char *)si->framebuffer) + 1024);
	//ai->fbc.frame_buffer_dma = (void *)(((char *)si->framebuffer_pci) + 1024);
	
	// Set up supported mode info.
	// We support everything in principle, though card memory may limit the modes
	// offered.
	// As checking is done in the mode set and frame buffer manipulation routines,
	// setting this accurately (instead of just claiming to support everything) is
	// more a courtesy to Be than anything else, as none of our routines (except
	// possibly mode set) bother to check this, and applications should be checking
	// the mode set return codes anyways.
	
	//MemAvailable = ((unsigned char *) ai->VramEnd) - ((unsigned char *) ai->FrameBase);
	//ai->Display.Space = AvailableModeVector(MemAvailable);
	
	
	// Initialize the hardware cursor fields of ai->
	
	// Put the cursor buffer at the beginning of card memory, before the frame
	// buffer.
	ai->Cursor.pCursorBuffer = si->framebuffer;
	
	// Set cursor to "not defined", and zero the rest of the cursor fields.
	ai->Cursor.CursorDefined		= 0;
	ai->Cursor.Width				= 0;
	ai->Cursor.Height			= 0;
	ai->Cursor.HotX				= 0;
	ai->Cursor.HotY				= 0;

	ai->fbc.frame_buffer = ai->fbc.frame_buffer_dma = 0;
	memset(&(ai->fb_spec), 0, sizeof(ai->fb_spec));
	memset(&(ai->ovl_buffer_specs[0]), 0, sizeof(ai->ovl_buffer_specs));
	memset(&(ai->ovl_buffers[0]), 0, sizeof(ai->ovl_buffers));
	memset(&(ai->ovl_tokens[0]), 0, sizeof(ai->ovl_tokens));
	/* init the mem-types driver.  Ignore errors for now. */
	ddprintf(("initing MemTypes\n"));
	(void)InitMemTypes();
	ddprintf(("MemTypes initialized - memtypefd: %d\n", memtypefd));

	/* if using memtypes, allocate cursor here */
	if (memtypefd >= 0) {
		/* alloc cursor from genpool */
		BMemSpec ms;

		// init the memspec
		memset(&ms, 0, sizeof(ms));
		ms.ms_PoolID = ai->poolid;
		// cursor data takes 1KB
		ms.ms_MR.mr_Size = 1024;
		// aligned on 1KB boundary
		ms.ms_AddrCareBits = (1024 - 1);
		ms.ms_AddrStateBits = 0;
		if (ioctl(memtypefd, B_IOCTL_ALLOCBYMEMSPEC, &ms, sizeof(ms)) == 0)
			ai->Cursor.pCursorBuffer =
			 (uchar *) si->framebuffer + ms.ms_MR.mr_Offset;
		else {
			/* we can't allocate 1K out of the 2MB+ frame buffer??? */
			/* shut down memtypes */
			UninitMemTypes();
		}
		ddprintf(("after ioctl(B_IOCTL_MEMALLOC)\n"));
	}

	/* create a list of video modes that this device can generate */
	create_mode_list();
#if 0
	/* how fast can we go? */
	calc_max_clocks();
#endif

	/* init the shared semaphore */
	ai->engine_sem = create_sem(0, "an ati engine sem");
	ai->engine_ben = 0;
	/* count of issued parameters or commands */
	ai->last_idle_fifo = ai->fifo_count = 0;
	/* init fifo depth */
	switch(regs[GUI_CNTL] & 0x3) {
		case 0: ai->fifo_limit = 192; break;
		case 1: ai->fifo_limit = 128; break;
		case 2: ai->fifo_limit = 64; break;
		default:
			/* this is reserved, if we see it pretend we didn't */
			ai->fifo_limit = 64; break;
	}
	/* notify cursor routines to set the color */
	ai->set_cursor_colors = 1;
	/* ensure cursor state */
	SHOW_CURSOR(false);
	/* a winner! */
	result = B_OK;
error0:
	return result;
}

ssize_t ACCELERANT_CLONE_INFO_SIZE(void) {
	return MAX_ATI_DEVICE_NAME_LENGTH;
}

void GET_ACCELERANT_CLONE_INFO(void *data) {
	/* a null terminated string with the device name */
	ati_device_name dn;
	status_t result;
	
	dn.magic = ATI_PRIVATE_DATA_MAGIC;
	dn.name = (char *)data;
	result = ioctl(fd, ATI_DEVICE_NAME, &dn, sizeof(dn));
	ddprintf(("ioctl(ATI_DEVICE_NAME) returns %ld\n", result));
}

status_t CLONE_ACCELERANT(void *data) {
	status_t result = B_OK;
	ati_get_private_data gpd;
	char path[MAXPATHLEN];

	/* the data is the device name */
	strcpy(path, "/dev/");
	strcat(path, (const char *)data);
	ddprintf(("CLONE_ACCELERANT: opening %s\n", path));
	/* open the device, the permissions aren't important */
	fd = open(path, B_READ_WRITE);
	if (fd < 0) {
		result = fd;
		ddprintf(("Open failed: %d/%d (%s)\n", result, errno, strerror(errno)));
		goto error0;
	}

	/* note that we're a clone accelerant */
	accelerantIsClone = 1;

	/* set the magic number so the driver knows we're for real */
	gpd.magic = ATI_PRIVATE_DATA_MAGIC;
	/* contact driver and get a pointer to the registers and shared data */
	result = ioctl(fd, ATI_GET_PRIVATE_DATA, &gpd, sizeof(gpd));
	ddprintf(("ioctl(ATI_GET_PRIVATE_DATA) returns %ld\n", result));
	if (result != B_OK) goto error0;
	
	/* transfer the info to our globals */
	si = gpd.si;
	ai = SET_AI_FROM_SI(si);
	regs = si->regs;
	ddprintf(("cloned regs is 0x%08lx\n", (uint32)regs));

	/* get shared area for display modes */
	result = my_mode_list_area = clone_area(
		"ATI cloned display_modes",
		(void **)&my_mode_list,
		B_ANY_ADDRESS,
		B_READ_AREA,
		ai->mode_list_area
	);
	if (result < B_OK) goto error1;

	/* get memtypes access for this copy of the driver */
	result = InitMemTypes();
	if (result != B_OK) goto error1;

	/* all done */
	result = B_OK;
	goto error0;
	
error1:
	close(fd);
error0:
	return result;
}

void UNINIT_ACCELERANT(void) {
	/* free our mode list area */
	delete_area(my_mode_list_area);
	/* if we're using memtypes */
	if (memtypefd >= 0) {
		/* if we're the primay accelerant */
		if (!accelerantIsClone) {
			/* whack the pool */
			BPoolInfo	pi;
		
			memset (&pi, 0, sizeof (pi));
			pi.pi_PoolID = ai->poolid;
			ioctl (memtypefd, B_IOCTL_DELETEPOOL, &pi, sizeof (pi));

			/* nuke the pool id */
			ai->poolid = -1;
		}
		/* close the memtypes driver */
		close(memtypefd);
		memtypefd = -1;
	}

	/* close the file handle ONLY if we're the clone */
	if (accelerantIsClone) close(fd);
}
