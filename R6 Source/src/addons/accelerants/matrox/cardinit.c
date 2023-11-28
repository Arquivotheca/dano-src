//////////////////////////////////////////////////////////////////////////////
// Card-specific Initialization Routines
//    This is a routing file.  It's a little questionable at the moment,
// and I'm half inclined to make it a big switch.  Or more accurately, use
// the switch that is used to call these functions and drag it into a single
// function here.
//
// Device Dependance: Moderate
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Includes //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include <Accelerant.h>
#include <Drivers.h>

#include <registers.h>
#include <private.h>
#include "globals.h"
#include "mga_gx00.h"
#include "mga_millennium.h"
#include "mga_mystique.h"
#include "mga_util.h"
#include "cardinit.h"

//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// 1064 Initialization Routine (Mystique)
//    HACK - for now, just call the old initialization routine.

void Init_1064(void)
{
  powerUpMystique();
}


//////////////////////////////////////////////////////////////////////////////
// 2064 Initialization Routine (Millennium I)
//    HACK - for now, just use the old initialization method.

void Init_2064(void)
{
  // The global variable "regs" is required for the register write macros, but
  // it should already have been set appropriately by the calling function.

  // reset the DAC
  DAC8IW(TVP3026_RESET, 0);
  snooze(1000);
  countRAM();   /* initial count, but mclck may not be on */
  resetVRAM();  /* reset RAM, program controler */
  countRAM();   /* re-count RAM, as we've got a working controler */
  resetVRAM();  /* reprogram controler for the real RAM size */
}


//////////////////////////////////////////////////////////////////////////////
// 2164 Initialization Routine (Millennium II)
//    HACK - for now, just use the old initialization method.

void Init_2164(void)
{
  // The global variable "regs" is required for the register write macros, but
  // it should already have been set appropriately by the calling function.

  // reset the DAC
  DAC8IW(TVP3026_RESET, 0);
  snooze(1000);
  countRAM();   /* initial count, but mclck may not be on */
  resetVRAM();  /* reset RAM, program controler */
  countRAM();   /* re-count RAM, as we've got a working controler */
  resetVRAM();  /* reprogram controler for the real RAM size */
}


//////////////////////////////////////////////////////////////////////////////
// G100 Initialization Routine
//    HACK - for now, just call the old initialization routine.

void Init_G100(void)
{
  powerUpGx00();
}


//////////////////////////////////////////////////////////////////////////////
// G200 Initialization Routine
//    HACK - for now, just call the old initialization routine.

void Init_G200(void)
{
  powerUpGx00();
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
