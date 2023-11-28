//////////////////////////////////////////////////////////////////////////////
// Standard BeOS Display Modes
//    This file contains the declarations for display_mode values for the
// standard modes used under BeOS, extended to cover additional refresh
// rates.
//
// Device Dependance: None.
//
//////////////////////////////////////////////////////////////////////////////

//
// Macros.
//

#define STANDARD_RESOLUTION_COUNT 27
#define STANDARD_REFRESH_RATE_COUNT 11
#define STANDARD_COLOUR_SPACE_COUNT 4

//
// Constants.
//

// This holds display_mode structures defining 60 Hz 8 bpp modes for the standard
// resolutions.
extern const display_mode standard_display_modes[STANDARD_RESOLUTION_COUNT];

// This holds a list of standard refresh rates, in Hz. This list has been extended
// a bit.
extern const uint32 standard_refresh_rates[STANDARD_REFRESH_RATE_COUNT];

// This holds the colour space values for each of the standard colour spaces.
extern const uint32 standard_colour_spaces[STANDARD_COLOUR_SPACE_COUNT];

// This holds display_mode structures for a number of modes used to test special
// cases and features.
// A record with all zero values terminates the list.
extern const display_mode diagnostic_display_modes[];
