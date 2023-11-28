#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <Application.h>
#include <Box.h>
#include <Debug.h>
#include <FindDirectory.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <ScrollView.h>
#include <View.h>

#include "IconEditView.h"
#include "IconWindow.h"

const int32 kSaveFile = 'save';

const int32 kMenuHeight = 15;

TIconWindow::TIconWindow(IconEditView *parent, const char* iconName,
	BBitmap *largeIcon, BBitmap *miniIcon)
	:	BWindow(BRect(0,0,1,1), "Icon-O-Matic",
		B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_FRAME_EVENTS | B_WILL_DRAW | B_NOT_RESIZABLE,
		B_CURRENT_WORKSPACE),
		fParent(parent)
{
	GetPrefs();

	ResizeTo(kIconEditorWidth, kIconEditorHeight + kMenuHeight);

	BRect mbarRect = Bounds();
	mbarRect.bottom = mbarRect.top + kMenuHeight;
	fMenuBar = new BMenuBar(mbarRect, "mbar");

	BRect boxFrame(Bounds());
	boxFrame.top = fMenuBar->Bounds().bottom + 1;

	fIconEditor = new TIconEditor(boxFrame, miniIcon, largeIcon);
	AddChild(fIconEditor);

	BMenuItem *item;
	BMenu *menu = new BMenu("File");
	
	menu->AddItem(new BMenuItem("Save", new BMessage(kSaveFile), 'S'));
	item = new BMenuItem("Dump Icons", new BMessage(kDumpIcons), 'D');
	item->SetTarget(fIconEditor);
	menu->AddItem(item);
		
	item = new BMenuItem("Dump Selection", new BMessage(kDumpSelection), 'D',
		B_COMMAND_KEY | B_OPTION_KEY);
	item->SetTarget(fIconEditor);
	menu->AddItem(item);

	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Close", new BMessage(B_CLOSE_REQUESTED), 'W'));

	fMenuBar->AddItem(menu);	// File menu

	menu = new BMenu("Edit");

	item = new BMenuItem("Undo", new BMessage(B_UNDO), 'Z');
	item->SetTarget(fIconEditor);
	menu->AddItem(item);
	menu->AddSeparatorItem();

	item = new BMenuItem("Cut", new BMessage(B_CUT), 'X');
	item->SetTarget(fIconEditor);
	menu->AddItem(item);
	item = new BMenuItem("Copy", new BMessage(B_COPY), 'C');
	item->SetTarget(fIconEditor);
	menu->AddItem(item);
	item = new BMenuItem("Paste", new BMessage(B_PASTE), 'V');
	item->SetTarget(fIconEditor);
	menu->AddItem(item);
	
	menu->AddSeparatorItem();
	
	item = new BMenuItem("Clear", new BMessage(kClear));
	menu->AddItem(item);
	item->SetTarget(fIconEditor);
	
	item = new BMenuItem("Select All", new BMessage(B_SELECT_ALL), 'A');
	item->SetTarget(fIconEditor);
	menu->AddItem(item);

	item = new BMenuItem("Deselect", new BMessage(msg_deselect), 'F');
	item->SetTarget(fIconEditor);
	menu->AddItem(item);

	menu->AddSeparatorItem();

	item = new BMenuItem("Set Background Color...", new BMessage(msg_set_bg_color), 'B');
	item->SetTarget(fIconEditor);
	menu->AddItem(item);
	
	fMenuBar->AddItem(menu);	// Edit menu
	
	menu = new BMenu("Tools");

	item = new BMenuItem("Large Icon", new BMessage(msg_large_icon), 'L');
	item->SetTarget(fIconEditor);
	menu->AddItem(item);
	item = new BMenuItem("Small Icon", new BMessage(msg_small_icon), 'M');
	item->SetTarget(fIconEditor);
	menu->AddItem(item);
	
	menu->AddSeparatorItem();

	item = new BMenuItem("Previous Tool", new BMessage(msg_prev_tool), '1');
	item->SetTarget(fIconEditor);
	menu->AddItem(item);
	item = new BMenuItem("Next Tool", new BMessage(msg_next_tool), '2');
	item->SetTarget(fIconEditor);
	menu->AddItem(item);

	menu->AddSeparatorItem();

	item = new BMenuItem("Selection", new BMessage(msg_selection_tool), '3');
	item->SetTarget(fIconEditor);
	menu->AddItem(item);
	item = new BMenuItem("Eraser", new BMessage(msg_eraser_tool), '4');
	item->SetTarget(fIconEditor);
	menu->AddItem(item);
	item = new BMenuItem("Pencil", new BMessage(msg_pencil_tool), '5');
	item->SetTarget(fIconEditor);
	menu->AddItem(item);
	item = new BMenuItem("Eye Dropper", new BMessage(msg_eye_tool), '6');
	item->SetTarget(fIconEditor);
	menu->AddItem(item);
	item = new BMenuItem("Fill", new BMessage(msg_fill_tool), '7');
	item->SetTarget(fIconEditor);
	menu->AddItem(item);
	item = new BMenuItem("Line", new BMessage(msg_line_tool),'8');
	item->SetTarget(fIconEditor);
	menu->AddItem(item);
	item = new BMenuItem("Rectangle", new BMessage(msg_rect_tool),'9');
	item->SetTarget(fIconEditor);
	menu->AddItem(item);
	item = new BMenuItem("Filled Rectangle", new BMessage(msg_frect_tool));
	item->SetTarget(fIconEditor);
	menu->AddItem(item);
	item = new BMenuItem("Round Rectangle", new BMessage(msg_rrect_tool));
	item->SetTarget(fIconEditor);
	menu->AddItem(item);
	item = new BMenuItem("Filled Round Rectangle", new BMessage(msg_frrect_tool));
	item->SetTarget(fIconEditor);
	menu->AddItem(item);
	item = new BMenuItem("Oval", new BMessage(msg_oval_tool));
	item->SetTarget(fIconEditor);
	menu->AddItem(item);
	item = new BMenuItem("Filled Oval", new BMessage(msg_foval_tool));
	item->SetTarget(fIconEditor);
	menu->AddItem(item);

/*	menu->AddSeparatorItem();

	item = new BMenuItem("Foreground Color", new BMessage(msg_fore_color), 'F');
	item->SetTarget(fIconEditor);
	menu->AddItem(item);
	item = new BMenuItem("Background Color", new BMessage(msg_back_color), 'B');
	item->SetTarget(fIconEditor);
	menu->AddItem(item);*/		
	
	fMenuBar->AddItem(menu);
	AddChild(fMenuBar);
	
	if (iconName) {
		char windowName[1024];
		sprintf(windowName,"Icon-O-Matic:  %s", iconName);
		SetTitle(windowName);
	}

	SetPulseRate(100000);
}


