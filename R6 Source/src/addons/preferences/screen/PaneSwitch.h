// PaneSwitch.h
//
//   Theft history:
//   Stolen from Installer and made asynchronous
//   Stolen from the Tracker's DialogPane.h and hacked
//

#ifndef _PANE_SWITCH_H_
#define _PANE_SWITCH_H_

#include <Control.h>

class IPaneSwitch : public BControl
{
public:
		IPaneSwitch(BRect frame, const char *name,
				uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
				uint32 flags = B_WILL_DRAW | B_NAVIGABLE);

		virtual void Draw(BRect );
		virtual void MouseDown(BPoint point);
		virtual	void MouseMoved(BPoint point, uint32 code, const BMessage *a_message);
		virtual void MouseUp(BPoint point);
		virtual void KeyDown(const char *bytes, int32 numBytes);

protected:

		enum State {
				kCollapsed,
				kPressed,
				kExpanded
		};

		virtual void DrawInState(IPaneSwitch::State state);

		bool pressing;
		bool tracking;
};

#endif // _PANE_SWITCH_H_
