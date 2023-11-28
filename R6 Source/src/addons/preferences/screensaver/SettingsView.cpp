#include "SettingsView.h"

#include "Settings.h"
#include "ssdefs.h"
#include "ModuleListView.h"
#include "ModuleListItem.h"
#include "ModulePreviewView.h"
#include "ModuleRoster.h"
#include "Sliders.h"

#include <Message.h>
#include <Rect.h>
#include <Messenger.h>
#include <CheckBox.h>
#include <TextControl.h>
#include <Roster.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Beep.h>
#include <StringView.h>
#include <Slider.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <InterfaceDefs.h>
#include <Screen.h>
#include <Button.h>
#include <FilePanel.h>
#include <ScrollView.h>
#include <Message.h>
#include <Box.h>
#include <List.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class MiniScreenView : public BView
{
public:
	MiniScreenView(BRect frame)
	 : BView(frame, "miniscreen", 0, B_WILL_DRAW)
	{
		SetViewColor(B_TRANSPARENT_32_BIT);
	}

	void Draw(BRect)
	{
		BRect r = Bounds();
		rgb_color	back = Parent()->ViewColor();
		rgb_color	shade = { 184, 184, 184, 255 };
		rgb_color	frame = { 156, 156, 156, 255 };
		BeginLineArray(16);
		AddLine(BPoint(2, 0), BPoint(r.right - 2, 0), frame);	// top frame
		AddLine(BPoint(1, 1), BPoint(r.right - 1, 1), frame);	// top frame
		AddLine(BPoint(1, r.bottom - 1), BPoint(r.right - 1, r.bottom - 1), frame);	// bottom frame
		AddLine(BPoint(2, r.bottom), BPoint(r.right - 2, r.bottom), frame);	// bottom frame
		AddLine(BPoint(0, 2), BPoint(0, r.bottom - 2), frame);	// left frame
		AddLine(BPoint(1, 1), BPoint(1, r.bottom - 1), frame);	// left frame
		AddLine(BPoint(r.right, 2), BPoint(r.right, r.bottom - 2), frame);	// right frame
		AddLine(BPoint(r.right - 1, 1), BPoint(r.right - 1, r.bottom - 1), frame);	// right frame
		AddLine(BPoint(0, 1), BPoint(1, 0), shade);	// corner shade
		AddLine(BPoint(0, r.bottom - 1), BPoint(1, r.bottom), shade);	// corner shade
		AddLine(BPoint(r.right - 1, r.bottom), BPoint(r.right, r.bottom - 1), shade);	// corner shade
		AddLine(BPoint(r.right - 1, 0), BPoint(r.right, 1), shade);	// corner shade
		AddLine(BPoint(0, 0), BPoint(0, 0), back);	// corner
		AddLine(BPoint(0, r.bottom), BPoint(0, r.bottom), back);	// corner
		AddLine(BPoint(r.right, 0), BPoint(r.right, 0), back);	// corner
		AddLine(BPoint(r.right, r.bottom), BPoint(r.right, r.bottom), back);	// corner
		EndLineArray();
	}
};

