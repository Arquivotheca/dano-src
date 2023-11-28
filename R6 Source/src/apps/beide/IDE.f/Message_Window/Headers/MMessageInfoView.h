//========================================================================
//	MMessageInfoView.h
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MMESSAGEINFOVIEW_H
#define _MMESSAGEINFOVIEW_H

#include <View.h>
#include <StringView.h>

class MMessageInfoView : public BView
{
public:
	// Info view with a subview showing some title or caption
								MMessageInfoView(const char* infoString, 
												 const BRect& inArea,
												 const BPoint& titleLocation,
												 uint32 resizeMask = B_FOLLOW_ALL_SIDES);


	// Info view with no subview
								MMessageInfoView(BRect inArea, 
												 uint32 resizeMask = B_FOLLOW_ALL_SIDES);

	virtual void				Draw(BRect inArea);
};

class MMessageInfoTitleView : public BStringView
{
public:
						MMessageInfoTitleView(BRect bounds, const char *name, const char *text);
	
	virtual	void		AttachedToWindow();
	virtual	void		Draw(BRect bounds);
};

#endif
