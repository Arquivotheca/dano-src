//******************************************************************************
//
//	File:			TraverseViews.h
//
//	Description:	a class for traversing a view hierarchy
//
//	Written by:		Peter Potrebic
//
//	Copyright 1996, Be Incorporated. All Rights Reserved.
//
//******************************************************************************

#ifndef _TRAVERSE_VIEWS_H
#define _TRAVERSE_VIEWS_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#ifndef _DEBUG_H
#include <Debug.h>
#endif

class BView;
class BWindow;

/*------------------------------------------------------------*/

class BTraverseViews {
public:
				BTraverseViews(	BWindow *window,
								BView *start,
								bool wrap = TRUE,
								bool exclusive = TRUE);
				BTraverseViews(	BView *start,
								bool wrap = TRUE,
								bool exclusive = TRUE);
				BTraverseViews(BWindow *start, bool wrap = TRUE);

		BView	*Current();
		BView	*Next();
		BView	*Previous();

		void	Reset(	BWindow *window,
						BView *start,
						bool wrap = TRUE,
						bool exclusive = TRUE);

private:
		BView	*LastView(BView *start_pt);
		BView	*LastViewInWindow(BWindow *w);

		BView	*fCurrent;
		BView	*fFirstView;
		BView	*fLastView;
		BView	*fInitialView;
		bool	fWrap;
		bool	fInited;
};

#endif
