/*
	
	VideoControls.cpp
	
	Copyright 1997-8 Be Incorporated, All Rights Reserved.
	
*/

#include "VideoControls.h"

//------------------------------------------------------------------------------

BVideoControls::BVideoControls(const char * name)
{
	strncpy(fName,name,32);
}

//------------------------------------------------------------------------------

BVideoControls::~BVideoControls()
{

}

//------------------------------------------------------------------------------

char *	
BVideoControls::Name()
{
	return fName;
}

//------------------------------------------------------------------------------

void	
BVideoControls::SetBrightness(int32 brightness)
{

	fBrightness = brightness;
}

//------------------------------------------------------------------------------

int32	
BVideoControls::Brightness() const
{
	return(fBrightness);
}

//------------------------------------------------------------------------------

void	
BVideoControls::SetContrast(int32 contrast)
{
	fContrast = contrast;
}

//------------------------------------------------------------------------------

int32	
BVideoControls::Contrast() const
{
	return(fContrast);
}

//------------------------------------------------------------------------------

void	
BVideoControls::SetHue(int32 hue)
{
	fHue = hue;
}

//------------------------------------------------------------------------------

int32	
BVideoControls::Hue() const
{
	return(fHue);
}

//------------------------------------------------------------------------------

void	
BVideoControls::SetSaturation(int32 saturation)
{
	fSaturation = saturation;
}

//------------------------------------------------------------------------------

int32	
BVideoControls::Saturation() const
{
	return(fSaturation);
}

//------------------------------------------------------------------------------

void	
BVideoControls::SetGammaCorrectionRemoval(bool setting)
{

	fGamma = setting;
}

//------------------------------------------------------------------------------

bool	
BVideoControls::GammaCorrectionRemoval() const
{
	return(fGamma);
}

//------------------------------------------------------------------------------

void	
BVideoControls::SetErrorDiffusion(bool setting)
{

	fErrorDiffusion = setting;
}

//------------------------------------------------------------------------------

bool	
BVideoControls::ErrorDiffusion() const
{
	return(fErrorDiffusion);
}

//------------------------------------------------------------------------------

void	
BVideoControls::SetLumaCoring(bool setting)
{

	fLumaCoring = setting;
}

//------------------------------------------------------------------------------

bool	
BVideoControls::LumaCoring() const
{
	return(fLumaCoring);
}

//------------------------------------------------------------------------------

void	
BVideoControls::SetLumaCombFilter(bool setting)
{

	fLumaComb = setting;
}

//------------------------------------------------------------------------------

bool	
BVideoControls::LumaCombFilter() const
{
	return(fLumaComb);
}

//------------------------------------------------------------------------------

void	
BVideoControls::SetChromaCombFilter(bool setting)
{

	fChromaComb = setting;
}

//------------------------------------------------------------------------------

bool	
BVideoControls::ChromaCombFilter() const
{
	return(fChromaComb);
}

//------------------------------------------------------------------------------

void
BVideoControls::_ReservedVideoControls1()
{

}

//------------------------------------------------------------------------------

void
BVideoControls::_ReservedVideoControls2()
{

}


//------------------------------------------------------------------------------

void
BVideoControls::_ReservedVideoControls3()
{

}

