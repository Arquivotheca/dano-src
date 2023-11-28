//*****************************************************************************
//
//	File:		 3dmovWindow.cpp
//
//	Description: Window object used to include 3dView demo of the 3d kit.
//
//	Copyright 1996, Be Incorporated
//
//*****************************************************************************

#ifndef _3D_MOV_APP_H
#include "3dmovApp.h"
#endif

#ifndef _3D_MOV_WINDOW_H
#include "3dmovWindow.h"
#endif

// default positions of the 4 demos windows
static BRect Rects[4] = {
	BRect( 30.0, 30.0, 479.0, 479.0 ),
	BRect( 495.0, 12.0, 794.0, 310.0 ),
	BRect( 495.0, 295.0, 794.0, 593.0 ),
	BRect( 5.0, 285.0, 564.0, 591.0 ),
};

// names of the 4 demo windows
static char Titles[4][16] = {
	"Cube",
	"Sphere",
	"Pulse",
	"Book",
};

Z3dWindow::Z3dWindow(long index):
BWindow(Rects[index], Titles[index], B_TITLED_WINDOW, 0) {
	BRect	aRect;
	BRect	mbrect;		// Menu Bar area
	BMenu	*tmenu;		// temporary holder for menus
	BMenuItem	*item;		// temporary menu items
	
// keep a copy of its index in the application window list
	myIndex = index;
	aRect = Rects[index];
	aRect.OffsetTo(B_ORIGIN);
	
	mbrect.Set( 0, 0, 1000, 15 );
	
// Create the menu bar and modify aRect to prevent myView from drawing on top of it:
	mBar = new BMenuBar(mbrect, "MB");
	
	tmenu = new BMenu("File");
	
	mItems[0] = new BMenuItem(Titles[0], new BMessage(FIRST_WINDOW), '1');
	mItems[0]->SetTarget( be_app );
	tmenu->AddItem(mItems[0]);
	
	mItems[1] = new BMenuItem(Titles[1], new BMessage(SECOND_WINDOW), '2');
	mItems[1]->SetTarget( be_app );
	tmenu->AddItem(mItems[1]);
	
	mItems[2] = new BMenuItem(Titles[2], new BMessage(THIRD_WINDOW), '3');
	mItems[2]->SetTarget( be_app );
	tmenu->AddItem(mItems[2]);
	
	mItems[3] = new BMenuItem(Titles[3], new BMessage(FOURTH_WINDOW), '4');
	mItems[3]->SetTarget( be_app );
	tmenu->AddItem(mItems[3]);
		
	tmenu->AddSeparatorItem();

	mItems[4] = new BMenuItem("Pause", new BMessage(PAUSE_WINDOWS), 'P');
	if (((Z3dApplication*)be_app)->Pause) mItems[4]->SetMarked(TRUE);
	mItems[4]->SetTarget( be_app );
	tmenu->AddItem(mItems[4]);

	item = new BMenuItem("Close", new BMessage(B_CLOSE_REQUESTED), 'W');
	tmenu->AddItem(item);
	
	tmenu->AddSeparatorItem();

	item = new BMenuItem("About 3dmov", new BMessage(B_ABOUT_REQUESTED));
	item->SetTarget( be_app );
	tmenu->AddItem(item);
	
	tmenu->AddSeparatorItem();
	
	item = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q');
	item->SetTarget(be_app);
	tmenu->AddItem(item);
		
	mBar->AddItem(tmenu); // duh... so we can actually see something...
	
	AddChild(mBar);
	SetKeyMenuBar(mBar);
	
	aRect.top += mBar->Bounds().Height() + 1; // push myView down the size of the menu bar
	
// send the index reference to the View constructor to differentiate the 4 demos.
	if (index < 3)
		myView = new Z3dView(aRect, "3dView", index);
	else
		myView = new B3dBookView(aRect, "3dView");
	AddChild(myView);
	
// so we don't shrink the window too small (Xmemset crashes when it gets too small):
	SetSizeLimits(30.0, 1600.0, 30.0, 1600.0);
}

bool Z3dWindow::QuitRequested() {
	((Z3dApplication*)be_app)->ActiveWindow(myIndex, 0);
	if (myIndex < 3)
		((B3dView*)myView)->Stop();
	else if (myIndex == 3)
		((B3dBookView*)myView)->Stop();
	return(TRUE);
}














