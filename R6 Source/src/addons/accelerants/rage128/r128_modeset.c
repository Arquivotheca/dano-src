//////////////////////////////////////////////////////////////////////////////
// Mode Set Related Code
//
//    This file contains all functions relating to mode manipulation.
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Includes //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include <math.h>

#include <common_includes.h>
#include <accel_includes.h>
#include <registersR128.h>
#include <hardware_specific_ioctl.h>

//////////////////////////////////////////////////////////////////////////////
// Local Type Definitions ////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// HACK - These should be in a header somewhere else.

// Structure contianing data required for DDA register sets. These registers
// govern transfer of data from memory to the DAC during display. I've
// already expressed my opinion on this elsewhere.

typedef struct
{
  // The fairly complex calculations in the Rage 128 sample code boil down to
  // much simpler formulae if you lump together all factors that are constant
  // for a given card. The revised formulae are:

  // XClks per xfer = (XClk * dda_fifo_width) / (VClk * storage bpp), shifted
  // to be an integer.fraction representation.

  // On = on_constant + XClks per xfer (not shifted), shifted to be an
  // integer.fraction representation.

  // Off = XClks per xfer (shifted) * off_constant

  uint32 on_constant;
  uint32 off_constant;
  uint32 loop_latency;
  uint32 dda_fifo_width;
  uint32 dda_fifo_depth;
} DDA_INFO;


// Structure for table entries describing DDA info for various cards.
//    Terminated by an entry with device_id 0xFFFF. This entry contains
// default DDA_INFO values.

typedef struct
{
  uint16 device_id; // PCI device ID. Set to 0xFFFF to terminate the list.
  uint32 mem_type; // Memory type, from MEM_CNTL.
  DDA_INFO card_dda_info; // DDA_INFO for this card configuration.
} DDA_TABLE_ROW;


//////////////////////////////////////////////////////////////////////////////
// Local Constants ///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// Table describing DDA info for the various card configurations. Terminated
// by an entry with 0xFFFF device_id, which contains default DDA values.
// See the ATI source code for the origin of these values. That's the only
// source of documentation on them that I have.

// HACK - This should probably be somewhere else.

DDA_TABLE_ROW dda_info_table[] =
{
  // SDR SGRAM (1:1).

  // NOTE: off_const is 28 according to ATI's sample code, which specifies
  // a display FIFO 32 entries deep. However, mode set snooping indicates
  // that an off_const of 60 is being used, corresponding to a display FIFO
  // 64 entries deep. Keeping this at the 32-deep value for now. Both work.

  // 128-bit cards.
  { 0x5245,     0,    { 30,     28,     16,     128,     32 } },
  { 0x5246,     0,    { 30,     28,     16,     128,     32 } },
  // 64 bit cards.
  { 0x524B,     0,    { 46,     28,     17,     128,     32 } },
  { 0x524C,     0,    { 46,     28,     17,     128,     32 } },

  // SDR-SGRAM (2:1).
  { 0x5245,     1,    { 24,     28,     16,     128,     32 } },
  { 0x5246,     1,    { 24,     28,     16,     128,     32 } },
  { 0x524B,     1,    { 24,     28,     16,     128,     32 } },
  { 0x524C,     1,    { 24,     28,     16,     128,     32 } },

  // DDR SGRAM (?:?).
  { 0x5245,     2,    { 31,     28,     16,     128,     32 } },
  { 0x5246,     2,    { 31,     28,     16,     128,     32 } },
  { 0x524B,     2,    { 31,     28,     16,     128,     32 } },
  { 0x524C,     2,    { 31,     28,     16,     128,     32 } },

  // Default / End of List.
  // Use values for SDR SGRAM (1:1) 64-bit.
  { 0xFFFF,     0,    { 46,     28,     17,     128,     32 } },
};


//////////////////////////////////////////////////////////////////////////////
// Local Function Prototypes /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Initialize Constant Registers
//    This function initializes several registers that don't vary with mode
// sets to default values.

static void InitConstantRegs(void);


//////////////////////////////////////////////////////////////////////////////
// Initialize Gamma Correction Table
//    This function initializes the gamma correction table to a linear ramp.
// In 8 bpp mode, this will actually wind up setting the palette to a grey
// scale instead of setting the gamma table, but we'll call that a feature.

static void InitGammaTable(void);


//////////////////////////////////////////////////////////////////////////////
// Get XClk Frequency
//    This function infers the current XClk frequency (in kHz) and returns it.
// HACK - This shouldn't be necessary, or at worst should be done only once
// during initialization. The XClk frequency should be stored in a
// SHARED_INFO field.

uint32 GetXClk(void);


//////////////////////////////////////////////////////////////////////////////
// Get PClk Frequency
//    This function infers the current PClk frequency (in kHz) and returns it.
// This is used as a diagnostics function.

uint32 GetPClk(void);


//////////////////////////////////////////////////////////////////////////////
// Dump Clocking Info
//    This function dumps XClk, PClk, and DDA register information to debug
// output. Used as a diagnostics function.

void DumpClockingInfo(void);


//////////////////////////////////////////////////////////////////////////////
// Dump CRTC Info
//    This function dumps the current CRTC configuration to debug output.
// Used as a diagnostics function.

void DumpCRTCInfo(void);


//////////////////////////////////////////////////////////////////////////////
// Dump General Register Contents
//    This function dumps the contents of the PLL registers, CRTC registers,
// DAC registers, and various configuration registers. Pretty much everything
// not related to 2D or 3D drawing, in other words.

void DumpGeneralRegisters(void);


//////////////////////////////////////////////////////////////////////////////
// Get DDA-related Information
//    This function extracts information about the graphics card's
// configuration that is required in order to set the DDA registers properly.
//    HACK - This should be moved to the initialization routines.

DDA_INFO GetDDAInfo(void);


//////////////////////////////////////////////////////////////////////////////
// Set the DDA Registers
//    This function sets the registers that govern the transfer of data from
// memory to the DAC. These shouldn't exist; the appropriate FIFO should
// just drain at a rate defined by the pixel clock and request data whenever
// it runs low. But, ATI cards require that we explicitly spell out the ratio
// of memory fetches to XClk, so we call this. Both clocks are specified in
// kHz.

void SetDDARegs(uint32 pixel_clock, uint32 x_clock, uint32 color_space);


//////////////////////////////////////////////////////////////////////////////
// Set the Pixel Clock
//    This function attempts to set the specified pixel clock (in kHz). It
// returns the actual pixel clock value set (also in kHz), or 0 if an error
// occurs.

static uint32 SetPixelClock(uint32 pixel_clock);

//////////////////////////////////////////////////////////////////////////////
// Read Bios Crystal IOCTL
//    This function attempts to read the crystal clock frequency 
// stored in rom by issuing an IOCTL to the driver.

static int32 ReadBiosCrystalIOCTL(SHARED_INFO *si);




//////////////////////////////////////////////////////////////////////////////
// Local Functions ///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

static int32 ReadBiosCrystalIOCTL(SHARED_INFO *si)
{
  int32 gpd;
  
  // Contact driver and get a pointer to the shared data structure.
  return ioctl(ai.fd, B_R128_READ_CRYSTAL_FREQ, &gpd, sizeof(gpd));
  
}

//////////////////////////////////////////////////////////////////////////////
// Initialize Constant Registers
//    This function initializes several registers that don't vary with mode
// sets to default values.

static void InitConstantRegs(void)
{
	WRITE_REG(OVR_CLR,0);
	WRITE_REG(OVR_WID_LEFT_RIGHT,0);
	WRITE_REG(OVR_WID_TOP_BOTTOM,0);
	WRITE_REG(OV0_SCALE_CNTL,0);
	WRITE_REG(MPP_TB_CONFIG,0);
	WRITE_REG(MPP_GP_CONFIG,0);
	WRITE_REG(SUBPIC_CNTL,0);
	WRITE_REG(VIPH_CONTROL,0);
	WRITE_REG(I2C_CNTL_1,0);
  // NOTE: There are many nifty interrupts here that would allow us to
  // optimize our driver considerably. Defer this for the time being.
  // NOTE: Always leaving vertical blanking interrupts enabled. This might
  // cause strangeness during mode sets, but it shouldn't be anything
  // significant and definitely will be transient.
  // HACK - Disabling the vertical blanking interrupt for the moment, as
  // enabling it during mode set causes the system to hang.
//	WRITE_REG(GEN_INT_CNTL,0x00000001);
	WRITE_REG(GEN_INT_CNTL,0x00000000);
	WRITE_REG(CAP0_TRIG_CNTL,0);
	WRITE_REG(CAP1_TRIG_CNTL,0);
}


