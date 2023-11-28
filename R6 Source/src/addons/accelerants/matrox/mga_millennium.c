//////////////////////////////////////////////////////////////////////////////
// Millenium Specific Routines
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
#include "mga_millennium.h"


//////////////////////////////////////////////////////////////////////////////
// Defines ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#define _ClockRef       14318L 
#define ClockRef        ((ulong)(8 * _ClockRef))
#define VCO_MIN         ((ulong)110000)
#define VCO_ADJUST      ((ulong)224500)

#define PCLK_PLL     0
#define LCLK_PLL     1
#define MCLK_PLL     2
#define VCLOCK          0
#define MCLOCK          1


//////////////////////////////////////////////////////////////////////////////
// Globals ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


static uchar MGADACregs[] =
{
  0x0F, 0x18, 0x19, 0x1A,
  0x1C, 0x1D, 0x1E, 0x2A,
  0x2B, 0x30, 0x31, 0x32,
  0x33, 0x34, 0x35, 0x36,
  0x37, 0x38, 0x3A
};


/* values for register 0x19 will be filled in at run time */

static uchar MGADACbpp8[] =
{
  0x06, 0x80, 0x00, 0x25,
  0x00, 0x00, 0x0C, 0x00,
  0x1E, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00			// index 0x37 set to 0x00 to fix loss of sync bug (as per TI tech support)
};


static uchar MGADACbpp16[] =
{
  /* 0x04 for 1555,
     0x05 for 565  */
  0x06, 0x04, 0x00, 0x15,
  0x00, 0x00, 0x20, 0x00,
  0x1E, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00
};


static uchar MGADACbpp32[] =
{
  0x07, 0x06, 0x00, 0x05,
  0x00, 0x00, 0x20, 0x00,
  0x1E, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00
};


//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Calculate Millennium Clock

static uint32 calcMillenniumMCLK(void)
{
  uint32 mem_clock;
        
  if((si->device_id == MGA_2164W) || (si->device_id == MGA_2164W_AGP))
    {
      /* calc mem clock */
      uint8 ClkGE, ClkMem;
      uint32 SysClk;
                
      ClkGE = ai->bios.pins.ClkGE;
      ClkMem = ai->bios.pins.ClkMem;

      if(ai->mem_size == 0)
        {
          ai->mem_size = VIDRAM2MB;
        }
                
      if((ai->mem_size == VIDRAM4MB) &&
         (ai->bios.pins.Clk4MB != 0xff))
        {
          ClkGE = ai->bios.pins.Clk4MB;
        }

      if((ai->mem_size >= VIDRAM4MB) &&
         (ai->bios.pins.Clk8MB != 0xff))
        {
          ClkGE = ai->bios.pins.Clk8MB;
        }

      if((ai->mem_size == VIDRAM12MB) &&
         (ai->bios.pins.Clk12MB != 0xff))
        {
          ClkGE = ai->bios.pins.Clk12MB;
        }

      if((ai->mem_size == VIDRAM16MB) &&
         (ai->bios.pins.Clk16MB != 0xff))
        {
          ClkGE = ai->bios.pins.Clk16MB;
        }

      // Adjust SysClk.
        
      if((ClkGE != 0xff) && (ClkMem == 0xff))
        {
          SysClk = (uint32)ClkGE; /* in MHz */
        }
      else if((ClkGE == 0xff) && (ClkMem != 0xff))
        {
          SysClk = (uint32)ClkMem;
        }
      else if((ClkGE == ClkMem) && (ClkGE != 0xff))
        {
          SysClk = (uint32)ClkGE;
        }
      else
        {
          SysClk = 60;
        }

      /*SysClk = 72; */
      mem_clock = SysClk * 1000000L;
    }
  else 
    {
      ulong c2mb, c4mb, c8mb;
      c2mb = ai->bios.orig.ClkBase * 10000L;
      c4mb = ai->bios.orig.Clk4MB  * 10000L;
      c8mb = ai->bios.orig.Clk8MB  * 10000L;
      if(c2mb == 0) c2mb = 45000000L;
      if(c4mb == 0) c4mb = c2mb;
      if(c8mb == 0) c8mb = c4mb;
                
      if(ai->mem_size == 0) ai->mem_size = VIDRAM2MB;

      switch(ai->mem_size)
        {
        case VIDRAM2MB: mem_clock = c2mb; break;
        case VIDRAM4MB: mem_clock = c4mb; break;
        case VIDRAM8MB: mem_clock = c8mb; break;

        default:
          ddprintf(("Yeek!  Bogus ai->mem_size in calcMilleniumMCLK()!\n"));
          mem_clock = 0; // Heh.  This should cause fires to break out.
          break;
        }
    }
  return mem_clock;
}


