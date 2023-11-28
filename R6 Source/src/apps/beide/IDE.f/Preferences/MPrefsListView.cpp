//==================================================================
//	MPrefsListView.cpp
//	Copyright 1996 - 97 Metrowerks Corporation, All Rights Reserved.
//==================================================================
//	BDS

#include "MPrefsListView.h"

// ---------------------------------------------------------------------------
//		MPrefsListView
// ---------------------------------------------------------------------------
//	Constructor

MPrefsListView::MPrefsListView(
	BRect			inFrame,
	const char*		inName)
	: MTriangleListView(
		inFrame,
		inName)
{
}

// ---------------------------------------------------------------------------
//		~MPrefsListView
// ---------------------------------------------------------------------------
//	Destructor

MPrefsListView::~MPrefsListView()
{
	// Need to delete it here because our DeleteItem won't be called
	// from the MListView destructor
	if (CountRows() > 0)
		DeleteRows(0, CountRows());
}

// ---------------------------------------------------------------------------
//		DrawRow
// ---------------------------------------------------------------------------

void
MPrefsListView::DrawRow(
	int32 	inRow,
	void * 	inData,
	BRect	inArea,
	BRect	inIntersection)
{
	PrefsRowData*		rowData = (PrefsRowData*) inData;

	MTriangleListView::DrawRow(inRow, inData, inArea, inIntersection);
	DrawString(rowData->name);
}

// ---------------------------------------------------------------------------
//		DeleteItem
// ---------------------------------------------------------------------------

void
MPrefsListView::DeleteItem(
	void * inItem)
{
	delete (PrefsRowData*) inItem;
}