TIconWindow::~TIconWindow()
{
}

void 
TIconWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case kSaveFile:
			Save();
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

bool 
TIconWindow::QuitRequested()
{
	SetPrefs();
	fParent->Looper()->PostMessage(kEditorClosing, fParent);
	return false;
}

void
TIconWindow::GetPrefs()
{
	BPath path;
	
	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		long ref;
		BPoint loc;
				
		path.Append (kIconEditorPrefsfileName);
		if ((ref = open(path.Path(), O_RDWR)) >= 0) {
			loc.x = -1; loc.y = -1;
		
			lseek (ref, 0, SEEK_SET);
			read(ref, &loc, sizeof(BPoint));
			close(ref);
			
			if (BScreen(B_MAIN_SCREEN_ID).Frame().Contains(loc)) {
				MoveTo(loc);
				return;
			}			
		} else {
			// go back up 1 level
			find_directory (B_USER_SETTINGS_DIRECTORY, &path);
			BDirectory dir;
			dir.SetTo(path.Path());
			BFile prefsFile;
			dir.CreateFile(kIconEditorPrefsfileName, &prefsFile);
		}
	}
	
	// 	if prefs dont yet exist, simply center the window
	BScreen screen(B_MAIN_SCREEN_ID);
	float x = screen.Frame().Width()/2 - kIconEditorWidth/2;
	float y = screen.Frame().Height()/2 - kIconEditorHeight/2;

	MoveTo(x, y);
}

