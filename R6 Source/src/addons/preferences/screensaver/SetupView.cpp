#include "SetupView.h"

#include "Settings.h"
#include "ssdefs.h"
#include "ModuleRoster.h"
#include "SettingsView.h"
#include "AdvancedView.h"

#include <TabView.h>

#define WX	400
#define WY	330

SetupView::SetupView()
 : BView(BRect(0, 0, WX, WY), "screensaver", B_FOLLOW_LEFT | B_FOLLOW_TOP, 0)
{
	// load settings, initialize module roster
	InitSettings();
	DefaultSettings();	// initialize default values
	SaveSettings();

	// start module roster
	roster = new ModuleRoster();
	roster->Run();

	BTab		*tab;
	rgb_color	col = ui_color(B_PANEL_BACKGROUND_COLOR);
	BRect		r;

	r = Bounds();

	tabView = new BTabView(r, "tab_view");
	tabView->SetViewColor(col);

	r = tabView->Bounds();
	r.InsetBy(5,5);
	r.bottom -= tabView->TabHeight();

	tab = new BTab();
	tabView->AddTab(set = new SettingsView(r, "settings", roster), tab);
	tab->SetLabel("Settings");

	tab = new BTab();
	tabView->AddTab(new AdvancedView(r, "advanced"), tab);
	tab->SetLabel("Advanced");

	AddChild(tabView);
}

void SetupView::AttachedToWindow()
{
	BMessage *m = AcquireSettings();
	int32 sel = 0;
	m->FindInt32(kWindowTab, &sel);
	ReleaseSettings();

	tabView->Select(sel);

	// initialize ModuleRoster
	roster->Init(set, Window());
}

void SetupView::DetachedFromWindow()
{
	BMessage *m = AcquireSettings();
	m->ReplaceInt32(kWindowTab, tabView->Selection());
	ReleaseSettings();
	SaveSettings();
}

SetupView::~SetupView()
{
	roster->PostMessage(B_QUIT_REQUESTED);
}
