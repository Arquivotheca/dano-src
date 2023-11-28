
#include "GehmlBWindow.h"

GehmlBWindow::GehmlBWindow(
	const BRect &frame,
	window_look look,
	window_feel feel,
	uint32 flags,
	uint32 workspace)
{
	SetSize(frame.right-frame.left+1,frame.bottom-frame.top+1);
	Constraints()->axis[HORIZONTAL].pos = frame.left;
	Constraints()->axis[VERTICAL].pos = frame.right;
	OpenSession(look,feel,flags,workspace);
}

GehmlBWindow::~GehmlBWindow()
{
}

void 
GehmlBWindow::Acquired()
{
	GehmlWindow::Acquired();
}

status_t 
GehmlBWindow::HandleMessage(BMessage *msg)
{
	return GehmlWindow::HandleMessage(msg);
}

void 
GehmlBWindow::SetVisible(bool visibility)
{
	if (visibility && !Parent()) SetParent(rootWindow);
	else if (!visibility && Parent()) SetParent(NULL);
}

