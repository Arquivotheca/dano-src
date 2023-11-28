/*
	
	VideoControls.h
	
	Copyright 1997-8 Be Incorporated, All Rights Reserved.
	
*/

#ifndef _VIDEO_CONTROLS_H
#define _VIDEO_CONTROLS_H

#include <string.h>

#include <SupportDefs.h>

class BVideoControls
{
public:
							BVideoControls(const char *name);
virtual						~BVideoControls();
							
virtual char *				Name();

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

virtual	void				_ReservedVideoControls1();
virtual	void				_ReservedVideoControls2();
virtual	void				_ReservedVideoControls3();

							BVideoControls(const BVideoControls &);
		BVideoControls			&operator=(const BVideoControls &);

		char 				fName[32];
		int32				fBrightness;
		int32				fContrast;
		int32				fHue;
		int32				fSaturation;
		int32				fGamma;
		int32				fErrorDiffusion;
		int32				fLumaCoring;
		int32				fLumaComb;
		int32				fChromaComb;
		uint32				_reserved[3];

};

#endif


