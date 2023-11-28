//////////////////////////////////////////////////////////////////////////////
// DPMS
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Includes //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include <Accelerant.h>
#include <Drivers.h>
#include <registers.h>
#include <private.h>

#include "defines.h"
#include "globals.h"
#include "hooks_dpms.h"


//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// DPMS Capabilities

uint32 DPMS_Capabilities(void)
{
  return (B_DPMS_ON       |
          B_DPMS_STAND_BY |
          B_DPMS_SUSPEND  |
          B_DPMS_OFF      );
}


//////////////////////////////////////////////////////////////////////////////
// DPMS Mode

uint32 DPMS_Mode(void)
{
  uchar tmpByte;

  STORM8W(VGA_CRTCEXT_INDEX, 1);
  STORM8R(VGA_CRTCEXT_DATA, tmpByte);

  switch((tmpByte & 0x30) >> 4)  // what modes are set?
    {
    case 0:  return B_DPMS_ON;       break; // H:  on, V:  on
    case 1:  return B_DPMS_STAND_BY; break; // H: off, V:  on
    case 2:  return B_DPMS_SUSPEND;  break; // H:  on, V: off
    case 3:  return B_DPMS_OFF;      break; // H: off, V: off
    }
        
  return B_DPMS_ON; // Fallback.
}


//////////////////////////////////////////////////////////////////////////////
// Set DPMS Mode

status_t SetDPMS_Mode(uint32 dpms_flags)
{
  uchar tmpByte;

  STORM8W(VGA_CRTCEXT_INDEX, 1);
  STORM8R(VGA_CRTCEXT_DATA, tmpByte);

  tmpByte &= 0xcf; // clear the syncoff bits

  switch(dpms_flags)
    {
    case B_DPMS_ON:       // H:  on, V:  on
      STORM8W(VGA_CRTCEXT_DATA, tmpByte); // Bits already clear.
      SCREEN_ON;
      break;

    case B_DPMS_STAND_BY: // H: off, V:  on
      tmpByte |= 0x10;
      STORM8W(VGA_CRTCEXT_DATA, tmpByte);
      SCREEN_OFF;
      break;

    case B_DPMS_SUSPEND:  // H:  on, V: off
      tmpByte |= 0x20;
      STORM8W(VGA_CRTCEXT_DATA, tmpByte);
      SCREEN_OFF;
      break;

    case B_DPMS_OFF:      // H: off, V: off
      tmpByte |= 0x30;
      STORM8W(VGA_CRTCEXT_DATA, tmpByte);
      SCREEN_OFF;
      break;

    default:
      return B_ERROR;
      break;
    }

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
