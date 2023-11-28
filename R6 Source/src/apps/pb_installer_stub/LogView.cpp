/*
	
	HelloView.cpp
	
	Copyright 1995 Be Incorporated, All Rights Reserved.
	
*/

#include "LogListView.h"
#include "LogView.h"
#include <ScrollView.h>

LogView::LogView(BRect rect)
	   	   : BView(rect, "logview", B_FOLLOW_ALL, B_WILL_DRAW)
{
	BRect r;
	
	r = Bounds();
	r.InsetBy(12,12);
	
	r.right -= B_V_SCROLL_BAR_WIDTH;
	
	BScrollView *sv = new BScrollView("scroller",
									new LogListView(r),
									B_FOLLOW_ALL, 0,
									false, true,
									B_FANCY_BORDER);
	
	AddChild(sv);	
	//SetViewColor(230, 230, 230);
}

void LogView::AttachedToWindow()
{
	BView::AttachedToWindow();

	BView *sv = FindView("listview");
	if (sv) {	
		BRect fr = sv->Frame();
		sv->FrameResized(fr.Width(),fr.Height());
	}
}


/****************************/



