#ifndef INTERACTOR_H
#define INTERACTOR_H

#include <Point.h>

// Declarations of things we use
class BMessage;
class BView;
class BGraphPort;

// Things created in here
class BMMouseInteractor;

//================================================================
// Interface: BMMouseInteractor
//
// An interactor is just a little bit of defined functionality
// that is invoked for user interaction.  In this case, the
// MouseInteractor is invoked whenever you want to deal with mouse
// interaction.  You will typically use it like this.
//
// myview::MouseDown(const BPoint pt)
// {
// 		BMMouseInteractor actor(this, pt);
//		actor.Run();
// }
//================================================================

class BMMouseTracker
{
public:
			BMMouseTracker(BGraphPort *aPort=0, const BPoint aPoint=BPoint(0,0), const bool async=false);
			
	virtual	void	MouseDown(const BPoint where, BView *aView = 0, BGraphPort *aPort = 0);
	virtual	void	MouseUp(const BPoint where, BView *aView = 0, BGraphPort *aPort = 0);
	virtual	void	MouseMoved(	const BPoint where, uint32 code,
								const BMessage *a_message, BView *aView = 0, BGraphPort *aPort = 0);

	virtual bool	IsTracking(){return fIsTracking;};
	virtual bool	IsActive() { return fIsActive;};
	
	virtual void	SetActive(const bool active=true) {fIsActive = active;};
	
protected:
	bool	fIsActive;
	bool	fIsAsync;
	BGraphPort	*fGraphPort;
	BPoint	fStartPoint;
	BPoint	fLastPoint;
	bool	fIsTracking;
	
private:
};


#endif
