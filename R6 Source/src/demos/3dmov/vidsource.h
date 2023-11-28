/*
	
	vidsource.h
	
	Copyright 1997 Be Incorporated, All Rights Reserved.
	
*/

#ifndef VIDSOURCE_H
#define VIDSOURCE_H

#pragma once

#include <View.h>

#include "Bt848_driver.h"

#define	WHITEBALANCE	0
#define	CONTRAST		0
#define	BRIGHTNESS		0
#define HUE				0
#define SATURATION		0

class BTSVideoSource 
{
public:
				BTSVideoSource(char *name);
				~BTSVideoSource();
	
	virtual	void	Init();
	virtual	void	Start();
	virtual void	Stop();
	virtual void	Restart();
	virtual BBitmap	*GetNextFrame();
	
	virtual void	SetBrightness(int bright);
	virtual void	SetContrast(int contrast);
	virtual void	SetHue(int hue);
	virtual void	SetSaturation(int saturation);
	
			

protected:
	Bt848_config 	fConfig;
	long			fRunning;	
	BBitmap			*bm;
	BBitmap			*bitmap;
	BBitmap			*nextbitmap;
	BBitmap 		*bitmap1;
	BBitmap			*bitmap2;

	area_id			isoc_area1;
	area_id			isoc_area2;

	unsigned char	*ib;
	unsigned char	*isocbuffer;
	unsigned char	*nextisocbuffer;
	unsigned char	*isocbuffer1;
	unsigned char	*isocbuffer2;

	void	*even[512];			// Pointers to even scan lines
	void	*odd[512];			// Pointers to odd scan lines
};

#endif
