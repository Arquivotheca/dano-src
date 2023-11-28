//////////////////////////////////////////////////////////////////////////////
// G200 Accelerant Stuff
//    This file is going to need smashing and hacking and destroying and
// rebuilding before it's ready to go.  The second pass will be over by
// the time you read this comment.  Think on this, and know true horror.
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Includes //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include <Accelerant.h>
#include <Drivers.h>
#include <registers.h>
#include <cardid.h>
#include <private.h>

#include "defines.h"
#include "debugprint.h"
#include "mga_bios.h"
#include "accelerant_info.h"
#include "globals.h"
#include "mga_util.h"
#include "mga_gx00.h"


//////////////////////////////////////////////////////////////////////////////
// Defines ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#define _ClockRef       14318L 


//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Reset the Gx00 SGRAM

void Gx00ResetSGRAM(void);

void Gx00ResetSGRAM(void)
{
  ulong tmpUlong;
  uint32 MemType = (uint32)ai->bios.pins31.MemType;

  // Procedure take from MGA-1064SG 5-21
  // SGRAM/SDRAM Initialization

  // Step 1) Set scroff blanking field to prevent any transfer

  SCREEN_OFF;

#if 0
  // Step 2) Program the casltncy field of the MCTLWTST register

  //  STORM32W(MYSTI_MCTLWTST, 0x00030101);

  ddprintf(("Setting MCTLWTST to 0x%08x, default is 0x02032521\n",
            ai->bios.pins31.MCTLWTST));

  STORM32W(MYSTI_MCTLWTST, ai->bios.pins31.MCTLWTST);
#endif

  // Step 3) Program the memconfig field of the OPTION register

  tmpUlong = get_pci(STORM_OPTION, 4);

  // mask out memconfig bits

  switch(si->device_id)
    {
    case MGA_G100_AGP:
    case MGA_G100_PCI:
      tmpUlong &= 0xffffefff;
      tmpUlong |= (MemType & 0x04) << 10;
      break;

    case MGA_G200_AGP:
    case MGA_G200_PCI:
      tmpUlong &= 0xffffecff;
      tmpUlong |= (MemType & 0x07) << 10;
      break;
    }

  // set bits based on S[GD]RAM

  set_pci(STORM_OPTION, 4, tmpUlong);
  tmpUlong = get_pci(STORM_OPTION, 4);
  ddprintf(("Gx00 OPTION: 0x%08x\n", tmpUlong));

  // Step 4) Wait a minimum of 200us

  snooze(250);

  // Step 5) Set the memreset field and clear the jedecrst field of MACCESS

  STORM32W(STORM_MACCESS, 0x00008000);

  // NOTE: The G100 and G200 use different reset sequences!
  // If we're initializing a G200, we've done enough to trigger the reset.
  // If we're initializing a G100, we need to set both memreset and
  // jedecrst fields after a delay.

  switch(si->device_id)
    {
    case MGA_G100_AGP:
    case MGA_G100_PCI:
      // Step 6) Wait a minimum of (100 * the current MCLK period)
      snooze(2500);

      // Our code based on the G100 documentation.
      STORM32W(STORM_MACCESS, 0x0000C000);
      break;
    }

  // Step 8) Start the refresh by programming the rfhcnt field of
  //         the OPTION register
  // HACK - not calculating the refresh value; just using a value
  // assumed to be small enough.

  set_pci(STORM_OPTION, 4, 0x00078000 | (tmpUlong & 0xffe07fff));
  tmpUlong = get_pci(STORM_OPTION, 4);

  ddprintf(("Gx00 OPTION: 0x%08x\n", tmpUlong));

  switch(si->device_id)
    {
    case MGA_G100_AGP:
    case MGA_G100_PCI:
      tmpUlong = ((MemType & 0x0f0) >> 4) | ((MemType & 0x03) << 12);
      set_pci(STORM_OPTION2, 4, tmpUlong);
      ddprintf(("SysClk #5b OPTION2: 0x%08x\n", tmpUlong));
      break;
    }
}


//////////////////////////////////////////////////////////////////////////////
// Calculate Frequency
//    This one's ugly, but I'm not touching it just yet.

ulong Gx00CalcFreq(uchar m, uchar n, uchar p);

