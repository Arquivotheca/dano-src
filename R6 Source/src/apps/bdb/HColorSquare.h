/*
	HColorSquare.h
	
	Copyright Hekkelman Programmatuur
	
	Created: 10/10/97 11:16:32
*/

#ifndef HCOLORSQUARE_H
#define HCOLORSQUARE_H

#include <View.h>

const unsigned long
	msg_ColorSquareChanged = 'ChnS';

class HColorSquare : public BView {
public:
		HColorSquare(BRect frame, const char *name);
		~HColorSquare();
		
virtual	void Draw(BRect update);
virtual	void MouseDown(BPoint where);

		void SetColor(rgb_color);
		rgb_color Color() const;
		
		void SetValue(float value);
		
private:
		void UpdateBitmap();
		
		BBitmap *fBitmap;
		int fX, fY;
		float fValue;
};

#endif