//////////////////////////////////////////////////////////////////////////////
// Initialize Gamma Correction Table
//    This function initializes the gamma correction table to a linear ramp.
// In 8 bpp mode, this will actually wind up setting the palette to a grey
// scale instead of setting the gamma table, but we'll call that a feature.

static void InitGammaTable(void)
{
  int index;

  // We need a critical section here, as we're messing with si.
  // HACK - We don't have one, yet. Put in proper si locking at some point.
  // In the meantime, the user can mess up the gamma table by performing a
  // palette set during a mode set.

   // tattletail
   ddprintf(("[R128 GFX] InitGammaTable.\n"));

  // Write the desired color data into kernel shared area.
  for (index = 0; index < 768; index++)
  {
    si->display.color_data[index] = (uint8) (index / 3);
  }

  // Specify the range of colors to set.
  si->display.color_count = 256;
  si->display.first_color = 0;

  // Tell the kernel driver to update the palette/gamma table during the
  // next VBI.
  atomic_or((int32 *)&(si->vbi.flags), GDS_PROGRAM_CLUT);
}


//////////////////////////////////////////////////////////////////////////////
// Get XClk Frequency
//    This function infers the current XClk frequency (in kHz) and returns it.
// HACK - This shouldn't be necessary, or at worst should be done only once
// during initialization. The XClk frequency should be stored in a
// SHARED_INFO field.

uint32 GetXClk(void)
{
  float XClk;
  uint32 M, N, P;
  uint32 c_c_index;
  uint32 LTemp;

  // Diagnostics.
  ddprintf(("[R128 GFX] Detecting XClk.\n"));

  // NOTE: We should sanity-check CrystalFreq anyways, though probably in the
  // init routines that look for it. Not all ROMs will be trustworthy.


  // XClk PLL frequency = 2 * N * CrystalFreq / M.

  // Read the M and N values for the XClk PLL.
  // NOTE: Using read-modify-write to avoid stomping the feedback divider
  // selection for PClk.
  READ_REG(CLOCK_CNTL_INDEX, c_c_index);
  c_c_index &= 0x00000300;
  WRITE_REG(CLOCK_CNTL_INDEX, c_c_index | 0x0000000A);
  READ_REG(CLOCK_CNTL_DATA, LTemp);
  M = LTemp & 0x000000FF;
  N = (LTemp & 0x0000FF00) >> 8;

  // Read the XCLK_SRC_SEL field of XCLK_CNTL to find the effective post
  // divider (P). If XCLK is based on the output of a DLL or on HCLK0/1,
  // put in plausible values and pray. If we configure this clock ourselves,
  // we should stick to XClk = XPLL / P configurations to save headaches.
  WRITE_REG(CLOCK_CNTL_INDEX, c_c_index | 0x0000000D);
  READ_REG(CLOCK_CNTL_DATA, LTemp);
  LTemp &= 0x00000007;
  switch (LTemp)
  {
    // XClk = "not CPUCLK".
    case 0:
      // HACK - Assume that this is a 33 MHz bus clock. AGP and PCI-66 might
      // muck this up a bit.

      // Diagnostics.
      ddprintf(("[R128 GFX] Warning: XClk = !CPUCLK. Assuming 33 MHz and bailing.\n"));

      // Bail right now with our nominal value.
      return 33000;
      break;

    // Standard post divider scheme. XClk = XPLL / P.
    case 1: P = 1; break;
    case 2: P = 2; break;
    case 3: P = 4; break;
    case 4: P = 8; break;

    // XClk = XDLL0Clk.
    case 7:
      // HACK - Blithely assume this works out to a post divider of 2.
      P = 2;

      // Diagnostics.
      ddprintf(("[R128 GFX] Warning: XClk = XDLL0CLK. Assuming XPLL/2.\n"));
      break;

    default:
      // XClk is either HCLK0 or HCLK1. These could be anything, as I think
      // they're external inputs.

      // Diagnostics.
      ddprintf(("[R128 GFX] Warning: XClk = HCLK0/1. Assuming 66 MHz and bailing.\n"));

      // HACK - Assume 66 MHz and pray.
      return 66000;
  }


  // Calculate the XClk value from the reference frequency and M, N, and P
  // parameters.

  // Diagnostics.
  ddprintf(("[R128 GFX] XClk M: %lu  N: %lu  P: %lu\n", M, N, P));

  // XClk PLL frequency = 2 * N * CrystalFreq / M.
  // XClk = XPLL / P.
  XClk = 2.0 * ((float) N) * si->hw_specific.CrystalFreq / ((float) M);
  XClk /= (float) P;
  // Convert to kHz, and CrystalFreq is in MHz.
  XClk *= 1.0e3;
  if (XClk - floor(XClk) >= 0.5)
    LTemp = (uint32) ceil(XClk);
  else
    LTemp = (uint32) floor(XClk);

  // Diagnostics.
  ddprintf(("[R128 GFX] XClk: %lu kHz\n", LTemp));

  // Return the XClk value.
  return LTemp;
}


//////////////////////////////////////////////////////////////////////////////
// Get PClk Frequency
//    This function infers the current PClk frequency (in kHz) and returns it.
// This is used as a diagnostics function.

uint32 GetPClk(void)
{
  float PClk;
  uint32 M, N, P;
  uint32 divider_select;
  uint32 c_c_index;
  uint32 LTemp;

  // Diagnostics.
  ddprintf(("[R128 GFX] Detecting PClk.\n"));

  // HACK - CrystalFreq isn't initialized at the moment, so hardwire a value
  // here. Fix this ASAP.
  si->hw_specific.CrystalFreq = 29.498; // MHz.

  // NOTE: We should sanity-check CrystalFreq anyways, though probably in the
  // init routines that look for it. Not all ROMs will be trustworthy.


  // Make sure that the crystal really is the reference for PPLL. If it
  // isn't, bail here.
  // Read the feedback divider number also.
  READ_REG(CLOCK_CNTL_INDEX, c_c_index);
  c_c_index &= 0x00000300;
  divider_select = c_c_index >> 8;
  WRITE_REG(CLOCK_CNTL_INDEX, c_c_index | 0x00000003);
  READ_REG(CLOCK_CNTL_DATA, LTemp);

  switch ((LTemp & 0x00030000) >> 16)
  {
    case 0:
      // We're fine. The reference frequency is XTALIN.
      break;

    case 1:
      // PPLL_REF = PLLMCLK/2.
      ddprintf(("[R128 GFX] Unable to determine PClk frequency - Reference is MClk/2.\n"));
      return 0;
      break;

    case 2:
      // PPLL_REF = PLLXCLK/2.
      ddprintf(("[R128 GFX] Unable to determine PClk frequency - Reference is XClk/2.\n"));
      return 0;
      break;

    default:
      // This shouldn't happen.
      ddprintf(("[R128 GFX] Unable to determine PClk frequency - Unknown reference clock.\n"));
      return 0;
  }

  // LTemp still holds the contents of the reference divider register.


  // PClk frequency = (N * CrystalFreq / M) / divider(P).

  // Read the M, N, and P values for PClk.

  // M value.
  // LTemp still holds the contents of the reference divider register.
  M = LTemp & 0x000003FF;

  // N and P values. These depend on the feedback divider selected.
  WRITE_REG(CLOCK_CNTL_INDEX, c_c_index | (0x00000004 + divider_select));
  READ_REG(CLOCK_CNTL_DATA, LTemp);
  N = LTemp & 0x000007FF;
  P = (LTemp & 0x00070000) >> 16;

  // Translate the P value into a post divider value.
  switch (P)
  {
    case 0: P = 1; break;
    case 1: P = 2; break;
    case 2: P = 4; break;
    case 3: P = 8; break;
    case 4: P = 3; break;
    case 6: P = 6; break;
    default: // This had better be 7, as 5 is undefined.
      P = 12;
  }


  // Calculate the PClk value from the reference frequency and M, N, and P
  // parameters.

  // Diagnostics.
  ddprintf(("[R128 GFX] PClk M: %lu  N: %lu  post divider: %lu\n", M, N, P));

  // PClk frequency = (N * CrystalFreq / M) / divider(P).
  PClk = ((float) N) * si->hw_specific.CrystalFreq
    / ( ((float) M) * ((float) P) );
  // Convert to kHz, and CrystalFreq is in MHz.
  PClk *= 1.0e3;
  if (PClk - floor(PClk) >= 0.5)
    LTemp = (uint32) ceil(PClk);
  else
    LTemp = (uint32) floor(PClk);

  // Diagnostics.
  ddprintf(("[R128 GFX] PClk: %lu kHz\n", LTemp));

  // Return the PClk value.
  return LTemp;
}


