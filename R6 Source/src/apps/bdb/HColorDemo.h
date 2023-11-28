/*
	HColorDemo.h
	
	Copyright Hekkelman Programmatuur
	
	Created: 10/10/97 13:42:02
*/

#ifndef HCOLORDEMO_H
#define HCOLORDEMO_H

#include <View.h>

class HColorDemo : public BView {
public:
		HColorDemo(BRect frame, const char *name);

virtual	void Draw(BRect update);
		void SetOldColor(rgb_color c);
		void SetNewColor(rgb_color c);

private:
		rgb_color fOldColor, fNewColor;
};

#endif
