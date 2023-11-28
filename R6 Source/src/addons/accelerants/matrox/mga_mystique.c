//////////////////////////////////////////////////////////////////////////////
// Mystique Stuff
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
#include "mga_mystique.h"


//////////////////////////////////////////////////////////////////////////////
// Defines ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#define _ClockRef       14318L 


//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Calculate the Frequency

ulong midCalcFreq(uchar m, uchar n, uchar p);

ulong midCalcFreq(uchar m, uchar n, uchar p)
{
  ulong retval = ((_ClockRef * (n + 1)) / (m + 1)) / (p + 1);
  return retval;
}


//////////////////////////////////////////////////////////////////////////////
// Calculate MNPS
//    inputs: freq in KHz

ulong midCalcMNPS(ulong freq, uchar *m, uchar *n, uchar *p, uchar *s);

ulong midCalcMNPS(ulong freq, uchar *m, uchar *n, uchar *p, uchar *s)
{
  ulong Fvco;
  long delta, d;
  uchar nt, mt, pt;
        
  /* find m and n such that: */
  /*  'freq' - ((14318 * (n + 1)) / (m + 1)) / (p + 1) */
  /* is as close to 0 as possible */
  delta = 220000;

  for(pt = 1; pt <= 8; pt <<= 1)
    {
      Fvco = freq * pt;
      // Don't put an upper boundary on frequency if the post divider is 1.
      // The VCO limit is higher than 220 MHz for many cards.
      if((Fvco > 50000) && ((Fvco < 220000) || (pt == 1))) 
        {
          for(nt = 127; nt >= 100; nt--) 
            {
              for(mt = 1; mt < 32; mt++) 
                {
                  /* mt = ((_ClockRef * (nt + 1)) / Fvco) - 1; */
                  /*if ((mt > 0) && (mt < 32)) { */
                  d = (long)midCalcFreq(mt, nt, (pt - 1)) - (long)freq;

                  if(d < 0) d = -d;

                  if(d < delta) 
                    {
                      *n = nt;
                      *m = mt;
                      *p = pt - 1;
                      delta = d;
                      *s = 0;
                      if (Fvco > 100000) (*s)++;
                      if (Fvco > 140000) (*s)++;
                      if (Fvco > 180000) (*s)++;
                    }
                }
            }
        }
    }
  Fvco = midCalcFreq(*m, *n, *p);
  ddprintf(("midCalcMNP requested freq %ld, actual %ld, m %d, n %d, p %d, s %d\n", freq, Fvco, *m, *n, *p, *s));
  return Fvco;
}


//////////////////////////////////////////////////////////////////////////////
// Set System Clock

uint32 midSetSysClock(void);