//////////////////////////////////////////////////////////////////////////////
// Dump Clocking Info
//    This function dumps XClk, PClk, and DDA register information to debug
// output. Used as a diagnostics function.

void DumpClockingInfo(void)
{
  // Clock frequencies.
  uint32 XClk, PClk;
  // DDA parameter values.
  uint32 XClksPerXfer;
  uint32 Precision, Latency;
  uint32 DDA_On, DDA_Off;
  // VGA DDA parameter values.
  uint32 VGA_XClksPerXfer;
  uint32 VGA_Precision, VGA_PrecBy2;
  uint32 VGA_DDA_On, VGA_DDA_Off;
  // Scratch variables.
  uint32 XPerXFrac, OnFrac, OffFrac;
  uint32 LTemp;


  // Read clock settings.

  XClk = GetXClk();
  PClk = GetPClk();


  // Read DDA settings.

  // XClocks per transfer, precision, and loop latency.
  READ_REG(DDA_CONFIG, LTemp);
  XClksPerXfer = LTemp & 0x00003FFF;
  Precision = (LTemp & 0x000F0000) >> 16;
  Latency = (LTemp & 0x01F00000) >> 20;

  // DDA On and Off values.
  READ_REG(DDA_ON_OFF, LTemp);
  DDA_Off = LTemp & 0x0000FFFF;
  DDA_On = (LTemp & 0xFFFF0000) >> 16;


  // Read VGA DDA settings.

  // XClocks per transfer, precision, and pclk-by-2 precision.
  READ_REG(VGA_DDA_CONFIG, LTemp);
  VGA_XClksPerXfer = LTemp & 0x00003FFF;
  VGA_PrecBy2 = (LTemp & 0x00F00000) >> 20;
  VGA_Precision = (LTemp & 0x0F000000) >> 24;

  // VGA DDA On and Off values.
  READ_REG(VGA_DDA_ON_OFF, LTemp);
  VGA_DDA_Off = LTemp & 0x0000FFFF;
  VGA_DDA_On = (LTemp & 0xFFFF0000) >> 16;


  // Report.

  ddprintf(("[R128 GFX] Clock and DDA settings detected as follows:\n"));


  // Clock settings.

  ddprintf(("[R128 GFX]   XClk: %lu kHz   PClk: %lu kHz\n", XClk, PClk));


  // DDA settings.

  ddprintf(("[R128 GFX]   XClks/Xfer raw: %lu   On raw: %lu   Off raw: %lu\n",
    XClksPerXfer, DDA_On, DDA_Off));
  ddprintf(("[R128 GFx]   Precision: %lu   Loop latency: %lu\n",
    Precision, Latency));

  // Convert to integer.fraction format, with two decimal digits in the
  // fraction.
  if (Precision >= 11)
  {
    XClksPerXfer <<= Precision - 11;
    XPerXFrac = 0;
    DDA_On <<= Precision - 11;
    OnFrac = 0;
    DDA_Off <<= Precision - 11;
    OffFrac = 0;
  }
  else
  {
    XPerXFrac = ((XClksPerXfer * 100) >> (11 - Precision)) % 100;
    XClksPerXfer >>= 11 - Precision;
    OnFrac = ((DDA_On * 100) >> (11 - Precision)) % 100;
    DDA_On >>= 11 - Precision;
    OffFrac = ((DDA_Off * 100) >> (11 - Precision)) % 100;
    DDA_Off >>= 11 - Precision;
  }

  ddprintf(("[R128 GFX]   XClks/Xfer: %lu.%02lu   On: %lu.%02lu   Off: %lu.%02lu\n",
    XClksPerXfer, XPerXFrac, DDA_On, OnFrac, DDA_Off, OffFrac));


  // VGA DDA settings.

  ddprintf(("[R128 GFX]   (VGA)  XClks/Xfer raw: %lu   On raw: %lu   Off raw: %lu\n",
    VGA_XClksPerXfer, VGA_DDA_On, VGA_DDA_Off));
  ddprintf(("[R128 GFx]   (VGA)  Precision: %lu   PClk x2 Precision: %lu\n",
    VGA_Precision, VGA_PrecBy2));

  // Convert to integer.fraction format, with two decimal digits in the
  // fraction.
  if (VGA_Precision >= 11)
  {
    VGA_XClksPerXfer <<= VGA_Precision - 11;
    XPerXFrac = 0;
    VGA_DDA_On <<= VGA_Precision - 11;
    OnFrac = 0;
    VGA_DDA_Off <<= VGA_Precision - 11;
    OffFrac = 0;
  }
  else
  {
    XPerXFrac = ((VGA_XClksPerXfer * 100) >> (11 - VGA_Precision)) % 100;
    VGA_XClksPerXfer >>= 11 - VGA_Precision;
    OnFrac = ((VGA_DDA_On * 100) >> (11 - VGA_Precision)) % 100;
    VGA_DDA_On >>= 11 - VGA_Precision;
    OffFrac = ((VGA_DDA_Off * 100) >> (11 - VGA_Precision)) % 100;
    VGA_DDA_Off >>= 11 - VGA_Precision;
  }

  ddprintf(("[R128 GFX]   (VGA)  XClks/Xfer: %lu.%02lu   On: %lu.%02lu   Off: %lu.%02lu\n",
    VGA_XClksPerXfer, XPerXFrac, VGA_DDA_On, OnFrac, VGA_DDA_Off, OffFrac));
}


//////////////////////////////////////////////////////////////////////////////
// Dump CRTC Info
//    This function dumps the current CRTC configuration to debug output.
// Used as a diagnostics function.

