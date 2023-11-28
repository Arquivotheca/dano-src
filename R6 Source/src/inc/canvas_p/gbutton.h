#ifndef GBUTTON_H
#define GBUTTON_H

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


//======================================================
// Class Interface: BMAbsStateButton
//
// The idea behind a state button is that it can go through
// many states depending on the mouse and keyboard actions.
// These states may including highlited, pressed, inactive,
// or whatever specific states you may want to create.
//
// The BMAbsStateButton acts as the Abstract base class 
// for other button implementations.  A state transition
// object, plus some sort of interactor make for a good 
// button.
//======================================================

/*
class BMMomentaryButton : public BMGraphic, public BMMouseTracker
{
public:
					BMControl(const char *name="bmcontrol");
	
	virtual void	SetState(const int32);
	
			int32	State() const {return fState;};
			float	Position() const {return fPosition;};
			void	GetRange(float &min, float &max);
	
	virtual void	StateChanged();	// Must be implemented
	
protected:
	BMRangeStepper	fStepper;
	float	fMin;				// Minimum value slider can have
	float	fMax;				// Maximum value slider can have
	float	fCurrentValue;		// This is between fMin and fMax
	float	fPosition;			// The position is between 0.0 and 1.0
	
private:
};
*/


//======================================================
// A Active area may or may not have any visual
// representation.  You give it a frame and a notice
// to send out on a mouse click.
//======================================================

class BGActiveArea : public BMGraphic, public BMMouseTracker, public BObservable
{
public:
	BGActiveArea(const char *name, const BRect rect, const int32 notice);
	
	// From BMMouseTracker
	virtual	void	MouseDown(const BPoint where, BView *aView = 0, BGraphPort *aPort=0);
	virtual void	MouseUp(const BPoint where, BView *aView=0, BGraphPort *aPort=0);

protected:
	bool	fIsDepressed;
	int32	fNotice;
	
private:
};

//======================================================
// A switch changes from state to state
//======================================================
class BMSwitch : public BGraphicView, public BInvoker, public BObservable, public BMAbsHighlitable
{
public:
					BMSwitch(const char *name, BMGraphic *off=0, BMGraphic *on=0,
							BMGraphic *highlited=0, BMGraphic *pressing=0, const bool async=false);
					BMSwitch(const char *name, BBitmap *off, BBitmap *on=0,
							BBitmap *highlited=0, BBitmap *pressing=0, const bool async=false);
	
	// From BMGraphic
	virtual void	Draw(BGraphPort *, const BRect rect);
	
	// From BMAbsHighlitable
	virtual void	Highlite(BGraphPort *, const bool showHighlite=true);
	virtual const char * ShortDescription() {return BMGraphic::GraphicName().String();};
	virtual const char * LongDescription() {return BMGraphic::GraphicName().String();};

	// From BMMouseTracker
	virtual	void	MouseDown(const BPoint where, BView *aView = 0, BGraphPort *aPort=0);
	virtual void	MouseUp(const BPoint where, BView *aView=0, BGraphPort *aPort=0);
	
	virtual void	SetValue(const int32, const bool invoke = true);
	virtual int32	Value()const;

	virtual void	ValueChanged(const int32, const bool invoke = true);

	// From BInvoker	
	virtual	status_t	Invoke(BMessage *msg = NULL);

			void	SetNotification(const int32 notice) { fNotice = notice;};
			
protected:
	virtual void	SetGraphics(BMGraphic *step1, BMGraphic *step2=0, BMGraphic *highlited=0, BMGraphic *pressing=0);
	
	BMRangeStepper	fStepper;
	BMGraphic	*fStep1Graphic;
	BMGraphic	*fStep2Graphic;
	BMGraphic	*fPressingGraphic;
	BMGraphic	*fHighlitedGraphic;
	int32		fNotice;
	bool		fIsDepressed;
	
private:
};


//=========================================================
//
//=========================================================

class BMPushButton : public BGraphicView, public BInvoker, public BObservable, public BMAbsHighlitable
{
public:
					BMPushButton(const char *name, BMGraphic *normal=0, BMGraphic *active=0,
						BMGraphic *highlited=0, BMGraphic *pressing = 0, const bool async=false);
					BMPushButton(const char *name, BBitmap *off, BBitmap *on=0, BBitmap *highlited=0, BBitmap *pressing = 0, const bool async=false);
	
	virtual void	Draw(BGraphPort *, const BRect rect);
	
	// From BMAbsHighlitable
	virtual void	Highlite(BGraphPort *, const bool showHighlite=true);
	virtual const char * ShortDescription() {return GraphicName().String();};
	virtual const char * LongDescription() {return GraphicName().String();};

	virtual	void	AsyncMouseDown(const BPoint where, BView *aView = 0, BGraphPort *aPort=0);
	virtual	void	MouseDown(const BPoint where, BView *aView = 0, BGraphPort *aPort=0);
	virtual	void	MouseMoved(	const BPoint where, uint32 code,
								const BMessage *a_message, BView *aView = 0, BGraphPort *aPort=0);
	virtual void	MouseUp(const BPoint where, BView *aView=0, BGraphPort *aPort=0);
	

	virtual void	SetValue(const int32, const bool invoke=true);
	virtual int32	Value()const;
	virtual void	ValueChanged(const int32, const bool invoke=true);

	// From BInvoker	
	virtual	status_t	Invoke(BMessage *msg = NULL);

			void	SetNotification(const int32 notice) { fNotice = notice;};
		
protected:
	virtual void	SetGraphics(BMGraphic *step1, BMGraphic *step2=0, BMGraphic *highlited=0, BMGraphic *pressing=0);
	
	BMRangeStepper	fStepper;
	BMGraphic	*fStep1Graphic;
	BMGraphic	*fStep2Graphic;
	BMGraphic	*fHighlitedGraphic;
	BMGraphic	*fPressingGraphic;
	int32		fNotice;
	bool		fIsDepressed;
	bool		fIsAsync;
	
private:
};


//======================================================
// Class Interface: BMTextSwitch
//
// A text switch takes a single string and creates a
// switch out of it.  It will take two colors to represent 
// on and off states.
//======================================================

const rgb_color gblack = {0,0,0,255};
const rgb_color gwhite = {255,255,255,255};

class BMTextSwitch : public BMSwitch
{
public:
				BMTextSwitch(const char *name=0, const char *title=0, const float size=10.0,
					const rgb_color highcolor=gblack, 
					const rgb_color lowcolor=gwhite);
				
protected:
	rgb_color	fHighColor;
	rgb_color	fLowColor;
	
private:
};





#endif