uint32 midSetSysClock(void)
{
  uchar m, n, p, s;
  ulong tmpUlong;
  uint8 ClkGE, ClkMem;
  uint32 SysClk;
  bool threeTwoDivider;
        
  ClkGE = ai->bios.pins.ClkGE;
  ClkMem = ai->bios.pins.ClkMem;
        
  if((ai->mem_size == VIDRAM4MB) &&
     (ai->bios.pins.Clk4MB != 0xff))
    {
      ClkGE = ai->bios.pins.Clk4MB;
    }

  if((ai->mem_size == VIDRAM8MB) &&
     (ai->bios.pins.Clk8MB != 0xff))
    {
      ClkGE = ai->bios.pins.Clk8MB;
    }
        
  threeTwoDivider = true;

  if((ClkGE != 0xff) && (ClkMem == 0xff))
    {
      SysClk = (uint32)ClkGE * 3; /* in MHz */
    }
  else if((ClkGE == 0xff) && (ClkMem != 0xff))
    {
      SysClk = (uint32)ClkMem * 2;
    }
  else if((ClkGE == ClkMem) && (ClkGE != 0xff))
    {
      SysClk = (uint32)ClkMem;
      threeTwoDivider = false;
    } 
  else
    {
      SysClk = 132;
    }

  midCalcMNPS(SysClk * 1000, &m, &n, &p, &s);

  /* MGA1064S 5-77 */
  /* Step 1) Set sysclkdis to '1' (disable system clock) */
  /* 1. Disable system clock */
  tmpUlong = get_pci(STORM_OPTION, 4);
  tmpUlong &= 0xfffffffb;
  tmpUlong |= 0x00000004;
  set_pci(STORM_OPTION, 4, tmpUlong);
  tmpUlong = get_pci(STORM_OPTION, 4);
  ddprintf(("SysClk #1 OPTION: 0x%08x\n", tmpUlong));
        
  /* Step 2) Select PCI bus clock (sysclksl = '00') */
  /*tmpUlong = get_pci(STORM_OPTION, 4); */
  tmpUlong &= 0xfffffffc;
  set_pci(STORM_OPTION, 4, tmpUlong);
  tmpUlong = get_pci(STORM_OPTION, 4);
  ddprintf(("SysClk #2 OPTION: 0x%08x\n", tmpUlong));

  /* Step 3) Set sysclkdis to '0' (enable) */
  /*tmpUlong = get_pci(STORM_OPTION, 4); */
  tmpUlong &= 0xfffffffb;
  set_pci(STORM_OPTION, 4, tmpUlong);
  tmpUlong = get_pci(STORM_OPTION, 4);
  ddprintf(("SysClk #3 OPTION: 0x%08x\n", tmpUlong));
        
  /* Step 4) program PLL */
  DAC8W(MID_INDEX, MID_XSYSPLLM);
  DAC8W(MID_X_DATAREG, m & 0x1f);
  DAC8W(MID_INDEX, MID_XSYSPLLN);
  DAC8W(MID_X_DATAREG, n & 0x7f);
  DAC8W(MID_INDEX, MID_XSYSPLLP);
  DAC8W(MID_X_DATAREG, ((s << 3) & 0x18) | (p & 0x07));

  /* Step 5) wait for lock */
  DAC8W(MID_INDEX, MID_XSYSPLLSTAT);
  { bigtime_t start = system_time();
  DAC8POLL(MID_X_DATAREG, 0x40, 0x40)
    if ((system_time() - start) > 1000000)
      break;
  }

  /* Step 5a) set MCLK and GCLK dividers */
  /*tmpUlong = get_pci(STORM_OPTION, 4); */
  if (threeTwoDivider) tmpUlong &= 0xffffffe7;
  else tmpUlong |= 0x018;
  set_pci(STORM_OPTION, 4, tmpUlong);
  tmpUlong = get_pci(STORM_OPTION, 4);
  ddprintf(("SysClk #5a OPTION: 0x%08x\n", tmpUlong));
        
  /* Step 6) disable clock */
  /*tmpUlong = get_pci(STORM_OPTION, 4); */
  tmpUlong &= 0xfffffffb;
  tmpUlong |= 0x00000004;
  set_pci(STORM_OPTION, 4, tmpUlong);
  tmpUlong = get_pci(STORM_OPTION, 4);
  ddprintf(("SysClk #6 OPTION: 0x%08x\n", tmpUlong));
        
  /* Step 7) select sys PLL (sysclksl = '01') */
  /*tmpUlong = get_pci(STORM_OPTION, 4); */
  tmpUlong |= 0x01;
  set_pci(STORM_OPTION, 4, tmpUlong);
  tmpUlong = get_pci(STORM_OPTION, 4);
  ddprintf(("SysClk #7 OPTION: 0x%08x\n", tmpUlong));

  /* Step 8) sysclkdis = '0' */
  /*tmpUlong = get_pci(STORM_OPTION, 4); */
  tmpUlong &= 0xfffffffb;
  set_pci(STORM_OPTION, 4, tmpUlong);
  tmpUlong = get_pci(STORM_OPTION, 4);
  ddprintf(("SysClk #8 OPTION: 0x%08x\n", tmpUlong));

  return (SysClk * 1000000) / (threeTwoDivider ? 2 : 1);
}


