#ifndef GSLIDER_H
#define GSLIDER_H

#include "graphic.h"
#include "interactor.h"
#include "observable.h"
#include "highlites.h"
#include "graphicview.h"

#include <Invoker.h>
#include <InterfaceDefs.h>

//======================================================
// Abstract Class Interface: BAbsSlider
//
// This class implements the protocol for a slider class.
// If you want to create a slider, then you should
// inherit from this class and implement the specific
// functions that you need.
//======================================================
class BMAbsSlider
{
public:
					BMAbsSlider(const float min, const float max, const float initialValue=0.0);
	
	virtual void	SetValue(const float, const bool invoke=true);
	virtual void	SetValueBy(const BPoint, const bool invoke = true){};
	virtual void	SetPosition(const float, const bool invoke = true);
	virtual void	SetPositionBy(const BPoint, const bool invoke = true){};
	virtual void	SetRange(const float min, const float max);
	
			float	Value() const {return fCurrentValue;};
			float	Position() const {return fPosition;};
			void	GetRange(float &min, float &max);
	
	virtual void	ValueChanged(const float, const bool invoke = true){};
	virtual void	PositionChanged(const float, const bool invoke = true){};	// Must be implemented
	
protected:
	double	fMin;				// Minimum value slider can have
	double	fMax;				// Maximum value slider can have
	double	fCurrentValue;		// This is between fMin and fMax
	double	fPosition;			// The position is between 0.0 and 1.0
	
private:
};

//======================================================
// Abstract Class Interface: BMGSlider
//
// This class acts as the base class for something that 
// will truly be a control in the Be sense.  It implements
// all the graphic stuff as well as the Invoker so that a 
// message can be sent to someone who is interested.
//======================================================
class BMGSlider : public BGraphicView, public BMAbsSlider, public BInvoker, public BObservable, public BMAbsHighlitable
{
public:
				BMGSlider(const char *name, const BRect rect=BRect(0,0,0,0), 
					const float min=0.0, const float max=1.0, 
					const float defaultValue=0.0);
	
	// A little something of our own
	virtual void	SetEnabled(const bool enabled=true);
	virtual void	SetActive(const bool active=true);
	
	virtual void	Draw(BGraphPort *, const BRect rect=BRect(0,0,0,0));
	
protected:
	bool		fIsEnabled;
	bool		fIsActive;
	
private:
};

/*
	Interface: BGTrackSlider
	
	A TrackSlider is your typical horizontal or vertical slider.
	It takes two graphics which represent the track and the thumb
	that slides in that track.  You can set the orientation to be
	either horzontal or vertical.
*/

class BGTrackSlider : public BMGSlider
{
public:
				BGTrackSlider(const char *name,
					BMGraphic *track, BMGraphic *thumb,
					const orientation orient,
					const float min=0.0, const float max=1.0, 
					const float defaultValue=0.0);
	virtual		~BGTrackSlider();
				
	// From BMMouseTracker
	virtual	void	MouseDown(const BPoint where, BView *aView = 0, BGraphPort *aPort=0);
	virtual	void	MouseUp(const BPoint where, BView *aView = 0, BGraphPort *aPort=0);
	virtual	void	MouseMoved(	const BPoint where, uint32 code,
								const BMessage *a_message, BView *aView = 0, BGraphPort *aPort=0);

	virtual void	SetPositionBy(const BPoint, const bool invoke = true);
	virtual void	SetValueBy(const BPoint, const bool invoke = true);

	virtual void	ValueChanged(const float, const bool invoke = true);
	virtual void	PositionChanged(const float, const bool invoke = true);	// Must be implemented

	// For the BObservable, this is the notification that will be sent out
	// whenever the slider is moved.
			void	SetNotification(const int32 notice) { fNotice = notice;};

protected:
	BMGraphic	*fTrackGraphic;
	BMGraphic	*fThumbGraphic;

	orientation	fOrientation;
	int32		fNotice;
	
private:
};

#endif
