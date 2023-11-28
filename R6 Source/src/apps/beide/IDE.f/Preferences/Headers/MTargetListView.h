//==================================================================
//	MTargetListView.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//==================================================================

#ifndef _MTARGETLISTVIEW_H
#define _MTARGETLISTVIEW_H

#include "MDLOGListView.h"
#include "CString.h"


class MTargetListView : public MDLOGListView
{
public:
								MTargetListView(
									BRect			inFrame,
									const char*		inName,
									uint32 			resizeMask = B_FOLLOW_ALL_SIDES,
									uint32 			flags = B_WILL_DRAW | B_NAVIGABLE);
								~MTargetListView();

virtual	void					DrawRow(
									int32 index,
									void * data,
									BRect rowArea,
									BRect intersectionRect);

private:
			String		fText;
};

#endif
