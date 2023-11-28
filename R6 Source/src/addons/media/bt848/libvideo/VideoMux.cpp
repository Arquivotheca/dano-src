/*
	
	Bt848VideoMux.cpp
	
	Copyright 1997-8 Be Incorporated, All Rights Reserved.
	
*/

#include <string.h>

#include  "VideoMux.h"

//-----------------------------------------------------------------

BVideoMux::BVideoMux(const char *name)
{
	strncpy(fName,name,32);
}

//-----------------------------------------------------------------

BVideoMux::~BVideoMux()
{

}

//-----------------------------------------------------------------

char *
BVideoMux::Name()
{
	return fName;
}

//-----------------------------------------------------------------

uint32
BVideoMux::NumberInputs()
{
	return 0;
}

//-----------------------------------------------------------------

status_t	
BVideoMux::SetSource(uint32 source)
{
	fSource = source;
	return B_NO_ERROR;
}

//-----------------------------------------------------------------

uint32	
BVideoMux::Source() const
{
	return (fSource);
}

//-----------------------------------------------------------------

void
BVideoMux::_ReservedVideoMux1()
{

}

//-----------------------------------------------------------------

void
BVideoMux::_ReservedVideoMux2()
{

}

//-----------------------------------------------------------------

void
BVideoMux::_ReservedVideoMux3()
{

}
