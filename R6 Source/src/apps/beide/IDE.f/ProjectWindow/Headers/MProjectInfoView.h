//========================================================================
//	MProjectInfoView.h
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MPROJECTINFOVIEW_H
#define _MPROJECTINFOVIEW_H

#include <StringView.h>

class MProjectWindow;

// ---------------------------------------------------------------------------
//	MProjectInfoView - draws like a menu and handles a popup
// ---------------------------------------------------------------------------

class MProjectInfoView : public BStringView
{
public:
								MProjectInfoView(
									MProjectWindow&	inWindow,
									BRect 			inArea,
									const char *	text,
									uint32			resizeMask = B_FOLLOW_ALL_SIDES);
		void					Draw(
									BRect inArea);
virtual	void					MouseDown(
									BPoint where);

private:
		MProjectWindow&			fWindow;
};

// ---------------------------------------------------------------------------
//	MStatusInfoView - just a gray outlined string view
// ---------------------------------------------------------------------------

class MStatusInfoView : public BStringView
{
public:
	MStatusInfoView(BRect bounds,
					const char *name, 
					const char *text,
					uint32 resizeFlags = B_FOLLOW_LEFT | B_FOLLOW_TOP,
					uint32 flags = B_WILL_DRAW);

	void	Draw(BRect inArea);
};

#endif
