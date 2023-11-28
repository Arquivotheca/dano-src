#ifndef INTERACTIVEGRAPHIC_H
#define INTERACTIVEGRAPHIC_H

#include "graphicview.h"
#include "observable.h"

// Classes we make reference to
class BMAbsHighlitable;


enum ETrackingEvents
{
	TRACKING_ENTERED,
	TRACKING_EXITED,
	TRACKING_MOVED
};


/*

	Interface: BMessageCatcher
	
	Defines a API that anyone who wants to accept message drops
	can implement.  The MessageDropped method will be called
	with the message that was dropped.
*/

class BMessageCatcher
{
public:
	virtual void	MessageDropped(BMessage *msg, const BPoint where);

protected:
private:
};



/*
	Interface: BMInteractiveGraphicGroup
*/

class BMInteractiveGraphicGroup : public BGraphicView, public BObservable, public BMessageCatcher
{
public:
			BMInteractiveGraphicGroup(const char *name=0, const BRect frame=BRect(0,0,0,0));
	
	virtual void	SetGraphPort(BGraphPort *);
	
	virtual BMMouseTracker	* GetMouseTrackerAt(const BPoint);

	virtual	void	AsyncMouseDown(const BPoint where, BView *aView = 0, BGraphPort *aPort=0);
	
	// Inherited from BMMouseTracker
	virtual	void	MouseDown(const BPoint where, BView *aView = 0, BGraphPort *aPort=0);
	virtual	void	MouseUp(const BPoint where, BView *aView = 0, BGraphPort *aPort=0);
	virtual	void	MouseMoved(	const BPoint where, uint32 code,
								const BMessage *a_message, BView *aView = 0, BGraphPort *aPort=0);

	// From BMessageCatcher
	virtual void	MessageDropped(BMessage *msg, const BPoint where);
	
	
protected:
	BMAbsHighlitable	*fLastHighlited;		// Last graphic to be highlited
	BMMouseTracker		*fFirstResponder;		// The graphic that has the focus
	
private:
};

// Function to find a mouse tracker in a group.  It will find the
// topmost tracker if there is one.
BMMouseTracker *
GetMouseTrackerAt(BMGraphicGroup *aGroup, const BPoint where);

#endif