void DumpCRTCInfo(void)
{
  uint32 gen_cntl;
  uint32 h_total_disp, v_total_disp, hsync_start_wid, vsync_start_wid;

  // Load raw CRTC register values.
  READ_REG(CRTC_GEN_CNTL, gen_cntl);
  READ_REG(CRTC_H_TOTAL_DISP, h_total_disp);
  READ_REG(CRTC_V_TOTAL_DISP, v_total_disp);
  READ_REG(CRTC_H_SYNC_STRT_WID, hsync_start_wid);
  READ_REG(CRTC_V_SYNC_STRT_WID, vsync_start_wid);

  // Report.

  ddprintf(("[R128 GFX] CRTC settings detected as follows:\n"));

  // Screen dimensions.
  ddprintf(("  Displayed: %lu x %lu   Total: %lu x %lu\n",
    (((h_total_disp & 0x00FF0000) >> 16) + 1) << 3,
    ((v_total_disp & 0x07FF0000) >> 16) + 1,
    ((h_total_disp & 0x01FF) + 1) << 3,
    (v_total_disp & 0x07FF) + 1));
  ddprintf(( ((gen_cntl & 0x01) ? "  double-scan   " : "  " )));
  ddprintf(( ((gen_cntl & 0x02) ? "interlaced\n" : "non-interlaced\n") ));

  // Sync pulses.
  ddprintf(("  Hsync Start: %lu   Width: %lu   Polarity: ",
    hsync_start_wid & 0x0000FFFF,
    ((hsync_start_wid & 0x003F0000) >> 16) << 3));
  ddprintf(( ((hsync_start_wid & 0x00800000) ? "-\n" : "+\n") ));
  ddprintf(("  Vsync Start: %lu   Width: %lu   Polarity: ",
    vsync_start_wid & 0x00007FFF,
    (vsync_start_wid & 0x001F0000) >> 16));
  ddprintf(( ((vsync_start_wid & 0x00800000) ? "-\n" : "+\n") ));

  // Colour depth.
  switch ((gen_cntl & 0x00000700) >> 8)
  {
    case 1:
      ddprintf(("  4 bpp\n"));
      break;

    case 2:
      ddprintf(("  8 bpp\n"));
      break;

    case 3:
      ddprintf(("  15 bpp\n"));
      break;

    case 4:
      ddprintf(("  16 bpp\n"));
      break;

    case 5:
      ddprintf(("  24 bpp\n"));
      break;

    case 6:
      ddprintf(("  32 bpp\n"));
      break;

    default:
      ddprintf(("  *unknown color depth (%lu)*\n", (gen_cntl & 0x00000700) >> 8));
  }
}


//////////////////////////////////////////////////////////////////////////////
// Dump General Register Contents
//    This function dumps the contents of the PLL registers, CRTC registers,
// DAC registers, and various configuration registers. Pretty much everything
// not related to 2D or 3D drawing, in other words.

void DumpGeneralRegisters(void)
{
  uint32 index;
  uint32 saved_reg;
  uint32 LTemp;

  ddprintf(("[R128 GFX] Dumping general card registers.\n"));

  // Dump everything in non-hardware-drawing register space.

  ddprintf(("[R128 GFX] General registers:\n"));

  // HACK - This hangs at around 0x0BF0 or so, so stop at 0x0B00 instead of
  // 0x0F00.
  for (index = 0; index < 0x0B00; index += 4)
  {
    READ_REG(index, LTemp);
    ddprintf(("   Register %04lx:  %08lx\n", index, LTemp));
  }

  // Dump the copies of the PCI configuration registers.
  // NOTE: This doesn't seem to actually contain copies of the PCI
  // configuration registers! Doublecheck the address at some point.

  ddprintf(("[R128 GFX] PCI configuration space registers:\n"));

  for (index = 0; index < 0x0100; index += 4)
  {
    READ_REG(0x0F00 | index, LTemp);
    ddprintf(("   PCI Register %02lx:  %08lx\n", index, LTemp));
  }

  ddprintf(("[R128 GFX] PLL registers:\n"));

  READ_REG(CLOCK_CNTL_INDEX, saved_reg);
  saved_reg &= 0x00000300;
  for (index = 0; index < 0x20; index++)
  {
    WRITE_REG(CLOCK_CNTL_INDEX, saved_reg | index);
    READ_REG(CLOCK_CNTL_DATA, LTemp);
    ddprintf(("   PLL Register %02lx:  %08lx\n", index, LTemp));
  }

  ddprintf(("[R128 GFX] Finished general register dump.\n"));
}


//////////////////////////////////////////////////////////////////////////////
// Get DDA-related Information
//    This function extracts information about the graphics card's
// configuration that is required in order to set the DDA registers properly.
//    HACK - This should be moved to the initialization routines.

DDA_INFO GetDDAInfo(void)
{
  uint32 mem_type;
  int index;

  // Extract the card's memory configuration from MEM_CNTL.
  READ_REG(MEM_CNTL, mem_type);
  mem_type &= 0x00000003;

  // Scan through the DDA info table until we find our card or hit the end
  // of the list.
  for(index = 0;
    (dda_info_table[index].device_id != 0xFFFF) &&
    ( (dda_info_table[index].device_id != si->card.device_id) ||
      (dda_info_table[index].mem_type != mem_type) );
    index++)
    ; // Go to the next entry.

  // Diagnostics.
  ddprintf(("[R128 GFX] GetDDAInfo returning row %lu (device_id %04lx, mem_type %lu).\n",
    (uint32) index, (uint32) dda_info_table[index].device_id,
    dda_info_table[index].mem_type));
  ddprintf(("[R128 GFX] on_constant: %lu   off_constant: %lu   loop_latency: %lu\n",
    dda_info_table[index].card_dda_info.on_constant,
    dda_info_table[index].card_dda_info.off_constant,
    dda_info_table[index].card_dda_info.loop_latency));
  ddprintf(("[R128 GFX] dda_fifo_width: %lu   dda_fifo_depth: %lu\n",
    dda_info_table[index].card_dda_info.dda_fifo_width,
    dda_info_table[index].card_dda_info.dda_fifo_depth));

  // Return appropriate info.
  return dda_info_table[index].card_dda_info;
}


//////////////////////////////////////////////////////////////////////////////
// Set the DDA Registers
//    This function sets the registers that govern the transfer of data from
// memory to the DAC. These shouldn't exist; the appropriate FIFO should
// just drain at a rate defined by the pixel clock and request data whenever
// it runs low. But, ATI cards require that we explicitly spell out the ratio
// of memory fetches to XClk, so we call this. Both clocks are specified in
// kHz.

void SetDDARegs(uint32 pixel_clock, uint32 x_clock, uint32 color_space)
{
  DDA_INFO our_dda_info;
  float IdealXClksPerXfer;
  uint32 XClksPerXfer;
  uint32 Precision;
  uint32 DDA_On, DDA_Off;
  uint32 LTemp;

  // Diagnostics.
  ddprintf(("[R128 GFX] SetDDARegs called.\n"));


  // Get hardware-specific parameters.
  // HACK - Do this in initialization and store the results in SHARED_INFO.
  our_dda_info = GetDDAInfo();

  // Calculate the ideal number of xclocks per FIFO transfer.
  // Pixel clock and XClk are in the same units, so don't bother converting
  // to Hz or MHz.
  IdealXClksPerXfer =
    ( ((float) x_clock) * ((float) our_dda_info.dda_fifo_width) )
    / ( ((float) pixel_clock) * ((float) CalcBitsPerPixel(color_space)) );

  // Diagnostics.
  // HACK - This has failed to handle %f clauses in other sections, so I'm
  // taking no chances here.
  ddprintf(("[R128 GFX] Ideal XClocks per transfer: %lu.%02lu\n",
    (uint32) floor(IdealXClksPerXfer),
    (uint32) ((IdealXClksPerXfer - floor(IdealXClksPerXfer)) * 100.0) ));

  // Select the best possible int.fraction precision level.
  // We have 14 bits available, so XClks/Xfer should max out at 16383.
  // NOTE: The ATI sample code used a minimum Precision value of 2.
  Precision = 11;
  while ((Precision > 0) && (IdealXClksPerXfer <= 8191.74))
  {
    Precision--;
    IdealXClksPerXfer += IdealXClksPerXfer;
  }

  // Store the actual XClksPerXfer count.
  XClksPerXfer = (uint32) floor(IdealXClksPerXfer + 0.5);

  // Calculate On and Off values.
//  DDA_On = (our_dda_info.on_constant << (11 - Precision)) + XClksPerXfer;
  // HACK - Use ATI's formula for DDA_On, even though it should be less accurate.
  DDA_On = ( our_dda_info.on_constant
    + (( XClksPerXfer + ((1 << (11 - Precision)) >> 1) ) >> (11 - Precision)) )
    << (11 - Precision);
  DDA_Off = our_dda_info.off_constant * XClksPerXfer;

  // Adjust the precision until all values fit into their respective bit
  // fields.
  while ((DDA_On > 0xFFFF) || (DDA_Off > 0xFFFF) || (XClksPerXfer > 0x3FFF))
  {
    XClksPerXfer >>= 1;
    DDA_On >>= 1;
    DDA_Off >>= 1;
    Precision++;
  }


  // Diagnostics.
  ddprintf(("[R128 GFX] XClocks per transfer: %lu   Precision: %lu\n",
    XClksPerXfer, Precision));
  ddprintf(("[R128 GFX] DDA_On: %lu   DDA_Off: %lu\n", DDA_On, DDA_Off));


  // If precision is greater than 15, we're really in trouble. This should
  // never happen, but if it does, make sure that we at least don't try to
  // write anything that insane to the DDA registers.
  if (Precision > 0x0F)
  {
    ddprintf(("[R128 GFX] Warning: Precision adjusted to be greater than 15 Clipping.\n"));
    Precision = 0x0F;
  }


  // Sanity check.
  // NOTE: Not doing a range check. This may cause memory access failure
  // if one of the values produced is wider than its supported field width.
  // Check on/off/latency restrictions.
  if (DDA_On + our_dda_info.loop_latency >= DDA_Off)
  {
    // Give an error message, but don't modify anything.
    ddprintf(("[R128 GFX] Warning - DDA_On + loop_latency is not less than DDA_Off.\n"));
  }


  // Program the DDA registers.

  LTemp = (XClksPerXfer & 0x00003FFF)
    | ((Precision & 0x0000000F) << 16)
    | ((our_dda_info.loop_latency & 0x0000001F) << 20);
  WRITE_REG(DDA_CONFIG, LTemp);

  LTemp = (DDA_Off & 0x0000FFFF)
    | ((DDA_On & 0x0000FFFF) << 16);
  WRITE_REG(DDA_ON_OFF, LTemp);
}


