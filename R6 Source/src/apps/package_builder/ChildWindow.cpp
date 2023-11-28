#include <Be.h>


#include "ChildWindow.h"
#include "Util.h"

#include "MyDebug.h"

ChildWindow::ChildWindow(BRect frame,
				const char *title,
				window_type type,
				ulong flags, BWindow *parentWindow, ulong workspace)
	: 	BWindow(frame,title,type,flags,workspace),
		parentWin(parentWindow),
		fWindowDirty(FALSE)
{
	parentTitle = NULL;
	realTitle = NULL;

	UpdateParTitle(parentWin->Title());
	Lock();

	PositionWindow(this,0.5,0.5);
	SetTitle(title);
	
	// don't show the window becaus this is a base class
	Unlock();
}

ChildWindow::~ChildWindow()
{
	if (parentTitle)
		delete parentTitle;
	if (realTitle)
		delete realTitle;
}


void ChildWindow::SetTitle(const char *nTitle)
{
	long nlen, plen;
	
	if (nTitle) {
		if (realTitle)
			delete realTitle;
		long len = strlen(nTitle);
		realTitle = new char[len+1];
		strcpy(realTitle,nTitle);
	}
	
	nlen = strlen(realTitle);
	plen = strlen(parentTitle);
	char *buf = new char[nlen + plen + 4];
	
	sprintf(buf,"%s \"%s\"",realTitle,parentTitle);

	BWindow::SetTitle(buf);
	
	delete buf;
}

void ChildWindow::SetDirty(bool state)
{
	fWindowDirty = state;
}

void ChildWindow::DispatchMessage(BMessage *msg,BHandler *hand)
{
	const char *newTitle;
	
	switch (msg->what) {
		case M_DO_ACTIVATE:
			Activate(TRUE);
			break;
		case M_NEW_TITLE:
			PRINT(("got title message\n"));
			newTitle = msg->FindString("title");
			PRINT(("new title is %s\n",newTitle));
			if (!newTitle)
				break;
			UpdateParTitle(newTitle);
			SetTitle(NULL);
			break;
		default:
			BWindow::DispatchMessage(msg,hand);
	}
} 


bool ChildWindow::QuitRequested()
{
	BMessage *msg = CurrentMessage();
	if (msg->IsSourceWaiting()) {
		BMessage reply(M_QUIT_FULFILLED);
		reply.AddBool("result",TRUE);
		msg->SendReply(&reply);
	}
	return BWindow::QuitRequested();
}

void ChildWindow::UpdateParTitle(const char *parTitle)
{
	long len = strlen(parTitle);
	if (parentTitle) delete parentTitle;
	parentTitle = new char[len+1];
	strcpy(parentTitle,parTitle);
}