//////////////////////////////////////////////////////////////////////////////
// Program the PLL

static void tiProgramPLL(uchar par, uchar clock, uchar n, uchar m, uchar p)
{

  DAC8W(TVP3026_INDEX, TVP3026_PLL_ADDR);
  DAC8W(TVP3026_DATA, par);
  /* program */
  DAC8W(TVP3026_INDEX, clock);
  DAC8W(TVP3026_DATA, n);
  DAC8W(TVP3026_DATA, m);
  DAC8W(TVP3026_DATA, p);
  /* wait for lock */
  { bigtime_t start = system_time();
  DAC8POLL(TVP3026_DATA, 0x40, 0x40)
    if ((system_time() - start) > 1000000)
      break;
  }
}


//////////////////////////////////////////////////////////////////////////////
// Caculate the TI Frequency

static ulong tiCalcFreq(uchar n, uchar m, uchar p)
{
  return (ClockRef * (65 - m) / (65 - n)) >> p;
}


//////////////////////////////////////////////////////////////////////////////
// Caculate the TI FVCO

static ulong tiCalcFvco(uchar n, uchar m) 
{
  return ClockRef * (65L - m) / (65L - n);
}


//////////////////////////////////////////////////////////////////////////////
// Caculate the TI NMP

static ulong tiCalcNMP(ulong freq, uchar *n, uchar *m, uchar *p) 
{
  ulong Fvco;
  long delta, d;
  uchar nt, mt, pt;

  // [Chris:] I haven't encountered any such dead zones, and my refresh rate
  // scan should have caught them...
  // Keep this for now anyways and live with the refresh rate holes.
  /* adjust for "dead zones" in the 3026 clock generator */
  if((freq > 55000) && (freq < 57250)) 
    {
      if(freq > 56125) freq = 57350;
      else freq = 54900;
    }
  else if((freq > 114000) && (freq < 119000)) 
    {
      if(freq > 116500) freq = 119150;
      else freq = 112500;
    }
  /* find n, m, and p such that: */
  /*  'freq' - ((14318 * (65 - m)) / (65 - n)) / (1 << p) */
  /* is as close to 0 as possible */
  delta = 250000;
  for(pt = 0; pt <= 3; pt++) 
    {
      Fvco = freq << pt;
      // Don't put an upper boundary on frequency if the post divider is 0.
      // The VCO limit is higher than 220 MHz for many cards.
      if((Fvco >= 110000) && ((Fvco <= 220000) || (pt == 0))) 
        {
          for(nt = 40; nt <= 62; nt++) 
            {
              for(mt = 1; mt <= 62; mt++) 
                {
                  d = (long)tiCalcFvco(nt, mt) - (long)Fvco;
                  /* ddprintf(("d is %d with nt: %d, mt: %d, pt: %d\n", d, nt, mt, pt)); */
                  if(d < 0) d = -d;
                  if(d < delta)
                    {
                      *n = nt;
                      *m = mt;
                      *p = pt;
                      delta = d;
                    }
                }
            }
        }
    }
  ddprintf(("tiCalcNMP requested freq %ld, actual %ld, n %d, m %d, p %d\n",
            freq, tiCalcFreq(*n, *m, *p), *n, *m, *p));
  return tiCalcFreq(*n, *m, *p);
}


//////////////////////////////////////////////////////////////////////////////
// Set the TI Memory Clock

void tiSetMemClock(ulong freq);