//////////////////////////////////////////////////////////////////////////////
// Set the Pixel Clock
//    This function attempts to set the specified pixel clock (in kHz). It
// returns the actual pixel clock value set (also in kHz), or 0 if an error
// occurs.

static uint32 SetPixelClock(uint32 pixel_clock)
{
  float DesiredPClk;
  float MaxVCO;
  float FTemp;
  uint32 M, N, P;
  uint32 LTemp;

  // NOTE: We should sanity-check CrystalFreq anyways, though probably in the
  // init routines that look for it. Not all ROMs will be trustworthy.


  // Do calculations in floating-point to save headaches.
  DesiredPClk = ((float) pixel_clock) * 1.0e3;


  // Diagnostics.
  ddprintf(("[R128 GFX] SetPixelClock() called.\n"));
  // HACK - WTF? This won't accept _%f_???
  ddprintf(("[R128 GFX] Desired pixel clock: %lu kHz.\n", (uint32) (DesiredPClk / 1.0e3)));


  // Calculate M, N, and P values.
  // The pixel PLL frequency is N * crystal / M. The pixel clock frequency
  // is the pixel PLL frequency divided by a post divider determined by P.
  // See the ATI documentation for the exact mapping of post divider values
  // to P.

  // HACK - Doing this here instead of a utility function. If we ever do set
  // other PLL values, move this to a utility function to avoid duplication,
  // if possible.

  // HACK - Use the coarse-granularity but quick method for proof-of-concept.

  // HACK - Copy the maximum VCO frequency from the documentation instead of
  // reading it from the SHARED_INFO structure.
  MaxVCO = 2.5e8;

  // Set the post-divider value as high as possible to minimize granularity.
  if (DesiredPClk <= MaxVCO / 12.0)
  {
    P = 7; // /12.
    DesiredPClk *= 12.0;
  }
  else if (DesiredPClk <= MaxVCO / 8.0)
  {
    P = 3; // /8.
    DesiredPClk *= 8.0;
  }
  else if (DesiredPClk <= MaxVCO / 6.0)
  {
    P = 6; // /6.
    DesiredPClk *= 6.0;
  }
  else if (DesiredPClk <= MaxVCO / 4.0)
  {
    P = 2; // /4.
    DesiredPClk *= 4.0;
  }
  else if (DesiredPClk <= MaxVCO / 3.0)
  {
    P = 4; // /3.
    DesiredPClk *= 3.0;
  }
  else if (DesiredPClk <= MaxVCO / 2.0)
  {
    P = 1; // /2.
    DesiredPClk *= 2.0;
  }
  else if (DesiredPClk <= MaxVCO)
  {
    P = 0; // /1.
  }
  else
  {
    // Desired clock frequency is not in the supported range.
    // Return failure.
    return 0;
  }

  // Sanity check. Make sure that frequency bottoms out at something
  // reasonable.
  if (DesiredPClk < MaxVCO / 2.0)
    DesiredPClk = MaxVCO / 2.0;


  // Diagnostics.
  // HACK - WTF? This won't accept _%f_???
  ddprintf(("[R128 GFX] Desired pixel PLL frequency: %lu kHz.\n", (uint32) (DesiredPClk / 1.0e3)));


  // Set the denominator value to be as high as possible to minimize
  // granularity.
  // Crystal frequency / M must be >= 200 kHz. Also, N is at most 2047,
  // which places a limit to how low crystal frequency / M can be.
  // Fortunately, this is moot point for the time being (200 kHz means
  // a PPLL limit of more than 400 MHz).
  FTemp = (si->hw_specific.CrystalFreq * 1.0e6) / 2.0e5;
  // This should err low, so that the resulting frequency is >= 200 kHz.
  M = (uint32) floor(FTemp);


  // Clip to sane values, just in case.
  if (M > 1023)
  {
    ddprintf(("[R128 GFX] Clipping PClk M value (was %lu).\n", M));
    M = 1023;
  }


  // Calculate the reqired numerator value.
  // NOTE: N has a minimum value of 4, according to the ATI documentation.
  // This shouldn't be a problem here.
  FTemp = DesiredPClk * ((float) M) / (si->hw_specific.CrystalFreq * 1.0e6);
  // Round to the closest value.
  if (FTemp - floor(FTemp) >= 0.5)
    N = (uint32) ceil(FTemp);
  else
    N = (uint32) floor(FTemp);


  // Clip to sane values, just in case.
  if (N > 2047)
  {
    ddprintf(("[R128 GFX] Clipping PClk N value (was %lu).\n", N));
    M = 2047;
  }


  // Diagnostics.
  // HACK - WTF? This won't accept _%f_???
  ddprintf(("[R128 GFX] Crystal frequency: %lu kHz.\n", (uint32) (si->hw_specific.CrystalFreq * 1.0e3)));
  ddprintf(("[R128 GFX] Pixel clock M, N, P calculated as %lu, %lu, %lu.\n", M, N, P));


  // Program the pixel clock.

  // Power down the pixel PLL.
  WRITE_REG(CLOCK_CNTL_INDEX, 0x00000382);
  READ_REG(CLOCK_CNTL_DATA, LTemp);
  LTemp &= 0x0000FF00;
  LTemp |= 0x00000002;
  WRITE_REG(CLOCK_CNTL_DATA, LTemp);

  // Set reference divider (M) and select crystal as the reference source.
  WRITE_REG(CLOCK_CNTL_INDEX, 0x00000383);
  LTemp = 0x00000000;
  LTemp |= (M & 0x000003FF);
  WRITE_REG(CLOCK_CNTL_DATA, LTemp);

  // Set feedback divider (N) and post divider (P) in divider register #3.
  WRITE_REG(CLOCK_CNTL_INDEX, 0x00000387);
  LTemp = 0x00000000;
  LTemp |= (N & 0x000007FF);
  LTemp |= (P & 0x07) << 16;
  WRITE_REG(CLOCK_CNTL_DATA, LTemp);

  // Reset the pixel PLL.
  WRITE_REG(CLOCK_CNTL_INDEX, 0x00000382);
  READ_REG(CLOCK_CNTL_DATA, LTemp);
  LTemp &= 0x0000FF00;
  LTemp |= 0x00000001;  // Set the reset bit, clear the power-down bit.
  WRITE_REG(CLOCK_CNTL_DATA, LTemp);
  snooze(1000); // Wait a microsecond or so.
  LTemp &= 0x0000FF00;  // Clear the reset bit.
  WRITE_REG(CLOCK_CNTL_DATA, LTemp);

  // Disable write access, keep divider #3 selected.
  WRITE_REG(CLOCK_CNTL_INDEX, 0x00000302);


  // Wait for a bit, so that we don't return until the PLL has stabilized.
  snooze(10000);


  // Calculate the pixel clock that we actually set.
  FTemp = ((float) N) * (si->hw_specific.CrystalFreq) / ((float) M);
  switch (P)
  {
    case 0: FTemp /= 1.0; break;
    case 1: FTemp /= 2.0; break;
    case 2: FTemp /= 4.0; break;
    case 3: FTemp /= 8.0; break;
    case 4: FTemp /= 3.0; break;
    case 6: FTemp /= 6.0; break;
    default: // Had better be 7.
      FTemp /= 12.0; break;
  }
  FTemp *= 1.0e3; // CrystalFreq was in MHz, we want kHz.

  if (FTemp - floor(FTemp) >= 0.5)
    LTemp = (uint32) ceil(FTemp);
  else
    LTemp = (uint32) floor(FTemp);


  // Diagnostics.
  ddprintf(("[R128 GFX] Pixel clock set: %lu kHz.\n", LTemp));


  // Return the pixel clock value that we actually set.
  return LTemp;
}


