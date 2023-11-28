//==================================================================
//	MDLOGListView.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//==================================================================

#ifndef _MDLOGLISTVIEW_H
#define _MDLOGLISTVIEW_H

#include "MListView.h"

class MFocusBox;

class MDLOGListView : public MListView
{
public:
								MDLOGListView(
									BRect			inFrame,
									const char*		inName,
									uint32 			resizeMask = B_FOLLOW_ALL_SIDES,
									uint32 			flags = B_WILL_DRAW | B_PULSE_NEEDED | B_NAVIGABLE);
virtual							~MDLOGListView();

virtual	void					AttachedToWindow();
virtual	void					MakeFocus(
									bool focusState = TRUE);

virtual	void					DrawRow(
									int32 index,
									void * data,
									BRect rowArea,
									BRect intersectionRect);
virtual	void					SelectRow(
									int32 row,
									bool keepOld = FALSE,
									bool toSelect = TRUE);
virtual	void					SelectRows(
									int32 rowFrom,
									int32 rowTo,
									bool keepOld = FALSE,
									bool toSelect = TRUE);
		void					SetFocusBox(
									MFocusBox*	inBox);
virtual	void					SetTarget(
									BHandler *target);
		BHandler*				Target() const
								{
									return fTarget;
								}
protected:

	void						PostToTarget(
									uint32	inMessageKind);

private:

	MFocusBox*			fFocusBox;
	rgb_color			fHighColor;
	BHandler*			fTarget;
};

#endif
