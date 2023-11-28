//*****************************************************************************
//
//	File:		 GraphicsDevice.h
//
//	Description: Application interface for graphics drivers
//
//	Copyright 1996-97, Be Incorporated
//
//******************************************************************************/

#ifndef _GRAPHICS_DEVICE_H
#define _GRAPHICS_DEVICE_H

#include <SupportDefs.h>
#include <GraphicsDriver.h>


class BGraphicsDevice
{
  
public:
  BGraphicsDevice();
  ~BGraphicsDevice();

  long Open( const char *deviceName );
  void Close( void );
  bool IsOpen( void );
  
  // Returns true if driver is open and either a monitor is attached
  // or the driver doesn't support monitor sensing (we'll just have to
  // assume a monitor is attached).  Otherwise, returns false.
  bool SenseMonitor( void );
  
  // Obtains the drawing environment
  status_t GetFrameBufferInfo( frame_buffer_info *fb )
      { return driver_call( B_GET_FRAME_BUFFER_INFO, ic ); }
  
  // individual colors...
  status_t GetIndexedColor( int index, char *r, char *g, char *b,
							char *alpha=NULL );
  status_t SetIndexedColor( int index, char r, char g, char b,
							char alpha=0 );

  // groups of colors...
  status_t GetIndexedColors( indexed_colors *ic )
      { return driver_call( B_GET_INDEXED_COLORS, ic ); }
  status_t SetIndexedColors( const indexed_colors *ic )
      { return driver_call( B_SET_INDEXED_COLORS, ic ); }
  
  // gamma...
  status_t GetRGBGamma( rgb_gamma* )
      { return driver_call( B_GET_RGB_GAMMA, ic ); }
  status_t SetRGBGamma( const rgb_gamma* )
      { return driver_call( B_SET_RGB_GAMMA, ic ); }

  /*
  // Manipulate VESA power state
  status_t GetPowerState( );
  status_t SetPowerState( );

  // Interrogates and returns Apples cable sense.
  status_t GetCableSense( );

  // pan/zoom/cursor
  */

private:
  int ffd;
  status_t driver_call( ulong msg, void *data );
  uint32  _fReserved[5];
};