//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Set the Display Mode
//    This sets the current display mode as specified. Any error checking
// should have been done prior to making this call.
//    The display mode and frame buffer configuration that result after the
// mode set are stored in final_dm and final_fbc, respectively.

void do_set_display_mode_crt(display_mode *dm, display_mode *final_dm,
  frame_buffer_config *final_fbc)
{
  uint32 GenCntl, ExtCntl; // Local copies of CRTC_GEN_CNTL and CRTC_EXT_CNTL.
  uint32 LTemp; // Scratch variable for register calculations.


  // Tattletale.
  ddprintf(("[R128 GFX]  do_set_display_mode() called.\n"));


  // HACK - No locking whatsoever at the moment. Fix this at some point.

  { si->card.regs[GEN_INT_CNTL] &= ~0x04; }  // turn off card vb interrupts 

  // Diagnostics.
  DumpCRTCInfo();
  DumpClockingInfo();


	ReadBiosCrystalIOCTL(si);	// read crystal frequency


  // Blank the screen and disable sync pulses.
  // While we're here, stomp CRTC_EXT_CNTL with known-sane values.
  ExtCntl = 0x00800700;
  WRITE_REG(CRTC_EXT_CNTL, ExtCntl);
  
snooze(5000);

  // Initialize constant registers. NOTE: This should probably be moved to
  // initialization at some point.
  InitConstantRegs();


  // Reset the CRTC controller and set CRTC_GEN_CNTL to known-sane values
  // while we're here.

  // Reset and stomp. We hide the cursor here as well.
  // NOTE: We're disabling display requests here also. I think that this
  // refers to DAC fetches from frame buffer memory.
  GenCntl = 0x05000000;
  // Specify the colour depth.
  switch (CalcColorDepth(dm->space))
    {
    case 32:
      GenCntl |= 0x00000600;
      break;

    case 16:
      GenCntl |= 0x00000400;
      break;

    case 15:
      GenCntl |= 0x00000300;
      break;

    default: // 8 bpp or error; we need to set _something_.
      GenCntl |= 0x00000200;
    }
  // Specify settings based on timing flags.
  if (dm->timing.flags & B_TIMING_INTERLACED)
    GenCntl |= 0x00000002;
  if (dm->timing.flags & B_SYNC_ON_GREEN)
    GenCntl |= 0x00000010;
  // Stomp/Reset.
  WRITE_REG(CRTC_GEN_CNTL, GenCntl);

  // Wait a moment, engage the controller, wait again, and then hold down
  // the reset flag.
  // I'm not sure if this is necessary for the 128, but it was on earlier
  // Rage cards, and certainly doesn't hurt.
  snooze(5000); // About 1 ms delay.
  WRITE_REG(CRTC_GEN_CNTL, GenCntl | 0x02000000);
  snooze(5000); // About 1 ms delay.
snooze(5000);
  WRITE_REG(CRTC_GEN_CNTL, GenCntl);

  snooze(5000);
snooze(5000);
  // Program the display mode parameters.

  // In theory we don't need any sanity checking at all here, but in practice
  // I am most certainly going to make sure that these values fit within their
  // respective register fields.

  // h_total and h_display.
  // Both of these are one less than the actual count (i.e. they specify the
  // index of the last character).
  LTemp = ((uint32) (dm->timing.h_total >> 3) - 1) & 0x000001FF;
  LTemp |= ( ((uint32) (dm->timing.h_display >> 3) - 1) & 0x000000FF ) << 16;
  WRITE_REG(CRTC_H_TOTAL_DISP, LTemp);

snooze(5000);

  // h_sync_start, h_sync_end, and horizontal sync polarity.
  // NOTE: I'm assuming that these are actual addresses, and don't need to
  // be decremented.
  // The sync pulse starting point is stored in pixels instead of characters.
  LTemp = ((uint32) dm->timing.h_sync_start) & 0x00000FFF;
  // Store the sync pulse width, as opposed to its actual end point.
  // The sync pulse width is stored in characters, though.
  LTemp |= ( ( ((uint32) (dm->timing.h_sync_end - dm->timing.h_sync_start))
    >> 3 ) & 0x0000003F ) << 16;
  if (!(dm->timing.flags & B_POSITIVE_HSYNC))
    LTemp |= 0x00800000;
  WRITE_REG(CRTC_H_SYNC_STRT_WID, LTemp);

snooze(5000);

  // v_total and v_display.
  // Both of these are one less than the actual count (i.e. they specify the
  // index of the last line).
  LTemp = ((uint32) dm->timing.v_total - 1) & 0x000007FF;
  LTemp |= ( ((uint32) dm->timing.v_display - 1) & 0x000007FF ) << 16;
  WRITE_REG(CRTC_V_TOTAL_DISP, LTemp);

snooze(5000);

  // v_sync_start, v_sync_end, and vertical sync polarity.
  // NOTE: I'm assuming that these are actual addresses, and don't need to
  // be decremented.
  LTemp = ((uint32) dm->timing.v_sync_start) & 0x000007FF;
  // Store the sync pulse width, as opposed to its actual end point.
  LTemp |= ( ((uint32) (dm->timing.v_sync_end - dm->timing.v_sync_start))
    & 0x0000001F ) << 16;
  if (!(dm->timing.flags & B_POSITIVE_VSYNC))
    LTemp |= 0x00800000;
  WRITE_REG(CRTC_V_SYNC_STRT_WID, LTemp);


  // NOTE: Ignoring the VLINE registers, as we've disabled the vertical line
  // interrupt.


  // Program the memory pitch and the starting address of displayed data in
  // frame buffer memory.

  // Make sure that we're accessing memory in linear mode instead of tiled
  // mode, and that nothing else strange is going on.
 snooze(5000);
  WRITE_REG(CRTC_OFFSET_CNTL, 0x0);
  // Write the starting offset. This is in 64-bit words, and the lower 3 bits
  // of the offset must be 0 (in other words, 64-byte granularity).
  // HACK - Setting this to the base of the frame buffer always (i.e. no
  // display repositioning).
snooze(5000);
  WRITE_REG(CRTC_OFFSET, 0x0);
  // Write the pitch. This is in characters (multiples of 8 pixels).
  LTemp = ((uint32) dm->virtual_width >> 3) & 0x000003FF;
  WRITE_REG(CRTC_PITCH, LTemp);
snooze(5000);

  // Initialize the DAC_CNTL register, setting 8/6 bpp DAC status,
  // blanking pedestal state.
  // HACK - Force 8 bpp DAC, no blanking pedestal.
  WRITE_REG(DAC_CNTL, 0xFF000900);

snooze(5000);
  // Program the new pixel clock. This had better work. Hopefully,
  // ProposeVideoMode weeded out pixel clocks that couldn't be supported.
  dm->timing.pixel_clock = SetPixelClock(dm->timing.pixel_clock);


  // Set the DDA registers. These precisely specify the rate at which data
  // passes from memory to the output DAC. In any sane system, this wouldn't
  // be necessary (the FIFO would just request more data when it runs low).
  // This is a holdover from the older Rage cards.
snooze(5000);
  // Proceed only if we managed to set the pixel clock.
  if (dm->timing.pixel_clock)
  {
    // Set the DDA registers.
    // HACK - Infer XClk here instead of reading it from the SHARED_INFO
    // structure.
    SetDDARegs(dm->timing.pixel_clock, GetXClk(), dm->space);
  }
snooze(5000);

  // Activate the CRTC controller again.
  GenCntl |= 0x02000000;
  // Re-enable display requests. I think that this refers to DAC fetches
  // from frame buffer memory.
  GenCntl &= ~(0x04000000ul);
  WRITE_REG(CRTC_GEN_CNTL, GenCntl);

snooze(5000);
  // Initialize gamma table. This will set the palette to a grey scale in
  // 8 bpp modes.
  InitGammaTable();
snooze(5000);

  // Enable sync pulses and unblank the screen.
  ExtCntl &= ~(0x00000700ul);
  WRITE_REG(CRTC_EXT_CNTL, ExtCntl);

snooze(5000);
  // Diagnostics.
  DumpCRTCInfo();
snooze(5000);
  DumpClockingInfo();

  // More diagnostics.
//  DumpGeneralRegisters();


  // Record the final display mode set.
  // NOTE: We're taking it on faith that ProposeVideoMode adjusted these
  // parameters correctly for granularity. We clip them again above, but
  // don't bother re-saving them.
  *final_dm = *dm;

  // Record the final frame buffer configuration.
  // The frame buffer starts at the base of the frame buffer aperture.
  final_fbc->frame_buffer = si->card.fb_base;
  final_fbc->frame_buffer_dma = si->card.fb_base_pci;
  final_fbc->bytes_per_row = final_dm->virtual_width
    * (CalcBitsPerPixel(final_dm->space) >> 3);

  { si->card.regs[GEN_INT_CNTL] |= 0x04; /* enable vblank status */
    si->card.regs[GEN_INT_STATUS] = 0x04; /* reset current VB status */ } 

  // More tattletales; give a detailed description of the final display
  // configuration.

  ddprintf(("[R128 GFX]  Set display mode to:\n"));
  ddprintf(("    HD: %lu  HT: %lu  HSS: %lu  HSE: %lu\n",
    (uint32) final_dm->timing.h_display, (uint32) final_dm->timing.h_total,
    (uint32) final_dm->timing.h_sync_start,
    (uint32) final_dm->timing.h_sync_end));
  ddprintf(("    VD: %lu  VT: %lu  VSS: %lu  VSE: %lu\n",
    (uint32) final_dm->timing.v_display, (uint32) final_dm->timing.v_total,
    (uint32) final_dm->timing.v_sync_start,
    (uint32) final_dm->timing.v_sync_end));
  ddprintf(("    PCLK: %lu kHz  Timing flags: 0x%lx\n",
    (uint32) final_dm->timing.pixel_clock, (uint32) final_dm->timing.flags));
  ddprintf(("    Frame Buffer: %lu x %lu\n",
    (uint32) final_dm->virtual_width, (uint32) final_dm->virtual_height));
  ddprintf(("    Position: (%lu,%lu)\n",
    (uint32) final_dm->h_display_start, (uint32) final_dm->v_display_start));
  ddprintf(("    Colour space: %s  Mode flags: 0x%lx\n",
    NameOfSpace(final_dm->space), (uint32) final_dm->flags));
  ddprintf(("[R128 GFX]  Set frame buffer configuration to:\n"));
  // HACK - Apparently this can't handle %p clauses.
  ddprintf(("    Virtual pointer: %08lx  Physical pointer: %08lx\n",
    (uint32) final_fbc->frame_buffer, (uint32) final_fbc->frame_buffer_dma));
  ddprintf(("    Bytes per row: %lu\n", (uint32) final_fbc->bytes_per_row));
  
  	
  // Initialize Graphic Engine.

	r128_InitEngine(si);
  
  // update cursor image, since upper frame buffer is used to 
  // hold the cursor, and may have been stompted by the mode change
  do_set_cursor_image(si->cursor.cursor0, si->cursor.cursor1);

}

