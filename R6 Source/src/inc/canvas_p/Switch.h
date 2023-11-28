#ifndef _SWITCH_H
#define _SWITCH_H

#include "graphic.h"
#include "interactor.h"
#include "stepper.h"
#include "highlites.h"
#include "observable.h"
#include "graphicview.h"

#include <Invoker.h>
#include <GraphicsDefs.h>
#include <Handler.h>
#include <Control.h>

// Things we will address
class BMRangeStepper;
class AStringLabel;

//======================================================
// Class Interface: BSwitch
//
// A generic view control that allows you to assign any
// number of graphic objects to different states of the
// switch.
//======================================================
class BSwitch : public BControl, public BObservable
{
public:
					BSwitch(BRect frame, const char *name, BMGraphic *off=0, BMGraphic *on=0,
							BMGraphic *highlited=0, BMGraphic *pressing=0);
					BSwitch(BRect frame, const char *name, BBitmap *off, BBitmap *on=0,
							BBitmap *highlited=0, BBitmap *pressing=0);
	
	// From BView
	virtual void	Draw(BRect rect);
	
	virtual	void	MouseDown(BPoint where);
	virtual	void	MouseUp(BPoint where);
	virtual	void	MouseMoved(	BPoint where,
									uint32 code,
									const BMessage *a_message);

	
	// From BInvoker	
	virtual	status_t	Invoke(BMessage *msg = NULL);


	// Our own interface
			void	SetNotification(const int32 notice) { fNotice = notice;};
	virtual void	SetValue(int32);	
	virtual void	SetValue(const int32, const bool invoke);
	virtual void	SetGraphics(BMGraphic *step1, BMGraphic *step2=0, BMGraphic *highlited=0, BMGraphic *pressing=0);

	virtual int32	Value()const;

	virtual void	ValueChanged(const int32, const bool invoke = true);


protected:
	
	BMRangeStepper	fStepper;
	BMGraphicGroup	fGroup;
	BMGraphic	*fStep1Graphic;
	BMGraphic	*fStep2Graphic;
	BMGraphic	*fPressingGraphic;
	BMGraphic	*fHighlitedGraphic;
	int32		fNotice;
	bool		fIsDepressed;
	
private:
};


/*
	Interface: BMomentarySwitch

	This is a push button.  It will send out its message when the button
	is released.  If the user drags off of the control before releasing
	the button, then the message won't be sent.
*/

class BMomentarySwitch : public BSwitch
{
public:
					BMomentarySwitch(BRect frame, const char *name, BMGraphic *off=0, BMGraphic *on=0,
							BMGraphic *highlited=0, BMGraphic *pressing=0);
					BMomentarySwitch(BRect frame, const char *name, BBitmap *off, BBitmap *on=0,
							BBitmap *highlited=0, BBitmap *pressing=0);
	
	// From BView
	virtual	void	MouseUp(BPoint where);

protected:

private:

};


//======================================================
// Interface: BGTitledToggle
//
// This is a highly specialized class.  It is a toggle
// button that has a title above it.  The highliting
// behavior is to brighten the text as the highlite, and
// set it back to normal for non highlited.
//======================================================
class BTitledSwitch : public BSwitch
{
public:
					BTitledSwitch(const char *name, const char *title, const float size, const rgb_color normalColor, const rgb_color highlited, BMGraphic *step1=0, BMGraphic *step2=0);
	
	// From BMGraphic
	virtual void	Draw(BRect rect);
	
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
