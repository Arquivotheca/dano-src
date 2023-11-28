//--------------------------------------------------------------------
//	
//	IconWindow.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1994-96 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef ICON_WINDOW_H
#include "IconWindow.h"
#endif
#ifndef ICON_WORLD_H
#include "IconWorld.h"
#endif
#ifndef ICON_VIEW_H
#include "IconView.h"
#endif
#ifndef ICON_COLOR_H
#include "IconColor.h"
#endif
#ifndef ICON_PAT_H
#include "IconPat.h"
#endif
#ifndef ICON_TOOLS_H
#include "IconTools.h"
#endif

#include <Debug.h>

#include <stdio.h>


//====================================================================

TIconWindow::TIconWindow(BRect rect, entry_ref *ref)
			:BWindow(rect, "Icon", B_TITLED_WINDOW, B_NOT_RESIZABLE |
													B_NOT_ZOOMABLE)
{
	char		str[256];
	short		loop;
	BRect		r;
	BMenu		*menu;	
	BMenuBar	*menu_bar;
  	BMenuItem   *item;

	SetPulseRate(100000);
	r.Set(0, 0, 32767, 15);
	menu_bar = new BMenuBar(r, "");

	fFileMenu = new BMenu("File");
	menu_bar->AddItem(fFileMenu);
	
	fFileMenu->AddItem(item = new BMenuItem("About IconWorld",new BMessage(B_ABOUT_REQUESTED)));
	item->SetTarget(be_app);
	fFileMenu->AddSeparatorItem();
	fFileMenu->AddItem(item = new BMenuItem("New", new BMessage(M_NEW), 'N'));
	item->SetTarget(be_app);
	fFileMenu->AddItem(item = new BMenuItem("Open", new BMessage(M_OPEN), 'O'));
	item->SetTarget(be_app);

	fFileMenu->AddItem(new BMenuItem("Save", new BMessage(M_SAVE), 'S'));
	fFileMenu->AddItem(new BMenuItem("Save As...", new BMessage(M_SAVE_AS)));

	fFileMenu->AddSeparatorItem();
	fFileMenu->AddItem(new BMenuItem("Close", new BMessage(B_CLOSE_REQUESTED), 'W'));
	
//+	fFileMenu->AddItem(new BSeparatorItem());
//+	fFileMenu->AddItem(new BMenuItem("File Info...", new BMessage(M_INFO), 'I'));

	fEditMenu = new BMenu("Edit");
	menu_bar->AddItem(fEditMenu);
	fEditMenu->AddItem(new BMenuItem("Undo", new BMessage(M_UNDO), 'Z'));
	fEditMenu->AddItem(new BSeparatorItem());
	fEditMenu->AddItem(new BMenuItem("Cut", new BMessage(M_CUT), 'X'));
	fEditMenu->AddItem(new BMenuItem("Copy", new BMessage(M_COPY), 'C'));
	fEditMenu->AddItem(new BMenuItem("Paste", new BMessage(M_PASTE), 'V'));
	fEditMenu->AddItem(new BMenuItem("Clear", new BMessage(M_CLEAR)));
	fEditMenu->AddItem(new BSeparatorItem());
	fEditMenu->AddItem(new BMenuItem("Select All", new BMessage(M_SELECT), 'A'));

	fIconMenu = new BMenu("Icon");
	menu_bar->AddItem(fIconMenu);
	fIconMenu->AddItem(new BMenuItem("Create", new BMessage(M_CREATE), 'K'));
	fIconMenu->AddItem(new BMenuItem("Delete", new BMessage(M_DELETE), 'D'));

	menu = new BMenu("Pen");
	menu_bar->AddItem(menu);
	fSizeMenu = new BMenu("Size");
	for (loop = 0; loop < 8; loop++) {
		sprintf(str, "%d", loop);
		fSizeMenu->AddItem(new BMenuItem(str, new BMessage(M_PEN), '0' + loop));
	}
	menu->AddItem(new BMenuItem(fSizeMenu));

	fModeMenu = new BMenu("Mode");
	fModeMenu->AddItem(new BMenuItem("OP_Copy", new BMessage(M_MODE_COPY)));
	fModeMenu->AddItem(new BMenuItem("OP_Over", new BMessage(M_MODE_OVER)));
	fModeMenu->AddItem(new BMenuItem("OP_Erase", new BMessage(M_MODE_ERASE)));
	fModeMenu->AddItem(new BMenuItem("OP_Invert", new BMessage(M_MODE_INVERT)));
	fModeMenu->AddItem(new BMenuItem("OP_Add", new BMessage(M_MODE_ADD)));
	fModeMenu->AddItem(new BMenuItem("OP_Subtract", new BMessage(M_MODE_SUBTRACT)));
	fModeMenu->AddItem(new BMenuItem("OP_Blend", new BMessage(M_MODE_BLEND)));
	fModeMenu->AddItem(new BMenuItem("OP_Min", new BMessage(M_MODE_MIN)));
	fModeMenu->AddItem(new BMenuItem("OP_Max", new BMessage(M_MODE_MAX)));
	menu->AddItem(new BMenuItem(fModeMenu));

#ifdef APPI_EDIT
	fAPPIMenu = new BMenu("App Info");
	menu_bar->AddItem(fAPPIMenu);
	fAPPIMenu->AddItem(new BMenuItem("Create APPI", new BMessage(M_CREATE_APPI)));
	fAPPIMenu->AddItem(new BSeparatorItem());
	fAPPIMenu->AddItem(new BMenuItem("Single Launch", new BMessage(M_SINGLE_LAUNCH)));
	fAPPIMenu->AddItem(new BMenuItem("Multiple Launch", new BMessage(M_MULTI_LAUNCH)));
	fAPPIMenu->AddItem(new BMenuItem("Exclusive Launch", new BMessage(M_EXCLUSIVE_LAUNCH)));
	fAPPIMenu->AddItem(new BSeparatorItem());
	fAPPIMenu->AddItem(new BMenuItem("Background App", new BMessage(M_BACKGROUND_APP)));
	fAPPIMenu->AddItem(new BMenuItem("Argv Only", new BMessage(M_ARGV_ONLY)));
	fAPPIMenu->AddItem(new BSeparatorItem());
	fAPPIMenu->AddItem(new BMenuItem("Set Signature...", new BMessage(M_SIGNATURE)));
#endif

	Lock();
	AddChild(menu_bar);
	ResizeBy(0, menu_bar->Bounds().bottom + 1);

	r = Frame();
	r.OffsetTo(0, menu_bar->Bounds().bottom + 1);
	fView = new TIconView(r, ref);
	AddChild(fView);
	Unlock();

	AddShortcut('N', B_COMMAND_KEY, new BMessage(M_NEW));
	AddShortcut('O', B_COMMAND_KEY, new BMessage(M_OPEN));
}

