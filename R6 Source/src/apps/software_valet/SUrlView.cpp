#include "SUrlView.h"
#include <malloc.h>
#include <string.h>
#include <Roster.h>
#include <Window.h>

SUrlView::SUrlView(BRect bounds,
			const char *name, 
			const char *text,
			uint32 resizeFlags,
			uint32 flags)
	:	BStringView(bounds,name,text,resizeFlags,flags),
		fValid(false),
		appSig(NULL)
{
	SetHighColor(0,0,0);
	SetAppSig("application/x-vnd.Be-NPOS");
}

SUrlView::~SUrlView()
{
	free(appSig);
}

void SUrlView::SetAppSig(const char *sig)
{
	free(appSig);
	appSig = strdup(sig);
}

void SUrlView::Draw(BRect up)
{
	up;
	
	//BStringView::Draw(up);
	BRect bounds = Bounds();
	const char *txt = Text();
	char	*resultArray[1];
	resultArray[0] = (char *)malloc(strlen(txt)+1+3);
	BFont	f;
	GetFont(&f);
 	f.GetTruncatedStrings(&txt, 
							1, 
							B_TRUNCATE_MIDDLE,
							bounds.Width(),
							resultArray);
	DrawString(resultArray[0], BPoint(bounds.left, bounds.bottom-3));
	if (fValid) {
		StrokeLine(bounds.LeftBottom(),
				BPoint( bounds.left + StringWidth(resultArray[0]) + 1, 
						bounds.bottom ));
	}
	free(resultArray[0]);
}

void SUrlView::Highlight(bool state)
{
	if (fValid) {
		if (state)
			SetHighColor(100,0,255);
		else
			SetHighColor(0,0,200);
		Invalidate();
		Window()->UpdateIfNeeded();
	}
}

void SUrlView::SetValid(bool state)
{
	fValid = state;
	
	if (fValid)
		SetHighColor(0,0,200);
	else
		SetHighColor(0,0,0);
	Invalidate();
}


void SUrlView::MouseDown(BPoint pt)
{
	if (!fValid)
		return;
		
	bool wasin = true;
	Highlight();
	uint32 btns;
	do {
		GetMouse(&pt, &btns);
		
		if (Bounds().Contains(pt)) {
			if (!wasin) {
				Highlight();
				wasin = true;
			}
		}
		else if (wasin) {
			// unhighlight
			wasin = false;
			Highlight(wasin);
		}
		
		snooze(20*1000);
	} while (btns);
	
	if (Bounds().Contains(pt)) {
		const char	*args[1];
		args[0] = Text();
		be_roster->Launch(appSig,1,(char **)args);
	}
	
	// unhighlight
	Highlight(false);
}
