/*
	
	VideoControls.h
	
	Copyright 1997-8 Be Incorporated, All Rights Reserved.
	
*/

#ifndef BT848_VIDEO_CONTROLS_H
#define BT848_VIDEO_CONTROLS_H

#include <unistd.h>
#include <bt848_driver.h>
#include <SupportDefs.h>

#include "VideoControls.h"

class Bt848VideoControls : public BVideoControls
{
public:
							Bt848VideoControls(	const char *name,
												int32 bt848,
												bt848_config *config);
virtual						~Bt848VideoControls();
							
virtual void				SetBrightness(int32 brightness);
virtual int32				Brightness() const;
virtual void				SetContrast(int32 contrast);
virtual int32				Contrast() const;
virtual void				SetHue(int32 hue);
virtual int32				Hue() const;
virtual void				SetSaturation(int32 saturation);
virtual int32				Saturation() const;
virtual void				SetGammaCorrectionRemoval(bool setting);
virtual bool				GammaCorrectionRemoval() const;
virtual void				SetErrorDiffusion(bool on);
virtual bool				ErrorDiffusion() const;
virtual void				SetLumaCoring(bool on);
virtual bool				LumaCoring() const;
virtual void				SetLumaCombFilter(bool on);
virtual bool				LumaCombFilter() const;
virtual void				SetChromaCombFilter(bool on);
virtual bool				ChromaCombFilter() const;

private:

							Bt848VideoControls(const Bt848VideoControls &);
		Bt848VideoControls	&operator=(const Bt848VideoControls &);

		uint32				fBt848;
		bt848_config		*fConfig;
};

#endif


