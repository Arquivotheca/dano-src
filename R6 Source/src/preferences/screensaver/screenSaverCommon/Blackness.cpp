#include "Blackness.h"
#include <StringView.h>
#include <stdlib.h>

Blackness::Blackness(BMessage *message, image_id image)
 : BScreenSaver(message, image)
{
}

void Blackness::StartConfig(BView *view)
{
	view->AddChild(new BStringView(BRect(10, 10, 200, 35), B_EMPTY_STRING, "Basic blackness"));
}

status_t Blackness::StartSaver(BView *v, bool /* preview */)
{
	SetTickSize(10000000);
	v->SetLowColor(0, 0, 0);
	return B_OK;
}

void Blackness::Draw(BView *v, int32 frame)
{
	if(frame == 0)
		v->FillRect(v->Bounds(), B_SOLID_LOW);
}
