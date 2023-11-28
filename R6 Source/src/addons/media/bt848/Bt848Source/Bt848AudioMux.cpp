/*
	
	Bt848AudioMux.cpp
	
	Copyright 1997-8 Be Incorporated, All Rights Reserved.
	
*/

#include  "Bt848AudioMux.h"

#define MUX(x...)

//------------------------------------------------------------------------------

Bt848AudioMux::Bt848AudioMux(	const char *name,
								uint32 bt848,
								bt848_config *config
								):BAudioMux(name)
{
	fBt848 = bt848;
	fConfig = config;
	fShift = 0;
	fMask = 0x000003;
	SetSource(0);
	SetMute(0);
}

//------------------------------------------------------------------------------

Bt848AudioMux::~Bt848AudioMux()
{

}


//------------------------------------------------------------------------------

uint32
Bt848AudioMux::NumberInputs() const
{
	return fMask + 1;
}

//------------------------------------------------------------------------------

status_t
Bt848AudioMux::SetSource(const uint32 source)
{
	if (fShift < 32)
	{
		ioctl(fBt848,BT848_READ_GPIO_DATA,fConfig);
		fConfig->gpio_data &= ~(fMask << fShift);	
		fConfig->gpio_data |= (source & fMask) << fShift;
		MUX("AUDIO MUX DATA = %06x\n",fConfig->gpio_data);
		ioctl(fBt848,BT848_WRITE_GPIO_DATA,fConfig);
	}
	return BAudioMux::SetSource(source);
}

//------------------------------------------------------------------------------

void
Bt848AudioMux::SetMute(const uint32 source)
{
	fMute = source;
}

//-----------------------------------------------------------------

void
Bt848AudioMux::Mute()
{
	fLastSource = Source();
	SetSource(fMute);
}

//-----------------------------------------------------------------

void
Bt848AudioMux::Unmute()
{
	SetSource(fLastSource);
}

//------------------------------------------------------------------------------

void
Bt848AudioMux::SetShift(const uint32 shift)
{
	fShift = shift;
}



