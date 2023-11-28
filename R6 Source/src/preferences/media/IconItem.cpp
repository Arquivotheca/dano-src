#include "IconItem.h"

IconItem::IconItem()
	:	mIn(false),
		mOut(false),
		mInputIcon(BRect(1, 1, kSmallIconWidth, kSmallIconHeight), kSmallIconColorSpace),
		mOutputIcon(BRect(1, 1, kSmallIconWidth, kSmallIconHeight), kSmallIconColorSpace),
		mName("name"),
		mIndent(0),
		mBaseline(4)
{
}


IconItem::~IconItem()
{
}

void 
IconItem::DrawItem(BView *owner, BRect rect, bool complete)
{
	if (IsSelected()) {
		owner->SetHighColor(152,152,152,255);
		owner->SetLowColor(152,152,152,255);
	} else {
		owner->SetHighColor(255,255,255,255);
		owner->SetLowColor(255,255,255,255);	
	}
	BRect fillRect(rect);
	fillRect.left = 0;
	owner->FillRect(rect);	

	if (mIn && mOut) {
		owner->SetDrawingMode(B_OP_OVER);
		owner->MovePenTo(rect.left + 4, rect.top + 1);
		owner->DrawBitmapAsync(&mInputIcon);
	}
	else if (mIn) {
		owner->SetDrawingMode(B_OP_OVER);
		owner->MovePenTo(rect.left + 4 + mIndent, rect.top + 1);
		owner->DrawBitmapAsync(&mInputIcon);		
	}
	
	if (mOut) {
		owner->SetDrawingMode(B_OP_OVER);
		owner->MovePenTo(rect.left + 4 + mIndent, rect.top + 1);
		owner->DrawBitmapAsync(&mOutputIcon);
	}
	
	owner->SetHighColor(0,0,0,255);
	owner->SetDrawingMode(B_OP_COPY);
	owner->MovePenTo(rect.left + 28 + mIndent, rect.bottom - mBaseline);
	owner->DrawString(mName.String());
	
}

void 
IconItem::SetBaseline(float baseline)
{
	mBaseline = baseline;
}

void 
IconItem::SetIndent(float indent)
{
	mIndent = indent;
}

void 
IconItem::SetName(const char *name)
{
	mName.SetTo(name);
}

void 
IconItem::SetInputIcon(void *bits)
{
	if (!bits) {
		mIn = false;
		return;
	}
	mInputIcon.SetBits(bits, kSmallIconWidth*kSmallIconHeight, 0, kSmallIconColorSpace);
	mIn = true;
}

void 
IconItem::SetOutputIcon(void *bits)
{
	if (!bits) {
		mOut = false;
		return;
	}
	mOutputIcon.SetBits(bits, kSmallIconWidth*kSmallIconHeight, 0, kSmallIconColorSpace);
	mOut = true;
}

const char *
IconItem::Name()
{
	return mName.String();
}