ulong Gx00CalcFreq(uchar m, uchar n, uchar p)
{
  ulong retval = (((ai->bios.pins31.ClkDiv & 0x20 ? 14318L : 27000L) * (n + 1)) / (m + 1)) / (p + 1);

  return retval;
}


//////////////////////////////////////////////////////////////////////////////
// Calculate M, N, P and S
//   The input value is the frequency in KHz.

ulong Gx00CalcMNPS(ulong freq, uchar *m, uchar *n, uchar *p, uchar *s);

ulong Gx00CalcMNPS(ulong freq,
                   uchar *m,
                   uchar *n,
                   uchar *p,
                   uchar *s)
{
  ulong Fvco;

  long delta = 250000;
  long d;

  uchar nt;
  uchar mt;
  uchar pt;

  uint32 FVCOMax = ((ai->bios.pins31.VCOMax + 100) * 1000);

  // find m and n such that:
  //  'freq' - ((14318 * (n + 1)) / (m + 1)) / (p + 1)
  // is as close to 0 as possible

  for(pt = 1; pt <= 8; pt <<= 1)
    {
      Fvco = freq * pt;

      // Don't put an upper boundary on frequency if the post divider is 1.
      // Clock setting will still fail, but at least we won't be feeding garbage
      // values to the clock set routines.
      if((Fvco >= 50000) && ((Fvco <= FVCOMax) || (pt == 1))) 
        {
          for(nt = 127; nt >= 7; nt--)
            {
              for(mt = 1; mt < 7; mt++)
                {
                  // mt = ((_ClockRef * (nt + 1)) / Fvco) - 1;
                  // if((mt > 0) && (mt < 32)) {
                  d = (long)Gx00CalcFreq(mt, nt, (pt - 1)) - (long)freq;

                  if(d < 0) d = -d;

                  if(d < delta)
                    {
                      *n = nt;
                      *m = mt;
                      *p = pt - 1;
                      delta = d;
                      *s = 0;
                      if(Fvco > 100000) (*s)++;
                      if(Fvco > 140000) (*s)++;
                      if(Fvco > 180000) (*s)++;
                    }
                }
            }
        }
    }

  Fvco = Gx00CalcFreq(*m, *n, *p);
  
  ddprintf(("Gx00CalcMNPS requested freq %ld, actual %ld, m %d, n %d, p %d, s %d\n", freq, Fvco, *m, *n, *p, *s));

  return Fvco;
}


//////////////////////////////////////////////////////////////////////////////
// Set Gx00 Pixel Clock

ulong Gx00SetPixClock(ulong freq)
{
  uchar m;
  uchar n;
  uchar p;
  uchar s;
  uchar tmpByte;
                
  Gx00CalcMNPS(freq, &m, &n, &p, &s);

  // Step 1) force screen off

  //  setScreenOn(false);
        
  // Step 2) Set pixclkdis to '1'

  DAC8W(MID_INDEX, MID_XPIXCLKCTRL);
  DAC8R(MID_X_DATAREG, tmpByte);
  tmpByte |= 0x04;
  DAC8W(MID_X_DATAREG, tmpByte);
        
  // Step 3) reprogram pix clock
  // program M, N, P, S

  DAC8W(MID_INDEX, MID_XPIXPLLCM);
  DAC8W(MID_X_DATAREG, m & 0x1f);
  DAC8W(MID_INDEX, MID_XPIXPLLCN);
  DAC8W(MID_X_DATAREG, n & 0x7f);
  DAC8W(MID_INDEX, MID_XPIXPLLCP);
  DAC8W(MID_X_DATAREG, ((s << 3) & 0x18) | (p & 0x07));

  // Select register set C

  STORM8R(VGA_MISC_R, tmpByte);
  tmpByte &= 0xf2; tmpByte |= 0x09;
  STORM8W(VGA_MISC_W, tmpByte);
        
  // Step 4) wait for frequency lock

  DAC8W(MID_INDEX, MID_XPIXPLLSTAT);
  { bigtime_t start = system_time();
  DAC8POLL(MID_X_DATAREG, 0x40, 0x40)
    if ((system_time() - start) > 1000000)
      break;
  }

  // Step 5) set pixclkdis to '0'

  DAC8W(MID_INDEX, MID_XPIXCLKCTRL);
  DAC8R(MID_X_DATAREG, tmpByte);
  DAC8W(MID_X_DATAREG, tmpByte & 0xfb);
        
  // Step 6) turn screen back on

  //  setScreenOn(true);

  // Return the frequency set.
  return Gx00CalcFreq(m, n, p);
}


