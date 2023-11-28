//========================================================================
//	MBlueArrowList.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include "MBlueArrowList.h"
#include "Utils.h"

const float kBlueArrowHeight = 9.0;
const float kBlueArrowWidth = 9.0;

#define _ 0xff
#define B 0
#define u 0x6c

const char blueArrowData[] = {
	_,_,_,B,B,_,_,_,_,
	_,_,_,B,u,B,_,_,_,
	_,B,B,B,u,u,B,_,_,
	_,B,u,u,u,u,u,B,_,
	_,B,u,u,u,u,u,B,B,
	_,B,u,u,u,u,u,B,_,
	_,B,B,B,u,u,B,_,_,
	_,_,_,B,u,B,_,_,_,
	_,_,_,B,B,_,_,_,_,
};

#undef _
#undef B
#undef u


// ---------------------------------------------------------------------------
//		¥ MBlueArrowList
// ---------------------------------------------------------------------------
//	Constructor

MBlueArrowList::MBlueArrowList(
	BRect			inFrame,
	const char*		inName,
	uint32 			resizeMask,
	uint32 			flags)
	: MDLOGListView(
		inFrame,
		inName,
		resizeMask,
		flags)
{
	SetMultiSelect(false);
	fBlueArrow = LoadBitmap(blueArrowData, kBlueArrowWidth, kBlueArrowHeight);

}

// ---------------------------------------------------------------------------
//		¥ ~MBlueArrowList
// ---------------------------------------------------------------------------
//	Destructor

MBlueArrowList::~MBlueArrowList()
{
	delete fBlueArrow;
}

// ---------------------------------------------------------------------------
//		¥ DrawRow
// ---------------------------------------------------------------------------

void
MBlueArrowList::DrawRow(
	int32 	inRow,
	void * 	/*inData*/,
	BRect	inArea,
	BRect	/*inIntersection*/)
{
	if (IsRowSelected(inRow))
	{
		DrawBitmap(fBlueArrow, inArea.LeftTop());
	}
}

// ---------------------------------------------------------------------------
//		¥ DoSelectRow
// ---------------------------------------------------------------------------

void
MBlueArrowList::DoSelectRow(
	int32 	inRow,
	bool	inSelectIt)
{
	BRect		rowRect;
	
	GetRowRect(inRow, &rowRect);

	if (inSelectIt)
	{
		DrawBitmap(fBlueArrow, rowRect.LeftTop());		
	}
	else
	{
		FillRect(rowRect, B_SOLID_LOW);
	}
}

// ---------------------------------------------------------------------------
//		¥ HiliteRow
// ---------------------------------------------------------------------------

void
MBlueArrowList::HiliteRow(
	int32 		/*inRow*/,
	BRect 		/*inArea*/)
{
}
