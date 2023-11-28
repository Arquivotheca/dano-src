// KnobSwitch.h
//
//   Stolen from the Tracker's DialogPane.h, and modified to display bitmaps
//   of a little knob.
//

#ifndef _KNOB_SWITCH_H_
#define _KNOB_SWITCH_H_

#include <Control.h>

class BBitmap;

class KnobSwitch : public BControl {

public:
		KnobSwitch(BRect frame, const char *name,
				uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
				uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
		virtual ~KnobSwitch();
		
		virtual	void MouseMoved( BPoint where, uint32 code, const BMessage *msg );
		virtual void Draw(BRect );
		virtual void MouseDown(BPoint );
		virtual	void MouseUp( BPoint where );
		virtual void KeyDown(const char *bytes, int32 numBytes);
		virtual void GetPreferredSize(float *width, float *height);
		virtual void AttachedToWindow();
		virtual void SetValue(int32 value);
protected:

		enum State {
				kUp,
				kMiddle,
				kDown
		};

		virtual void DrawInState(KnobSwitch::State state);
		virtual State StateForPosition(BPoint mousePos);
		
		bool fPressing;
		bool fMovedSinceClick;
		
		State fStateWhenPressed, fCurrState;
		BPoint fClickPoint;

		BBitmap *fUpBmap, *fMiddleBmap, *fDownBmap;
};

#endif // _KNOB_SWITCH_H_
