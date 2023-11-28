//////////////////////////////////////////////////////////////////////////////
// Utilities
//    Some utility functions.  I'm going to have to check the organization
// here; there may need to be some changes for clarity.
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Includes //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//   Unfortunately, under -pedantic -Wall one of the system headers makes
// some bad noise.  The file: /boot/develop/headers/posix/bits/confname.h
// turns out to have a couple of enums that end in commas.  Anyone care to
// explain why the POSIX headers aren't ANSI?  Gah.

#include <ByteOrder.h>
#include <Accelerant.h>
#include <Drivers.h>
#include <string.h>
#include <malloc.h>
#include <registers.h>
#include <cardid.h>
#include <private.h>

#include "defines.h"
#include "debugprint.h"
#include "mga_bios.h"
#include "accelerant_info.h"
#include "globals.h"
#include "cardinit.h"
#include "mga_util.h"



//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Get PCI

uint32 get_pci(uint32 offset, uint32 size)
{
  MGA_GET_SET_PCI gsp;

  gsp.magic = MGA_PRIVATE_DATA_MAGIC;
  gsp.offset = offset;
  gsp.size = size;
  ioctl(fd, MGA_GET_PCI, &gsp, sizeof (gsp));
  return gsp.value;
}


//////////////////////////////////////////////////////////////////////////////
// Set PCI

void set_pci(uint32 offset, uint32 size, uint32 value)
{
  MGA_GET_SET_PCI gsp;

  gsp.magic = MGA_PRIVATE_DATA_MAGIC;
  gsp.offset = offset;
  gsp.size = size;
  gsp.value = value;
  ioctl(fd, MGA_SET_PCI, &gsp, sizeof (gsp));
}


//////////////////////////////////////////////////////////////////////////////
// Read the BIOS

