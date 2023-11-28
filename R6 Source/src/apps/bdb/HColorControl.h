/*
	HColorControl.h
*/

#ifndef HCOLORCONTROL_H
#define HCOLORCONTROL_H

#if __BEOS__
#include <View.h>
class HColorControl : public BView {
public:
		HColorControl(BRect r, const char *name, const char *label, rgb_color color);
		~HColorControl();

virtual	void Draw(BRect updateRect);
virtual	void MouseDown(BPoint where);
virtual void MessageReceived(BMessage *msg);
virtual void MakeFocus(bool focus);
virtual void KeyDown(const char *bytes, int32 numBytes);
		
		void SetColor(rgb_color color);
		rgb_color Color();

private:
		char *fLabel;
		rgb_color fColor;
		bool fDown;
};
#else
#include <LGAColorSwatchControl.h>
#include "BeCompat.h"

class HColorControl : public LGAColorSwatchControl
{
  public:
	HColorControl(LStream* inStream)
		: LGAColorSwatchControl(inStream)	{}
	
	enum { class_ID = 'HClr' };
	
	void SetColor(rgb_color color)			{ SetSwatchColor(color); }
	rgb_color Color() const 				{ RGBColor c; GetSwatchColor(c); return c; }
};
#endif

#endif
