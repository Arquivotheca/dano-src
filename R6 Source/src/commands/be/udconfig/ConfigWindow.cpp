//
//	Copyright 2000 Be, Incorporated.  All Rights Reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <Application.h>
#include <Message.h>
#include <Invoker.h>
#include <ScrollView.h>
#include <Box.h>
#include <TabView.h>

#include "ConfigWindow.h"
#include "ConfigView.h"
#include "MountView.h"

static const BRect kWinRect(200.0, 200.0, 800.0, 600.0);

enum {
	MOUNT_STATE,
	EDIT_STATE
};

#define MSG_MOUNT_DONE	'mMNT'
#define MSG_EDIT_DONE	'mEDT'

int
get_state(const char *fname)
{
	struct stat st;
	int r;

	if(stat(fname, &st) < 0) {
		return MOUNT_STATE;
	}
	else {
		return EDIT_STATE;
	}
}

ConfigWindow::ConfigWindow(BRect /*rect*/, const char *title, const char *fname) :
		BWindow(kWinRect, title, B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS),
		fEm(be_bold_font->StringWidth("m")),
		fFileName(NULL)
{
	fBgBox = new BBox(Bounds(), "Box:Background",
			B_FOLLOW_ALL_SIDES,
			B_WILL_DRAW|B_FRAME_EVENTS|B_NAVIGABLE_JUMP,
			B_PLAIN_BORDER);
	AddChild(fBgBox);

	fFileName = strdup(fname);

	int s = get_state(fFileName);
	if(s == MOUNT_STATE) {
		MountState(false);
	}
	else {
		EditState(false);
	}
}


ConfigWindow::~ConfigWindow()
{
}

void
ConfigWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
	case MSG_MOUNT_DONE:
		MountState(true);
		EditState(false);
		break;
	case MSG_EDIT_DONE:
		EditState(true);
		be_app->PostMessage(B_QUIT_REQUESTED, be_app);
		break;
	default:
		// not handling this msg
		BWindow::MessageReceived(msg);
		break;
	}
}


bool
ConfigWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED, be_app);
	return true;
}

void
ConfigWindow::MountState(bool teardown)
{
	if(teardown) {
		fMountView->RemoveSelf();
	}
	else {
		BInvoker *invoker = new BInvoker(new BMessage(MSG_MOUNT_DONE), this);
		BRect r = fBgBox->Bounds().InsetByCopy(fEm,fEm);

		fMountView = new MountView(r, "View:Mount", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS, fFileName, invoker);
		if(fMountView->InitCheck() != B_OK) {
			be_app->PostMessage(B_QUIT_REQUESTED, be_app);
			return;
		}
		fBgBox->AddChild(fMountView);
	}
}

void
ConfigWindow::EditState(bool teardown)
{
	if(teardown) {
		fConfigView->RemoveSelf();
	}
	else {
		BInvoker *invoker = new BInvoker(new BMessage(MSG_EDIT_DONE), this);
		BRect r = fBgBox->Bounds().InsetByCopy(fEm,fEm);

		fConfigView = new ConfigView(r, "View:Config", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS, fFileName, invoker);
		if(fConfigView->InitCheck() != B_OK) {
			be_app->PostMessage(B_QUIT_REQUESTED, be_app);
			return;
		}
		fBgBox->AddChild(fConfigView);
	}
}
