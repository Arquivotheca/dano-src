/*
	
	Bt848VideoControls.cpp
	
	Copyright 1997-8 Be Incorporated, All Rights Reserved.
	
*/

#include  "Bt848VideoControls.h"

//------------------------------------------------------------------------------

Bt848VideoControls::Bt848VideoControls(	const char * name,
										int32 bt848,
										bt848_config *config):BVideoControls(name)
{
	fBt848 = bt848;
	fConfig = config;
	SetBrightness(0);
	SetContrast(0);
	SetHue(0);
	SetSaturation(0);
	SetGammaCorrectionRemoval(true);
	SetErrorDiffusion(true);
	SetLumaCoring(true);
	SetLumaCombFilter(false);
	SetChromaCombFilter(true);

}

//------------------------------------------------------------------------------

Bt848VideoControls::~Bt848VideoControls()
{

}

//------------------------------------------------------------------------------

void	
Bt848VideoControls::SetBrightness(int32 brightness)
{

	fConfig->brightness = brightness;
	ioctl(fBt848,BT848_BRIGHTNESS,fConfig);
}

//------------------------------------------------------------------------------

int32	
Bt848VideoControls::Brightness() const
{
	return(fConfig->brightness);
}

//------------------------------------------------------------------------------

void	
Bt848VideoControls::SetContrast(int32 contrast)
{
	fConfig->contrast = contrast;
	ioctl(fBt848,BT848_CONTRAST,fConfig);
}

//------------------------------------------------------------------------------

int32	
Bt848VideoControls::Contrast() const
{
	return(fConfig->contrast);
}

//------------------------------------------------------------------------------

void	
Bt848VideoControls::SetHue(int32 hue)
{
	fConfig->hue = hue;
	ioctl(fBt848,BT848_HUE,fConfig);
}

//------------------------------------------------------------------------------

int32	
Bt848VideoControls::Hue() const
{
	return(fConfig->hue);
}

//------------------------------------------------------------------------------

void	
Bt848VideoControls::SetSaturation(int32 saturation)
{
	fConfig->saturation = saturation;
	ioctl(fBt848,BT848_SATURATION,fConfig);
}

//------------------------------------------------------------------------------

int32	
Bt848VideoControls::Saturation() const
{
	return(fConfig->saturation);
}

//------------------------------------------------------------------------------

void	
Bt848VideoControls::SetGammaCorrectionRemoval(bool setting)
{

	fConfig->gamma = setting;
	ioctl(fBt848,BT848_GAMMA,fConfig);
}

//------------------------------------------------------------------------------

bool	
Bt848VideoControls::GammaCorrectionRemoval() const
{
	return(fConfig->gamma);
}

//------------------------------------------------------------------------------

void	
Bt848VideoControls::SetErrorDiffusion(bool setting)
{

	fConfig->error_diffusion = setting;
	ioctl(fBt848,BT848_ERROR_DIFFUSION,fConfig);
}

//------------------------------------------------------------------------------

bool	
Bt848VideoControls::ErrorDiffusion() const
{
	return(fConfig->error_diffusion);
}

//------------------------------------------------------------------------------

void	
Bt848VideoControls::SetLumaCoring(bool setting)
{

	fConfig->luma_coring = setting;
	ioctl(fBt848,BT848_LUMA_CORING,fConfig);
}

//------------------------------------------------------------------------------

bool	
Bt848VideoControls::LumaCoring() const
{
	return(fConfig->luma_coring);
}

//------------------------------------------------------------------------------

void	
Bt848VideoControls::SetLumaCombFilter(bool setting)
{

	fConfig->luma_comb = setting;
	ioctl(fBt848,BT848_LUMA_COMB,fConfig);
}

//------------------------------------------------------------------------------

bool	
Bt848VideoControls::LumaCombFilter() const
{
	return(fConfig->luma_comb);
}

//------------------------------------------------------------------------------

void	
Bt848VideoControls::SetChromaCombFilter(bool setting)
{

	fConfig->chroma_comb = setting;
	ioctl(fBt848,BT848_CHROMA_COMB,fConfig);
}

//------------------------------------------------------------------------------

bool	
Bt848VideoControls::ChromaCombFilter() const
{
	return(fConfig->chroma_comb);
}

//------------------------------------------------------------------------------
