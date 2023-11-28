//========================================================================
//	MProjectLine.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#include "MProjectLine.h"
#include "MProjectView.h"
#include "CString.h"
#include "Utils.h"

#include <Region.h>
#include <Bitmap.h>

#define L 0x12
#define G 0x1b
#define D 0x12
#define W 0x3f
#define B 0x00
#define T 255
#define X 0x26
#define x 0x6c
#define V 0x16

const float kBitMapWidth = 10;
const float kBitMapHeight = 10;
const BPoint kArrowPoint(250.0, 2.0);

const char rightArrowData[] = {
	B,B,B,B,B,B,B,B,B,T,
	B,W,W,W,W,W,W,W,B,D,
	B,W,W,W,B,W,W,W,B,D,
	B,W,W,W,B,B,W,W,B,D,
	B,W,W,W,B,B,B,W,B,D,
	B,W,W,W,B,B,W,W,B,D,
	B,W,W,W,B,W,W,W,B,D,
	B,W,W,W,W,W,W,W,B,D,
	B,B,B,B,B,B,B,B,B,D,
	T,D,D,D,D,D,D,D,D,D,
};

const char downArrowData[] = {
	B,B,B,B,B,B,B,B,B,T,
	B,W,W,W,W,W,W,W,B,D,
	B,W,W,W,W,W,W,W,B,D,
	B,W,W,W,W,W,W,W,B,D,
	B,W,B,B,B,B,B,W,B,D,
	B,W,W,B,B,B,W,W,B,D,
	B,W,W,W,B,W,W,W,B,D,
	B,W,W,W,W,W,W,W,B,D,
	B,B,B,B,B,B,B,B,B,D,
	T,D,D,D,D,D,D,D,D,D,
};

const char checkmarkData[] = {
	T,T,T,T,T,T,T,T,x,x,
	T,T,T,T,T,T,T,x,X,X,
	T,T,T,T,T,T,x,X,X,X,
	T,X,x,T,T,x,X,X,X,T,
	X,X,x,T,x,X,X,X,T,T,
	X,X,x,x,X,X,X,T,T,T,
	X,X,X,X,X,X,T,T,T,T,
	X,X,X,X,X,T,T,T,T,T,
	X,X,X,X,T,T,T,T,T,T,
	T,X,X,T,T,T,T,T,T,T,
};

const char touchedData[] = {
	T,T,T,T,T,T,T,T,x,x,
	T,T,T,T,T,T,T,x,V,V,
	T,T,T,T,T,T,x,V,V,V,
	T,V,x,T,T,x,V,V,V,T,
	V,V,x,T,x,V,V,V,T,T,
	V,V,x,x,V,V,V,T,T,T,
	V,V,V,V,V,V,T,T,T,T,
	V,V,V,V,V,T,T,T,T,T,
	V,V,V,V,T,T,T,T,T,T,
	T,V,V,T,T,T,T,T,T,T,
};

#define z 33

const uchar check_icon_1[] = {
 T,T,T,T,T,T,T,T,T,T,
 T,T,T,T,T,T,T,T,z,z,
 T,T,T,T,T,T,T,z,z,z,
 T,T,T,T,T,T,z,z,z,T,
 z,z,T,T,T,z,z,z,T,T,
 z,z,z,T,z,z,z,T,T,T,
 T,z,z,z,z,z,T,T,T,T,
 T,T,z,z,z,T,T,T,T,T,
 T,T,T,z,T,T,T,T,T,T,
 T,T,T,T,T,T,T,T,T,T,
};

const uchar check_icon_2[] = {
 T,T,T,T,T,T,T,T,T,T,T,T,
 T,T,T,T,T,T,T,T,T,T,T,z,
 T,T,T,T,T,T,T,T,T,z,z,T,
 T,T,T,T,T,T,T,T,z,z,T,T,
 z,z,T,T,T,T,z,z,z,T,T,T,
 T,z,z,T,T,z,z,z,T,T,T,T,
 T,z,z,z,z,z,z,T,T,T,T,T,
 T,T,z,z,z,z,T,T,T,T,T,T,
 T,T,z,z,z,T,T,T,T,T,T,T,
 T,T,T,z,T,T,T,T,T,T,T,T,
};

