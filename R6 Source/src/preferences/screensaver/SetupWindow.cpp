#include "SetupWindow.h"
#include "Settings.h"
#include "FadeView.h"
#include "ModuleView.h"
#include "ssdefs.h"

#include <Application.h>
#include <TabView.h>
#include <Box.h>

#include <stdio.h>

SetupWindow::SetupWindow(BRect frame, BMessenger roster)
 : BWindow(frame, "ScreenSaver", B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
 	B_NOT_RESIZABLE | B_NOT_ZOOMABLE)// | B_ASYNCHRONOUS_CONTROLS)
{
	BTab		*tab;
	rgb_color	col = ui_color(B_PANEL_BACKGROUND_COLOR);
	BRect		r;

	r = Bounds();
	r.InsetBy(-1, -1);

	// build user interface
	BBox *back = new BBox(r, "back", B_FOLLOW_ALL, B_WILL_DRAW, B_PLAIN_BORDER);
	AddChild(back);

	r.top += 6;

	tabView = new BTabView(r, "tab_view");
	tabView->SetViewColor(col);

	r = tabView->Bounds();
	r.InsetBy(5,5);
	r.bottom -= tabView->TabHeight();

	tab = new BTab();
	tabView->AddTab(new FadeView(r, "fade"), tab);
	tab->SetLabel("Fade");

	tab = new BTab();
	tabView->AddTab(new ModuleView(r, "modules", roster), tab);
	tab->SetLabel("Modules");

	back->AddChild(tabView);


	BMessage *m = AcquireSettings();
	int32 sel = 0;
	m->FindInt32(kWindowTab, &sel);
	ReleaseSettings();

	tabView->Select(sel);
}

bool SetupWindow::QuitRequested()
{
	BMessage *m = AcquireSettings();
	m->ReplaceInt32(kWindowTab, tabView->Selection());
	m->ReplaceRect(kWindowFrame, Frame());
	ReleaseSettings();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void SetupWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		default :
			BWindow::MessageReceived(msg);
			break;
	}
}