SettingsView::SettingsView(BRect frame, const char *name, ModuleRoster *rost)
 : BView(frame, name, B_WILL_DRAW, 0), list(0), box(0), roster(rost),
	currentmod(0), filepanel(0), preview(0)
{
	rgb_color	col = ui_color(B_PANEL_BACKGROUND_COLOR);

	SetViewColor(col);
	SetLowColor(col);

	float bottom = frame.Height();
	float right = frame.Width();

	// setup module stuff
#define PREVX 110
#define PREVY 82

#define SETTINGSX 241
#define SETTINGSY 244

	MiniScreenView *miniview = new MiniScreenView(BRect(5, 5, 9 + PREVX, 9 + PREVY));
	mockup = new BView(BRect(2, 2, 2 + PREVX, 2 + PREVY), "stub", 0, 0);
	mockup->SetViewColor(B_TRANSPARENT_32_BIT);
	blankpreview = new BView(BRect(0, 0, PREVX, PREVY), "preview", 0, 0);
	blankpreview->SetViewColor(0, 0, 0);
	mockup->AddChild(blankpreview);
	miniview->AddChild(mockup);
	AddChild(miniview);

#define TOP 100
	list = new ModuleListView(BRect(7, TOP, 117 - B_V_SCROLL_BAR_WIDTH, bottom - 40), "Modules");
	list->SetInvocationMessage(new BMessage('tsel'));
	list->SetSelectionMessage(new BMessage('sele'));

	BScrollView *sv = new BScrollView("module list", list, B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true);
	AddChild(sv);

	test = new BButton(BRect(5, bottom - 30, 60, bottom - 10), "Test", "Test", new BMessage('test'));
	AddChild(test);
	add = new BButton(BRect(64, bottom - 30, 119, bottom - 10), "Add", "Add" B_UTF8_ELLIPSIS, new BMessage('inst'));
	AddChild(add);

	dofade = new BCheckBox(BRect(135, 5, 220, 20), "dolock", "Invoke after:", new BMessage('dofd'));
	AddChild(dofade);
	timefade = new BStringView(BRect(225, 5, 295, 21), "timefade", "");
	AddChild(timefade);
	timefade->SetAlignment(B_ALIGN_RIGHT);
	fade = new RunSlider(BRect(300, 6, right - 10, 22), "fade", 0, timefade, new BMessage('fade'));
	rgb_color that_blue = { 115, 120, 184, 255 };
	fade->UseFillColor(true, &that_blue);
	AddChild(fade);

	box = new BBox(BRect(135, bottom - SETTINGSY - 30, 141 + SETTINGSX, bottom - 10), "Module settings");
	box->SetLabel("Module Settings:");
	AddChild(box);

	BMessage *m = AcquireSettings();
	const char *modname = 0;
	m->FindString(kModuleName, &modname);
	ReleaseSettings();
	if(modname)
		strcpy(modulename, modname);
	else
		*modulename = 0;

	// blackness module
	ModuleListItem *mod = new BlacknessListItem();
	AddMod(mod);
}

SettingsView::~SettingsView()
{
	delete filepanel;

	SelectMod(-1, true);
	int32 max = list->CountItems();
	for(int32 i = max - 1; i >= 0; i--)
	{
		ModuleListItem	*it = (ModuleListItem *)list->ItemAt(i);
		list->RemoveItem(it);
		delete it;
	}
	list->MakeEmpty();
}

void SettingsView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case 'enab' :
		case 'dofd' :
			CheckDependencies();
			SaveState();
			break;

		case 'fadc' :
		case '!fdc' :
			SaveState();
			break;

		case 'test' :	// the test button has been clicked
		case 'tsel' :	// a module has been double clicked
			if(currentmod)
			{
				currentmod->SaveSettings();
				SaveSettings();
				currentmod->Test();
			}
			break;

		case 'inst' :
			if(! filepanel)
			{
				BMessenger r(roster);
				filepanel = new BFilePanel(B_OPEN_PANEL, &r, 0, B_FILE_NODE, true, 0, 0);
			}

			filepanel->Show();
			break;

		case 'sele' :
			// show module panel
			SelectMod(list->CurrentSelection(), false);
			if(currentmod)
			{
				currentmod->ModulesChanged(&metadata);
				strcpy(modulename, currentmod->InternalName());
			}
			else
				*modulename = 0;
			SaveState();
			SaveSettings();
			break;

		default :
			BView::MessageReceived(msg);
			break;
	}
}

void SettingsView::AllAttached(void)
{
	// fetch current settings
	BMessage *m = AcquireSettings();

	int32	flags;
	if(m->FindInt32(kTimeFlags, &flags) == B_OK)
		dofade->SetValue(flags & kDoFade ? 1 : 0);

	int32	t;
	if(m->FindInt32(kTimeFade, &t) == B_OK)
		fade->SetTime(t);

	CheckDependencies();

	ReleaseSettings();
}

void SettingsView::CheckDependencies()
{
	bool	ena = dofade->Value();
	fade->SetEnabled(ena);
	timefade->SetDrawingMode(ena ? B_OP_OVER : B_OP_BLEND);
	timefade->Invalidate();
}

