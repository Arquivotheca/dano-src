// DWaitWindow.cpp

#include "DWaitWindow.h"
#include <TextView.h>

DWaitWindow::DWaitWindow(BRect frame, const char *msg)
	: BWindow(frame, "Please wait", B_MODAL_WINDOW, 0)
{
	BRect r = Bounds();
	r.InsetBy(3, 3);
	mText = new BTextView(r, "msg", r, 0);
	AddChild(mText);
	mText->MakeEditable(false);
	mText->MakeSelectable(false);
	mText->SetText(msg);
}
