#include "BitsContainer.h"

TBitsContainer::TBitsContainer(BRect frame, uchar *bits, color_space cspace,
	bool makeChild)
	: BView(frame,"bits",B_FOLLOW_NONE,0)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
	fHasChild = makeChild;
	fColorSpace = cspace;
	fBitmap = new BBitmap(Bounds(), cspace, true);
	if (bits)
		fBitmap->SetBits(bits, fBitmap->BitsLength(), 0, B_COLOR_8_BIT);
	else {
		// 	set all the bits to transparent
		uchar *tempbits = (uchar*)fBitmap->Bits();
		memset(tempbits, B_TRANSPARENT_8_BIT, fBitmap->BitsLength());
	}
	
	if (fHasChild)
		fBitmap->AddChild(this);
}


TBitsContainer::~TBitsContainer()
{
	if (fHasChild)
		fBitmap->RemoveChild(this);

	delete fBitmap;
}

bool
TBitsContainer::HasChild()
{
	return fHasChild;
}

color_space
TBitsContainer::ColorSpace()
{
	return fColorSpace;
}

BBitmap*
TBitsContainer::Bitmap()
{
	return fBitmap;
}

uchar*
TBitsContainer::Bits()
{
	return (uchar*)fBitmap->Bits();
}

int32 
TBitsContainer::BitsLength()
{
	return fBitmap->BitsLength();
}

void
TBitsContainer::NewBitmap(BBitmap *b)
{
//	fBitmap->SetBits(b->Bits(),b->BitsLength(),0,fColorSpace);	// this doesn't really work
	SetBits((uchar*)b->Bits(), b->Bounds());
}

void
TBitsContainer::SetBits(uchar* bits, BRect r)
{
	if (fBitmap)
		if (fHasChild)
			RemoveSelf();	// if we had a child view, don't delete it
		
	BBitmap* newBitmap = new BBitmap(r, fColorSpace, true);
	if (newBitmap->Lock()) {

		if (bits)
			newBitmap->SetBits(bits, newBitmap->BitsLength(), 0, B_COLOR_8_BIT);
		else {
			uchar *tempbits = (uchar*)newBitmap->Bits();
			memset(tempbits, B_TRANSPARENT_8_BIT, newBitmap->BitsLength());
		}
	
		if (fHasChild)
			newBitmap->AddChild(this);
			
		delete fBitmap;
		fBitmap = newBitmap;
	}
}

void 
TBitsContainer::ResizeContainer(BRect frame)
{
	if (fBitmap)
		if (fHasChild)
			RemoveSelf();	// if we had a child view, don't delete it
		
	BBitmap* newBitmap = new BBitmap(frame, fColorSpace, true);
	if (newBitmap->Lock()) {

		uchar *tempbits = (uchar*)newBitmap->Bits();
		memset(tempbits, B_TRANSPARENT_8_BIT, newBitmap->BitsLength());

		if (fHasChild)
			newBitmap->AddChild(this);
			
		delete fBitmap;
		fBitmap = newBitmap;
	}
}

bool 
TBitsContainer::Lock()
{
	return fBitmap->Lock();
}

void 
TBitsContainer::Unlock()
{
	fBitmap->Unlock();
}

void 
TBitsContainer::Invalidate()
{
	if (!HasChild())
		return;
	
	if (fBitmap->Lock()) {
		Invalidate();
		fBitmap->Unlock();
	}
}
