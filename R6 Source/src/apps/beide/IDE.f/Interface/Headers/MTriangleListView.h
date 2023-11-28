//==================================================================
//	MTriangleListView.h
//	Copyright 1995 - 97 Metrowerks Corporation, All Rights Reserved.
//==================================================================

#ifndef _MTRIANGLELISTVIEW_H
#define _MTRIANGLELISTVIEW_H

#include "MDLOGListView.h"
#include "MTriangleList.h"


class MTriangleListView : public MDLOGListView
{
public:
								MTriangleListView(
									BRect			inFrame,
									const char*		inName);
								~MTriangleListView();

virtual	void					DrawRow(
									int32 index,
									void * data,
									BRect rowArea,
									BRect intersectionRect);

virtual	bool					ClickHook(
									BPoint	inWhere,
									int32	inRow,
									uint32	modifiers,
									uint32	buttons);
	void						InsertCollapsableRow(
									int32	inIndex,
									void*	inData,
									float	inHeight = 0.0);
virtual	void					InsertRow(
									int32	inIndex,
									void*	inData,
									float	inHeight = 0.0);
		void					InsertRowWideOpen(
									int32	inIndex,
									void*	inData,
									float	inHeight = 0.0);
virtual	void					RemoveRows(
									int32 inVisibleIndexFrom,
									int32 inNumRows = 1);
		void					RemoveRowsWideOpen(
									int32 inVisibleIndexFrom,
									int32 inNumRows = 1);

	void						ContractRow(
									int32	inVisibleIndex);
	void						ExpandRow(
									int32	inVisibleIndex);

	int32						GetWideOpenIndex(
									int32 	inVisibleIndex) const;
	int32						GetVisibleIndex(
									int32 	inWideOpenIndex) const;
	int32						WideOpenIndexOf(
									void* inData) const;
	int32						VisibleIndexOf(
									void* inData) const;
	void*						RowData(
									int32 	inWideOpenIndex) const;
	int32						WideOpenRowCount() const;

private:

	static BRect				sTriangleRect;
	MTriangleList				fTriangleList;

	void						Contract(
									BRect inFrame);
	void						Expand(
									BRect inFrame);

	void						DrawContracted(
									BRect inFrame);
	void						DrawIntermediate(
									BRect inFrame);
	void						DrawExpanded(
									BRect inFrame);
};

#endif
