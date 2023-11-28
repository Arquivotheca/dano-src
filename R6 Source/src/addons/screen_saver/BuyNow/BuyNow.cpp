/* buynow.cpp
 *
 */

#include <stdio.h>
#include <AppKit.h>
#include <InterfaceKit.h>

#include "BuyNow.h"

extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *msg, image_id img)
{
	return new BuyNow(msg, img);
}

BuyNow::BuyNow(BMessage *msg, image_id img)
		: BScreenSaver(msg, img)
{
}

void
BuyNow::StartConfig(BView *view)
{
	view->AddChild(new BStringView(BRect(10, 10, 200, 35), B_EMPTY_STRING, 
						"BUY NOW"));
}

status_t
BuyNow::StartSaver(BView *view, bool)
{
	view->SetViewColor(0, 0, 100);

	BFont f, nf;
	font_height fh;
	float s;
	BRect r = view->Bounds();
			
	view->GetFont(&f);
	nf = f;
	nf.GetHeight(&fh);
	s = nf.Size();
	
	while ((fh.ascent + fh.descent + fh.leading) < (r.bottom - 40)/2) {
		s++;
		nf.SetSize(s);
		nf.GetHeight(&fh);
	}

	view->SetFont(&nf);
	view->SetHighColor(255, 255, 255);

	SetTickSize(500000);
	return B_OK;
}

void
BuyNow::Draw(BView *view, int32 frame)
{
	if (frame % 2 == 0) {
		view->Invalidate();
		return;
	}
	
	BRect r = view->Bounds();
	float w;
	font_height fh;
	view->GetFontHeight(&fh);

	w = view->StringWidth("BUY");
	view->MovePenTo((r.right - w) / 2, 10 + fh.ascent + fh.leading);
	view->DrawString("BUY");
	
	w = view->StringWidth("NOW");
	view->MovePenTo((r.right - w) / 2, 10 + 20 + (fh.ascent*2) + (fh.leading*2));
	view->DrawString("NOW");
}
