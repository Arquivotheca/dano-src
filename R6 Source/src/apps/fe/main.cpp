#include "twindow.h"
#include "bm_view.h"
#include "picker_view.h"
#include "select_view.h"
#include "sample_view.h"
#include "main.h"
#include <Debug.h>
#include <stdio.h>
#include <MenuBar.h>
#include <MenuItem.h>

#define	NN		24
#define	SWIDTH	16*16
#define	VP		5

SelectView		*s_view;

TApplication::TApplication() : BApplication("application/x-vnd.Be-FEDT")
{
	BRect			aRect;
	BMenuBar		*menubar;
	BMenuItem		*item;
	BMenu			*a_menu;

	// set up a rectangle and instantiate a new window
	aRect.Set(10, 20, 10 + 2*(NN*8) + 20 + 55 + SWIDTH + 60, 380 + 20 + 60);
	aWindow = new TWindow(aRect);
	
	aRect.Set( 0, 0, 1000, 15);
	menubar = new BMenuBar(aRect, "MB");
	
	a_menu = new BMenu("File");
	a_menu->AddItem(new BMenuItem("Edit", new BMessage(EDIT), 'E'));
	a_menu->AddItem(new BMenuItem("Save", new BMessage(SAVE), 'S'));
	a_menu->AddItem(new BMenuItem("Apply", new BMessage(APPLY), 'A'));
	menubar->AddItem(a_menu);

	a_menu = new BMenu("Edit");
	a_menu->AddItem(new BMenuItem("Undo", new BMessage(UNDO), 'Z'));
	a_menu->AddItem(new BMenuItem("Reverse", new BMessage(REVERSE), 'R'));
	a_menu->AddItem(new BMenuItem("Copy", new BMessage(COPY), 'C'));
	a_menu->AddItem(new BMenuItem("Cut", new BMessage(CUT), 'X'));
	a_menu->AddItem(new BMenuItem("Paste", new BMessage(PASTE), 'V'));
	a_menu->AddItem(new BMenuItem("Invert Horizontal", new BMessage(INVERTH), '_'));
	a_menu->AddItem(new BMenuItem("Invert Vertical", new BMessage(INVERTV), '|'));
	menubar->AddItem(a_menu);
	aWindow->background->AddChild(menubar);
		
	aRect.Set(60+(NN*8), 60 + 20+(NN*8)+32, 60+(NN*8)+32*5, 60 + 20+(NN*8)+32+32);
	aRect.OffsetBy(0,VP);
	pview = new PickView(aRect, "picker");
	aWindow->background->AddChild(pview);
	aRect.Set(20, 60 + 20, 20+(NN*8)-1 + 12 + (8*NN+20), 60 + 20+(NN*8)-1 + 12);
	aRect.OffsetBy(0,VP);
	bm_view = new BMView(aRect, "BMView");
	aWindow->background->AddChild(bm_view);
	
	aRect.Set(20+2*(NN*8)+90, 20, 20+2*(NN*8)+90+SWIDTH+1, 20 + SWIDTH+1 + 150);
	aRect.OffsetBy(0,VP);
	s_view = new SelectView(aRect, "select");	
	scr_view = new BScrollView("select", (BView*)s_view,
							   B_FOLLOW_LEFT|B_FOLLOW_TOP,
							   0, FALSE, TRUE);	
	aWindow->background->AddChild(scr_view);

	aRect.left = 10;
	aRect.top = 60+20+(NN*8)+32+32+10;
	aRect.right = 10+468+2;
	aRect.bottom = aRect.top + 82;
	
	aRect.OffsetBy(0,VP);
	samp_view = new SampleView(aRect, "sample", bm_view->fm);
	aWindow->background->AddChild(samp_view);
	
	aWindow->fm = bm_view->fm;
	aWindow->Lock();
	bm_view->MakeFocus(TRUE);
	aWindow->Unlock();
	// add view to window
	
	// make window visible
	aWindow->Show();
}

main()
{	
	TApplication *myApplication;

	myApplication = new TApplication();
	myApplication->Run();
	
	delete(myApplication);
	return(0);
}












