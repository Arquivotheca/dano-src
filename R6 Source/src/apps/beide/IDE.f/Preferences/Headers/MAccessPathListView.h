//==================================================================
//	MAccessPathListView.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//==================================================================

#ifndef _MACCESSPATHLISTVIEW_H
#define _MACCESSPATHLISTVIEW_H


#include "MDLOGListView.h"


class MProjectWindow;
class BPath;


class MAccessPathListView : public MDLOGListView
{
public:
							MAccessPathListView(BRect inFrame, const char* inName);
							~MAccessPathListView();

	virtual	void			MouseMoved(BPoint where,
									   uint32 code,
									   const BMessage* a_message);

	virtual	void			KeyDown(const char* bytes, 
									int32 numBytes);

	virtual	void			DrawRow(int32 index,
									void* data,
									BRect rowArea,
									BRect intersectionRect);

	virtual	void			DeleteItem(void * item);

	virtual	bool			ClickHook(BPoint inWhere,
									  int32	inRow,
									  uint32 /* modifiers */,
									  uint32 /* buttons */);

	virtual	void			InitiateDrag(BPoint where,
										 int32 row);

	int32					GetRow(float inYPosition)
							{
								return GetPosRow(inYPosition);
							}

	virtual status_t		ConvertItemToPath(int32 index, BPath& outPath);

	void					SetProject(MProjectWindow* inProject)
							{
								fProject = inProject;
							}
	
private:

	static BBitmap * 			sFoldersBitmap;
	static BRect				sFoldersRect;
	
	MProjectWindow*				fProject;
};

#endif
