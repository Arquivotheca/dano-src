//--------------------------------------------------------------------
//	
//	Slider.h
//
//	Written by: Robert Polic
//	
//	Copyright 1998 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef SLIDER_H
#define SLIDER_H

#include <Bitmap.h>
#include <Point.h>
#include <Rect.h>
#include <View.h>

#define SLIDER_HEIGHT		 24
#define THUMB_WIDTH			 15


//====================================================================

class TSliderView : public BView {

private:

off_t			fMax;
off_t			fValue;
BBitmap			*fOffScreenBits;
BView			*fOffScreenView;

public:

				TSliderView(BRect, off_t, off_t);
				~TSliderView(void);
virtual void	Draw(BRect);
virtual void	FrameResized(float, float);
virtual void	MessageReceived(BMessage*);
virtual void	MouseDown(BPoint);
BRect			BarFrame(void);
void			SetMax(off_t, off_t);
void			SetValue(off_t);
off_t			Value(void);
};
#endif