//--------------------------------------------------------------------

void TIconWindow::MenusBeginning(void)
{
	char	size[2];

	fFileMenu->FindItem(M_SAVE)->SetEnabled(fView->fDirty);

	fEditMenu->FindItem(M_UNDO)->SetEnabled(fView->fUndo32 | fView->fUndo16);
	fEditMenu->FindItem(M_PASTE)->SetEnabled(((TIconApp *)be_app)->fClipFlag);

	fIconMenu->FindItem(M_CREATE)->SetEnabled(fView->fIconCount < 6);
	fIconMenu->FindItem(M_DELETE)->SetEnabled(fView->fIconCount - 1);

	size[0] = '0' + fView->fPenSize;
	size[1] = 0;
	fSizeMenu->FindItem(size)->SetMarked(TRUE);
	fModeMenu->FindItem(fView->fPenMode)->SetMarked(TRUE);

#ifdef APPI_EDIT
	fAPPIMenu->FindItem(M_CREATE_APPI)->SetMarked(fView->fCreateAppi);
	fAPPIMenu->FindItem(M_SINGLE_LAUNCH)->SetMarked((fView->fAppFlags &
										B_LAUNCH_MASK) == B_SINGLE_LAUNCH);
	fAPPIMenu->FindItem(M_MULTI_LAUNCH)->SetMarked((fView->fAppFlags &
										B_LAUNCH_MASK) == B_MULTIPLE_LAUNCH);
	fAPPIMenu->FindItem(M_EXCLUSIVE_LAUNCH)->SetMarked((fView->fAppFlags &
										B_LAUNCH_MASK) == B_EXCLUSIVE_LAUNCH);
	fAPPIMenu->FindItem(M_BACKGROUND_APP)->SetMarked(fView->fAppFlags &
													B_BACKGROUND_APP);
	fAPPIMenu->FindItem(M_ARGV_ONLY)->SetMarked(fView->fAppFlags &
													B_ARGV_ONLY);
#endif
}

//--------------------------------------------------------------------

