// LicenseWindow.cpp
#include "LicenseWindow.h"
#include "LicenseView.h"
#include "Util.h"

#include "MyDebug.h"

LicenseWindow::LicenseWindow(char *text, bool *c, size_t sz, char *styles)
	:	BWindow(BRect(0,0,600,400),"license",
				B_MODAL_WINDOW,B_NOT_CLOSABLE),
		canceled(c)
{
	Lock();

	*canceled = FALSE;	
	
	PositionWindow(this,0.5,0.5);
	SetSizeLimits(300,8192,200,8192);
	
	AddChild(new LicenseView(Bounds(),text,sz,styles));
	
	Unlock();
	Show();
}

bool LicenseWindow::QuitRequested()
{
	BMessage *c = CurrentMessage();
	if (c->HasBool("cancelled"))
		*canceled = TRUE;

	return TRUE;
}
