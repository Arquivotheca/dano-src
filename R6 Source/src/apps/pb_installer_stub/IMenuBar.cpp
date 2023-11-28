#include "IMenuBar.h"
#include "MyDebug.h"

IMenuBar::IMenuBar(BRect frame,
				const char *title,
				ulong resizeMask,
				menu_layout layout,
				bool resizeToFit)
	: BMenuBar(frame,title,resizeMask,layout,resizeToFit)
{
	SetFlags(Flags() | B_FRAME_EVENTS);
	fMaxWidth = frame.Width();
}

void IMenuBar::FrameResized(float new_width, float new_height)
{
	if (new_width > fMaxWidth) {
		ResizeTo(fMaxWidth,new_height);
		Invalidate();
	}
	BMenuBar::FrameResized(new_width,new_height);
}

void IMenuBar::SetMaxWidth(float w)
{
	fMaxWidth = w;
}
