#include "module.h"
#include <StringView.h>
#include <stdlib.h>
#include <time.h>


// MAIN INSTANTIATION FUNCTION
extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *message, image_id image)
{
	return new module(message, image);
}

module::module(BMessage *message, image_id image)
 : BScreenSaver(message, image)
{
}

void module::StartConfig(BView *view)
{
	view->AddChild(new BStringView(BRect(10, 10, 200, 35), B_EMPTY_STRING, "module by Howard Berkey"));
}

status_t module::StartSaver(BView *view, bool /* preview */)
{
	view->FillRect(view->Bounds(), B_SOLID_LOW);	// Erase
	//SetTickSize(1000);
	return B_OK;
}

void module::Draw(BView *view, int32 frame)
{
	BRect r = view->Bounds();
	int rectwidth2 = (int) ((r.right - r.left) / 2);
	int rectheight2 = (int) ((r.bottom - r.top) / 2);

	if(frame % 1000 == 0) 
		view->FillRect(view->Bounds(), B_SOLID_LOW);	// Erase
	
	r.InsetBy(frame % rectwidth2, frame % rectheight2);	
	view->SetHighColor(((frame + ((rand() % 255) / 10))  % 255),((frame + ((rand() % 255) / 10)) % 255),((frame + ((rand() % 255) / 10)) % 255));
	view->StrokeEllipse(r);
}