#undef L
#undef G
#undef D
#undef W
#undef B
#undef T
#undef X
#undef x
#undef z
#undef V

// All of our static bitmaps
BBitmap* 	MProjectLinePainter::sRightBitmap;
BBitmap* 	MProjectLinePainter::sDownBitmap;
BBitmap* 	MProjectLinePainter::sCheckMarkBitmap;
BBitmap*	MProjectLinePainter::sTouchedBitmap;

// ---------------------------------------------------------------------------
// MProjectLinePainter member functions
// ---------------------------------------------------------------------------

MProjectLinePainter::MProjectLinePainter()
{
	InitBitmaps();
	fArrowRect.Set(0.0, 0.0, kBitMapWidth, kBitMapHeight);
	fArrowRect.OffsetBy(kArrowPoint);
	fNameRect.Set(0.0, 0.0, kBitMapWidth, kBitMapHeight);
	fCodeRect.Set(0.0, 0.0, kCodeWidth, kBitMapHeight);
	fDataRect.Set(0.0, 0.0, kDataWidth, kBitMapHeight);
}

// ---------------------------------------------------------------------------

void
MProjectLinePainter::InitBitmaps()
{
	if (! sRightBitmap) {
		sRightBitmap = LoadBitmap(rightArrowData, kBitMapWidth, kBitMapHeight);
		sDownBitmap = LoadBitmap(downArrowData, kBitMapWidth, kBitMapHeight);
		sCheckMarkBitmap = LoadBitmap(checkmarkData, kBitMapWidth, kBitMapHeight);
		sTouchedBitmap = LoadBitmap(touchedData, kBitMapWidth, kBitMapHeight);
	}
}

// ---------------------------------------------------------------------------

void
MProjectLinePainter::DrawLine(BRect inFrame, BRect inIntersection, MProjectView& inView, MProjectLine& inLine)
{
	String text;
	fCodeRect.OffsetTo(fNameRight + 1.0, inFrame.top);

	if (inIntersection.Intersects(fCodeRect)) {
		if (inLine.fCodeSize < 0) {
			text = "n/a";
		}
		else if (inLine.fCodeSize < 99999) {
			text = inLine.fCodeSize;
		}
		else {
			text = inLine.fCodeSize / 1024;
			text += "K";
		}
		
		inView.MovePenTo(fCodeRight - inView.StringWidth(text), inFrame.bottom - kSourceBottomMargin);
		inView.DrawString(text);
	}

	fDataRect.OffsetTo(fCodeRight + 1.0, inFrame.top);

	if (inIntersection.Intersects(fDataRect)) {
		if (inLine.fDataSize < 0) {
			text = "n/a";
		}
		else if (inLine.fDataSize < 99999) {
			text = inLine.fDataSize;
		}
		else {
			text = inLine.fDataSize / 1024;
			text += "K";
		}
		
		inView.MovePenTo(fDataRight - inView.StringWidth(text), inFrame.bottom - kSourceBottomMargin);
		inView.DrawString(text);
	}
}

// ---------------------------------------------------------------------------

void
MProjectLinePainter::DrawName(
	BRect 			inFrame, 
	BRect			inIntersection,
	MProjectView& 	inView,
	const char*	inName)
{
	fNameRect.OffsetTo(0.0, inFrame.top);

	if (inIntersection.Intersects(fNameRect)) {
		inView.MovePenTo(kSourceMargin, inFrame.bottom - kSourceBottomMargin);
		inView.ConstrainClippingRegion(&fNameClip);
		inView.DrawString(inName);
		inView.ConstrainClippingRegion(NULL);
	}
}

// ---------------------------------------------------------------------------

