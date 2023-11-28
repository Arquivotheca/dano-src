// PaneSwitch.h
//
//   Stolen from the Tracker's DialogPane.h
//

#ifndef _PANE_SWITCH_H_
#define _PANE_SWITCH_H_

#include <Control.h>
#include "Thread.h" // for MouseDownThread

class IPaneSwitch : public BControl {

public:
		IPaneSwitch(BRect frame, const char *name,
				uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
				uint32 flags = B_WILL_DRAW | B_NAVIGABLE);

		virtual void Draw(BRect );
		virtual void MouseDown(BPoint );
		virtual void KeyDown(const char *bytes, int32 numBytes);
protected:

		void DoneTracking(BPoint );
		void Track(BPoint, uint32);

		enum State {
				kCollapsed,
				kPressed,
				kExpanded
		};

		virtual void DrawInState(IPaneSwitch::State state);

		bool pressing;
};

#endif // _PANE_SWITCH_H_
