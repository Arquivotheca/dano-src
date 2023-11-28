#include "Crash.h"
#include <StringView.h>
#include <Font.h>
#include <stdio.h>

// MAIN INSTANTIATION FUNCTION
extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *message, image_id image)
{
	return new Crash(message, image);
}

Crash::Crash(BMessage *message, image_id image)
 : BScreenSaver(message, image)
{
}

void Crash::StartConfig(BView *view)
{
	view->AddChild(new BStringView(BRect(10, 10, 200, 35), B_EMPTY_STRING, "Crash: test crash protection"));
}

status_t Crash::StartSaver(BView *, bool preview)
{
	SetTickSize(1000000);
	previewmode = preview;
	return B_OK;
}

void Crash::Draw(BView *view, int32 frame)
{
	if(frame == 0)
	{
		BFont	f;
		view->SetHighColor(255, 255, 255);
		view->GetFont(&f);
		f.SetSize(500);
		view->SetFont(&f);
	}

	if(frame < 5 || previewmode)
	{
		char tmp[5];
		sprintf(tmp, "%ld", 5 - frame);
		view->FillRect(view->Bounds(), B_SOLID_LOW);
		view->DrawString(tmp, BPoint(400, 400));
	}
	else
		*(char *)0 = 0;	// crash and burn
}
