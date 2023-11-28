#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <InterfaceDefs.h>

#include "BackgroundBox.h"

// ************************************************************************

TBackgroundBox::TBackgroundBox(BRect frame)
	: BView(frame, NULL, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS),
	fLineCount(0)
{
}

TBackgroundBox::~TBackgroundBox()
{
}

void TBackgroundBox::Draw(BRect)
{
	if (!fLineCount) return;
	const rgb_color kWhite 		= ViewColor().tint(0.2);
	const rgb_color kMediumGray = ViewColor().tint(1.2);
	PushState();
	for (int i=0 ; i<fLineCount ; i++) {
		if (fLineList[i].orientation)
		{ // draw horizontal
			SetHighColor(kMediumGray);
			StrokeLine(fLineList[i].frame.LeftTop(), fLineList[i].frame.RightTop());
			SetHighColor(kWhite);			
			StrokeLine(fLineList[i].frame.LeftBottom(), fLineList[i].frame.RightBottom());			
		} else { // draw vertical
			SetHighColor(kMediumGray);
			StrokeLine(fLineList[i].frame.LeftTop(), fLineList[i].frame.LeftBottom());			
			SetHighColor(kWhite);
			StrokeLine(fLineList[i].frame.RightTop(), fLineList[i].frame.RightBottom());			
		}
	}
	PopState();
}

void TBackgroundBox::AttachedToWindow()
{
	SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	SetLowUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	BView::AttachedToWindow();
}

void  TBackgroundBox::AddLine(BRect f, bool o)
{
	if (fLineCount < sizeof(fLineList)/sizeof(*fLineList)) {
		fLineList[fLineCount].frame = f;
		fLineList[fLineCount].orientation = o;
		fLineCount++;		
	}
}