void
MProjectLinePainter::DrawBitmap(EBitmapKind which, BRect inFrame, MProjectView& inView)
{
	BBitmap* bitmap = nil;
	float left = 0.0;
	float top = inFrame.top + fArrowRect.top;
		switch (which) {
		case kDownArrow:
			left = fArrowRect.left;
			bitmap = sDownBitmap;
			break;
			
		case kRightArrow:
			left = fArrowRect.left;
			bitmap = sRightBitmap;
			break;
			
		case kCheckMark:
			left = kCheckMarkLeft;
			bitmap = sCheckMarkBitmap;
			break;
			
		case kTouchedMark:
			left = kCheckMarkLeft;
			bitmap = sTouchedBitmap;
			break;
	}
	
	inView.DrawBitmapAsync(bitmap, BPoint(left, top));
}

// ---------------------------------------------------------------------------

bool
MProjectLinePainter::PointInArrow(BPoint& inPoint)
{
	return fArrowRect.Contains(inPoint);
}

// ---------------------------------------------------------------------------

void
MProjectLinePainter::AdjustSizes(
	float	inWidth)
{
	InitBitmaps();

	fDataRight = inWidth - kArrowWidth;
	fCodeRight = fDataRight - kDataWidth;
	fNameRight = fCodeRight - kCodeWidth;
	
	fNameRect.right = fNameRight;
	fArrowRect.OffsetTo(fDataRight + kArrowLeftMargin, kArrowTopMargin);

	BRect r(0.0, 0.0, fNameRight, 30000.0);
	fNameClip.Set(r);
}

// ---------------------------------------------------------------------------
//		MProjectLine
// ---------------------------------------------------------------------------
//	Constructor

MProjectLine::MProjectLine(
	MSectionLine& inSection,
	MProjectView& inProjectView)
	: fSection(&inSection),
	fProjectView(inProjectView)
{
	fCodeSize = 0;
	fDataSize = 0;
}

// ---------------------------------------------------------------------------

MProjectLine::~MProjectLine()
{
}

// ---------------------------------------------------------------------------

void
MProjectLine::Draw(
	BRect 			inFrame, 
	BRect			inIntersection,
	MProjectView& 	inView)
{
	fProjectView.GetPainter().DrawLine(inFrame, inIntersection, inView, *this);
}

// ---------------------------------------------------------------------------

void
MProjectLine::DrawName(
	BRect 			inFrame, 
	BRect			inIntersection,
	MProjectView& 	inView,
	const char *	inName)
{
	// Draw the name in the name column with clipping.  Set any font characteristics
	// before calling this function.

	fProjectView.GetPainter().DrawName(inFrame, inIntersection, inView, inName);
}

// ---------------------------------------------------------------------------
//		DoClick
// ---------------------------------------------------------------------------

bool
MProjectLine::DoClick(
	BRect		/*inFrame*/,
	BPoint		/*inPoint*/,
	bool 		/*inIsSelected*/,
	uint32		/*inModifiers*/,
	uint32		/*inButtons*/)
{
	return false;
}

// ---------------------------------------------------------------------------
//		SelectImmediately
// ---------------------------------------------------------------------------

bool
MProjectLine::SelectImmediately(
	BRect	/*inFrame*/,
	BPoint	/*inPoint*/,
	bool	/*inIsSelected*/,
	uint32	/*inModifiers*/,
	uint32	/*inButtons*/)
{
	return false;
}

// ---------------------------------------------------------------------------
//		Invoke
// ---------------------------------------------------------------------------

void
MProjectLine::Invoke()
{
}

// ---------------------------------------------------------------------------
//		BuildCompileObj
// ---------------------------------------------------------------------------
//	Tell the compiler to compile this source file.

MCompile*
MProjectLine::BuildCompileObj(
	MakeActionT		/*inKind*/)
{
	return nil;
}

// ---------------------------------------------------------------------------
//		CanBeExecuted
// ---------------------------------------------------------------------------

bool
MProjectLine::CanBeExecuted() const
{
	return false;
}

// ---------------------------------------------------------------------------
//		BuildPopupMenu
// ---------------------------------------------------------------------------
//	Build a popup menu for this project line.

void
MProjectLine::BuildPopupMenu(
	MPopupMenu & 	/*inMenu*/) const
{
	ASSERT(!"Need to override this function");
}
