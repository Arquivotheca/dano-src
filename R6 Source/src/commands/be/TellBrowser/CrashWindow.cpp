
#include "CrashWindow.h"

#include <View.h>
#include <Screen.h>
#include <OS.h>
#include <string.h>

CrashWindow::CrashWindow(bool quitenabled) :
	BWindow(BScreen().Frame(), "CrashWin",
		B_NO_BORDER_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL, B_NOT_CLOSABLE)
{
	BRect frame = BScreen().Frame();
	BView *view = new BView(Bounds(), "transparent_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	view->SetViewColor(B_TRANSPARENT_32_BIT);
	AddChild(view);
	if (!quitenabled) RemoveShortcut('Q', B_COMMAND_KEY);
	Hide();
	Show();
}

void 
CrashWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case SHOW_MESSAGE:
//			printf("CrashWindow::SHOW_MESSAGE recvd\n");
			if (IsHidden()) Show();
			break;
		
		case HIDE_MESSAGE:
//			printf("CrashWindow::HIDE_MESSAGE recvd\n");
			if (!IsHidden()) Hide();
			break;

		default:	
			BWindow::MessageReceived(msg);
	}
}

