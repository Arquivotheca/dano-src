#ifndef __THREAD__
#define __THREAD__

#include <Debug.h>
#include <OS.h>
#include <Window.h>

template<class View>
class MouseDownThread {
public:
	static void TrackMouse(View *view, void (View::*)(BPoint),
		void (View::*)(BPoint, uint32) = 0, bigtime_t pressingPeriod = 100000);

protected:
	MouseDownThread(View *view, void (View::*)(BPoint),
		void (View::*)(BPoint, uint32), bigtime_t pressingPeriod);

	virtual ~MouseDownThread();
	
	void Go();
	virtual void Track();
	
	static status_t TrackBinder(void *);
private:
	
	BWindow *parent;
	View *view;
	void (View::*donePressing)(BPoint);
	void (View::*pressing)(BPoint, uint32);
	bigtime_t pressingPeriod;
	thread_id threadID;
};


template<class View>
void
MouseDownThread<View>::TrackMouse(View *view,
	void(View::*donePressing)(BPoint),
	void(View::*pressing)(BPoint, uint32), bigtime_t pressingPeriod)
{
	(new MouseDownThread(view, donePressing, pressing, pressingPeriod))->Go();
}


template<class View>
MouseDownThread<View>::MouseDownThread(View *view,
	void (View::*donePressing)(BPoint),
	void (View::*pressing)(BPoint, uint32), bigtime_t pressingPeriod)
	:	view(view),
		donePressing(donePressing),
		pressing(pressing),
		pressingPeriod(pressingPeriod)
{
	parent = view->Window();
}


template<class View>
MouseDownThread<View>::~MouseDownThread()
{
	if (threadID > 0) {
		kill_thread(threadID);
		// dead at this point
		TRESPASS();
	}
}


template<class View>
void 
MouseDownThread<View>::Go()
{
	threadID = spawn_thread(&MouseDownThread::TrackBinder, "MouseTrackingThread",
		B_NORMAL_PRIORITY, this);
	
	if (threadID <= 0 || resume_thread(threadID) != B_OK)
		// didn't start, don't leak self
		delete this;
}

template<class View>
status_t 
MouseDownThread<View>::TrackBinder(void *castToThis)
{
	MouseDownThread *self = (MouseDownThread *)castToThis;
	self->Track();
	// dead at this point
	TRESPASS();
	return B_NO_ERROR;
}

template<class View>
void 
MouseDownThread<View>::Track()
{
	for (;;) {
		if (!parent->Lock())
			break;
		
		uint32 buttons;
		BPoint location;
		view->GetMouse(&location, &buttons, false);
		if (!buttons) {
			(view->*donePressing)(location);
			parent->Unlock();
			break;
		}
		if (pressing)
			(view->*pressing)(location, buttons);
		
		parent->Unlock();
		snooze(pressingPeriod);
	}
	
	delete this;
	ASSERT(!"should not be here");
}


#endif
