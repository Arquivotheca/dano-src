// PaneSwitch.cpp
//
//   Stolen from the Tracker's DialogPane.cpp
//

#include "PaneSwitch.h"
//#include <interface_misc.h> // for general_info

//const rgb_color view_background_color = {216, 216, 216, 255};
//const rgb_color kBlack = {0, 0, 0, 255};
static const rgb_color kWhite = {255, 255, 255, 255};
static const rgb_color kNormalColor = {150, 150, 150, 255};
static const rgb_color kHighlightColor = {100, 100, 0, 255};


IPaneSwitch::IPaneSwitch(BRect frame, const char *name,
	uint32 resizeMask, uint32 flags)
	:	BControl(frame, name, "", 0, resizeMask, flags),
		pressing(false)
{
}

void 
IPaneSwitch::DoneTracking(BPoint point)
{
	BRect bounds(Bounds());
	bounds.InsetBy(-3, -3);

	pressing = false;
	Invalidate();
	if (bounds.Contains(point)) {
		SetValue(!Value());
		Invoke();
	}
}

void 
IPaneSwitch::Track(BPoint point, uint32)
{
	BRect bounds(Bounds());
	bounds.InsetBy(-3, -3);

	bool newPressing = bounds.Contains(point);
	if (newPressing != pressing) {
		pressing = newPressing;
		Invalidate();
	}
}


void 
IPaneSwitch::MouseDown(BPoint)
{
	if (!IsEnabled())
		return;
	
	pressing = true;
	MouseDownThread<IPaneSwitch>::TrackMouse(this, &IPaneSwitch::DoneTracking,
		&IPaneSwitch::Track);
	Invalidate();
}

// make sure that we redraw after the keyboard changes our state
void 
IPaneSwitch::KeyDown(const char *bytes, int32 numBytes)
{
	BControl::KeyDown(bytes, numBytes);
	Invalidate();
}


void
IPaneSwitch::Draw(BRect)
{
	if (pressing)
		DrawInState(kPressed);
	else if (Value())
		DrawInState(kExpanded);
	else
		DrawInState(kCollapsed);


	rgb_color mark_color = ui_color(B_KEYBOARD_NAVIGATION_COLOR);
	
	bool focused = IsFocus() && Window()->IsActive();
	BRect bounds(Bounds());
	BeginLineArray(2);
	AddLine(BPoint(bounds.left + 2, bounds.bottom - 1),
		BPoint(bounds.right - 2, bounds.bottom - 1), focused ? mark_color : ViewColor());
	AddLine(BPoint(bounds.left + 2, bounds.bottom),
		BPoint(bounds.right - 2, bounds.bottom), focused ? kWhite : ViewColor());
	EndLineArray();
}

void 
IPaneSwitch::DrawInState(IPaneSwitch::State state)
{
	BRect rect(0, 0, 10, 10);

	rgb_color outlineColor = {0, 0, 0, 255};
	rgb_color middleColor = state == kPressed ? kHighlightColor : kNormalColor;


	SetDrawingMode(B_OP_COPY);
	
	switch (state) {
		case kCollapsed:
			BeginLineArray(6);
			AddLine(BPoint(rect.left + 3, rect.top + 1), 
				BPoint(rect.left + 3, rect.bottom - 1), outlineColor);
			AddLine(BPoint(rect.left + 3, rect.top + 1), 
				BPoint(rect.left + 7, rect.top + 5), outlineColor);
			AddLine(BPoint(rect.left + 7, rect.top + 5), 
				BPoint(rect.left + 3, rect.bottom - 1), outlineColor);
				
			AddLine(BPoint(rect.left + 4, rect.top + 3), 
				BPoint(rect.left + 4, rect.bottom - 3), middleColor);
			AddLine(BPoint(rect.left + 5, rect.top + 4), 
				BPoint(rect.left + 5, rect.bottom - 4), middleColor);
			AddLine(BPoint(rect.left + 5, rect.top + 5), 
				BPoint(rect.left + 6, rect.top + 5), middleColor);
			EndLineArray();
			break;

		case kPressed:
			BeginLineArray(7);
			AddLine(BPoint(rect.left + 1, rect.top + 7), 
				BPoint(rect.left + 7, rect.top + 7), outlineColor);
			AddLine(BPoint(rect.left + 7, rect.top + 1), 
				BPoint(rect.left + 7, rect.top + 7), outlineColor);
			AddLine(BPoint(rect.left + 1, rect.top + 7), 
				BPoint(rect.left + 7, rect.top + 1), outlineColor);
				
			AddLine(BPoint(rect.left + 3, rect.top + 6), 
				BPoint(rect.left + 6, rect.top + 6), middleColor);
			AddLine(BPoint(rect.left + 4, rect.top + 5), 
				BPoint(rect.left + 6, rect.top + 5), middleColor);
			AddLine(BPoint(rect.left + 5, rect.top + 4), 
				BPoint(rect.left + 6, rect.top + 4), middleColor);
			AddLine(BPoint(rect.left + 6, rect.top + 3), 
				BPoint(rect.left + 6, rect.top + 4), middleColor);
			EndLineArray();
			break;

		case kExpanded:
			BeginLineArray(6);
			AddLine(BPoint(rect.left + 1, rect.top + 3), 
				BPoint(rect.right - 1, rect.top + 3), outlineColor);
			AddLine(BPoint(rect.left + 1, rect.top + 3), 
				BPoint(rect.left + 5, rect.top + 7), outlineColor);
			AddLine(BPoint(rect.left + 5, rect.top + 7), 
				BPoint(rect.right - 1, rect.top + 3), outlineColor);

			AddLine(BPoint(rect.left + 3, rect.top + 4), 
				BPoint(rect.right - 3, rect.top + 4), middleColor);
			AddLine(BPoint(rect.left + 4, rect.top + 5), 
				BPoint(rect.right - 4, rect.top + 5), middleColor);
			AddLine(BPoint(rect.left + 5, rect.top + 5), 
				BPoint(rect.left + 5, rect.top + 6), middleColor);
			EndLineArray();
			break;
	}
}