void SettingsView::SaveState()
{
	BMessage *m = AcquireSettings();

	int32 f;
	if(m->FindInt32(kTimeFlags, &f) == B_OK)
	{
		int32	mask = (f & ~kDoFade) | (dofade->Value() ? kDoFade : 0);

		m->ReplaceInt32(kTimeFlags, mask);
		m->ReplaceInt32(kTimeFade, fade->Time());
	}

	// current module
	m->RemoveName(kModuleName);
	if(*modulename)
		m->AddString(kModuleName, modulename);

	ReleaseSettings();

	// save to disk
	SaveSettings();
}

void SettingsView::AttachedToWindow()
{
	dofade->SetTarget(this);
	fade->SetTarget(this);
	list->SetTarget(this);
	test->SetTarget(this);
	add->SetTarget(this);
	test->SetEnabled(currentmod ? true : false);
	list->ScrollToSelection();
}

void SettingsView::AddMod(const BPath &path)
{
	AddMod(new ModuleListItem(path.Path()));
}

void SettingsView::AddMod(ModuleListItem *mod)
{
	int32 count = list->CountItems();
	bool select = strcmp(mod->InternalName(), modulename) == 0;

	switch(count)
	{
		case 0 :
		case 1 :
			list->AddItem(mod);
			if(select)
				SelectMod(count, true);
			break;

		default :
			int32 i;
			for(i = 1; i < count; i++)
			{
				ModuleListItem *it = (ModuleListItem *)list->ItemAt(i);
				if(strcasecmp(mod->Text(), it->Text()) < 0)
				{
					list->AddItem(mod, i);
					break;
				}
			}
			if(i == count)
				list->AddItem(mod);

			if(select)
				SelectMod(i, true);
			break;
	}
	list->ScrollToSelection();
}

void SettingsView::RemoveMod(const BPath &path)
{
	if(currentmod && path == *(currentmod->Path()))
	{
		list->SetSelectionMessage(0);
		list->Select(0);
		list->SetSelectionMessage(new BMessage('sele'));
		list->ScrollToSelection();

		SelectMod(0, true);
	}

	int32 count = list->CountItems();
	for(int i = 1; i < count; i++)
	{
		ModuleListItem *it = (ModuleListItem *)list->ItemAt(i);
		if(*(it->Path()) == path)
		{
			list->SetSelectionMessage(0);
			list->RemoveItem(i);
			list->SetSelectionMessage(new BMessage('sele'));
			delete it;
			break;
		}
	}
}

void SettingsView::SelectMod(int32 index, bool programmatic)
{
	if(currentmod)
	{
		BView *c = mockup->ChildAt(0);
		mockup->RemoveChild(c);
		preview = 0;
		mockup->AddChild(blankpreview);
		delete c;
		currentmod->view->Hide();
		box->RemoveChild(currentmod->view);
		// If the view had a thread showing a preview in the window
		// that owns this view (and that view, too), it will have quit
		// because we removed it from the window so it's looper is
		// no longer valid. We can stop the configuration.
		currentmod->mod->StopConfig();
		delete currentmod->view;
		currentmod->view = 0;
		currentmod->Unload();
	}

	currentmod = (ModuleListItem *)list->ItemAt(index);
	if(currentmod != 0)
	{
		if(currentmod->Load() == B_OK)
		{
			currentmod->view = new BView(BRect(2, 16, 3 + SETTINGSX, 16 + SETTINGSY), "", 0, 0);
			currentmod->view->SetViewColor(ViewColor());
			currentmod->view->Hide();
			// box->SetLabel(currentmod->GetName());
			box->AddChild(currentmod->view);

			currentmod->mod->StartConfig(currentmod->view);
			mockup->RemoveChild(blankpreview);
			preview = new ModulePreviewView(currentmod, blankpreview->Frame());
			mockup->AddChild(preview);

			if(programmatic)
			{
				list->SetSelectionMessage(0);
				list->Select(index);
				list->SetSelectionMessage(new BMessage('sele'));

				list->ScrollToSelection();
			}
	
			currentmod->view->Show();
		}
		else
			currentmod = 0;
	}
	test->SetEnabled(currentmod ? true : false);
}

void SettingsView::ModulesChanged(const BMessage &msg)
{
	metadata = msg;
	if(currentmod)
		currentmod->ModulesChanged(&metadata);
}
