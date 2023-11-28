#include <View.h>
#include <Bitmap.h>

#include "bdb.h"
#include "DTracepoint.h"

void 
DTracepoint::Draw(BView *inView, BPoint pos, uint32 kind)
{
	unsigned char *enabledIconBits = EnabledIconBits();
	
	if (kind & kDisabled) {
		inView->SetHighColor(kShadow);
		inView->StrokeLine(BPoint(pos.x, pos.y - 3), BPoint(pos.x + 4, pos.y - 3));
		inView->SetHighColor(kBlack);
	} else {
		BBitmap bm(BRect(0, 0, 4, 4), B_COLOR_8_BIT);
		bm.SetBits(enabledIconBits, 40, 0, B_COLOR_8_BIT);
	
		pos.y -= 5;
	
		inView->SetDrawingMode(B_OP_OVER);
		inView->DrawBitmap(&bm, pos);
		inView->SetDrawingMode(B_OP_COPY);
	}
}

bool 
DTracepoint::Track(BView *inView, BPoint pos, bool wasSet)
{
	BPoint where, p1, p2;
	ulong btns;
	bool in = true;
	float y = pos.y - 3;
	
	BRect rect(pos.x - 1, pos.y - 6, pos.x + 6, pos.y + 1);
	inView->FillRect(rect, B_SOLID_LOW);
	
	p1.x = pos.x;
	p1.y = y;
	p2.x = pos.x + 4;
	p2.y = y;
	
	inView->SetHighColor(255, 0, 0, 255);
	inView->StrokeLine(p1, p2);
	
	do
	{
		snooze(25000);
		inView->GetMouse(&where, &btns);

		if (in != rect.Contains(where)) {
			inView->FillRect(rect, B_SOLID_LOW);

			if (in) {
				in = false;
				if (wasSet)
					Draw(inView, pos, fKind | kDisabled);
				else
					Draw(inView, pos, fKind & ~kDisabled);
			} else {
				in = true;
				inView->SetHighColor(255, 0, 0, 255);
				inView->StrokeLine(p1, p2);
			}
		}
	}
	while (btns);
	
	return in;
}

