///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ATI 3D RAGE BEOS DRIVER - CARD INITIALIZATION SCRIPT                      //
//                                                                           //
// alt.software inc. 1998                                                    //
// Written by Christopher Thomas                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
// This file contains the instantiation of the register set startup scripts
// used by OpenGraphicsCard.c.

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//  Inclusions                                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "private.h"
#include "SetRegisters.h"
#include "CardStartScript.h"

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//  Constants                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

// Script run before memory and clock setup.
REGSET_STRUCT SetupStart[] =
{
	// Initialization preparation.

	// Reset CRTC controller.
	{ CRTC_GEN_CNTL,		0x0,			~0x02000000ul,	REGSET_RMW },
	{ CRTC_GEN_CNTL,		0x02000000,		~0x02000000ul,		REGSET_RMW },

	// Init CRTC controller to known-stable state.
	// Modification: do not enable accessing through VGA aperature.
	{ CRTC_GEN_CNTL,		0x03010400,		0x0,			REGSET_WRITE },

	// Blank the screen.
	{ CRTC_GEN_CNTL,		0x40,			~0x00000040ul,	REGSET_RMW },

	// Don't map the A and B segments, as aren't using them.

	// Hold down CRTC reset.
	{ CRTC_GEN_CNTL,		0x0,			~0x02000000ul,	REGSET_RMW },


	// Initialization

#if 0
	// Set the PLL registers.

	// 0x1B -> PLL 0x0D (VFC_CNTL)
	{ CLOCK_CNTL,			0x001B3600,		0xFF0001FF,		REGSET_RMW },

	// Set PLL 0x03.
	// MCLK = PLLMCLK/2, a la 4755 card.
	{ CLOCK_CNTL,			0x00140E00,		0xFF0001FF,		REGSET_RMW },

	// 0x01 -> PLL 0x0B (XCLK = PLLMCLK / 2)
	{ CLOCK_CNTL,			0x00012E00,		0xFF0001FF,		REGSET_RMW },

	// 0x03 -> PLL 0x05 (VCLK out = VCLK)
	{ CLOCK_CNTL,			0x00031600,		0xFF0001FF,		REGSET_RMW },
#endif
	// Switch back to read mode to avoid accidental writes.
	{ CLOCK_CNTL,			0x00000000,		0xFF0001FF,		REGSET_RMW },


	// End of script.

	{ 0x0,					0x0,			0x0,		REGSET_FENCE }
};

// Script run after memory and clock setup.
REGSET_STRUCT SetupEnd[] =
{
	// Additional setup that might help.
	// General configuration of several things; HCLK skew had an effect
	// when testing dual-monitor.
	//{ HW_DEBUG,				0x00843800,		0x0,			REGSET_WRITE },

	// Disable the hardware cursor.
	{ GEN_TEST_CNTL,		0x0,			~0x80ul,			REGSET_RMW },

	// Reset the draw engine.
	{ GEN_TEST_CNTL,		0x0100,			~0x0100ul,		REGSET_RMW },
	{ GEN_TEST_CNTL,		0x0,			~0x0100ul,		REGSET_RMW },

	// Reset the memory controller.
	{ GEN_TEST_CNTL,		0x0200,			~0x0200ul,		REGSET_RMW },
	{ GEN_TEST_CNTL,		0x0,			~0x0200ul,		REGSET_RMW },

	// Initialization cleanup.

	// Release CRTC reset.
	{ CRTC_GEN_CNTL,		0x02000000,		~0x02000000ul,		REGSET_RMW },

	// Reset CRTC controller.
	{ CRTC_GEN_CNTL,		0x0,			~0x02000000ul,	REGSET_RMW },
	{ CRTC_GEN_CNTL,		0x02000000,		~0x02000000ul,		REGSET_RMW },

	// Update: keep the screen blank. ConfigGraphicsCard will unblank, and at
	// present the screen would be in a wierd state with PLLs half set.
	// Unblank the screen.
//	{ CRTC_GEN_CNTL,		0x0,			~0x00000040ul,	REGSET_RMW },


	// End of script.

	{ 0x0,					0x0,			0x0,		REGSET_FENCE }
};

