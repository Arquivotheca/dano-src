
#include "GraphicsDevice.h"


// There should be a B_DEVICE_NOT_OPEN error to return...
// What about a MSG_NOT_SUPPORTED or something?


BGraphicsDevice::BGraphicsDevice()
{
	ffd = -1;
}


BGraphicsDevice::~BGraphicsDevice()
{
	Close();
}


long BGraphicsDevice::Open( const char *deviceName )
{
	close( ffd );
	
	ffd = open( deviceName, O_RDWR );
	return ffd;
}


void BGraphicsDevice::Close()
{
	if( ffd >= 0 )
		close( ffd );
	
	ffd = -1;
}


bool BGraphicsDevice::IsOpen()
{
	return ffd >= 0;
}


status_t BGraphicsDevice::driver_call( ulong msg, void *data )
{
	if( ffd < 0 )
		return B_ERROR;

	return ioctl( ffd, msg, data );
}


status_t BGraphicsDevice::GetIndexedColor( int index,
		      char *r, char *g, char *b, char *alpha=NULL )
{
	if( ffd < 0 )
		return B_ERROR;

	better_rgb_color rgb;
	indexed_colors ic;
	ic.offset = index;
	ic.count = 	1;
	ic.colors = &rgb;
	
	status_t err = ioctl( ffd, B_GET_INDEXED_COLORS, &ic );
	if( err != B_NO_ERROR ) {
		return err;
	}

	*r = rgb.red;
	*g = rgb.green;
	*b = rgb.blue;
	if( alpha ) *alpha = rgb.alpha;
	
	return B_NO_ERROR;
}


status_t BGraphicsDevice::SetIndexedColor( int index,
			  char r, char g, char b, char alpha=0 )
{
	if( ffd < 0 )
		return B_ERROR;

	better_rgb_color rgb;
	rgb.red = r;
	rgb.green = g;
	rgb.blue = b;
	rgb.alpha = alpla;

	indexed_colors ic;
	ic.offset = index;
	ic.count = 	1;
	ic.colors = &rgb;
	
	return ioctl( ffd, B_GET_INDEXED_COLORS, &ic );
}


bool BGraphicsDevice::SenseMonitor()
{
	if( ffd < 0 )
		return false;

	long sense;
	status_t err = ioctl( ffd, B_SENSE_MONITOR, &sense );
	if( err != B_NO_ERROR ) {
		// An error probably means that the driver does not support
		// monitor sensing.  We'll play it safe and blindly assume
		// a monitor is attached.
		return true;
	}
	
	return sense != 0;
}