//////////////////////////////////////////////////////////////////////////////

ulong midSetPixClock(ulong freq)
{
  uchar m, n, p, s;
  uchar tmpByte;
                
  midCalcMNPS(freq, &m, &n, &p, &s);

  /* MGA1064SG 5-77 */
  /* Step 1) force screen off */
  /*setScreenOn(false); */
        
  /* Step 2) Set pixclkdis to '1' */
  DAC8W(MID_INDEX, MID_XPIXCLKCTRL);
  DAC8R(MID_X_DATAREG, tmpByte);
  tmpByte |= 0x04;
  DAC8W(MID_X_DATAREG, tmpByte);
        
  /* Step 3) reprogram pix clock */
  /* program M, N, P, S */
  DAC8W(MID_INDEX, MID_XPIXPLLCM);
  DAC8W(MID_X_DATAREG, m & 0x1f);
  DAC8W(MID_INDEX, MID_XPIXPLLCN);
  DAC8W(MID_X_DATAREG, n & 0x7f);
  DAC8W(MID_INDEX, MID_XPIXPLLCP);
  DAC8W(MID_X_DATAREG, ((s << 3) & 0x18) | (p & 0x07));
  /* Select register set C */
  STORM8R(VGA_MISC_R, tmpByte);
  tmpByte &= 0xf2; tmpByte |= 0x09;
  STORM8W(VGA_MISC_W, tmpByte);
        
  /* Step 4) wait for frequency lock */
  DAC8W(MID_INDEX, MID_XPIXPLLSTAT);
  { bigtime_t start = system_time();
  DAC8POLL(MID_X_DATAREG, 0x40, 0x40)
    if ((system_time() - start) > 1000000)
      break;
  }

  /* Step 5) set pixclkdis to '0' */
  DAC8W(MID_INDEX, MID_XPIXCLKCTRL);
  DAC8R(MID_X_DATAREG, tmpByte);
  DAC8W(MID_X_DATAREG, tmpByte & 0xfb);
        
  /* Step 6) turn screen back on */
  /*setScreenOn(true); */


  // Return the frequency set.
  return midCalcFreq(m, n, p);
}


//////////////////////////////////////////////////////////////////////////////
// Reset the SGRAM

void resetSGRAM(uint32 mclock);