void tiSetMemClock(ulong freq)
{
  uchar n, m, p;
  uchar tmpByte;
  uchar pix_n, pix_m, pix_p;

  /* calculate the n, m, and p values for the desired frequency */
  tiCalcNMP(freq, &n, &m, &p);
  /* constrain values, just in case */
  n &= 0x3f;
  m &= 0x3f;
  p &= 0x03;

  /*
    since we only set the mclk during initalization, we just force
    a reasonable value for the pixel clock 
  */
  tiCalcNMP(25175, &pix_n, &pix_m, &pix_p);
        
  /* disable PCLK */
  DAC8IW(TVP3026_PLL_ADDR, 0xfe);
  DAC8IW(TVP3026_PIX_CLK_DATA, 0x00);
  /* program pixel clock with memclk values */
  tiProgramPLL(0xfc, TVP3026_PIX_CLK_DATA, n | 0xc0, m, p | 0xb0);

  /* select pixel clock PLL as dot clock source */
  tmpByte = p & 0x03;
  DAC8IW(TVP3026_PLL_ADDR, 0xfe);
  DAC8IW(TVP3026_PIX_CLK_DATA, tmpByte | 0x30);
  DAC8IW(TVP3026_PLL_ADDR, 0xfe);
  DAC8IW(TVP3026_PIX_CLK_DATA, tmpByte | 0xb0);
  /* wait for lock */
  { bigtime_t start = system_time();
  DAC8POLL(TVP3026_DATA, 0x40, 0x40)
    if ((system_time() - start) > 1000000)
      break;
  }

  /* enable dot clock */
  DAC8IR(TVP3026_MCLK_CTL, tmpByte);
  tmpByte &= 0xe7;
  DAC8(TVP3026_DATA) = tmpByte;
  DAC8(TVP3026_DATA) = tmpByte | 0x08;

  /* disable mem clock before programming */
  DAC8IW(TVP3026_PLL_ADDR, 0xfb);
  DAC8IW(TVP3026_MEM_CLK_DATA, 0x00);
  /* program mem clock */
  tiProgramPLL(0xf3, TVP3026_MEM_CLK_DATA, n | 0xc0, m, p | 0xb0);

  /* enable mem clock */
  DAC8(TVP3026_INDEX) = TVP3026_MCLK_CTL;
  /*tmpByte = DAC8(TVP3026_DATA) & 0xe7; */
  DAC8(TVP3026_DATA) = tmpByte | 0x10;
  DAC8(TVP3026_DATA) = tmpByte | 0x18;

  /* reset the pixel clock */
#if 0
  DAC8IW(TVP3026_PLL_ADDR, 0xfe);
  DAC8IW(TVP3026_PIX_CLK_DATA, 0x00);
  tiProgramPLL(0xfc, TVP3026_PIX_CLK_DATA, pix_n, pix_m, pix_p);
#endif
  DAC8IW(TVP3026_PLL_ADDR, 0xee);
  DAC8IW(TVP3026_PIX_CLK_DATA, 0x00);
  DAC8IW(TVP3026_LOAD_CLK_DATA, 0x00);
  tiProgramPLL(0xfc, TVP3026_PIX_CLK_DATA, pix_n | 0xc0, pix_m, pix_p | 0xb0);
}


//////////////////////////////////////////////////////////////////////////////
// Set the TI Pixel Clock

ulong tiSetPixClock(ulong freq, uint32 interleave, uint32 bits_per_pixel) 
{
  uchar tmpByte;
  ulong actualFreq;
  ulong z;
  uchar n, m, p, q;
        
  /* calculate the n, m, and p values for the desired frequency */
  actualFreq = tiCalcNMP(freq, &n, &m, &p);

  /* select custom clock */
  STORM8W(VGA_MISC_W, STORM8(VGA_MISC_R) | 0x0c);

  /* program pixel clock */
  DAC8IW(TVP3026_PLL_ADDR, 0xee);
  DAC8IW(TVP3026_PIX_CLK_DATA, 0x00);
  DAC8IW(TVP3026_LOAD_CLK_DATA, 0x00);
  tiProgramPLL(0xfc, TVP3026_PIX_CLK_DATA, n | 0xc0, m, p | 0xb0);
  ddprintf(("tiSetPixClock(%d) -  pixel clock programmed\n", freq));

  if(interleave) n = 65 - ((4 * 64) / bits_per_pixel);  /* 64 is the Pixel bus Width */
  else n = 65 - ((4 * 32) / bits_per_pixel);                                    /* 32 is the Pixel bus Width */

  m = 0x3d;

  /* We multiply by 100 to have a better precision */
  z = ((65L - n) * 2750L) / (actualFreq / 1000L);
        
  q = 0;
  p = 3;
  if (z <= 200)         p = 0;
  else if (z <= 400)    p = 1;
  else if (z <= 800)    p = 2;
  else if (z <=1600)    p = 3;
  else q = z/1600;
        
  if(
     ((bits_per_pixel == 16 && !interleave) ||
      (bits_per_pixel == 32 && interleave)
      ) &&
     (freq > 130000)
     ) n |= 0x80;
  else n |= 0xc0;
  p |= 0xf0;
        
  /** Program Q value for loop clock PLL **/
  DAC8W(TVP3026_INDEX, TVP3026_MCLK_CTL);
  DAC8R(TVP3026_DATA, tmpByte);
  DAC8W(TVP3026_DATA, ((tmpByte & 0xf8) | q) | 0x20); /*0x38 */

  /* program load clock */
  tiProgramPLL(0xcf, TVP3026_LOAD_CLK_DATA, n, m, p);
  ddprintf(("tiSetPixClock(%d) -  load clock programmed\n", freq));


  // Return the frequency set.
  return actualFreq;
}