status_t readBIOS(void)
{
  uchar    *p;
  status_t result = B_OK;

  // Initialize bios-derived fields to safe default values.
  ai->block_mode_ok = false; // SGRAM/WRAM block mode operations are *not* available.

  // locate bios signature at mapped location (in virtual address space)

  p = si->framebuffer;

        
  if (si->device_id == MGA_2064W)
    {
      if((p[0] == 0x55) && 
         (p[1] == 0xaa) &&
         (strncmp((const char *)p+45, "MATROX", 6) == 0))  // BIOS located.
        {
          ushort offset = B_LENDIAN_TO_HOST_INT16(*(ushort*)(p + 0x7ffc));
          // MGABiosInfo *b = (MGABiosInfo *)(p+offset);
          BIOS_2064 *b = (BIOS_2064 *)(p+offset);

          ai->bios.orig.StructLen = B_LENDIAN_TO_HOST_INT16(b->StructLen);
          ai->bios.orig.ProductID = B_LENDIAN_TO_HOST_INT16(b->ProductID);

          // memcpy(bi.bios.SerNo, b->SerNo, 10);
          {
            // Ick.

            int i;
            unsigned char *s;
            unsigned char *d;

            d = ai->bios.orig.SerNo;
            s = b->SerNo;
            for(i = 0; i < 10; i++) *d++ = *s++;
          }

          ai->bios.orig.ManufDate = B_LENDIAN_TO_HOST_INT16(b->ManufDate);
          ai->bios.orig.ManufId = B_LENDIAN_TO_HOST_INT16(b->ManufId);
          ai->bios.orig.PCBInfo = B_LENDIAN_TO_HOST_INT16(b->PCBInfo);
          ai->bios.orig.PMBInfo = B_LENDIAN_TO_HOST_INT16(b->PMBInfo);
          ai->bios.orig.RamdacType = B_LENDIAN_TO_HOST_INT16(b->RamdacType);
          ai->bios.orig.PclkMax = B_LENDIAN_TO_HOST_INT16(b->PclkMax);
          ai->bios.orig.LclkMax = B_LENDIAN_TO_HOST_INT16(b->LclkMax);
          ai->bios.orig.ClkBase = B_LENDIAN_TO_HOST_INT16(b->ClkBase);
          ai->bios.orig.Clk4MB = B_LENDIAN_TO_HOST_INT16(b->Clk4MB);
          ai->bios.orig.Clk8MB = B_LENDIAN_TO_HOST_INT16(b->Clk8MB);
          ai->bios.orig.ClkMod = B_LENDIAN_TO_HOST_INT16(b->ClkMod);
          ai->bios.orig.TestClk = B_LENDIAN_TO_HOST_INT16(b->TestClk);
          ai->bios.orig.VGAFreq1 = B_LENDIAN_TO_HOST_INT16(b->VGAFreq1);
          ai->bios.orig.VGAFreq2 = B_LENDIAN_TO_HOST_INT16(b->VGAFreq2);
          ai->bios.orig.ProgramDate = B_LENDIAN_TO_HOST_INT16(b->ProgramDate);
          ai->bios.orig.ProgramCnt = B_LENDIAN_TO_HOST_INT16(b->ProgramCnt);
          ai->bios.orig.Options = B_LENDIAN_TO_HOST_INT32(b->Options);
          ai->bios.orig.FeatFlag = B_LENDIAN_TO_HOST_INT32(b->FeatFlag);
          ai->bios.orig.VGAClk = B_LENDIAN_TO_HOST_INT16(b->VGAClk);
          ai->bios.orig.StructRev = B_LENDIAN_TO_HOST_INT16(b->StructRev);
        }
      else // No BIOS found, use defaults.
        {
          ai->bios.orig.StructLen = 64;
          ai->bios.orig.ClkBase = 5000;
          ai->bios.orig.Clk4MB  = 5000;
          ai->bios.orig.Clk8MB  = 5000;
          ai->bios.orig.RamdacType = 0;
          ai->bios.orig.ProductID  = 0;
          ai->bios.orig.VGAClk = 5000;
          ai->bios.orig.FeatFlag = 0x00000001;
        }

      // Set BIOS-derived information.

      // All 2x64 cards use WRAM.
      ai->block_mode_ok = true;
    }
  else  // Mystique, Millennium-II, Gx00 cards
    {
      uint16 *pin = (uint16 *)p;
      int count = 32768;

      // look for PInS structure

      while(count) // while there is space left to check...
        {
          // if the PIN marker is found

#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
#define PInSMAGIC 0x412e
#else
#define PInSMAGIC 0x2e41
#endif

          if(*pin == (uint16)PInSMAGIC)
            {
              p = (uchar *)(pin + 1);

              // if the size of the structure is correct, bail as found.

              if (*p == 0x40) break;
            }

          pin++;
          count--;
        }

      if(count && (*p == 0x40)) // If quit before finished and size matches...
        {
          // Verify the checksum?
                        
          switch(si->device_id)
            {
            case MGA_1064S:
            case MGA_2164W:
            case MGA_2164W_AGP:
              {
                BIOS_1064 *pm = (BIOS_1064 *)pin;

                // fill in the structure
                ai->bios.pins = *pm;

                // swap the non-byte fields
                ai->bios.pins.PinID = B_LENDIAN_TO_HOST_INT16(ai->bios.pins.PinID);
                ai->bios.pins.StructRev = B_LENDIAN_TO_HOST_INT16(ai->bios.pins.StructRev);
                ai->bios.pins.ProgramDate = B_LENDIAN_TO_HOST_INT16(ai->bios.pins.ProgramDate);
                ai->bios.pins.ProgramCnt = B_LENDIAN_TO_HOST_INT16(ai->bios.pins.ProgramCnt);
                ai->bios.pins.ProductID = B_LENDIAN_TO_HOST_INT16(ai->bios.pins.ProductID);
                ai->bios.pins.PCBInfo = B_LENDIAN_TO_HOST_INT16(ai->bios.pins.PCBInfo);
                ai->bios.pins.FeatFlag = B_LENDIAN_TO_HOST_INT32(ai->bios.pins.FeatFlag);
              }
              break;

            case MGA_G100_AGP:
            case MGA_G200_AGP:
            case MGA_G100_PCI:
            case MGA_G200_PCI:
              // fill in the structure
              // ai->bios.pins3 = *((pins31 *)pin);
              ai->bios.pins31 = *((BIOS_G100 *)pin);

              // swap the non-byte fields
              ai->bios.pins31.PinID
                = B_LENDIAN_TO_HOST_INT16(ai->bios.pins31.PinID);

              ai->bios.pins31.StructRev
                = B_LENDIAN_TO_HOST_INT16(ai->bios.pins31.StructRev);

              ai->bios.pins31.ProgramDate
                = B_LENDIAN_TO_HOST_INT16(ai->bios.pins31.ProgramDate);

              ai->bios.pins31.ProgramCnt
                = B_LENDIAN_TO_HOST_INT16(ai->bios.pins31.ProgramCnt);

              ai->bios.pins31.ProductID
                = B_LENDIAN_TO_HOST_INT16(ai->bios.pins31.ProductID);

              ai->bios.pins31.PCBInfo
                = B_LENDIAN_TO_HOST_INT16(ai->bios.pins31.PCBInfo);

              ai->bios.pins31.MCTLWTST
                = B_LENDIAN_TO_HOST_INT32(ai->bios.pins31.MCTLWTST);

              break;
            }
        }
      else
        {
          // use defaults

          memset(&ai->bios.pins, 0xff, sizeof(ai->bios.pins));
          switch (si->device_id)
            {
            case MGA_1064S:
            case MGA_2164W:
            case MGA_2164W_AGP:
              ai->bios.pins.VidCtrl &= 0xCE;
              break;

            case MGA_G100_AGP:
            case MGA_G200_AGP:
            case MGA_G100_PCI:
            case MGA_G200_PCI:
              result = B_ERROR;
              ai->bios.pins31.VidCtrl = 0xEE;
              ai->bios.pins31.HiResGClk = 63;

              ai->bios.pins31.MCTLWTST 
                = (si->device_id == MGA_G100_AGP ? 0x02032521 : 0x00244CA1);

              break;
            }

          ddprintf(("Oops - No PInS data found!\n"));
        }


      // Set BIOS-derived information.

      switch (si->device_id)
        {
        case MGA_1064S:
          // SDRAM/SGRAM selection is determined by the product ID bits in the
          // "option" register. Bit 0x01000000 is set if S_G_RAM is present.
          if (get_pci(STORM_OPTION, 4) & 0x01000000)
            ai->block_mode_ok = true;
          break;

        case MGA_2164W:
        case MGA_2164W_AGP:
          // All 2x64 cards use WRAM.
          ai->block_mode_ok = true;
          break;

        case MGA_G100_AGP:
        case MGA_G200_AGP:
        case MGA_G100_PCI:
        case MGA_G200_PCI:
          // Check ClkDiv to see if SGRAM is installed.
          // ClkDiv & 0x10 is set if S_D_RAM is being used.
          if (!(ai->bios.pins31.ClkDiv & 0x10))
            ai->block_mode_ok = true;
          break;
        }
    }

  return result;
}