//////////////////////////////////////////////////////////////////////////////
// Set Gx00 System Clock

void Gx00SetSysClock(void);

void Gx00SetSysClock(void)
{
  uchar m;
  uchar n;
  uchar p;
  uchar s;
  uint32 option;
  uint32 option2;
  uint8 ClkDiv;
  uint32 SysClk;
  uint32 Opt2;
  uint32 MemType = ai->bios.pins31.MemType;

  ClkDiv = ai->bios.pins31.ClkDiv;
  Opt2 = ai->bios.pins31.Option2;

  ddprintf(("SetSysClock()\n  ClkDiv: 0x%02x\n   Opt2: 0x%02x\n",
            ClkDiv,
            Opt2));

  switch(si->device_id)
    {
    case MGA_G100_AGP:
    case MGA_G100_PCI:
      SysClk = (uint32)ai->bios.pins31.HiResGClk * (ClkDiv & 0x01 ? 3 : 2);
      break;

    case MGA_G200_AGP:
    case MGA_G200_PCI:
      if(Opt2 & 0x04)
        {
          SysClk = (uint32)ai->bios.pins31.HiResGClk;
        }
      else
        {
          SysClk = (uint32)ai->bios.pins31.HiResGClk;
          SysClk += SysClk >> (ClkDiv & 0x01);
        }
      break;

    default: // Can't Happen (tm).
      ddprintf(("Yeek!  Unknown device ID in Gx00SetSysClock()!\n"));
      SysClk = 0; // Completely bogus...
      break;
    }

  Gx00CalcMNPS(SysClk * 1000, &m, &n, &p, &s);

  // MGA_G100 5-82
  // Step 1) Set sysclkdis to '1' (disable system clock)
  // 1. Disable system clock

  option = get_pci(STORM_OPTION, 4);
  option &= 0xfffffffb;
  option |= 0x00000004;
  set_pci(STORM_OPTION, 4, option);
  option = get_pci(STORM_OPTION, 4);
  ddprintf(("SysClk #1 OPTION: 0x%08x\n", option));
        
  // Step 2) Select PCI bus clock (sysclksl = '00')

  //  tmpUlong = get_pci(STORM_OPTION, 4);
  option &= 0xfffffffc;
  set_pci(STORM_OPTION, 4, option);
  option = get_pci(STORM_OPTION, 4);
  ddprintf(("SysClk #2 OPTION: 0x%08x\n", option));

  // Step 3) Set sysclkdis to '0' (enable)

  //  tmpUlong = get_pci(STORM_OPTION, 4);
  option &= 0xfffffffb;
  set_pci(STORM_OPTION, 4, option);
  option = get_pci(STORM_OPTION, 4);
  ddprintf(("SysClk #3 OPTION: 0x%08x\n", option));
        
  // Step 4) program PLL

  DAC8W(MID_INDEX, MID_XSYSPLLM);
  DAC8W(MID_X_DATAREG, m & 0x1f);
  DAC8W(MID_INDEX, MID_XSYSPLLN);
  DAC8W(MID_X_DATAREG, n & 0x7f);
  DAC8W(MID_INDEX, MID_XSYSPLLP);
  DAC8W(MID_X_DATAREG, ((s << 3) & 0x18) | (p & 0x07));

  // Step 5) wait for lock

  DAC8W(MID_INDEX, MID_XSYSPLLSTAT);
  { bigtime_t start = system_time();
  DAC8POLL(MID_X_DATAREG, 0x40, 0x40)
    if ((system_time() - start) > 1000000)
      break;
  }

  // Non-Step 5a) set MCLK and GCLK dividers

  //  tmpUlong = get_pci(STORM_OPTION, 4);
  option &= 0xffffff67;
  option |= ((uint32)ClkDiv & 0x03) << 3;

  // fast mem clock divider

  switch(si->device_id)
    {
    case MGA_G100_AGP:
    case MGA_G100_PCI:
      option |= ((uint32)ClkDiv & 0x04) << 5;
      break;
    }

  set_pci(STORM_OPTION, 4, option);
  option = get_pci(STORM_OPTION, 4);

  ddprintf(("SysClk #5a OPTION: 0x%08x\n", option));

  // Non-Step 5b) option2 parameters

  switch(si->device_id)
    {
    case MGA_G100_AGP:
    case MGA_G100_PCI:
      option2 = ((MemType & 0x0f0) >> 4) | ((MemType & 0x03) << 12);
      break;

    case MGA_G200_AGP:
    case MGA_G200_PCI:
      option2 = ((Opt2 & 0x3f) << 12);
      break;

    default: // Shouldn't happen, but...
      ddprintf(("Yeek!  Unknown device ID in GX00SetSysClock()!\n"));
      option2 = 0; // Completely bogus...
      break;
    }

  set_pci(STORM_OPTION2, 4, option2);

  ddprintf(("SysClk #5b OPTION2: 0x%08x\n", option2));
        
  // Step 6) disable clock

  //  tmpUlong = get_pci(STORM_OPTION, 4);
  option &= 0xfffffffb;
  option |= 0x00000004;
  set_pci(STORM_OPTION, 4, option);
  option = get_pci(STORM_OPTION, 4);
  ddprintf(("SysClk #6 OPTION: 0x%08x\n", option));
        
  // Step 7) select sys PLL (sysclksl = '01')

  //  tmpUlong = get_pci(STORM_OPTION, 4);
  option |= 0x01;
  set_pci(STORM_OPTION, 4, option);
  option = get_pci(STORM_OPTION, 4);
  ddprintf(("SysClk #7 OPTION: 0x%08x\n", option));

  // Step 8) sysclkdis = '0'

  //  tmpUlong = get_pci(STORM_OPTION, 4);
  option &= 0xfffffffb;
  set_pci(STORM_OPTION, 4, option);
  option = get_pci(STORM_OPTION, 4);
  ddprintf(("SysClk #8 OPTION: 0x%08x\n", option));
}


