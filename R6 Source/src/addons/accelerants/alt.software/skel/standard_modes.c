//////////////////////////////////////////////////////////////////////////////
// Standard BeOS Display Modes
//    This file contains the instantiations for display_mode values for the
// standard modes used under BeOS, extended to cover additional refresh
// rates.
//
// Device Dependance: None.
//
//////////////////////////////////////////////////////////////////////////////

//
// Includes.
//

#include <common_includes.h>
#include <accel_includes.h>


//
// Constants.
//

// VESA Standard Modes

// This holds display_mode structures for VESA standard modes. An entry with all
// zero values terminates the list.

const display_mode vesa_standard_modes[] =
{
  {
    {
      25175,
      640,  656,  752,  800,
      480,  490,  492,  525,
      0
    },
    B_CMAP8,
    640,  480,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // Vesa_Monitor_@60Hz_(640X480X8.Z1)

  {
    {
      27500,
      640,  672,  768,  864,
      480,  488,  494,  530,
      0
    },
    B_CMAP8,
    640,  480,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // 640X480X60Hz

  {
    {
      30500,
      640,  672,  768,  864,
      480,  517,  523,  588,
      0
    },
    B_CMAP8,
    640,  480,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // SVGA_640X480X60HzNI

  {
    {
      31500,
      640,  664,  704,  832,
      480,  489,  492,  520,
      0
    },
    B_CMAP8,
    640,  480,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // Vesa_Monitor_@70-72Hz_(640X480X8.Z1)

  {
    {
      31500,
      640,  656,  720,  840,
      480,  481,  484,  500,
      0
    },
    B_CMAP8,
    640,  480,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // Vesa_Monitor_@75Hz_(640X480X8.Z1)

  {
    {
      36000,
      640,  696,  752,  832,
      480,  481,  484,  509,
      0
    },
    B_CMAP8,
    640,  480,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // Vesa_Monitor_@85Hz_(640X480X8.Z1)

  {
    {
      38100,
      800,  832,  960, 1088,
      600,  602,  606,  620,
      0
    },
    B_CMAP8,
    800,  600,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // SVGA_800X600X56HzNI

  {
    {
      40000,
      800,  840,  968, 1056,
      600,  601,  605,  628,
      B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
    },
    B_CMAP8,
    800,  600,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // Vesa_Monitor_@60Hz_(800X600X8.Z1)

  {
    {
      49500,
      800,  816,  896, 1056,
      600,  601,  604,  625,
      B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
    },
    B_CMAP8,
    800,  600,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // Vesa_Monitor_@75Hz_(800X600X8.Z1)

  {
    {
      50000,
      800,  856,  976, 1040,
      600,  637,  643,  666,
      B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
    },
    B_CMAP8,
    800,  600,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // Vesa_Monitor_@70-72Hz_(800X600X8.Z1)

  {
    {
      56250,
      800,  832,  896, 1048,
      600,  601,  604,  631,
      B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
    },
    B_CMAP8,
    800,  600,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // Vesa_Monitor_@85Hz_(800X600X8.Z1)

  {
    {
      46600,
      1024, 1088, 1216, 1312,
      384,  385,  388,  404,
      B_TIMING_INTERLACED
    },
    B_CMAP8,
    1024,  768,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // SVGA_1024X768X43HzI

  {
    {
      65000,
      1024, 1048, 1184, 1344,
      768,  771,  777,  806,
      0
    },
    B_CMAP8,
    1024,  768,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // Vesa_Monitor_@60Hz_(1024X768X8.Z1)

  {
    {
      75000,
      1024, 1048, 1184, 1328,
      768,  771,  777,  806,
      0
    },
    B_CMAP8,
    1024,  768,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // Vesa_Monitor_@70-72Hz_(1024X768X8.Z1)

  {
    {
      78750,
      1024, 1040, 1136, 1312,
      768,  769,  772,  800,
      B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
    },
    B_CMAP8,
    1024,  768,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // Vesa_Monitor_@75Hz_(1024X768X8.Z1)

  {
    {
      94500,
      1024, 1072, 1168, 1376,
      768,  769,  772,  808,
      B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
    },
    B_CMAP8,
    1024,  768,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // Vesa_Monitor_@85Hz_(1024X768X8.Z1)

  {
    {
      94200,
      1152, 1184, 1280, 1472,
      900,  901,  904,  950,
      B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
    },
    B_CMAP8,
    1152,  864,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // modified Vesa_Monitor_@70Hz_(1152X864X8.Z1)

  {
    {
      108000,
      1152, 1216, 1344, 1600,
      900,  901,  904,  936,
      B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
    },
    B_CMAP8,
    1152,  864,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // modified Vesa_Monitor_@75Hz_(1152X864X8.Z1)

  {
    {
      121500,
      1152, 1216, 1344, 1568,
      900,  901,  904,  947,
      B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
    },
    B_CMAP8,
    1152,  864,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // modified Vesa_Monitor_@85Hz_(1152X864X8.Z1)

  {
    {
      108000,
      1280, 1328, 1440, 1688,
      1024, 1025, 1028, 1066,
      B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
    },
    B_CMAP8,
    1280, 1024,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // Vesa_Monitor_@60Hz_(1280X1024X8.Z1)

  {
    {
      135000,
      1280, 1296, 1440, 1688,
      1024, 1025, 1028, 1066,
      B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
    },
    B_CMAP8,
    1280, 1024,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // Vesa_Monitor_@75Hz_(1280X1024X8.Z1)

  {
    {
      157500,
      1280, 1344, 1504, 1728,
      1024, 1025, 1028, 1072,
      B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
    },
    B_CMAP8,
    1280, 1024,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // Vesa_Monitor_@85Hz_(1280X1024X8.Z1)

  {
    {
      162000,
      1600, 1664, 1856, 2160,
      1200, 1201, 1204, 1250,
      B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
    },
    B_CMAP8,
    1600, 1200,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // Vesa_Monitor_@60Hz_(1600X1200X8.Z1)

  {
    {
      175500,
      1600, 1664, 1856, 2160,
      1200, 1201, 1204, 1250,
      B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
    },
    B_CMAP8,
    1600, 1200,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // Vesa_Monitor_@65Hz_(1600X1200X8.Z1)

  {
    {
      189000,
      1600, 1664, 1856, 2160,
      1200, 1201, 1204, 1250,
      B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
    },
    B_CMAP8,
    1600, 1200,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // Vesa_Monitor_@70Hz_(1600X1200X8.Z1)

  {
    {
      202500,
      1600, 1664, 1856, 2160,
      1200, 1201, 1204, 1250,
      B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
    },
    B_CMAP8,
    1600, 1200,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // Vesa_Monitor_@75Hz_(1600X1200X8.Z1)

  {
    {
      216000,
      1600, 1664, 1856, 2160,
      1200, 1201, 1204, 1250,
      B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
    },
    B_CMAP8,
    1600, 1200,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  }, // Vesa_Monitor_@80Hz_(1600X1200X8.Z1)

  {
    {
      229500,
      1600, 1664, 1856, 2160,
      1200, 1201, 1204, 1250,
      B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
    },
    B_CMAP8,
    1600, 1200,
    0, 0,
    B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
  },  // Vesa_Monitor_@85Hz_(1600X1200X8.Z1)


	{
		{
			0,
			0, 0, 0, 0,
			0, 0, 0, 0,
			0
		},
		0,
		0, 0,
		0, 0,
		0
	} // End of list.
};


// Algorithmically Generated Modes

// This holds display_mode structures defining 60 Hz 8 bpp modes for the standard
// resolutions.

const display_mode standard_display_modes[STANDARD_RESOLUTION_COUNT] =
{
	// Sync polarities swiped from Trey's mode tables.
	// CRTC parameters calculated using our formulae.
  // Update: Modified these templates to more closely match the VESA timings.

	// 640x480.
	{
		{
			26560, // This should be overridden.
			640, 672, 760, 848,
			480, 489, 492, 522,
			0x0
		},
		B_CMAP8, // This should be overridden.
		640, 480,
		0, 0,
		B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
	},

	// 800x600.
	{
		{
			39540, // This should be overridden.
			800, 832, 928, 1056,
			600, 601, 604, 624,
			B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
		},
		B_CMAP8, // This should be overridden.
		800, 600,
		0, 0,
		B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
	},

	// 1024x768.
	{
		{
			64830, // This should be overridden.
			1024, 1056, 1168, 1344,
			768, 769, 772, 804,
			B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
		},
		B_CMAP8, // This should be overridden.
		1024, 768,
		0, 0,
		B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
	},

	// 1152x900.
  {
    {
      88810, // This should be overridden.
			1152, 1216, 1344, 1568,
			900, 901, 904, 944,
			B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
		},
		B_CMAP8, // This should be overridden.
		1152, 900,
		0, 0,
		B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
	},

	// 1280x1024.
	{
		{
			109700, // This should be overridden.
			1280, 1320, 1472, 1712,
			1024, 1025, 1028, 1068,
			B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
		},
		B_CMAP8, // This should be overridden.
		1280, 1024,
		0, 0,
		B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
	},

	// 1600x1200.
	{
		{
			162260, // This should be overridden.
			1600, 1664, 1856, 2160,
			1200, 1201, 1204, 1252,
			B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
		},
		B_CMAP8, // This should be overridden.
		1600, 1200,
		0, 0,
		B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
	}
};


// This holds a list of standard refresh rates, in Hz. This list has been extended
// a bit.

const uint32 standard_refresh_rates[STANDARD_REFRESH_RATE_COUNT] =
{
  48,  56,  60,  70,
  72,  75,  85,  90,
  110, 115, 120
};


// This holds the colour space values for each of the standard colour spaces.
#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
  const uint32 standard_colour_spaces[STANDARD_COLOUR_SPACE_COUNT] =
    { B_CMAP8, B_RGB15_LITTLE, B_RGB16_LITTLE, B_RGB32_LITTLE };
#else
  const uint32 standard_colour_spaces[STANDARD_COLOUR_SPACE_COUNT] =
    { B_CMAP8, B_RGB15_BIG, B_RGB16_BIG, B_RGB32_BIG };
#endif


// This holds display_mode structures for a number of modes used to test special
// cases and features.
// A record with all zero values terminates the list.

const display_mode diagnostic_display_modes[] =
{
  // Test modes.
  // These are based on a 1024x768 8 bpp 60 Hz display_mode.

  // Test Sync-On-Green.
	{
		{
			69420, // Pixel clock (kHz).
			1024, 1056, 1168, 1344, // Horizontal parameters.
			768, 769, 772, 804, // Vertical parameters.
      // Timing flags.
			B_POSITIVE_HSYNC | B_POSITIVE_VSYNC | B_SYNC_ON_GREEN
		},
		B_CMAP8, // Colour depth.
		1024, 768, // Frame buffer resolution.
		0, 0, // Position on the frame buffer.
    // Display mode flags.
		B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
	},

  // Test interlacing.
  // Use 48 Hz interlaced so that the vertical retrace rate (96 Hz) is still sane.
	{
		{
			55540, // Pixel clock (kHz).
			1024, 1056, 1168, 1344, // Horizontal parameters.
			384, 385, 388, 402, // Vertical parameters.
      // Timing flags.
			B_POSITIVE_HSYNC | B_POSITIVE_VSYNC | B_TIMING_INTERLACED
		},
		B_CMAP8, // Colour depth.
		1024, 768, // Frame buffer resolution.
		0, 0, // Position on the frame buffer.
    // Display mode flags.
		B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
	},


  // End of list.

	{
		{
			0, // Pixel clock (kHz).
			0, 0, 0, 0, // Horizontal parameters.
			0, 0, 0, 0, // Vertical parameters.
      // Timing flags.
			0
		},
		0, // Colour depth.
		0, 0, // Frame buffer resolution.
		0, 0, // Position on the frame buffer.
    // Display mode flags.
		0
	}
};
