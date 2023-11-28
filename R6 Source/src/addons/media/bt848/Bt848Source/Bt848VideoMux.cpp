/*
	
	Bt848VideoMux.cpp
	
	Copyright 1997-8 Be Incorporated, All Rights Reserved.
	
*/

#include  "Bt848VideoMux.h"

//------------------------------------------------------------------------------

Bt848VideoMux::Bt848VideoMux(	const char *name,
								uint32 bt848,
								bt848_config *config
								):BVideoMux(name)
{
	fBt848 = bt848;
	fConfig = config;
	SetColorBars(false);
	SetSource(0);
}

//------------------------------------------------------------------------------

Bt848VideoMux::~Bt848VideoMux()
{

}

//------------------------------------------------------------------------------

uint32
Bt848VideoMux::NumberInputs()
{
	return 9; // 8 inputs plus color bars
}

//------------------------------------------------------------------------------

status_t	
Bt848VideoMux::SetSource(uint32 source)
{

	switch (source)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			fConfig->video_source = source;
			break;
		case 8:
			SetColorBars(true);
			break;
		default:
			return B_ERROR;
	}
	
	if (!ColorBars())
		ioctl(fBt848,BT848_SELECT_VIDEO_SOURCE,fConfig);
		
	return BVideoMux::SetSource(source);
}

//------------------------------------------------------------------------------

void	
Bt848VideoMux::SetColorBars(bool setting)
{

	fConfig->color_bars = setting;
	ioctl(fBt848,BT848_COLOR_BARS,fConfig);
}

//------------------------------------------------------------------------------

bool	
Bt848VideoMux::ColorBars() const
{
	return(fConfig->color_bars);
}

//------------------------------------------------------------------------------



