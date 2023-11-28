//////////////////////////////////////////////////////////////////////////////
// Utility Functions
//
//    This file implements utility functions used by the skeleton accelerant.
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Includes //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include <common_includes.h>
#include <accel_includes.h>


//////////////////////////////////////////////////////////////////////////////
// Local Typedefs ////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct
{
  uint32 space;
  char *name;
} SPACE_NAME_TABLE_ROW;


//////////////////////////////////////////////////////////////////////////////
// Local Constants ///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// Terminated by a NULL name field.
SPACE_NAME_TABLE_ROW SpaceNameTable[] =
  {
    { B_RGB32_BIG, "B_RGB32_BIG" },
    { B_RGBA32_BIG, "B_RGBA32_BIG" },
    { B_RGB32_LITTLE, "B_RGB32_LITTLE" },
    { B_RGBA32_LITTLE, "B_RGBA32_LITTLE" },

    { B_RGB24_BIG, "B_RGB24_BIG" },
    { B_RGB24_LITTLE, "B_RGB24_LITTLE" },

    { B_RGB16_BIG, "B_RGB16_BIG" },
    { B_RGB15_BIG, "B_RGB15_BIG" },
    { B_RGBA15_BIG, "B_RGBA15_BIG" },
    { B_RGB16_LITTLE, "B_RGB16_LITTLE" },
    { B_RGB15_LITTLE, "B_RGB15_LITTLE" },
    { B_RGBA15_LITTLE, "B_RGBA15_LITTLE" },

    { B_CMAP8, "B_CMAP8" },

    // End of list.
    { 0x0, NULL }
  };

char *UnknownSpaceStr = "[unknown color space]";


//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Calculate Storage Bits Per Pixel
//    Utility function to calculate the number of storage bits per pixel
// given a colour space.

uint32 CalcStorageBPP(uint32 cs)
{
  switch(cs)
    {
    case B_RGB32_BIG:
    case B_RGBA32_BIG:
    case B_RGB32_LITTLE:
    case B_RGBA32_LITTLE:
      return 32;
      break;

    case B_RGB24_BIG:
    case B_RGB24_LITTLE:
      return 24;
      break;

    case B_RGB16_BIG:
    case B_RGB15_BIG:
    case B_RGBA15_BIG:
    case B_RGB16_LITTLE:
    case B_RGB15_LITTLE:
    case B_RGBA15_LITTLE:
      return 16;
      break;

    case B_CMAP8:
      return 8;
      break;
    }

  return 0;
}


//////////////////////////////////////////////////////////////////////////////
// Calculate Color Depth Bits Per Pixel
//    Utility function to calculate the number of color depth bits per pixel
// given a colour space.
//    NOTE: We're ignoring alpha bits for the moment.

uint32 CalcColorDepth(uint32 cs)
{
  switch(cs)
    {
    case B_RGB32_BIG:
    case B_RGBA32_BIG:
    case B_RGB32_LITTLE:
    case B_RGBA32_LITTLE:
      return 32;
      break;

    case B_RGB24_BIG:
    case B_RGB24_LITTLE:
      return 24;
      break;

    case B_RGB16_BIG:
    case B_RGB16_LITTLE:
      return 16;
      break;

    case B_RGB15_BIG:
    case B_RGBA15_BIG:
    case B_RGB15_LITTLE:
    case B_RGBA15_LITTLE:
      return 15;
      break;

    case B_CMAP8:
      return 8;
      break;
    }

  return 0;
}


//////////////////////////////////////////////////////////////////////////////
// Name of Colour Space
//    Utility function to provide a name string for the given colour space.
// This string is a pointer to an entry in a static array.

char *NameOfSpace(uint32 cs)
{
  int index;

  // Scan through the list of known spaces for an entry corresponding to
  // the specified space.
  index = 0;
  while ((SpaceNameTable[index].name != NULL)
    && (SpaceNameTable[index].space != cs))
    index++;

  // If we found an entry for this space, return the appropriate name
  // string.
  if (SpaceNameTable[index].name != NULL)
    return SpaceNameTable[index].name;

  // We didn't find an entry for this space in the table, so return a
  // pointer to the "unknown space" string.
  return UnknownSpaceStr;
}


