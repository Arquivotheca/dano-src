//******************************************************************************
//
//	File:		PrivateScreen.h
//
//	Description:	BPrivateScreen class header.
//	
//	Copyright 1993-97, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#ifndef _PRIVATE_SCREEN_H
#define _PRIVATE_SCREEN_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#include <Screen.h>
#include "interface_misc.h"
#include "shared_defs.h"

class BPrivateScreen {
private:
  // You cannot construct or destruct BPrivateScreen objects yourself.
  // you need to use CheckOut and Return to access these objects.
  BPrivateScreen();
  ~BPrivateScreen();
  
  
public:		// For external view, using BScreen...

  static BPrivateScreen* CheckOut( screen_id id );
  static BPrivateScreen* CheckOut( BWindow *win );
  static void     Return( BPrivateScreen *screen );

  status_t		SetToNext();
  color_space	ColorSpace();
  BRect			Frame();
  screen_id		ID();
  sem_id		RetraceSemaphore();

  // These 3 calls are not currently implemented, but they will be.
  void*			BaseAddress();
  uint32		BytesPerRow();
  status_t		WaitForRetrace(bigtime_t);
  
  const color_map* ColorMap();

  // Calls to make using the ColorMap easier...
  uint8			IndexForColor( rgb_color rgb );
  uint8			IndexForColor( uint8 r, uint8 g, uint8 b, uint8 a=0 );
  rgb_color		ColorForIndex( const uint8 index );
  uint8			InvertIndex( uint8 index );

  status_t		GetBitmap(BBitmap **screen_shot, bool draw_cursor, BRect *bound);
  status_t		ReadBitmap(BBitmap *buffer, bool draw_cursor, BRect *bound);

  rgb_color		DesktopColor(uint32 index);
  void			SetDesktopColor( rgb_color rgb, uint32 index, bool stick=true );
  
	status_t		ProposeMode(display_mode *target, const display_mode *low, const display_mode *high);
	status_t		GetModeList(display_mode **mode_list, uint32 *count);
	status_t		GetMode(uint32 workspace, display_mode *mode);
	status_t		SetMode(uint32 workspace, display_mode *mode, bool makeDefault);
	status_t		GetDeviceInfo(accelerant_device_info *adi);
	status_t		GetPixelClockLimits(display_mode *mode, uint32 *low, uint32 *high);
	status_t		GetTimingConstraints(display_timing_constraints *dtc);
	status_t		SetDPMS(uint32 dpms_state);
	uint32			DPMSState(void);
	uint32			DPMSCapabilites(void);

public: 	// for Interface Kit routines, but not for external view...
  
  void			get_screen_desc( screen_desc *desc );
  

private:
  color_map		*system_cmap;
  sem_id		retrace;		// was unused[0]
  uint32		free_cmap;
  uint32		unused;			// unused for now...
  
  friend class BApplication;
  static void init_screens();
};


// syntactic sugar...
inline uint8 BPrivateScreen::IndexForColor( rgb_color rgb )
  { return IndexForColor( rgb.red, rgb.green, rgb.blue, rgb.alpha ); }


extern BPrivateScreen *_the_screen_;


#endif

