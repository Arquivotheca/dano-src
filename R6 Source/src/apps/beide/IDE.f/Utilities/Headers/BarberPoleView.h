// ---------------------------------------------------------------------------
/*
	BarberPoleView.h
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	taken from Zip-O-Matic implementation by Pavel Cisler

*/
// ---------------------------------------------------------------------------

#ifndef __BARBER_POLE_VIEW__
#define __BARBER_POLE_VIEW__

#include <View.h>
#include <Bitmap.h>

#include "BarberPoleBitmaps.h"

const int32 kBarberPoleSize = kBarberPoleBitmapWidth * kBarberPoleBitmapHeight;
const int32 kBevel = 2;

class BarberPoleView : public BView {
public:
	BarberPoleView(BRect, const char *, const unsigned char *const *barberPoleArray,
		int32 barberPoleCount, const unsigned char *dropHereBits);

	void SetPaused();
	void SetProgressing();
	void SetWaitingForDrop();

protected:
	virtual	void Draw(BRect);
	virtual void Pulse();

	void DrawCurrentBarberPole();
private:
	BBitmap bitmap;	
	const unsigned char *const *bits;
	const unsigned char *dropHereBits;
	int32 count;
	int32 indx;
	bool progress;
	bool paused;
};

#endif
