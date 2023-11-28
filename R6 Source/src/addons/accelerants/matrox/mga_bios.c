//////////////////////////////////////////////////////////////////////////////
// BIOS Structures
//    This isn't really part of the driver yet.  Things like si->revision
// are of dubious validity, and so I don't know if they can be counted on
// at all.  At present, there is no attempt made to compile this file into
// the driver at all.
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Includes //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include <SupportDefs.h>
#include "mga_bios.h"


//////////////////////////////////////////////////////////////////////////////
// Defines ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// MAXREV
//    This is a quick hack that gives us a maximum revision number for
// hardware.  I'm assuming that nobody will rev a card more than this
// number of times without changing the device id.  Maybe I'm wrong...

enum
{
  MAXREV = 0xDEAD
};


//////////////////////////////////////////////////////////////////////////////
// Globals ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Card Static Data
//    This is a table with some information about the cards that this driver
// supports.  

CARD_STATIC_DATA CardStaticData =
{
  { MGA_1064S,          2, "Matrox Mystique",               "MGA 1064 PCI" },
  { MGA_1064S,     MAXREV, "Matrox Mystique 220",           "MGA 1064 PCI" },
  { MGA_2064W,     MAXREV, "Matrox Millennium (original)" , "MGA 2064 PCI" },
  { MGA_2164W,     MAXREV, "Matrox Millennium-II PCI",      "MGA 2164 PCI" },
  { MGA_2164W_AGP, MAXREV, "Matrox Millennium-II AGP",      "MGA 2164 AGP" },
  { MGA_G100_PCI,  MAXREV, "Matrox G100 PCI",               "MGA G100 PCI" },
  { MGA_G100_AGP,  MAXREV, "Matrox Productiva G100",        "MGA G100 AGP" },
  { MGA_G200_PCI,  MAXREV, "Matrox G200 PCI",               "MGA G200 PCI" },
  { MGA_G200_AGP,  MAXREV, "Matrox G200 based card",        "MGA G200 AGP" },
  { 0,             MAXREV, "Whoa.  Don't know this one."    "Unknown."     }
}


//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Get Card ID
//    This function generates an identifier number for the card which can
// then be used to look up card data in the corresponding structures.  If
// you get a negative number out of the call, it can't find a coresponding
// card.  This would be a bad thing.

int32 GetCardID(void)
{
  while(CardStaticData[index].DeviceID)
    {
      if((CardStaticData[index].DeviceID == si->device_id) &&
	 (CardStaticData[index].Revision <= si->revision))
	{
	  return index;
	}

      index++;
    }

  return -1;
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////