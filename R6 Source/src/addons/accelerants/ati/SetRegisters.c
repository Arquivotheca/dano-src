///////////////////////////////////////////////////////////////////////////////
// Set Registers
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Includes ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "private.h"
#include "SetRegisters.h"


///////////////////////////////////////////////////////////////////////////////
// Functions //////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint32 SetRegisters(REGSET_STRUCT *Table)
{
  uint32 count = 0;

  uint32 scratch32;
  volatile uint32 *pointer32;

  volatile uint32 delay;


  // Critical section - accessing graphics card registers.
  //lock_card();


  for (count = 0; Table[count].Flags != REGSET_FENCE; count++)
    {
      switch(Table[count].Flags)
        {
        case REGSET_WRITE:
          pointer32  = (regs + Table[count].Register);
          *pointer32 = Table[count].Value[0];
          break;

        case REGSET_RMW:
          pointer32   = (regs + Table[count].Register);
          scratch32   = *pointer32;
          scratch32  &= Table[count].Value[1];
          scratch32  |= Table[count].Value[0];

		  for(delay = 0; delay < 20000; delay++); // Wait a lot.
          *pointer32  = scratch32;
          break;

        default: // Wierd error.  Just return.
		  // End critical section.
		  //unlock_card();
          return 0;
          break;
        }

      //   This is a mindless little test delay loop.  It's here in case
      // there are some timing dependancies on the chip setup.  In future
      // (if it's needed at all) we'll be replacing this with something a
      // little less stupid.  Like, say, something that has a delay that
      // doesn't vary with processor speed.

      for(delay = 0; delay < 20000; delay++);
    }


  // End critical section.
  //unlock_card();


  return 1;
}


///////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