void resetSGRAM(uint32 mclock)
{
  ulong tmpUlong;

  /* Procedure take from MGA-1064SG 5-21 */
  /* SGRAM/SDRAM Initialization */
  /* Step 1) Set scroff blanking field to prevent any transfer */
  SCREEN_OFF;
  /* Step 2) Program the casltncy field of the MCTLWTST register */
  /* STORM32W(MYSTI_MCTLWTST, 0x00030101); */
  tmpUlong = (ai->bios.pins.MCTLWTST & 0x01) | /* castlncy */
    ((ulong)(ai->bios.pins.MCTLWTST & 0x02) << 7) | /* rcdelay */
    ((ulong)(ai->bios.pins.MCTLWTST & 0x0c) << 14); /* rasmin */
  STORM32W(MYSTI_MCTLWTST, tmpUlong);
  /* Step 3) Program the memconfig field of the OPTION register */
  tmpUlong = get_pci(STORM_OPTION, 4);
  /* mask out memconfig bits */
  tmpUlong &= 0xffff81ff;
  /* set bits based on S[GD]RAM */
  tmpUlong |= (tmpUlong & 0x01000000) ? 0x00004e00 : 0x00001e00;
  set_pci(STORM_OPTION, 4, tmpUlong);
  tmpUlong = get_pci(STORM_OPTION, 4);
  ddprintf(("Mystique OPTION: 0x%08x\n", tmpUlong));

  /* Step 4) Wait a minimum of 200us */
  snooze(250);
  /* Step 5) Set the memreset field and clear the jedecrst field of MACCESS */
  STORM32W(STORM_MACCESS, 0x00008000);
  /* Step 6) Wait a minimum of (100 * the current MCLK period) */
  snooze(2500);
  /* Step 7) Set the memreset and jedcrst fields of MACCESS */
  STORM32W(STORM_MACCESS, 0x0000c000);
  /* Step 8) Start the refresh by programming the rfhcnt field of the OPTION register */
#if 1
  {
   /* caculate the refresh counter by the book */
   uint32 xx = 10000 / (mclock / 1000000);
   uint32 x256, x64;
   uint32 val;
   /* 33.2 usec >= ((x256 * 256) + (x64 * 64) + 1) * (1/mclock) * 1 */
   x256 = 0;  while (332000 >= (xx * ((256 * (x256+1)) + 1))) x256++;
   x64 = 0; if (332000 >= (xx * ((256 * x256) + (64 + 1)))) x64 = 1;
   val = (x256 << 1) + x64;
   if (val > 15) val = 15;
   set_pci(STORM_OPTION, 4, ((val & 0x0f) << 16) | (tmpUlong & 0xfff0ffff));
  }
#else
  set_pci(STORM_OPTION, 4, 0x000F0000 | (tmpUlong & 0xfff0ffff)); // our new constant value
  //set_pci(STORM_OPTION, 4, 0x00090000 | (tmpUlong & 0xfff0ffff)); // our old constant value
#endif
  tmpUlong = get_pci(STORM_OPTION, 4);
  ddprintf(("Mystique OPTION: 0x%08x\n", tmpUlong));
}


//////////////////////////////////////////////////////////////////////////////
// Calculate the Mid Regs

void calcMidRegs(uint32 cs, uint32 flags)
{
  uchar tmpByte;
  int i;
        
  /* not much to do here */
  switch (cs) 
    {
    case B_CMAP8:
      tmpByte = 0x00;  
      break;

    case B_RGB15_LITTLE:
    case B_RGB15_BIG:
      tmpByte = 0x01;
      DAC8W(MID_PALWTADD, 0);

      for (i = 0; i < 32; i++)
        {
          DAC8W(MID_PALDATA, (i << 3) + (i >> 2));
          DAC8W(MID_PALDATA, (i << 3) + (i >> 2));
          DAC8W(MID_PALDATA, (i << 3) + (i >> 2));
        }
      break;

    case B_RGB16_LITTLE:
    case B_RGB16_BIG:
      tmpByte = 0x02;
      DAC8W(MID_PALWTADD, 0);
      for (i = 0; i < 64; i++)
        {
          DAC8W(MID_PALDATA, (i << 3) + (i >> 2)); /* check this formula */
          DAC8W(MID_PALDATA, (i << 2) + (i >> 4)); /* check this formula */
          DAC8W(MID_PALDATA, (i << 3) + (i >> 2)); /* check this formula */

          ddprintf(("rgb: %d, %d, %d\n",
                    (i << 3) + (i >> 2),
                    (i << 2) + (i >> 4),
                    (i << 3) + (i >> 2)));

          // DAC8W(MID_PALDATA, (i << 3) + (i >> 2));
          // DAC8W(MID_PALDATA, (i << 3) + (i >> 2));
        }
      break;

    case B_RGB32_LITTLE:
    case B_RGB32_BIG:
    case B_RGBA32_LITTLE:
    case B_RGBA32_BIG:
      tmpByte = 0x04;
      break;

    default: // Can't (shouldn't) Happen.
      ddprintf(("Yeek!  Bad arg to calcMidRegs()!"));
      tmpByte = 0; // Completely bogus value.
      break;
    }

  DAC8W(MID_INDEX, MID_XCOLKEYH);
  DAC8W(MID_X_DATAREG, 0);
  DAC8W(MID_INDEX, MID_XCOLKEYL);
  DAC8W(MID_X_DATAREG, 0);
  DAC8W(MID_INDEX, MID_XCOLKEYMSKH);
  DAC8W(MID_X_DATAREG, 0);
  DAC8W(MID_INDEX, MID_XCOLKEYMSKL);
  DAC8W(MID_X_DATAREG, 0);
  DAC8W(MID_PIXRDMSK, 0xff);
  DAC8W(MID_INDEX, MID_XMULCTRL);
  DAC8W(MID_X_DATAREG, tmpByte);
  DAC8W(MID_INDEX, MID_XGENCTRL); /* force to defaults */
  tmpByte = 0x20; // SOG off by default
  if (flags & B_SYNC_ON_GREEN) tmpByte &= ~0x20; // 0 == enable for this bit
  if (flags & B_BLANK_PEDESTAL) tmpByte |= 0x10; // 7.5 ire blank pedestal
  DAC8W(MID_X_DATAREG, tmpByte);
}


