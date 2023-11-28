/*
	HColorSlider
	
	Copyright 1997, Hekkelman Programmatuur
*/

#ifndef HCOLORSLIDER_H
#define HCOLORSLIDER_H

#include <View.h>

const unsigned long
	msg_SliderChanged = 'Slid',
	msg_EndTracking = 'EndT';

class HColorSlider : public BView {
public:
		HColorSlider(BRect frame, const char *name, rgb_color max);
		~HColorSlider();
	
		void SetMax(rgb_color);
virtual	void Draw(BRect updateRect);
virtual	void MouseDown(BPoint where);
virtual	void KeyDown(const char *bytes, int32 numBytes);
		
		float Value() const;
		void SetValue(float v);
		
virtual	void MakeFocus(bool focus);
	
private:
		float fValue;
		bool fHorizontal;
		rgb_color fMax;
};

#endif
