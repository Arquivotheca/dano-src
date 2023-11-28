//==================================================================
//	MBlueArrowList.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//==================================================================

#ifndef _MBLUEARROWLIST_H
#define _MBLUEARROWLIST_H

#include "MDLOGListView.h"

enum BlueArrowT
{
	bDontDrawArrow,
	bDrawArrow
};

class MBlueArrowList : public MDLOGListView
{
public:
								MBlueArrowList(
									BRect			inFrame,
									const char*		inName,
									uint32 			resizeMask = B_FOLLOW_ALL_SIDES,
									uint32 			flags = B_PULSE_NEEDED | B_WILL_DRAW);
virtual							~MBlueArrowList();

virtual	void					DrawRow(
									int32	inRow,
									void *	inData,
									BRect	inArea,
									BRect	inIntersection);
virtual	void					HiliteRow(
									int32	inRow,
									BRect	inArea);

virtual	void					DoSelectRow(
									int32 inRow,
									bool inSelectIt);

private:

	BBitmap*			fBlueArrow;
};

#endif