// #pragma mark -

void do_set_display_mode_fp(display_mode *dm, display_mode *final_dm, frame_buffer_config *final_fbc)
{
	uint16 xres = dm->virtual_width;
	uint16 yres = dm->virtual_height;
	
	uint32 fp_horz_stretch;
	uint32 fp_vert_stretch;
	
	uint32 orig_horz_stretch,orig_vert_stretch;
	uint32 orig_fp_gen_cntl,fp_gen_cntl;
	uint32 crtc_ext_cntl,dac_cntl,lvds_gen_cntl,fp_panel_cntl;
	
	float Hratio, Vratio;
		
/*	if (xres > info->PanelXRes) xres = info->PanelXRes;*/
/*	if (yres > info->PanelYRes) yres = info->PanelYRes;*/

	//dprintf("do_set_display_mode_fp\n");	
	Hratio = (float)xres / (float)1024.0;
	Vratio = (float)yres / (float)768.0;
	
	READ_REG(R128_FP_HORZ_STRETCH,orig_horz_stretch);
	
	fp_horz_stretch = (int)(Hratio * R128_HORZ_STRETCH_RATIO_MAX + 0.5);
	fp_horz_stretch = (fp_horz_stretch & R128_HORZ_STRETCH_RATIO_MASK) << R128_HORZ_STRETCH_RATIO_SHIFT;
	fp_horz_stretch &= ~R128_HORZ_AUTO_RATIO_FIX_EN;
	
	if (Hratio == 1.0)
		fp_horz_stretch &= ~(R128_HORZ_STRETCH_BLEND | R128_HORZ_STRETCH_ENABLE);
	else
		fp_horz_stretch |= (R128_HORZ_STRETCH_BLEND | R128_HORZ_STRETCH_ENABLE);

	
	// Write the horizontal stretch
	WRITE_REG(R128_FP_HORZ_STRETCH, fp_horz_stretch |
								(orig_horz_stretch & (R128_HORZ_PANEL_SIZE |
													   R128_HORZ_FP_LOOP_STRETCH |
													   R128_HORZ_STRETCH_RESERVED)));
													   

	READ_REG(R128_FP_VERT_STRETCH,orig_vert_stretch);

	fp_vert_stretch = (int)(Vratio * R128_VERT_STRETCH_RATIO_MAX + 0.5);
	fp_vert_stretch = (fp_vert_stretch & R128_VERT_STRETCH_RATIO_MASK) << R128_VERT_STRETCH_RATIO_SHIFT;
	fp_vert_stretch &= ~R128_VERT_AUTO_RATIO_EN;
	
	if (Vratio == 1.0)
		fp_vert_stretch &= ~(R128_VERT_STRETCH_ENABLE | R128_VERT_STRETCH_BLEND);
	else
		fp_vert_stretch |= (R128_VERT_STRETCH_ENABLE | R128_VERT_STRETCH_BLEND);


	// Write the vertical stretch
	WRITE_REG(R128_FP_VERT_STRETCH, fp_vert_stretch |
								(orig_vert_stretch & (R128_VERT_PANEL_SIZE |
													   R128_VERT_STRETCH_RESERVED)));
													   
													   
	READ_REG(R128_FP_GEN_CNTL,orig_fp_gen_cntl);
	
	fp_gen_cntl=(orig_fp_gen_cntl & ~(R128_FP_SEL_CRTC2 |
					       R128_FP_CRTC_USE_SHADOW_VEND |
					       R128_FP_CRTC_HORZ_DIV2_EN |
					       R128_FP_CRTC_HOR_CRT_DIV2_DIS |
					       R128_FP_USE_SHADOW_EN));
					       
    if (orig_fp_gen_cntl & R128_FP_DETECT_SENSE)
    {
		fp_gen_cntl |= (R128_FP_CRTC_DONT_SHADOW_VPAR |
								      R128_FP_TDMS_EN);
    }
    
    WRITE_REG(R128_FP_GEN_CNTL,fp_gen_cntl);
    
    READ_REG(R128_CRTC_EXT_CNTL,crtc_ext_cntl);
    crtc_ext_cntl &= ~R128_CRTC_CRT_ON;
    WRITE_REG(R128_CRTC_EXT_CNTL,crtc_ext_cntl);
    
    READ_REG(R128_DAC_CNTL,dac_cntl);
    dac_cntl|=R128_DAC_CRT_SEL_CRTC2;
    WRITE_REG(R128_DAC_CNTL,dac_cntl);

	WRITE_REG(R128_CRTC2_GEN_CNTL,0);
	
	/* WARNING: Be careful about turning on the flat panel */    
    
#if 0
    READ_REG(R128_LVDS_GEN_CNTL,lvds_gen_cntl);
    lvds_gen_cntl |= (R128_LVDS_ON | R128_LVDS_BLON);
    WRITE_REG(R128_LVDS_GEN_CNTL,lvds_gen_cntl);
#else
    READ_REG(R128_FP_PANEL_CNTL,fp_panel_cntl);    
    fp_panel_cntl  |= (R128_FP_DIGON | R128_FP_BLON);
    WRITE_REG(R128_FP_PANEL_CNTL,fp_panel_cntl);    
    
    fp_gen_cntl    |= (R128_FP_FPON);
    WRITE_REG(R128_FP_GEN_CNTL,fp_gen_cntl);
#endif
}

