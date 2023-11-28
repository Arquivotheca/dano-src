#ifndef PANE_SWITCH_H
#define PANE_SWITCH_H

#include <Control.h>

class PaneSwitch : public BControl {

public:
	PaneSwitch(BRect frame, const char *name, bool leftAligned = true,
		uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP, 
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE); 

	virtual	void Draw(BRect );
	virtual	void MouseDown(BPoint );
protected:

	void DoneTracking(BPoint );
	void Track(BPoint, uint32);

	enum State {
		kCollapsed,
		kPressed,
		kExpanded
	};

	virtual void DrawInState(PaneSwitch::State state);
	
	bool leftAligned;
	bool pressing;
};

#endif