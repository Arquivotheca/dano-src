#include "Blanket.h"
#include "ModuleView.h"
#include "ModuleListView.h"
#include "ModuleListItem.h"
#include "Settings.h"
#include "MockupView.h"
#include "ssdefs.h"
#include "ModulePreviewView.h"
#include "ModuleRoster.h"

#include <FilePanel.h>
#include <Button.h>
#include <ScrollView.h>
#include <Message.h>
#include <Box.h>
#include <List.h>
#include <Application.h>

#include <stdio.h>
#include <string.h>

ModuleView::ModuleView(BRect frame, const char *name, BMessenger roster)
 : BView(frame, name, B_WILL_DRAW, 0), list(0), box(0), r(roster), currentmod(0)
{
	rgb_color	col = ui_color(B_PANEL_BACKGROUND_COLOR);

	SetViewColor(col);
	SetLowColor(col);

	blankpreview = new BView(BRect(25, 20, 145, 110), "preview", 0, 0);
	blankpreview->SetViewColor(0, 0, 0, 255);
	mockup = new MockupView("screen", blankpreview);
	AddChild(mockup);

	float bottom = Bounds().bottom - 10;
	test = new BButton(BRect(10, bottom - 20, 85, bottom), "Test", "Test", new BMessage('test'));
	AddChild(test);

	add = new BButton(BRect(95, bottom - 20, 170, bottom), "Add", "Add" B_UTF8_ELLIPSIS, new BMessage('inst'));
	AddChild(add);

	list = new ModuleListView(BRect(13, 135, 167 - B_V_SCROLL_BAR_WIDTH, bottom - 28), "Modules");
	list->SetInvocationMessage(new BMessage('tsel'));
	list->SetSelectionMessage(new BMessage('sele'));

	BScrollView *sv = new BScrollView("module list", list, B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true);
	AddChild(sv);

	box = new BBox(BRect(180, 10, 190 + 241, 40 + 244), "Module settings");
	box->SetLabel("Module settings");
	box->SetFont(be_plain_font);
	AddChild(box);

	BMessage *m = AcquireSettings();
	const char *modname = 0;
	m->FindString(kModuleName, &modname);
	ReleaseSettings();
	if(modname)
		strcpy(modulename, modname);
	else
		*modulename = 0;

	filepanel = 0;
}

ModuleView::~ModuleView()
{
	delete filepanel;
}

void ModuleView::SaveState()
{
	BMessage *m = AcquireSettings();
	m->RemoveName(kModuleName);
	if(*modulename)
		m->AddString(kModuleName, modulename);

	ReleaseSettings();
	SaveSettings();
}

void ModuleView::AttachedToWindow()
{
	list->SetTarget(this);
	test->SetTarget(this);
	add->SetTarget(this);
	test->SetEnabled(currentmod ? true : false);

	ModuleListItem *mod = new BlacknessListItem();
	AddMod(mod);

	// initialize ModuleRoster
	BMessage start(BLANKET_ROSTER_START);
	start.AddMessenger("target", this);
	BMessage addm('addm');
	BMessage remm('remm');
	BMessage modm('modm');
	start.AddMessage("addmsg", &addm);
	start.AddMessage("remmsg", &remm);
	start.AddMessage("modmsg", &modm);
	r.SendMessage(&start);
}

void ModuleView::DetachedFromWindow()
{
	SelectMod(-1);
	int32 max = list->CountItems();
	for(int32 i = max - 1; i >= 0; i--)
	{
		ModuleListItem	*it = (ModuleListItem *)list->ItemAt(i);
		list->RemoveItem(it);
		delete it;
	}
	list->MakeEmpty();
}

void ModuleView::MessageReceived(BMessage *msg)
{
	const char *path;

	switch(msg->what)
	{
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
				filepanel = new BFilePanel(B_OPEN_PANEL, &r, 0, B_FILE_NODE, true, 0, 0);

			filepanel->Show();
			break;

		case 'sele' :
			// show module panel
			SelectMod(list->CurrentSelection());
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

		case 'addm' :
			if(msg->FindString("module", &path) == B_OK)
				AddMod(new ModuleListItem(path));
			break;

		case 'remm' :
			if(msg->FindString("module", &path) == B_OK)
				RemoveMod(path);
			break;

		case 'modm' :
			if(msg->FindMessage("metadata", &metadata) == B_OK && currentmod)
				currentmod->ModulesChanged(&metadata);
			break;

		default :
			BView::MessageReceived(msg);
			break;
	}
}

void ModuleView::AddMod(ModuleListItem *mod)
{
	int32 count = list->CountItems();
	bool select = strcmp(mod->InternalName(), modulename) == 0;

	switch(count)
	{
		case 0 :
		case 1 :
			list->AddItem(mod);
			if(select)
				SelectMod(count);
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
				SelectMod(i);
			break;
	}
	list->ScrollToSelection();
}

void ModuleView::RemoveMod(BPath path)
{
	if(currentmod && path == *(currentmod->Path()))
	{
		list->SetSelectionMessage(0);
		list->Select(0);
		list->SetSelectionMessage(new BMessage('sele'));
		list->ScrollToSelection();

		SelectMod(0);
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

void ModuleView::SelectMod(int32 index)
{
	if(currentmod)
	{
		BView *c = mockup->ChildAt(0);
		mockup->RemoveChild(c);
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
			currentmod->view = new BView(BRect(3, 21, 3 + 241, 21 + 244), "", 0, 0);
			currentmod->view->SetViewColor(ViewColor());
			currentmod->view->Hide();
			// box->SetLabel(currentmod->GetName());
			box->AddChild(currentmod->view);
	
			currentmod->mod->StartConfig(currentmod->view);
			mockup->RemoveChild(blankpreview);
			BView *nicepreview = new ModulePreviewView(currentmod, blankpreview->Frame());
			mockup->AddChild(nicepreview);
	
			list->SetSelectionMessage(0);
			list->Select(index);
			list->SetSelectionMessage(new BMessage('sele'));

			list->ScrollToSelection();
	
			currentmod->view->Show();
		}
		else
			currentmod = 0;
	}
	test->SetEnabled(currentmod ? true : false);
}
