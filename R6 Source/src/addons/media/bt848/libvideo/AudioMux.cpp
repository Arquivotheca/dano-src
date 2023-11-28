/*
	
	AudioMux.cpp
	
	Copyright 1997-8 Be Incorporated, All Rights Reserved.
	
*/

#include <string.h>

#include  "AudioMux.h"

//-----------------------------------------------------------------

BAudioMux::BAudioMux(const char *name)
{
	fSource = 0;
	fMute = 0;
	fLastSource = 0;
	strncpy(fName,name,32);
}

//-----------------------------------------------------------------

BAudioMux::~BAudioMux()
{

}

//-----------------------------------------------------------------

char *
BAudioMux::Name() const
{
	return const_cast<BAudioMux *>(this)->fName;
}

//-----------------------------------------------------------------

uint32
BAudioMux::NumberInputs() const
{
	return 0;
}

//-----------------------------------------------------------------

status_t	
BAudioMux::SetSource(const uint32 source)
{
	fSource = source;
	return B_NO_ERROR;
}

//-----------------------------------------------------------------

uint32	
BAudioMux::Source() const
{
	return (fSource);
}

//-----------------------------------------------------------------

void	
BAudioMux::SetMute(const uint32 source)
{
	fMute = source;
}

//-----------------------------------------------------------------

void
BAudioMux::Mute()
{
	fLastSource = Source();
	SetSource(fMute);
}

//-----------------------------------------------------------------

void
BAudioMux::Unmute()
{
	SetSource(fLastSource);
}

//-----------------------------------------------------------------

void
BAudioMux::SetShift(const uint32 shift)
{
}

//-----------------------------------------------------------------

void
BAudioMux::_ReservedAudioMux1()
{

}

//-----------------------------------------------------------------

void
BAudioMux::_ReservedAudioMux2()
{

}

//-----------------------------------------------------------------

void
BAudioMux::_ReservedAudioMux3()
{

}
