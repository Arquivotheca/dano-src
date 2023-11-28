//==================================================================
//	MPrefsListView.h
//	Copyright 1995 - 97 Metrowerks Corporation, All Rights Reserved.
//==================================================================

#ifndef _MPREFSLISTVIEW_H
#define _MPREFSLISTVIEW_H

#include "MTriangleListView.h"
#include "CString.h"

class MPreferencesView;

class PrefsRowData
{
public:
						PrefsRowData(MPreferencesView* inView,
									 void* viewCookie,
									 bool inSection,
									 const char* inName)
						: view(inView), cookie(viewCookie), isSection(inSection), name(inName)
						{}
	MPreferencesView*	view;
	void*				cookie;
	bool				isSection;
	String				name;
};

class MPrefsListView : public MTriangleListView
{
public:
								MPrefsListView(BRect inFrame, const char* inName);
								~MPrefsListView();

virtual	void					DrawRow(int32 index,
										void* data,
										BRect rowArea,
										BRect intersectionRect);

virtual	void					DeleteItem(void* item);
};

#endif
