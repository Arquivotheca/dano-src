#ifndef __DWATCHPOINT_WINDOW__
#define __DWATCHPOINT_WINDOW__

#include <Window.h>
#include "DWatchpoint.h"

class DListBox;
class BMessage;

// ToDo:
// inherit DBreakpointWindow from DTracepointWindow

class DTracepointWindow : public BWindow {
public:
	DTracepointWindow(BRect frame, const char *name, window_look look,
		window_feel feel, uint32 flags,  BWindow *owner)
		:	BWindow(frame, name, look, feel, flags),
			fOwner(owner)
		{}

	virtual void ToggleTracepoint(DTracepoint *) = 0;

protected:
	BWindow *fOwner;
};


class DWatchpointWindow : public DTracepointWindow {
 public:
	DWatchpointWindow(BWindow *owner);

	void ClearWatchpoint(ptr_t pc);
	void EnableWatchpoint(ptr_t pc);
	void DisableWatchpoint(ptr_t pc);
	

protected:
	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage *);
	virtual void ToggleTracepoint(DTracepoint *);

private:

//	void DisplayWatchpoint();
	void DeleteWatchpoint();
	void ToggleWatchpoint();

	void ReloadWatchpoints();

	DListBox *fList;
};

#endif