void
TIconWindow::SetPrefs()
{
	BPath path;
	BPoint loc = Frame().LeftTop();

	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
		long ref;
		
		path.Append (kIconEditorPrefsfileName);
		if ((ref = creat(path.Path(), O_RDWR)) >= 0) {

			lseek (ref, 0, SEEK_SET);
			write (ref, &loc, sizeof (BPoint));
			close(ref);
			
		}
	}
}

void
TIconWindow::MenusBeginning()
{
	BMenuItem *menu,*menu2;
	
	menu = fMenuBar->FindItem("Save");
	if (menu)
		menu->SetEnabled(fIconEditor->Dirty());

	menu = fMenuBar->FindItem("Undo");
	if (menu) {
		if (fIconEditor->SelectedIcon()) 
			menu->SetEnabled(fIconEditor->LargeIconFBE()->CanUndo());
		else
			menu->SetEnabled(fIconEditor->SmallIconFBE()->CanUndo());
	}
	
	menu = fMenuBar->FindItem("Cut");
	menu2 = fMenuBar->FindItem("Copy");
	if (menu && menu2) {
		if (fIconEditor->SelectedIcon()) {
			bool enable = fIconEditor->LargeIconFBE()->CanCopy();
			menu->SetEnabled(enable);
			menu2->SetEnabled(enable);
		} else {
			menu->SetEnabled(fIconEditor->SmallIconFBE()->CanCopy());
			menu2->SetEnabled(fIconEditor->SmallIconFBE()->CanCopy());
		}
	}

	menu = fMenuBar->FindItem("Paste");
	if (menu) {
		if (fIconEditor->SelectedIcon()) 
			menu->SetEnabled(fIconEditor->LargeIconFBE()->CanPaste());
		else
			menu->SetEnabled(fIconEditor->SmallIconFBE()->CanPaste());
	}

	menu = fMenuBar->FindItem("Clear");
	if (menu) {
		if (fIconEditor->SelectedIcon()) 
			menu->SetEnabled(fIconEditor->LargeIconFBE()->CanClear());
		else
			menu->SetEnabled(fIconEditor->SmallIconFBE()->CanClear());
	}
	
	menu = fMenuBar->FindItem("Deselect");
	if (menu) {
		if (fIconEditor->SelectedIcon()) 
			menu->SetEnabled(fIconEditor->LargeIconFBE()->HaveSelection());
		else
			menu->SetEnabled(fIconEditor->SmallIconFBE()->HaveSelection());
	}

	menu = fMenuBar->FindItem("Dump Selection");
	if (menu) {
		if (fIconEditor->SelectedIcon()) 
			menu->SetEnabled(fIconEditor->LargeIconFBE()->HaveSelection());
		else
			menu->SetEnabled(fIconEditor->SmallIconFBE()->HaveSelection());
	}
}

bool
TIconWindow::Save()
{
	if (!fParent->Window()->Lock()) {
		TRESPASS();
		return true;
	}
	
	if (!Dirty())
		return true;
	
	BRect r(0,0,31, 31);
	BBitmap *icon = fIconEditor->LargeIcon();
	BBitmap *tempIcon = new BBitmap(r, B_COLOR_8_BIT, false);
	tempIcon->SetBits(icon->Bits(), icon->BitsLength(), 0, B_COLOR_8_BIT);
	
	fParent->CutPasteSetLargeIcon(tempIcon);
	
	icon = fIconEditor->SmallIcon();
	r.right = 15; r.bottom = 15;
	tempIcon = new BBitmap(r, B_COLOR_8_BIT, false);
	tempIcon->SetBits(icon->Bits(), icon->BitsLength(), 0, B_COLOR_8_BIT);
	fParent->CutPasteSetMiniIcon(tempIcon);
			
	fParent->Window()->Unlock();
	
	fIconEditor->SetDirty(false);
	
	return true;
}

bool 
TIconWindow::Dirty() const
{
	return fIconEditor->Dirty();
}

