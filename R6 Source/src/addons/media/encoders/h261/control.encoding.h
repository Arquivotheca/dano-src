//=============================================================================
#ifndef __INCLUDE_CONTROL_ENCODING_H
#define __INCLUDE_CONTROL_ENCODING_H
//=============================================================================
#include "control.yuv_image.h"
#include "H261Encoder.h"
//=============================================================================
class encoding_control
{
public:
	encoding_control();
	~encoding_control();
public:
	void new_frame_352_288_422(unsigned char *buffer);
private:
	yuv_image previously_sent;
	yuv_image actual;
	yuv_image delta;
	
	H261EncoderStream2 stream;
};
//=============================================================================
#endif
//=============================================================================
