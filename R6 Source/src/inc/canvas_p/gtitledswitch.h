#ifndef GTITLEDSWITCH_H
#define GTITLEDSWITCH_H

#include "gbutton.h"

class AStringLabel;

//======================================================
// Interface: BGTitledToggle
//
// This is a highly specialized class.  It is a toggle
// button that has a title above it.  The highliting
// behavior is to brighten the text as the highlite, and
// set it back to normal for non highlited.
//======================================================
class BGTitledSwitch : public BMSwitch
{
public:
					BGTitledSwitch(const char *name, const char *title, const float size, const rgb_color normalColor, const rgb_color highlited, BMGraphic *step1=0, BMGraphic *step2=0);
	
	// From BMGraphic
	virtual void	Draw(BGraphPort *, const BRect rect);
	
	virtual void	SetTitle(const char *title);
	
protected:
	virtual void	SetGraphics(BMGraphic *step1, BMGraphic *step2=0, BMGraphic *highlited=0, BMGraphic *pressing=0);

	rgb_color	fNormalColor;		// Color to use for title for normal display
	rgb_color	fHighlitedColor;	// Color used for title when mouse is moving over us
	rgb_color	fDimColor;	// Color used for title when mouse is moving over us
	AStringLabel	*fTitleGraphic;
	
private:
};



#endif