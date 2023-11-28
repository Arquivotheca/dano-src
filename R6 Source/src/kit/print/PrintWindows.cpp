// ***********************************************************************
// libbe.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#include <Window.h>
#include <View.h>
#include <Rect.h>
#include <Button.h>
#include <Screen.h>

#include <print/PrintConfigView.h>

// For the ressources
#include "AboutBox.h"

#include "PrintWindows.h"


#define BORDER 		4.0f
#define MSG_OK		'save'
#define MSG_CANCEL	'cncl'

BSimpleWindow::BSimpleWindow(const char *title, BPrintConfigView *view)
	:	BWindow(	view->Bounds(), title,
					B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
					B_NOT_ZOOMABLE | B_NOT_H_RESIZABLE | B_NOT_V_RESIZABLE | B_ASYNCHRONOUS_CONTROLS),
		fAsyncSem(0),
		fReturnValue(B_ERROR),
		fUserView(view)
{
	BRect r = Bounds();
	
	// the Ok/Cancel view
	BView *controlView = new ControlView;
	controlView->MoveTo(r.right - (controlView->Frame().Width()+1.0f), r.bottom+1);
	r.bottom += controlView->Frame().Height()+1.0f;

	// resize the window
	ResizeTo(r.Width(), r.Height());

	// the background view
	BView *background = new BView(r, "be:background", B_FOLLOW_ALL, B_WILL_DRAW);
	background->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	background->AddChild(fUserView);
	background->AddChild(controlView);

	// Attach everyone to the window	
	AddChild(background);	
}

status_t BSimpleWindow::Go()
{
	// Determine if the current thread is a window
	// thread. If so then call UpdateIfNeeded to keep window updated.
	BWindow *window = dynamic_cast<BWindow *>(BLooper::LooperForThread(find_thread(NULL)));
	fAsyncSem = create_sem(0, "async");

	// Center the window, and possibly change it's look
	BRect screenFrame;
	BPoint pt(B_ORIGIN);
	if (window)
	{
		screenFrame = window->Frame();
		pt = window->Frame().LeftTop();
		SetLook(B_MODAL_WINDOW_LOOK);
		SetFeel(B_MODAL_APP_WINDOW_FEEL);
	}
	else
	{
		screenFrame = BScreen(B_MAIN_SCREEN_ID).Frame();
		SetLook(B_TITLED_WINDOW_LOOK);
		SetFeel(B_NORMAL_WINDOW_FEEL);
	}
	pt.x += (screenFrame.Width()- Bounds().Width())*0.5f;
	pt.y += (screenFrame.Height() - Bounds().Height())*0.5f;
	MoveTo(pt);	

	// Dislay the window
	Show();

	status_t err;
	if (window)
	{ // A window is being blocked. We'll keep the window updated by calling UpdateIfNeeded.
		while (true) 
		{
			while ((err = acquire_sem_etc(fAsyncSem, 1, B_TIMEOUT, 50000)) == B_INTERRUPTED)
			{ // Nothing to do
			}
			if (err == B_BAD_SEM_ID)
				break;
			window->UpdateIfNeeded();
		}
	}
	else
	{
		do
		{
			err = acquire_sem(fAsyncSem);
		} while (err == B_INTERRUPTED);
	}

	if (Lock() != B_OK)
	{
		err = fReturnValue;
		Close();
	}

	return err;
}

bool BSimpleWindow::QuitRequested()
{
	if (fAsyncSem)
	{
		fReturnValue = B_CANCELED;
		delete_sem(fAsyncSem);
		return false;
	}
	return true;
}

void BSimpleWindow::MessageReceived(BMessage *m)
{
	switch(m->what)
	{
		case MSG_OK:
			fReturnValue = fUserView->Save();
			delete_sem(fAsyncSem);
			break;

		case MSG_CANCEL:
			fReturnValue = B_CANCELED;
			delete_sem(fAsyncSem);
			break;
		
		default:
			BWindow::MessageReceived(m);
	}
}


// ***********************************************************************
#pragma mark -

ControlView::ControlView()
	: 	BView(BRect(0,0,0,0), "be:controlview", B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM, B_WILL_DRAW|B_NAVIGABLE_JUMP)
{
	size_t size;
	const char *save = (const char *)GetLibbeResources()->LoadResource(B_STRING_TYPE, "be:print:save", &size);
	const char *cancel = (const char *)GetLibbeResources()->LoadResource(B_STRING_TYPE, "be:print:cancel", &size);

	fOkButton = new BButton(		BRect(0,0,0,0),
									"be:ok", save,
									new BMessage(MSG_OK),
									B_FOLLOW_RIGHT | B_FOLLOW_V_CENTER);

	fCancelButton = new BButton(	BRect(0,0,0,0),
									"be:cancel", cancel,
									new BMessage(MSG_CANCEL),
									B_FOLLOW_RIGHT | B_FOLLOW_V_CENTER);

	fOkButton->MakeDefault(true);
	fOkButton->ResizeToPreferred();	
	fCancelButton->ResizeToPreferred();
	
	ResizeToPreferred();
	AddChild(fCancelButton);
	AddChild(fOkButton);	
}

void ControlView::AttachedToWindow()
{
	SetViewColor(Parent()->ViewColor());
	LayoutChildren();
	fOkButton->SetTarget(Window());	
	fCancelButton->SetTarget(Window());
	Window()->AddCommonFilter(new MsgFilter(this));
}

void ControlView::GetPreferredSize(float *width, float *height)
{
	*width = fOkButton->Frame().Width() + fCancelButton->Frame().Width() + 4*BORDER;
	*height = fOkButton->Frame().Height() + 2*BORDER;
}

void ControlView::LayoutChildren()
{
	BRect b = Bounds();
	fOkButton->MoveTo(b.right - BORDER - fOkButton->Bounds().Width(), (b.Height()-fOkButton->Bounds().Height())*0.5f);
	fCancelButton->MoveTo(fOkButton->Frame().left - 2*BORDER - fCancelButton->Bounds().Width(), (b.Height()-fCancelButton->Bounds().Height())*0.5f);
}

void ControlView::MessageReceived(BMessage *message)
{
	switch (message->what)
	{
		case ENABLE_OK:		fOkButton->SetEnabled(true);	break;
		case DISABLE_OK:	fOkButton->SetEnabled(false);	break;
		default:
			BView::MessageReceived(message);
	}
}

