#include <Screen.h>
#include "PrivateScreen.h"

#if __INTEL__
#define INTEL_BCC_CONDITIONAL_FIX 1
#endif

BScreen::BScreen( screen_id id )
{
	screen = BPrivateScreen::CheckOut(id);
}

BScreen::BScreen( const BScreen & )
{
}

BScreen &BScreen::operator=( const BScreen & )
{
	return *this;
}

BScreen::BScreen( BWindow *win )
{
	screen = BPrivateScreen::CheckOut(win);
}


BScreen::~BScreen()
{
	if(screen) BPrivateScreen::Return(screen);
}


status_t BScreen::SetToNext()
{
	return screen ? screen->SetToNext() : B_ERROR;
}


bool BScreen::IsValid()
{
	return screen != NULL;
}
  

color_space	BScreen::ColorSpace()
{
	return screen ? screen->ColorSpace() : (color_space)0;
}


BRect BScreen::Frame()
{
	return screen ? screen->Frame() : BRect(0,0,0,0);
}


screen_id BScreen::ID()
{
	return screen ? screen->ID() : B_MAIN_SCREEN_ID;
}

void *BScreen::BaseAddress()
{
	return screen ? screen->BaseAddress() : NULL;
}


uint32 BScreen::BytesPerRow()
{
	return screen ? screen->BytesPerRow() : 0;
}


status_t BScreen::WaitForRetrace()
{
	return screen ? screen->WaitForRetrace(B_INFINITE_TIMEOUT) : B_ERROR;
}

status_t BScreen::WaitForRetrace(bigtime_t timeout)
{
	return screen ? screen->WaitForRetrace(timeout) : B_ERROR;
}


  
uint8 BScreen::IndexForColor( uint8 r, uint8 g, uint8 b, uint8 a )
{
	return screen ? screen->IndexForColor(r,g,b,a) : 0;
}


rgb_color BScreen::ColorForIndex( const uint8 index )
{
#if INTEL_BCC_CONDITIONAL_FIX
	if(screen)
		return screen->ColorForIndex(index);
	else
		return B_TRANSPARENT_32_BIT;
#else
	return screen ? screen->ColorForIndex(index) : B_TRANSPARENT_32_BIT;
#endif
}


uint8 BScreen::InvertIndex( uint8 index )
{
	return screen ? screen->InvertIndex(index) : 0;
}
  

const color_map *BScreen::ColorMap()
{
	return screen ? screen->ColorMap() : NULL;
}


status_t BScreen::GetBitmap(BBitmap **screen_shot, bool draw_cursor, BRect *bound)
{
	return screen ? screen->GetBitmap(screen_shot, draw_cursor, bound) : B_ERROR;
}
  
  
status_t BScreen::ReadBitmap(BBitmap *buffer, bool draw_cursor, BRect *bound)
{
	return screen ? screen->ReadBitmap(buffer, draw_cursor, bound) : B_ERROR;
}


rgb_color BScreen::DesktopColor() 
{
	return DesktopColor(0xffffffff);
}

rgb_color BScreen::DesktopColor(uint32 index) 
{
#if INTEL_BCC_CONDITIONAL_FIX
	if(screen)
		return screen->DesktopColor(index);
	else
		return B_TRANSPARENT_32_BIT;
#else
	return screen ? screen->DesktopColor(index) : B_TRANSPARENT_32_BIT;
#endif
}


void BScreen::SetDesktopColor( rgb_color rgb, bool stick )
{
	if(screen) screen->SetDesktopColor(rgb, 0xffffffff, stick);
}

void BScreen::SetDesktopColor( rgb_color rgb, uint32 index, bool stick )
{
	if(screen) screen->SetDesktopColor(rgb,index,stick);
}

status_t 
BScreen::ProposeDisplayMode(display_mode *target, const display_mode *low, const display_mode *high)
{
	return ProposeMode(target, low, high);
}

status_t 
BScreen::ProposeMode(display_mode *target, const display_mode *low, const display_mode *high)
{
	return screen ? screen->ProposeMode(target, low, high) : B_ERROR;
}



status_t
BScreen::GetModeList(display_mode **mode_list, uint32 *count)
{
	return screen ? screen->GetModeList(mode_list, count) : B_ERROR;
}

status_t 
BScreen::GetMode(display_mode *mode)
{
	return GetMode(0xFFFFFFFF, mode);
}

status_t 
BScreen::GetMode(uint32 workspace, display_mode *mode)
{
	return screen ? screen->GetMode(workspace, mode) : B_ERROR;
}

status_t 
BScreen::SetMode(display_mode *mode, bool makeDefault)
{
	return SetMode(0xFFFFFFFF, mode, makeDefault);
}

status_t 
BScreen::SetMode(uint32 workspace, display_mode *mode, bool makeDefault)
{
	return screen ? screen->SetMode(workspace, mode, makeDefault) : B_ERROR;
}

status_t 
BScreen::GetPixelClockLimits(display_mode *mode, uint32 *low, uint32 *high)
{
	return screen ? screen->GetPixelClockLimits(mode, low, high) : B_ERROR;
}

status_t 
BScreen::GetTimingConstraints(display_timing_constraints *dtc)
{
	return screen ? screen->GetTimingConstraints(dtc) : B_ERROR;
}

status_t 
BScreen::SetDPMS(uint32 dpms_state)
{
	return screen ? screen->SetDPMS(dpms_state) : B_ERROR;
}

uint32 
BScreen::DPMSState(void)
{
	return screen ? screen->DPMSState() : B_ERROR;
}

uint32 
BScreen::DPMSCapabilites(void)
{
	return screen ? screen->DPMSCapabilites() : B_ERROR;
}

status_t 
BScreen::GetDeviceInfo(accelerant_device_info *adi)
{
	return screen ? screen->GetDeviceInfo(adi) : B_ERROR;
}



BPrivateScreen*	BScreen::private_screen()
{
	return screen;
}
