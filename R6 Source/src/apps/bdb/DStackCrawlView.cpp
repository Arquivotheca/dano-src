/*	$Id: DStackCrawlView.cpp,v 1.4 1999/01/11 22:38:17 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/10/98 14:38:19
*/

#include "bdb.h"
#include "DStackCrawlView.h"
#include "DStackFrame.h"
#include "DStackCrawl.h"
#include "DMessages.h"
#include "DUtils.h"

#include <Window.h>
#include <ScrollView.h>

DStackCrawlView::DStackCrawlView(BRect frame, const char *name,
	DStackCrawl& sc, unsigned long resizeMask)
	: BView(frame, name, resizeMask, B_WILL_DRAW | B_FRAME_EVENTS)
	, fStackCrawl(sc)
{
	font_height fh;
	be_plain_font->GetHeight(&fh);
	
	BRect hRect = Bounds();
	hRect.bottom = hRect.top + fh.ascent + fh.descent + 6;

	BRect r(Bounds());
	r.top = hRect.bottom;
	
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.InsetBy(2, 2);
	
	fStack = new BListView(r, "stacklist", B_SINGLE_SELECTION_LIST,
		B_FOLLOW_ALL_SIDES);
	AddChild(new BScrollView("stackscroller", fStack,
		B_FOLLOW_ALL_SIDES, 0, false, true));
	
	fStack->SetSelectionMessage(new BMessage(kMsgStackListSelection));
	
//	SetViewColor(kViewColor);
} /* DStackCrawlView::DStackCrawlView */

void DStackCrawlView::Draw(BRect /*update*/)
{
	font_height fh;
	be_plain_font->GetHeight(&fh);
	
	BRect hRect = Bounds();
	hRect.bottom = hRect.top + fh.ascent + fh.descent + 4;
	
	SetHighColor(kVeryDark);
	StrokeRect(hRect);
	
	hRect.InsetBy(1, 1);
	
	SetHighColor(kWhite);
	StrokeLine(hRect.LeftBottom(), hRect.LeftTop());
	StrokeLine(hRect.LeftTop(), hRect.RightTop());
	
	SetHighColor(kDarkShadow);
	StrokeLine(hRect.LeftBottom(), hRect.RightBottom());
	StrokeLine(hRect.RightBottom(), hRect.RightTop());

	hRect.InsetBy(1, 1);
		
	SetLowColor(kViewColor);
	FillRect(hRect, B_SOLID_LOW);
	
	hRect.InsetBy(-1, 0);
	
	float y = hRect.bottom - fh.descent;
	float x = hRect.left + 4;
	
	SetHighColor(kBlack);
	DrawString("Stack", BPoint(x, y));
} // DStackCrawlView::Draw

void DStackCrawlView::Update()
{
	Window()->DisableUpdates();
	Clear();
	
	for (uint32 i = 0; i < fStackCrawl.CountFrames(); i++)
	{
		try
		{
			DStackFrame& sf = fStackCrawl.GetNthStackFrame(i);
			
			string name;
			sf.GetFunctionName(name);
			fStack->AddItem(new DItem<ptr_t>(name.c_str(), sf.GetPC()));
		}
		catch (...) {}
	}

	uint32 ix = fStackCrawl.DeepestUserCode();
	fStack->Select(ix);

	Window()->EnableUpdates();
} /* DStackCrawlView::Update */

void DStackCrawlView::Clear()
{
	while (fStack->CountItems())
		delete fStack->RemoveItem(fStack->CountItems() - 1);
} /* DStackCrawlView::Clear */

void DStackCrawlView::FrameResized(float w, float h)
{
	BView::FrameResized(w, h);
	Draw(Bounds());
} // DStackCrawlView::FrameResized
