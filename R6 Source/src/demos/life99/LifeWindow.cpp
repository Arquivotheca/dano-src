/*
	
	HelloWindow.cpp
	 
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef HELLO_WINDOW_H
#include "LifeWindow.h"
#endif
#ifndef HELLO_VIEW_H
#endif

#include <stdio.h>

#define REPEAT_0 'rpt0'
#define REPEAT_1 'rpt1'
#define RUN_CONTINUOUS 'runn'
#define CLEAR_SCREEN 'clyr'
#define	TIME_WARP 'warp'
#define	OPEN 'open'
#define SAVE 'save'
#define QUIT 'quit'
#define ALLOW_DEATH 'deth'
#define	WRAP_AROUND 'wrap'
#define ABOUT 'abut'

#define	MENU_BAR_HEIGHT	18.0

const char* kButtonName = "Press";


HelloWindow::HelloWindow(BRect frame)
				: BWindow(frame, "Life '99", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
//	fOpenPanel = new BFilePanel (B_OPEN_PANEL);
//	fSavePanel = new BFilePanel (B_SAVE_PANEL, (BMessenger*)this, NULL, B_FILE_NODE, false);

	BRect	menuBarRect (Bounds());
	
	menuBarRect.bottom = MENU_BAR_HEIGHT;
	
	fMenuBar = new BMenuBar (menuBarRect, "MenuBar");
	AddChild (fMenuBar);
	
	fFile = new BMenu ("File");
	fMenuBar->AddItem (fFile);
	fNew = new BMenuItem ("New", new BMessage(CLEAR_SCREEN), 'N', B_COMMAND_KEY );
	fFile->AddItem (fNew);
//	fOpen = new BMenuItem ("Open...", new BMessage(OPEN), 'O', B_COMMAND_KEY);
//	fFile->AddItem (fOpen);
//	fSave = new BMenuItem ("Save as...", new BMessage(SAVE), 'S', B_COMMAND_KEY);
//	fFile->AddItem(fSave);
	fFile->AddSeparatorItem();
	fAbout = new BMenuItem ("About Life '99..", new BMessage(ABOUT), 'A', B_COMMAND_KEY );
	fFile->AddItem (fAbout);
	fFile->AddSeparatorItem();
	fQuit = new BMenuItem ("Quit", new BMessage(QUIT), 'Q', B_COMMAND_KEY);
	fFile->AddItem(fQuit);
	
	fOptions = new BMenu ("Options");
	fMenuBar->AddItem(fOptions);
	fOld = new BMenuItem("Death from old age", new BMessage (ALLOW_DEATH));
	fOld->SetMarked(true);
	fOptions->AddItem(fOld);
	fWrap = new BMenuItem("Wrap around", new BMessage (WRAP_AROUND));
	fOptions->AddItem(fWrap);
	
	
	// set up rectangles and instantiate new views
	BRect aRect( Bounds() );
	aRect.top += 17;
	aRect.bottom -= 32;
	
	fView = new HelloView(aRect, "HelloView", "");
			
	aRect.bottom += 31;
	aRect.top = aRect.bottom - 32;
	fButtonsView = new BView ( aRect, "ButtonsView", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM, B_FRAME_EVENTS);
	
	
	// add views to window
	AddChild(fView);
	AddChild(fButtonsView);
	
	BRect	kButtonFrame;
	int32	width = (int32)aRect.right/5;
	int32	height = int32(aRect.bottom - aRect.top);
	
 	kButtonFrame.Set(0, 0, width, height );
	fButton = new BButton(kButtonFrame, "Stop", "Stop", new BMessage(REPEAT_0));
	fButtonsView->AddChild(fButton);

 	kButtonFrame.Set(width, 0, width*2, height );
	fButton = new BButton(kButtonFrame, "Step Once", "Step Once", new BMessage(REPEAT_1));
	fButtonsView->AddChild(fButton);
	
 	kButtonFrame.Set(width*2, 0, width*3, height );
	fButton = new BButton(kButtonFrame, "Run", "Run", new BMessage(RUN_CONTINUOUS));
	fButtonsView->AddChild(fButton);
	
 	kButtonFrame.Set(width*3, 0, width*4, height );
	fButton = new BButton(kButtonFrame, "Clear", "Clear", new BMessage(CLEAR_SCREEN));
	fButtonsView->AddChild(fButton);
	
 	kButtonFrame.Set(width*4, 0, width*5, height );
	fButton = new BButton(kButtonFrame, "Time Warp", "Time Warp", new BMessage(TIME_WARP));
	fButtonsView->AddChild(fButton);	
}

bool HelloWindow::QuitRequested()
{
	kill_thread(fLifeThread);			

	be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}

void	HelloWindow::MessageReceived(BMessage* message)
{

	switch (message->what)
	{
		case CLEAR_SCREEN:
			fView->ClearCells();
			//Intentional fallthrough
			
		case REPEAT_0:
			if ( send_data(fLifeThread,'stop', NULL, 0) != B_OK) kill_thread(fLifeThread);					
			fView->fEndless = false;
			fView->fRuns = 0;
			fView->fShow = true;
			fLifeThread = spawn_thread(HelloView::_Iterate, "Life Thread", 1, fView);
			resume_thread(fLifeThread);
			break;
			
		case REPEAT_1:
			if ( send_data(fLifeThread,'stop', NULL, 0) != B_OK) kill_thread(fLifeThread);					
			fView->fEndless = false;
			fView->fRuns = 1;
			fView->fShow = true;
			fLifeThread = spawn_thread(HelloView::_Iterate, "Life Thread", 1, fView);
			resume_thread(fLifeThread);
			break;
			
		case RUN_CONTINUOUS:
			if ( send_data(fLifeThread,'stop', NULL, 0) != B_OK) kill_thread(fLifeThread);			
			fView->fEndless = true;
			fView->fShow = true;
			fLifeThread = spawn_thread(HelloView::_Iterate, "Life Thread", 1, fView);
			resume_thread(fLifeThread);
			break;
			
		case TIME_WARP:
			if ( send_data(fLifeThread,'stop', NULL, 0) != B_OK) kill_thread(fLifeThread);			
			fView->fEndless = false;
			fView->fRuns = 1000;
			fView->fShow = false;
			fLifeThread = spawn_thread(HelloView::_Iterate, "Life Thread", 99, fView);
			resume_thread(fLifeThread);
			break;
			
/*		case OPEN:
			//fOpenPanel->Show();
			break;*/
			
/*		case SAVE:
			//fSavePanel->Show();
			break;*/
			
		case QUIT:
			QuitRequested();
			break;
			
		case ABOUT:
			be_app->PostMessage(B_ABOUT_REQUESTED);
			break;
			
		case ALLOW_DEATH:
			fOld->SetMarked( !(fOld->IsMarked()) );
			fView->fGrid.fOldAge = fOld->IsMarked();
			break;
			
		case WRAP_AROUND:
			fWrap->SetMarked( !(fWrap->IsMarked()) );
			fView->fGrid.fWrapAround = fWrap->IsMarked();
			break;
			
		default:
			BWindow::MessageReceived(message);
	}
}