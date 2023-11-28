/*
	ProgressWnd.cpp
	Implementation.
*/

#include <stdio.h>
#include <string.h>
#include <Screen.h>
#include <Autolock.h>
#include <View.h>
#include <StatusBar.h>
#include <Button.h>
#include "ProgressWnd.h"
#include "appconfig.h"
#include "CameraStrings.h"

class ProgressView : public BView {
public:
	ProgressView(BRect r, int32 numPics) :
		BView(r, NULL, B_FOLLOW_ALL, B_WILL_DRAW)
	{
		SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
		AddChild(new BStatusBar(BRect(7, 7, 7 + 304, 7 + 32),
				"PictureProgress",
				STR_PICTURE1,
				"0%"));
		if (numPics > 1)
		{
			AddChild(new BStatusBar(BRect(7, 47, 7 + 304, 47 + 32),
					"AllProgress",
					STR_ALLPICTURES,
					STR_ONEOFONE));
			AddChild(new BButton(BRect(125, 85, 125 + 66, 85 + 24),
					"Cancel", STR_CANCEL, new BMessage(MSG_CANCEL)));
		}
		else
		{
			AddChild(new BButton(BRect(125, 53, 125 + 66, 53 + 24),
					"Cancel", STR_CANCEL, new BMessage(MSG_CANCEL)));
		}
	}
};

ProgressWnd::ProgressWnd(sem_id cancelSem, int32 numPics) :
	BWindow(BRect(60, 120, 60 + 320, 120 + 116), STR_PROGRESS, B_MODAL_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	if (numPics < 2)
	{
		ResizeTo(320, 84);
	}

	fFrameID = 0;
	fTotal = 1;
	fCancel = cancelSem;
	acquire_sem(fCancel);
	AddChild(new ProgressView(Bounds(), numPics));

	// center the window
	BScreen		screen(this);
	BRect		r = screen.Frame();
	BRect		wndR = Frame();
	MoveTo((r.left + r.Width() / 2) - wndR.Width() / 2,
		(r.top + r.Height() / 2) - wndR.Height() / 2);
	Show();
}

ProgressWnd::~ProgressWnd()
{
	release_sem(fCancel);
}

void ProgressWnd::MessageReceived(BMessage *msg)
{
	BStatusBar	*sb;
	float		percent;
	char		str[64], str2[64];
	const char	*name;

	switch (msg->what)
	{
		case MSG_CANCEL:
			release_sem(fCancel);
			break;
		case PIC_PROGRESS:
			sb = (BStatusBar *)FindView("PictureProgress");
			if (sb != NULL)
			{
				if (msg->FindFloat("percent", &percent) != B_NO_ERROR)
					percent = 0.0f;
				sprintf(str, "%s", fCurName);
				sprintf(str2, "%0.1f%%", percent);
				sb->Reset(str, str2);
				sb->Update(percent, NULL, NULL);
			}
			sb = (BStatusBar *)FindView("AllProgress");
			if (sb != NULL)
			{
				percent = 100.0f * ((float)fFrameID + percent / 100.0f) / (float)fTotal;
				sprintf(str2, STR_XOFN, fFrameID + 1, fTotal);
				sb->Reset(STR_ALLPICTURES, NULL);
				sb->Update(percent, NULL, str2);
			}
			break;
		case ALL_PROGRESS:
			if (msg->FindInt32("current", &fFrameID) != B_NO_ERROR ||
				msg->FindInt32("total", &fTotal) != B_NO_ERROR)
			{
				fFrameID = 0;
				fTotal = 10;
			}
			if (msg->FindString("curName", &name) == B_NO_ERROR)
				strcpy(fCurName, name);
			else
				sprintf(fCurName, STR_PICTUREX, fFrameID + 1);
			sb = (BStatusBar *)FindView("PictureProgress");
			if (sb != NULL)
			{
				sprintf(str, "%s", fCurName);
				strcpy(str2, "0%");
				sb->Reset(str, str2);
			}
			sb = (BStatusBar *)FindView("AllProgress");
			if (sb != NULL)
			{
				percent = 100.0f * (float)fFrameID / (float)fTotal;
				sprintf(str2, STR_XOFN, fFrameID + 1, fTotal);
				sb->Reset(STR_ALLPICTURES, NULL);
				sb->Update(percent, NULL, str2);
			}
			break;
		default:
			BWindow::MessageReceived(msg);
	}
}