//////////////////////////////////////////////////////////////////////////////
// Reset the VRAM

void resetVRAM(void)
{
  uchar tmpByte, val;
  ulong tmpulong;
  ulong mclock = calcMillenniumMCLK();

  /*** For step see spec: WRAM reset sequence ***/

  /*------------*/
  /*** Step 1 ***/
  /*------------*/
  tiSetMemClock(mclock / 1000L);        /* set to something safe */
  ddprintf(("tiSetMemClock(%d / 1000L) completed\n", mclock));

  /*------------*/
  /*** Step 2 ***/
  /*------------*/

  /*** DAT 059 At bootup the system clock is 50MHz/4 because bit gscale=0
       (gclk is divided by 4). At this point we can toggle gscale to
       increase performance (MCLOCK=40MHz) ***/

  /*----- Reset bits for vgaioen, eeprompwt, noretry, biosen -------*/
  /*----- Program OPTION FOR BIT NOGSCALE, POWERPC, INTERLEAVE -----*/
#if !defined(__POWERPC__)
  tmpulong = get_pci(STORM_OPTION, 4) & 0x1feffeff;
  tmpulong |= ai->mem_size > VIDRAM2MB ? 0x00201000 : 0x00200000;
#else
  tmpulong = get_pci(STORM_OPTION, 4) & 0x9feffeff;
  tmpulong |= ai->mem_size > VIDRAM2MB ? 0x80201000 : 0x80200000;
#endif
  set_pci(STORM_OPTION, 4, tmpulong);
  ddprintf(("OPTION configured to ->0x%0lx<-\n", tmpulong));
        
  /*------------*/
  /*** Step 3 ***/
  /*------------*/
  /*** Put mgamode to 1 : Hi-Res ***/
  STORM8W(VGA_CRTCEXT_INDEX, VGA_CRTCEXT3);
  STORM8R(VGA_CRTCEXT_DATA, tmpByte);
  tmpByte |= 0x80;
  STORM8W(VGA_CRTCEXT_DATA, tmpByte);
  ddprintf(("Hi-res mode set\n", tmpulong));

  /*------------*/
  /*** Step 4 ***/
  /*------------*/
  /* SCREEN_OFF;
     ddprintf(("screen off\n")); */

  /*------------*/
  /*** Step 5 ***/
  /*------------*/
  /***** Program CRTC registers *****/
  /* programCRTC(initCrtc); */
  /* ddprintf(("CRTC programed to default values\n")); */

  /*------------*/
  /*** Step 6 ***/
  /*------------*/
  /*----- Program OPTION FOR BIT REFRESH COUNTER -----*/

  /*tmpulong = get_pci(STORM_OPTION, 4); */
  /*ddprintf(("OPTION configured to ->0x%0lx<-\n", tmpulong)); */
    
  tmpulong &= 0xfff0ffff;
  /** At this point, we know that gscaling_factor is 1 **/
  if (si->device_id == MGA_2064W) {
    val = (uchar) ( (332L * (mclock / 1000000L) + 50) / 1280) - 1;
    /* mclks greater than 60MHz will overflow, so clamp to a sane value */
    if (val > 15) val = 15;
  } else {
    /* Mill-II */
    uint32 xx = 10000 / (mclock / 1000000);
    uint32 x512, x64;
    /* 33.2 usec >= ((x512 * 512) + (x64 * 64) + 1) * (1/mclock) * 1 */
    x512 = 0;  while (332000 >= (xx * ((512 * (x512+1)) + 1))) x512++;
    x64 = 0; if (332000 >= (xx * ((512 * x512) + (64 + 1)))) x64 = 1;
    val = (x512 << 1) + x64;
  }
  /* val = (uchar) ( (332L * (mclock / 1000000L) ) / 1280) - 1; */
  tmpulong |= ((ulong)(val & 0x0f)) << 16;
  /*tmpulong |= 0x000c0000; */
  ddprintf(("OPTION re-configured to ->0x%0lx<-\n", tmpulong));
  set_pci(STORM_OPTION, 4, tmpulong);

  /*------------*/
  /*** Step 7 ***/
  /*------------*/
  /* ddprintf(("Calling DoSoftReset()\n")); */

  /*** See DAT 007 ***/

  /* 5) Set the softreset bit */
  STORM8W(STORM_RST, 0x01);

  ddprintf(("after setting softreset bit\n"));
        
  /* 6) Wait at least 200us */
  snooze(250);

  /* 7) Clear the softreset bit */
  STORM8W(STORM_RST, 0x00);

  ddprintf(("after clearing softreset bit\n"));
        
  /* 8) Wait at least 200us so that more than 8 refresh cycle are performed */
  snooze(250);

  ddprintf(("after 250 usec delay\n"));
        
  /* 9) Wait to be in vertical retrace */
  /*mgaPollBYTE(STORM_OFFSET + VGA_INSTS1, 0x08, 0x08); */

  /*ddprintf(("after polling for vertical retrace\n")); */
        
  /* 10) Enable the video */
  /* setScreenOn(true); */
  /* ddprintf(("after enabling video\n")); */
        
  /* 11) Wait for the next vertical retrace */
  /*mgaPollBYTE(STORM_OFFSET + VGA_INSTS1,0x08,0x08); */

  /*ddprintf(("after polling for vertical retrace again\n")); */
        
  /* 12) Set the bit 'memreset' in MACCESS register */
  STORM32W(STORM_MACCESS, 0x00008000);

  ddprintf(("after setting memreset bit in MACCESS\n"));
        
  /* 13) Wait 1us */
  snooze(1);

}


