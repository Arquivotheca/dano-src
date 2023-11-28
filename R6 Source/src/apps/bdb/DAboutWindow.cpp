// DAboutWindow

#include "DAboutWindow.h"
#include <TextView.h>
#include <stdio.h>

DAboutWindow::DAboutWindow(BRect frame)
	: BWindow(frame, "About bdb...", B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	char buf[128];
	sprintf(buf, "\nbdb - the Be debugger\n\nVersion %d.%d\n\n"
		"by Maarten Hekkelman, John Dance, Christopher Tate, and various others.",
		MAJOR_VERSION, MINOR_VERSION);
	BRect r = Bounds();
	r.InsetBy(3, 3);
	BTextView* tv = new BTextView(Bounds(), "z", r, 0);
	tv->SetAlignment(B_ALIGN_CENTER);
	tv->SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	tv->SetHighUIColor(B_UI_PANEL_TEXT_COLOR);
	tv->MakeSelectable(false);
	tv->MakeEditable(false);
	tv->SetText(buf);
	AddChild(tv);
}

bool
DAboutWindow::QuitRequested()
{
	Hide();
	return false;
}

// actual storage for this global window pointer
DAboutWindow* gAboutWin;
