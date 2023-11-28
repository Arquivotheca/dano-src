#include "CaptureSizeDialog.h"
#include "VideoRecorder.h"
#include <TextControl.h>
#include <Button.h>


CaptureSizeDialog::CaptureSizeDialog(BWindow *target, int width, int height)
	: BWindow(BRect(100,100,300,200), "Capture Size", B_MODAL_WINDOW,
		B_ASYNCHRONOUS_CONTROLS)
	, fTarget(target)
{
	BRect frame(Bounds());
	BBox* box = new BBox(frame, "bg", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER);
	AddChild(box);
	box->SetFont(be_plain_font); 
	
	char width_string[16];
	sprintf(width_string, "%d", width);
	frame.bottom = 10;
	frame.right /= 2;
	fWidth = new BTextControl(frame, "Width", "Width", width_string, NULL);
	box->AddChild(fWidth);
	char height_string[16];
	sprintf(height_string, "%d", height);
	frame.OffsetBy(frame.right, 0);
	fHeight = new BTextControl(frame, "Height", "Height", height_string, NULL);
	box->AddChild(fHeight);
	
	BButton* btn = new BButton(BRect(0,0,50,10), "", "Done",
	                           new BMessage(msg_capturesize),
	                           B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	box->AddChild(btn);
	btn->MoveTo(Bounds().Width() - 60, Bounds().bottom - 10 - btn->Frame().Height());
	
	MoveTo(target->Frame().LeftTop() + BPoint(20, 250));
}

void 
CaptureSizeDialog::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case msg_capturesize: {
			int width, height;
			width = strtol(fWidth->Text(), (char **)NULL, 0);
			height = strtol(fHeight->Text(), (char **)NULL, 0);
			message->AddInt32("width", width);
			message->AddInt32("height", height);
			message->AddBool("other", true);
			fTarget->PostMessage(message);
			Lock();
			Quit();
		} break;
			
		default:
			BWindow::MessageReceived(message);
			break;
	}
}

bool 
CaptureSizeDialog::QuitRequested()
{
	return true;
}