//////////////////////////////////////////////////////////////////////////////
// Caculate the DAC Registers

void calcDacRegs(uint32 cs, int interleave, uint32 flags)
{
  uchar *dacregs;
  unsigned int i;

  ddprintf(("** calcDacRegs - cs: %d, interleave_mode: %d\n", cs, interleave));

  // adjust tables for pixel mode and interleave mode

  switch(cs) 
    {
    case B_CMAP8:
      dacregs = MGADACbpp8;
      dacregs[2] = 0x4c;
      break;

    case B_RGB15_LITTLE:
    case B_RGB15_BIG:
      dacregs = MGADACbpp16;
      dacregs[1] = 0x04;
      dacregs[2] = 0x54;
      break;

    case B_RGB16_LITTLE:
    case B_RGB16_BIG:
      dacregs = MGADACbpp16;
      dacregs[1] = 0x05;
      dacregs[2] = 0x54;
      break;

    case B_RGB32_LITTLE:
    case B_RGB32_BIG:
    case B_RGBA32_LITTLE:
    case B_RGBA32_BIG:
      dacregs = MGADACbpp32;
      dacregs[2] = 0x5c;
      break;

    default: // Hopefully this won't happen.
      ddprintf(("Yeek!  Bad input to calcDacRegs()! \n"));
      dacregs = NULL;
      break;
    }

  // SOG and blank pedestal
  // NOTE: This either isn't working or doesn't have the desired effect.
  // Setting the sync_on_green flag fails to produce a green screen as seen with
  // the other cards.
  dacregs[5] = 0x00; // Restore the default value.
  if (flags & B_SYNC_ON_GREEN)  dacregs[5] |= 0x20; // sync-on-green
  if (flags & B_BLANK_PEDESTAL) dacregs[5] |= 0x10; // 7.5 ire blank pedestal

  // NOTE: Not setting the sync polarity here, as the 2x64 chip will send out
  // signals with the correct polarity without help.

  ddprintf(("Performing TI DAC calculations.\n"));
  ddprintf(("  flags: %lx\n", flags));
  ddprintf(("  B_SYNC_ON_GREEN?:  %s\n", ((flags & B_SYNC_ON_GREEN) ? "YES" : "NO") ));
  ddprintf(("  B_BLANK_PEDESTAL?:  %s\n", ((flags & B_BLANK_PEDESTAL) ? "YES" : "NO") ));
  ddprintf(("TI DAC writes:\n"));

  if (!interleave) dacregs[2]--;
  /* program the registers */
  for(i = 0; i < sizeof(MGADACregs); i++) 
    {
      ddprintf(("  Register %02lx  :  %02lx\n", (uint32) MGADACregs[i], (uint32) dacregs[i]));
      DAC8W(TVP3026_INDEX, MGADACregs[i]);
      DAC8W(TVP3026_DATA, dacregs[i]);
    }
  DAC8W(TVP3026_PIX_RD_MSK, 0xff);

  // Diagnostics - try reading back to confirm.
#if 0
  ddprintf(("Reading back to confirm:\n"));
  {
    uint8 temp_byte;
    for(i = 0; i < sizeof(MGADACregs); i++)
      {
        DAC8W(TVP3026_INDEX, MGADACregs[i]);
        DAC8R(TVP3026_DATA, temp_byte);

        if (temp_byte != dacregs[i])
        {
          ddprintf(("**Register %02lx  :  %02lx**\n", (uint32) MGADACregs[i], temp_byte));
        }
        else
        {
          ddprintf(("  Register %02lx  :  %02lx\n", (uint32) MGADACregs[i], temp_byte));
        }
      }
  }
#endif
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