// Script run before memory and clock setup.
REGSET_STRUCT SetupStartLT[] =
{
	// Initialization preparation.

	// Reset CRTC controllers.
	{ CRTC_GEN_CNTL,		0x0,			~0x02200000ul,	REGSET_RMW },
	{ CRTC_GEN_CNTL,		0x02200000,		~0x02200000ul,		REGSET_RMW },

	// Init CRTC controller to known-stable state.
	// Modification: do not enable accessing through VGA aperature.
	//{ CRTC_GEN_CNTL,		0x03010400,		0x0,			REGSET_WRITE },
	{ CRTC_GEN_CNTL,		0x03000400,		0x0,			REGSET_WRITE },

	// Blank the screen.
	{ CRTC_GEN_CNTL,		0x40,			~0x00000040ul,	REGSET_RMW },

	// Don't map the A and B segments, as aren't using them.

	// Hold down CRTC resets.
	{ CRTC_GEN_CNTL,		0x0,			~0x02200000ul,	REGSET_RMW },


	// Initialization

	// Set the PLL registers.

#if 0
	// 0x1B -> PLL 0x0D (VFC_CNTL)
	{ CLOCK_CNTL,			0x001B3600,		0xFF0001FF,		REGSET_RMW },

	// Set PLL 0x03.
	// MCLK = SCLK. This is required with the LT cards, as MCLK
	// apparently runs more slowly than XCLK now.
	{ CLOCK_CNTL,			0x00640E00,		0xFF0001FF,		REGSET_RMW },

	// SCLK = SPLL/2
	{ CLOCK_CNTL,			0x00105E00,		0xFF0001FF,		REGSET_RMW },

	// 0x01 -> PLL 0x0B (XCLK = PLLMCLK / 2)
	// 0x00 -> PLL 0x0B (XCLK = PLLMCLK)
	{ CLOCK_CNTL,			0x00002E00,		0xFF0001FF,		REGSET_RMW },

	// 0x03 -> PLL 0x05 (VCLK out = VCLK)
	{ CLOCK_CNTL,			0x00031600,		0xFF0001FF,		REGSET_RMW },

	// 0x00 -> PLL 0x19 (Use VPLL in the standard way)
	{ CLOCK_CNTL,			0x00006600,		0xFF0001FF,		REGSET_RMW },
#endif
	// Switch back to read mode to avoid accidental writes.
	{ CLOCK_CNTL,			0x00000000,		0xFF0001FF,		REGSET_RMW },


	// End of script.

	{ 0x0,					0x0,			0x0,		REGSET_FENCE }
};

// Script run after memory and clock setup.
REGSET_STRUCT SetupEndLT[] =
{
	// Additional setup that might help.
	// General configuration of several things; HCLK skew had an effect
	// when testing dual-monitor.
	// Update: a few bits have been added to the top of this register.
	//{ HW_DEBUG,				0x48843800,		0x0,			REGSET_WRITE },

	// Disable the hardware cursor, disable test modes.
	{ GEN_TEST_CNTL,		0x0,			0x0,			REGSET_WRITE },

	// Set the CRT as the primary display and disable the LCD and CRT.
	{ LCD_INDEX,			0x02,			0x0,			REGSET_WRITE },
	{ LCD_DATA,				0x00800000,		0x0,			REGSET_WRITE },

	{ LCD_INDEX,			0x01,			0x0,			REGSET_WRITE },
	{ LCD_DATA,				0x03,			~0xC8000023ul,	REGSET_RMW },
	// FFB { LCD_INDEX,			0x0500,			~0x0500ul,		REGSET_RMW },
	{ LCD_INDEX,			0x0401,			~0x070ful,		REGSET_RMW },

	// Reset the draw engine.
	{ GEN_TEST_CNTL,		0x0100,			~0x0100ul,		REGSET_RMW },
	{ GEN_TEST_CNTL,		0x0,			~0x0100ul,		REGSET_RMW },

	// Reset the memory controller.
	{ GEN_TEST_CNTL,		0x0200,			~0x0200ul,		REGSET_RMW },
	{ GEN_TEST_CNTL,		0x0,			~0x0200ul,		REGSET_RMW },

	// Initialization cleanup.

	// Release CRTC resets.
	{ CRTC_GEN_CNTL,		0x02200000,		~0x02200000ul,		REGSET_RMW },

	// Reset CRTC controllers.
	{ CRTC_GEN_CNTL,		0x0,			~0x02200000ul,		REGSET_RMW },
	{ CRTC_GEN_CNTL,		0x02200000,		~0x02200000ul,		REGSET_RMW },

	// Keep the screen blank.

	// End of script.

	{ 0x0,					0x0,			0x0,		REGSET_FENCE }
};