//////////////////////////////////////////////////////////////////////////////
// Power the Gx00 Up

void powerUpGx00(void)
{
  uchar tmpByte;
  ulong tmpUlong;

#if 0
  tmpUlong = get_pci(STORM_DEVCTRL, 4);
  ddprintf(("Gx00 DEVCTRL: 0x%08x\n", tmpUlong));
  tmpUlong = get_pci(STORM_ROMBASE, 4);
  ddprintf(("Gx00 ROMBASE: 0x%08x\n", tmpUlong));
  tmpUlong = get_pci(STORM_MGABASE1, 4);
  ddprintf(("Gx00 MGABASE1: 0x%08x\n", tmpUlong));
  tmpUlong = get_pci(STORM_MGABASE2, 4);
  ddprintf(("Gx00 MGABASE2: 0x%08x\n", tmpUlong));
  tmpUlong = get_pci(MYSTI_MGABASE3, 4);
  ddprintf(("Gx00 MGABASE3: 0x%08x\n", tmpUlong));
  ddprintf(("Gx00     REGS: 0x%08x\n", bi.control));
#endif

  tmpUlong = get_pci(STORM_OPTION, 4);
  ddprintf(("Gx00 OPTION: 0x%08x\n", tmpUlong));

#if !defined(__POWERPC__)
  // Step 0) Fill a scratch variable with a cryptic constant.

  tmpUlong &= 0x7ffffeff;
#else   
  // Step 0) Put card into PowerPC mode and disable vgaioen

  tmpUlong |= 0x80000000;
  tmpUlong &= 0xfffffeff;
#endif
  set_pci(STORM_OPTION, 4, tmpUlong);
  //  tmpUlong = get_pci(STORM_OPTION, 4);
  ddprintf(("Gx00 OPTION: 0x%08x\n", tmpUlong));

  // Step 2) Program the casltncy field of the MCTLWTST register

  ddprintf(("Setting MCTLWTST to 0x%08x, default is 0x02032521\n",
            ai->bios.pins31.MCTLWTST));

  STORM32W(MYSTI_MCTLWTST, ai->bios.pins31.MCTLWTST);

  // Procedure taken from MGA-G100 5-24
  // Analog Macro Power Up Sequence
  // Step 1a) Program XVREFCTRL

  DAC8W(MID_INDEX, MID_XVREFCTRL);
  DAC8W(MID_X_DATAREG, 0x3f);
  DAC8R(MID_X_DATAREG, tmpByte);
  ddprintf(("MID_XVREFCTRL: 0x%02x\n", tmpByte));

  // Step 1b) Wait 100mS for stabilization

  snooze(10000); // Don't need fine-grained sleep here.
        
  // Step 2) Power up system PLL by setting the syspllpdN field
  // of OPTION to '1'

  //  tmpUlong = get_pci(STORM_OPTION, 4);
  tmpUlong |= 0x20;
  set_pci(STORM_OPTION, 4, tmpUlong);
  //  tmpUlong = get_pci(STORM_OPTION, 4);
  ddprintf(("Gx00 OPTION: 0x%08x\n", tmpUlong));

  // Step 3) Wait for system PLL lock (syslock field of XSYSPLLSTAT is '1')

  DAC8W(MID_INDEX, MID_XSYSPLLSTAT);
  ddprintf((" waiting for syslock\n"));
  { bigtime_t start = system_time();
  DAC8POLL(MID_X_DATAREG, 0x40, 0x40)
    if ((system_time() - start) > 1000000)
      break;
  }
  ddprintf((" got syspll lock\n"));
        
  // Step 4) Power up pixel PLL by setting the pixpllpdN field
  // of XPIXCLKCTRL to '1'

  DAC8W(MID_INDEX, MID_XPIXCLKCTRL);
  DAC8R(MID_X_DATAREG, tmpByte);
  DAC8W(MID_X_DATAREG, tmpByte | 0x08);

  // Step 5) Wait for pixel PLL lock (pixlock field of XPIXPLLSTAT is '1')

  DAC8W(MID_INDEX, MID_XPIXPLLSTAT);
  ddprintf((" waiting for pixlock\n"));
  { bigtime_t start = system_time();
  DAC8POLL(MID_X_DATAREG, 0x40, 0x40)
    if ((system_time() - start) > 1000000)
      break;
  }
  ddprintf((" got pixel lock\n"));
        
  // Step 6) Power up LUT by setting the ramcs field of XMISCCTRL to '1'
  // Step 7) Power up DAC by setting the dacpdN field of XMISCCTRL to '1'
  //  Also, enable 8 bit palette instead of 6 bit palette

  DAC8W(MID_INDEX, MID_XMISCCTRL);
  DAC8W(MID_X_DATAREG, 0x19);
        
  // enable clocks

  // 1. Disable system clock

  //  tmpUlong = get_pci(STORM_OPTION, 4);
  tmpUlong &= 0xfffffff8;
  tmpUlong |= 0x00000004;
  set_pci(STORM_OPTION, 4, tmpUlong);
  //  tmpUlong = get_pci(STORM_OPTION, 4);
  ddprintf(("Gx00 OPTION: 0x%08x\n", tmpUlong));
        
  // 2. Select system PLL

  //  tmpUlong = get_pci(STORM_OPTION, 4);
  tmpUlong |= 0x01;
  set_pci(STORM_OPTION, 4, tmpUlong);
  //  tmpUlong = get_pci(STORM_OPTION, 4);

  ddprintf(("Gx00 OPTION: 0x%08x\n", tmpUlong));

  // 3. Enable system clock

  //  tmpUlong = get_pci(STORM_OPTION, 4);
  tmpUlong &= 0xfffffffb;
  set_pci(STORM_OPTION, 4, tmpUlong);
  //  tmpUlong = get_pci(STORM_OPTION, 4);
  ddprintf(("Gx00 OPTION: 0x%08x\n", tmpUlong));
  DAC8W(MID_INDEX, MID_XPIXCLKCTRL);
  DAC8R(MID_X_DATAREG, tmpByte);

  // 4. Disable pixel and video clocks

  DAC8W(MID_X_DATAREG, tmpByte | 0x04);

  // 5. Select pixel PLL '01'

  DAC8R(MID_X_DATAREG, tmpByte);
  DAC8W(MID_X_DATAREG, (tmpByte & 0xfc) | 0x01);

  // 6. Enable pixel and video clocks

  DAC8W(MID_INDEX, MID_XPIXCLKCTRL);
  DAC8R(MID_X_DATAREG, tmpByte);
  DAC8W(MID_X_DATAREG, tmpByte & 0xfb);

  // minimum value to get things going

  ai->mem_size = VIDRAM8MB;
  Gx00SetSysClock();

  // reset SGRAM

  Gx00ResetSGRAM();

  // figure out how much RAM we really have

  countRAM();

  // reset sys clock based on ram count

  Gx00SetSysClock();
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