//////////////////////////////////////////////////////////////////////////////
// Count RAM

void countRAM(void)
{
  vuchar tmpByte;
  ulong memCount;
  uchar *frame = si->framebuffer;

  ulong window_size = (((si->device_id == MGA_2164W) || 
                        (si->device_id == MGA_2164W_AGP)) 
                       ? 0x1000000 : 0x800000);

  uchar *window = frame + window_size;

  // make sure the card is in MGA mode (to access all of the frame buffer)

  STORM8W(VGA_CRTCEXT_INDEX, VGA_CRTCEXT3);
  STORM8R(VGA_CRTCEXT_DATA, tmpByte);
  tmpByte |= 0x80;
  STORM8W(VGA_CRTCEXT_DATA, tmpByte);
        
  tmpByte = 0x22;
  for (frame = (uchar *)si->framebuffer + VIDRAM2MB - 1;
       frame < window;
       frame += VIDRAM2MB)
    {
      *frame = tmpByte;
      tmpByte += 0x22;
    }

  // flush the cache
  MGA8(0x1fff) = 0;  // should be innocuous for the Mill-I
        
  // figure out how much memory

  tmpByte = 0x22;
  memCount = 0;

  for(frame = (uchar *)si->framebuffer + VIDRAM2MB - 1;
      frame < window;
      frame += VIDRAM2MB)
    {
      if (*frame == tmpByte)
        {
          tmpByte += 0x22;
          memCount += VIDRAM2MB;
        }
      else
        {
          break;
        }
    }

  // record our results

  ai->mem_size = memCount;
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