// #pragma mark -

//////////////////////////////////////////////////////////////////////////////
// Calculate Maximum Clocks
//    This calculates the maximum pixel clock rates at 8, 16, and 32 bpp
// colour depths (in kHz).

void calc_max_clocks(uint32 *max_pclk_8bpp, uint32 *max_pclk_16bpp,
  uint32 *max_pclk_32bpp)
{
  // Tattletale.
  ddprintf(("[R128 GFX]  calc_max_clocks() called.\n"));

  // HACK - Claim plausible values.
  *max_pclk_8bpp = 250000;
  *max_pclk_16bpp = 250000;
  *max_pclk_32bpp = 180000;
}


//////////////////////////////////////////////////////////////////////////////
// Propose a Video Mode
//    This function attempts to modify the specified target display_mode to
// satisfy hardware restrictions. If this can be done while keeping the
// range of all parameters within the ranges specified by "high" and "low",
// this returns B_OK. If this cannot be done, it returns B_ERROR.

status_t ProposeVideoMode(display_mode *target, const display_mode *low,
  const display_mode *high)
{
  uint32 bytes_per_row;
  uint32 max_height;
  uint32 granularity;
  uint32 old_h_sync_end;
  status_t IsOk;

  // Tattletale.
  ddprintf(("[R128 GFX]  ProposeVideoMode() called.\n"));


  // Initialize.
  IsOk = 1;

  // Check granularity restrictions.
  target->timing.h_display &= ~7;
  target->timing.h_total &= ~7;

  // H sync granularity is wierd. The pulse can start at any pixel offset,
  // but must be an integer number of characters wide.
  // Doing this the Nice Way and making sure that the sync pulse doesn't
  // get any narrower under normal conditions. This would cause hsync
  // failure under some conditions, in a pattern that would be annoyingly
  // difficult for a user to nail down.
  old_h_sync_end = target->timing.h_sync_end;
  target->timing.h_sync_end &= ~7;
  target->timing.h_sync_end |= target->timing.h_sync_start & 7;
  // Limit how high we can push h_sync_end. This cap is lower than the
  // storage limit but higher than anything the card could process.
  if ((target->timing.h_sync_end < old_h_sync_end)
    && (target->timing.h_sync_end < 16384))
    target->timing.h_sync_end += 8;

  // CalcBitsPerPixel returns the number of bits used for physical storage
  // in this format; i.e. 15 bpp modes still return 16.
  granularity = (64 << 3) / CalcBitsPerPixel(target->space);
  // This granularity will be a power of two. Subtract one and use masking
  // tricks.
  granularity--;
  target->virtual_width = (target->virtual_width + granularity)
    & (~granularity);

  // Sanity check virtual size.

  // Absolute size check.
  // NOTE: These limits haven't been rigorously checked.
  // A confusing point - the pitch register takes a maximum value of 1023
  // characters, which is silly. Using it anyways. This corresponds to a
  // maximum virtual width of 8184 pixels.
  // The maximum virtual height AFAICT is 16384, as 16383 is the largest
  // value that can be specified as a y coordinate in SRC_Y. Hopefully this
  // is the lowest limit used.
  if ((target->virtual_width > 8184) || (target->virtual_height > 16384))
    IsOk = 0;
  // For sanity's sake.
  if ((target->virtual_width < 320) || (target->virtual_height < 200))
    IsOk = 0;

  // Adjust virtual screen dimensions for available memory.
  if (IsOk)
  {
    bytes_per_row = target->virtual_width;
    bytes_per_row *= (CalcBitsPerPixel(target->space) >> 3);

    if (bytes_per_row == 0)
      IsOk = 0;
    else
    {
      // HACK - Assume 8 megabytes of card memory.
      max_height = 0x00800000 / bytes_per_row;

      if (max_height < (uint32) target->virtual_height)
        target->virtual_height = max_height;
    }
  }

  // Make sure that the real screen is smaller than the virtual screen,
  // taking positioning into account.
  if (IsOk)
  {
    if ((uint32) target->virtual_width < (uint32) target->timing.h_display
      + (uint32) target->h_display_start)
      IsOk = 0;

    if ((uint32) target->virtual_height < 
      ( (target->timing.flags & B_TIMING_INTERLACED) ?
        ((uint32) target->timing.v_display) << 1 :
        (uint32) target->timing.v_display )
      + (uint32) target->v_display_start)
      IsOk = 0;
  }

  // Make sure that the CRTC parameters are self-consistent.
  // The parameters must always be in the correct order, and the difference
  // between successive parameters must always be at least 1.
  if (IsOk)
  {
    if ((target->timing.h_total <= target->timing.h_sync_end)
      || (target->timing.h_sync_end <= target->timing.h_sync_start)
      || (target->timing.h_sync_start <= target->timing.h_display))
      IsOk = 0;

    if ((target->timing.v_total <= target->timing.v_sync_end)
      || (target->timing.v_sync_end <= target->timing.v_sync_start)
      || (target->timing.v_sync_start <= target->timing.v_display))
      IsOk = 0;
  }

  // Range checks on CRTC parameters.
  if (IsOk)
  {
    if ( (target->timing.h_total > 4096)
      || (target->timing.h_display > 2048)
      || (target->timing.v_total > 2048)
      || (target->timing.v_display > 2048) )
      IsOk = 0;

    // NOTE: I'm assuming that sync pulse parameters *aren't* decremented
    // (i.e. "1" means the first element, instead of "0"). This may not
    // be correct.
    if ( (target->timing.h_sync_start > 4095)
      || (target->timing.v_sync_start > 2047)
      // H sync pulse width is stored in characters.
      || ((target->timing.h_sync_end - target->timing.h_sync_start) > 504)
      || ((target->timing.v_sync_end - target->timing.v_sync_start) > 31) )
      IsOk = 0;
  }

  return (IsOk ? B_OK : B_ERROR);
}


//////////////////////////////////////////////////////////////////////////////
// Move the Display Area
//    This repositions the viewing window within the frame buffer. A fair bit
// of this should be abstracted out to the generic skeleton (pretty much all
// of what's in the dummy driver version, actually).
//    NOTE: We'll still need to address card-specific granularity limits,
// though.

status_t MoveDisplayArea(uint16 h_display_start, uint16 v_display_start)
{
  // Tattletale.
  ddprintf(("[R128 GFX]  MoveDisplayArea() called.\n"));


  // HACK - Not actually moving the display area for now.


  // Update the recorded position.

  // HACK - This should be in the device-independent section.
  // Update the stored record of the position.
  // HACK - No sanity checking, as per Trey's request.
  si->display.dm.h_display_start = h_display_start;
  si->display.dm.v_display_start = v_display_start;

  // Report success.
  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
