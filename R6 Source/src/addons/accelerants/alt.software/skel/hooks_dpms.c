//////////////////////////////////////////////////////////////////////////////
// DPMS - Power management routines.
//    These functions alter the power state of the graphics card.
//
//////////////////////////////////////////////////////////////////////////////
//    NOTE: This is just a set of skeleton routines that tells the caller
// that we don't support power management (or rather that we do, but for only
// one power management state).
//
//    HACK - Give definitions of the power management modes here.
//
//    NOTE: I've *seen* proper definitions of what each of these modes is
// supposed to do. However, they seem to have vanished into the Abyss.
// Ideally they'd be posted somewhere in this file for the benefit of anyone
// implementing proper power management.

//////////////////////////////////////////////////////////////////////////////
// Includes //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include <common_includes.h>
#include <accel_includes.h>


//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// DPMS Capabilities
//    This returns a bit vector indicating which power management modes we
// support. Constants are B_DPMS_ON, B_DPMS_STAND_BY, B_DPMS_SUSPEND, and
// B_DPMS_OFF.

uint32 DPMS_Capabilities(void)
{
  // We can turn the device on, and that's it.
  // We actually don't even do this much, as it's already on.
  return B_DPMS_ON;
}


//////////////////////////////////////////////////////////////////////////////
// DPMS Mode
//    This reads the current power management mode.

uint32 DPMS_Mode(void)
{
  // Return the current power management mode. For the skeleton, this is
  // pretty easy.
  return B_DPMS_ON;
}


//////////////////////////////////////////////////////////////////////////////
// Set DPMS Mode
//    This sets the current power management mode.

status_t SetDPMS_Mode(uint32 dpms_flags)
{
  // If we're asked to turn the device on, report success. Otherwise, report
  // failure.

  switch(dpms_flags)
    {
    case B_DPMS_ON:
      // Do nothing; the card is already in the "on" state.
      break;

    case B_DPMS_STAND_BY:
    case B_DPMS_SUSPEND:
    case B_DPMS_OFF:
      // Not supported; fall through to the error case.

    default:
      // We've been asked to set an invalid power management mode; return an
      // error code.
      return B_ERROR;
    }

  // If we've reached here, we were successful.
  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
