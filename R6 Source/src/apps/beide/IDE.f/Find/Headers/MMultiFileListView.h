//==================================================================
//	MMultiFileListView.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//==================================================================

#ifndef _MMULTIFILELISTVIEW_H
#define _MMULTIFILELISTVIEW_H

#include "MDLOGListView.h"


class MMultiFileListView : public MDLOGListView
{
public:
								MMultiFileListView(
									BRect			inFrame,
									const char*		inName,
									uint32 			resizeMask = B_FOLLOW_ALL_SIDES,
									uint32 			flags = B_WILL_DRAW | B_NAVIGABLE);
								~MMultiFileListView();

virtual	void					MouseMoved(	
									BPoint				where,
									uint32				code,
									const BMessage *	a_message);
virtual	void					InvokeRow(
									int32	inRow);
virtual	void					SetEnabled(
									bool	inEnabled = true);
	void						SetBlueRow(
									int32	inRow);

		int32					BlueRow()
								{
									return fBlueRow;
								}

private:

	BBitmap*			fBlueArrow;
	int32				fBlueRow;

virtual	bool					ClickHook(
									BPoint	inWhere,
									int32	inRow,
									uint32	modifiers,
									uint32	buttons);
virtual	void					DrawRow(
									int32 index,
									void * data,
									BRect rowArea,
									BRect intersectionRect);
virtual	void					DeleteItem(
									void * item);

};

#endif