void TIconWindow::MessageReceived(BMessage *msg)
{
	char		size[2];
	BFilePanel	*panel;
	BMenuItem	*item;
	BMessenger	window(this);

	switch(msg->what) {
		case M_NEW:
		case M_OPEN:
			DetachCurrentMessage();
			be_app->PostMessage(msg);
			break;

		case B_SAVE_REQUESTED:
			fView->SaveRequested(msg);
			break;

		case M_SAVE:
			if (fView->fFile) {
				fView->SaveFile();
				break;
			}
			// else fall through
		case M_SAVE_AS:
			panel = new BFilePanel(B_SAVE_PANEL, &window);
			panel->Window()->Show();
			break;
		case M_INFO:
			break;

		case M_UNDO:
			fView->Undo();
			break;
		case M_CUT:
			fView->Cut();
			break;
		case M_COPY:
			fView->Copy();
			break;
		case M_PASTE:
			fView->Paste();
			break;
		case M_CLEAR:
			fView->Clear();
			break;
		case M_SELECT:
			fView->SelectAll();
			break;

		case M_CREATE:
			fView->NewIcon();
			break;
		case M_DELETE:
			fView->DeleteIcon();
			break;

		case M_PEN:
			size[0] = '0' + fView->fPenSize;
			size[1] = 0;
			fSizeMenu->FindItem(size)->SetMarked(FALSE);

			CurrentMessage()->FindPointer("source", &item);
			fView->SetPen(*(item->Label()) - '0');
			break;

		case M_MODE_COPY:
		case M_MODE_OVER:
		case M_MODE_ERASE:
		case M_MODE_INVERT:
		case M_MODE_ADD:
		case M_MODE_SUBTRACT:
		case M_MODE_BLEND:
		case M_MODE_MIN:
		case M_MODE_MAX:
			fModeMenu->FindItem(fView->fPenMode)->SetMarked(FALSE);
			fView->SetMode(msg->what);
			break;

#ifdef APPI_EDIT
		case M_CREATE_APPI:
#endif
		case M_SINGLE_LAUNCH:
		case M_MULTI_LAUNCH:
		case M_EXCLUSIVE_LAUNCH:
		case M_BACKGROUND_APP:
		case M_ARGV_ONLY:
			PostMessage(msg->what, fView);
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

//--------------------------------------------------------------------

bool TIconWindow::QuitRequested(void)
{
	if (fView->SaveChanges()) {
		be_app->PostMessage(M_CLOSE_WINDOW);
		return TRUE;
	}
	else
		return FALSE;
}

//--------------------------------------------------------------------

void TIconWindow::Show(void)
{
	if (fView->fResult == B_NO_ERROR)
		BWindow::Show();
	else {
		((TIconApp *)be_app)->fWindowCount--;
		Quit();
	}
}

//--------------------------------------------------------------------

void TIconWindow::WindowActivated(bool active)
{
	TIconApp	*app = (TIconApp *)be_app;

	fView->MakeFocus(active);
	if (active) {
/*
		AddFloater(app->fColorWind);
		AddFloater(app->fToolWind);
		AddFloater(app->fPatWind);
*/
	}
}


//====================================================================

TColorWindow::TColorWindow(BRect rect, char *title)
	:BWindow(rect, title, B_FLOATING_WINDOW, B_NOT_RESIZABLE |
		B_NOT_RESIZABLE | B_NOT_CLOSABLE | B_WILL_ACCEPT_FIRST_CLICK | B_NOT_ZOOMABLE | B_AVOID_FOCUS)
{
	BRect	r;

	r = Frame();
	r.OffsetTo(0, 0);
	fView = new TColorView(r);
	AddChild(fView);
}


//====================================================================

TToolWindow::TToolWindow(BRect rect, char *title)
	:BWindow(rect, title, B_FLOATING_WINDOW, B_NOT_RESIZABLE |
		B_NOT_RESIZABLE | B_NOT_CLOSABLE | B_WILL_ACCEPT_FIRST_CLICK | B_NOT_ZOOMABLE | B_AVOID_FOCUS)
{
	BRect	r;

	r = Frame();
	r.OffsetTo(0, 0);
	fView = new TToolView(r);
	AddChild(fView);
}


//====================================================================

TPatWindow::TPatWindow(BRect rect, char *title)
	:BWindow(rect, title, B_FLOATING_WINDOW,
		B_NOT_RESIZABLE | B_NOT_CLOSABLE | B_WILL_ACCEPT_FIRST_CLICK | B_NOT_ZOOMABLE | B_AVOID_FOCUS)
{
	BRect	r;

	r = Frame();
	r.OffsetTo(0, 0);
	fView = new TPatView(r);
	AddChild(fView);
}