//////////////////////////////////////////////////////////////////////////////
// Count RAM
//    This function determines the amount of usable memory in the specified
// aperture, by writing to the apetrure and reading back in a pattern
// designed to detect most types of aliasing and artifacting.

uint32 CountMemory(void *Aperture, uint32 ApertureSize)
{
  // Allowed sizes go up to 32 megabytes. Our tests are limited to the
  // specified aperture size, so we shouldn't be stomping anything in
  // unmapped memory.
	const uint32 AllowedSizes[] =
	{ 0x00080000, 0x00100000, 0x00180000, 0x00200000,
	  0x00280000, 0x00300000, 0x00380000, 0x00400000,
	  0x00500000, 0x00600000, 0x00700000, 0x00800000,
	  0x00A00000, 0x00C00000, 0x00E00000, 0x01000000,
    0x01400000, 0x01800000, 0x01C00000, 0x02000000,
	  0x0 };

  volatile uint32 *FBPointer;
	uint32 MaxMem;
	uint32 RWIndex;
	int iMaxIndex, iTestIndex, iX;
	uint32 LTemp;
	int IsOk;


  // Initialize our local frame buffer pointer.
  FBPointer = (uint32 *) Aperture;

  // Perform the memory test.

	MaxMem = 0; // Default.
	IsOk = 1;
	// Step through ever-larger memory sizes, recording size if passes test and
	// ignoring otherwise.
	for (iMaxIndex = 0;
    (AllowedSizes[iMaxIndex] != 0)
    && (AllowedSizes[iMaxIndex] <= ApertureSize)
    && IsOk;
    iMaxIndex++)
	{
		// Write test values to the frame buffer.
		// Only need to do this for the farthest location, as previous locations
		// already have been written to in previous passes.
		RWIndex = AllowedSizes[iMaxIndex];
		RWIndex = (RWIndex - 16384) >> 2;
		for (iX = 0; iX < 4096; iX++)
		{
			LTemp = RWIndex;
			// Hash LTemp. As the parameters for the hash are prime, it should
			// be extremely unlikely to get these values through a glitch, and
			// the pattern only repeats at prime intervals, so aliasing shouldn't
			// fool the test either.
			LTemp = (263 * (LTemp % 65521) + 29) % 65521;
			// Extend this to 32 bits.
			LTemp |= (LTemp ^ 0x0000FFFFul) << 16;
			FBPointer[RWIndex] = LTemp;
			RWIndex++;
		}


    // HACK - Not flushing the card's memory cache (if any).


		// Verify that all test patterns are still intact. If values written past the
		// end of memory drop off the face of the frame buffer, the farthest pattern(s)
		// will not be what they should be. If values written past the end of memory
		// wrap, then previous patterns will be overwritten.
		// As soon as an invalid value is detected, IsOk is set to 0, which should
		// quickly terminate the test loops.
		for (iTestIndex = 0; (iTestIndex <= iMaxIndex) && IsOk; iTestIndex++)
		{
			RWIndex = AllowedSizes[iTestIndex];
			RWIndex = (RWIndex - 16384) >> 2;
			for (iX = 0; (iX < 4096) && IsOk; iX++)
			{
				LTemp = RWIndex;
				// Hash LTemp. As the parameters for the hash are prime, it should
				// be extremely unlikely to get these values through a glitch, and
				// the pattern only repeats at prime intervals, so aliasing shouldn't
				// fool the test either.
				LTemp = (263 * (LTemp % 65521) + 29) % 65521;
				// Extend this to 32 bits.
				LTemp |= (LTemp ^ 0x0000FFFFul) << 16;
				// Test against the value read from the frame buffer.
				if (FBPointer[RWIndex] != LTemp)
					IsOk = 0;
				RWIndex++;
			}
		}

		// If the test patterns check out, update MaxMem accordingly.
		if (IsOk)
			MaxMem = AllowedSizes[iMaxIndex];
	}

  // Diagnostics.
  ddprintf(("[CountMemory] Aperture memory detected: %lu bytes\n", MaxMem));

  // Return the resulting memory count.
  return MaxMem;
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