//////////////////////////////////////////////////////////////////////////////
// Powerup Sequence for the Mystique

void powerUpMystique(void)
{
  uchar tmpByte;
  ulong tmpUlong;
  uint32 mclock;

#if 0
  tmpUlong = get_pci(STORM_DEVCTRL, 4);
  ddprintf(("Mystique DEVCTRL: 0x%08x\n", tmpUlong));
  tmpUlong = get_pci(STORM_ROMBASE, 4);
  ddprintf(("Mystique ROMBASE: 0x%08x\n", tmpUlong));
  tmpUlong = get_pci(STORM_MGABASE1, 4);
  ddprintf(("Mystique MGABASE1: 0x%08x\n", tmpUlong));
  tmpUlong = get_pci(STORM_MGABASE2, 4);
  ddprintf(("Mystique MGABASE2: 0x%08x\n", tmpUlong));
  tmpUlong = get_pci(MYSTI_MGABASE3, 4);
  ddprintf(("Mystique MGABASE3: 0x%08x\n", tmpUlong));
  ddprintf(("Mystique   REGS: 0x%08x\n", bi.control));
#endif

  tmpUlong = get_pci(STORM_OPTION, 4);
  ddprintf(("Mystique OPTION: 0x%08x\n", tmpUlong));

#if !defined(__POWERPC__)
  tmpUlong &= 0x7ffffeff;
#else   
  /* Step 0) Put card into PowerPC mode and disable vgaioen */
  tmpUlong |= 0x80000000;
  tmpUlong &= 0xfffffeff;
#endif
  set_pci(STORM_OPTION, 4, tmpUlong);
  /*tmpUlong = get_pci(STORM_OPTION, 4); */
  ddprintf(("Mystique OPTION: 0x%08x\n", tmpUlong));

  /* Procedure taken from MGA-1064SG 5-21        */
  /* Analog Macro Power Up Sequence */
  /* Step 1a) Program XVREFCTRL */
  DAC8W(MID_INDEX, MID_XVREFCTRL);
  DAC8W(MID_X_DATAREG, 0x3f);
  DAC8R(MID_X_DATAREG, tmpByte);
  ddprintf(("MID_XVREFCTRL: 0x%02x\n", tmpByte));

  /* Step 1b) Wait 100mS for stabilization */
  snooze(100000);
        
  /* Step 2) Power up system PLL by setting the syspllpdN field of OPTION to '1' */
  /*tmpUlong = get_pci(STORM_OPTION, 4); */
  tmpUlong |= 0x20;
  set_pci(STORM_OPTION, 4, tmpUlong);
  /*tmpUlong = get_pci(STORM_OPTION, 4); */
  ddprintf(("Mystique OPTION: 0x%08x\n", tmpUlong));

  /* Step 3) Wait for system PLL lock (syslock field of XSYSPLLSTAT is '1') */
  DAC8W(MID_INDEX, MID_XSYSPLLSTAT);
  ddprintf((" waiting for syslock\n"));
  { bigtime_t start = system_time();
  DAC8POLL(MID_X_DATAREG, 0x40, 0x40)
    if ((system_time() - start) > 1000000)
      break;
  }
  ddprintf((" got syspll lock\n"));
        
  /* Step 4) Power up pixel PLL by setting the pixpllpdN field of XPIXCLKCTRL to '1' */
  DAC8W(MID_INDEX, MID_XPIXCLKCTRL);
  DAC8R(MID_X_DATAREG, tmpByte);
  DAC8W(MID_X_DATAREG, tmpByte | 0x08);

  /* Step 5) Wait for pixel PLL lock (pixlock field of XPIXPLLSTAT is '1') */
  DAC8W(MID_INDEX, MID_XPIXPLLSTAT);
  ddprintf((" waiting for pixlock\n"));
  { bigtime_t start = system_time();
  DAC8POLL(MID_X_DATAREG, 0x40, 0x40)
    if ((system_time() - start) > 1000000)
      break;
  }
  ddprintf((" got pixel lock\n"));
        
  /* Step 6) Power up LUT by setting the ramcs field of XMISCCTRL to '1' */
  /* Step 7) Power up DAC by setting the dacpdN field of XMISCCTRL to '1' */
  /*  Also, enable 8 bit palette instead of 6 bit palette */
  DAC8W(MID_INDEX, MID_XMISCCTRL);
  DAC8W(MID_X_DATAREG, 0x19);
        
  /* enable clocks */
  /* 1. Disable system clock */
  /*tmpUlong = get_pci(STORM_OPTION, 4); */
  tmpUlong &= 0xfffffff8;
  tmpUlong |= 0x00000004;
  set_pci(STORM_OPTION, 4, tmpUlong);
  /*tmpUlong = get_pci(STORM_OPTION, 4); */
  ddprintf(("Mystique OPTION: 0x%08x\n", tmpUlong));
        
  /* 2. Select system PLL */
  /*tmpUlong = get_pci(STORM_OPTION, 4); */
  tmpUlong |= 0x01;
  set_pci(STORM_OPTION, 4, tmpUlong);
  /*tmpUlong = get_pci(STORM_OPTION, 4); */
  ddprintf(("Mystique OPTION: 0x%08x\n", tmpUlong));

  /* 3. Enable system clock */
  /*tmpUlong = get_pci(STORM_OPTION, 4); */
  tmpUlong &= 0xfffffffb;
  set_pci(STORM_OPTION, 4, tmpUlong);
  /*tmpUlong = get_pci(STORM_OPTION, 4); */
  ddprintf(("Mystique OPTION: 0x%08x\n", tmpUlong));
        
  DAC8W(MID_INDEX, MID_XPIXCLKCTRL);
  DAC8R(MID_X_DATAREG, tmpByte);
  /* 4. Disable pixel and video clocks */
  DAC8W(MID_X_DATAREG, tmpByte | 0x04);
  /* 5. Select pixel PLL '01' */
  DAC8R(MID_X_DATAREG, tmpByte);
  DAC8W(MID_X_DATAREG, (tmpByte & 0xfc) | 0x01);

  /* 6. Enable pixel and video clocks */
  DAC8W(MID_INDEX, MID_XPIXCLKCTRL);
  DAC8R(MID_X_DATAREG, tmpByte);
  DAC8W(MID_X_DATAREG, tmpByte & 0xfb);

  /* minimum value to get things going */
  ai->mem_size = VIDRAM2MB;
  mclock = midSetSysClock();

  /* reset SGRAM */
  resetSGRAM(mclock);
  /* figure out how much RAM we really have */
  countRAM();
  /* reset sys clock based on ram count */
  mclock = midSetSysClock();
  /* reset SGRAM */
  resetSGRAM(mclock);
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
