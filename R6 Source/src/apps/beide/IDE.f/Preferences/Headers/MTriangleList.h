//==================================================================
//	MTriangleList.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//==================================================================

#ifndef _MTRIANGLELIST_H
#define _MTRIANGLELIST_H

#include "MList.h"

struct ItemRec;

const int32 kTInvalidIndex = -1;

class MTriangleList
{
public:

								MTriangleList();
								~MTriangleList();

	int32						GetWideOpenIndex(
									int32 	inVisibleIndex) const;
	int32						GetVisibleIndex(
									int32 	inWideOpenIndex) const;
	int32						WideOpenIndexOf(
									void* 	inData) const;
	int32						VisibleIndexOf(
									void* 	inData) const;

	void						InsertParentRow(
									int32 	inWideOpenIndex,
									void*	inData);
	void						InsertChildRow(
									int32 	inWideOpenIndex,
									void*	inData);
	void						RemoveRows(
									int32	inVisibleIndexFrom,
									int32 	inNumRows);
	void						RemoveRowsWideOpen(
									int32	inWideOpenIndexFrom,
									int32 	inNumRows);
	int32						ContractRow(
									int32 inVisibleIndex);
	int32						ExpandRow(
									int32 	inVisibleIndex);

	void*						RowData(
									int32 	inWideOpenIndex) const;

	int32						Rows() const;
	bool						Collapsable(
									int32	inVisibleRow) const;
	bool						Expanded(
									int32	inVisibleRow) const;
private:

	MList<ItemRec*>			fItemList;
	int32					fExposedRows;

	void						InsertRow(
									int32 	inWideOpenIndex,
									void*	inData,
									bool	inParent);
	void						AdjustIndexes(
									int32	inWideOpenIndexFrom,
									int32 	inDelta,
									bool	inVisible = true);

};

#endif
