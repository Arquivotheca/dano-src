// Simple window that exists solely to display the debugger() message

#include "DDebugMessageWin.h"
#include <TextView.h>
#include <String.h>

static BRect sWindowRect(100, 150, 600, 250);

class ResizingTextView : public BTextView
{
public:
	ResizingTextView(BRect bounds, const char* name, BRect textRect,
		uint32 resizeMask, uint32 flags = B_FRAME_EVENTS | B_WILL_DRAW | B_PULSE_NEEDED);

	void FrameResized(float width, float height);
};

ResizingTextView::ResizingTextView(BRect bounds, const char *name, BRect textRect, uint32 resizeMask, uint32 flags)
	: BTextView(bounds, name, textRect, resizeMask, flags)
{
}

void 
ResizingTextView::FrameResized(float width, float height)
{
	BTextView::FrameResized(width, height);
	SetTextRect(Bounds());
}

// #pragma mark -
DDebugMessageWin::DDebugMessageWin(const BString &msg)
	: BWindow(sWindowRect, "Debugger Message", B_TITLED_WINDOW, 0)
{
	fMessageView = new ResizingTextView(Bounds(), "dbgmsg", Bounds(), B_FOLLOW_ALL_SIDES);
	fMessageView->SetText(msg.String());
	fMessageView->MakeEditable(false);
	fMessageView->MakeSelectable(true);
	AddChild(fMessageView);
}

DDebugMessageWin::~DDebugMessageWin()
{
	sWindowRect = Frame();
}
