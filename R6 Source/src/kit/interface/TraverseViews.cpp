//******************************************************************************
//
//	File:			TraverseViews.cpp
//
//	Description:	a class for traversing a view hierarchy
//
//	Written by:		Peter Potrebic
//
//	Copyright 1996, Be Incorporated. All Rights Reserved.
//
//******************************************************************************

#ifndef _TRAVERSE_VIEWS_H
#include <TraverseViews.h>
#endif
#ifndef _WINDOW_H
#include <Window.h>
#endif
#ifndef _VIEW_H
#include <View.h>
#endif

/*------------------------------------------------------------*/

BTraverseViews::BTraverseViews(BView *start, bool wrap, bool exclusive)
{
	Reset(start->Window(), start, wrap, exclusive);
}

/*------------------------------------------------------------*/

BTraverseViews::BTraverseViews(BWindow *window, BView *start, bool wrap,
	bool exclusive)
{
	Reset(window, start, wrap, exclusive);
}

/*------------------------------------------------------------*/

BTraverseViews::BTraverseViews(BWindow *start, bool wrap)
{
	Reset(start, NULL, wrap, false);
}

/*------------------------------------------------------------*/

void BTraverseViews::Reset(BWindow *window, BView *start, bool wrap,
	bool exclusive)
{
	ASSERT(window);
	if (start) {
		fInitialView = fCurrent = start;
	} else {
		fInitialView = fCurrent = window->ChildAt(0);
	}

	fWrap = wrap;
	fInited = exclusive;
	fFirstView = window->ChildAt(0);
	fLastView = LastViewInWindow(window);
	if (fInitialView) {
		ASSERT(fFirstView);
		ASSERT(fLastView);
	}
}

/*------------------------------------------------------------*/

BView *BTraverseViews::Current()
{
	return fInited ? fCurrent : NULL;
}

#if DEBUG
/*------------------------------------------------------------*/
static void _print_all_views_(BView *v, bool forward)
{
	BTraverseViews	t(v, true);

	PRINT(("starting view = %s\n", v->Name() ? v->Name() : "null"));
	if (forward) {
		while ((v = t.Next()) != 0)
			PRINT(("view = %s\n", v->Name() ? v->Name() : "null"));
	} else {
		while ((v = t.Previous()) != 0)
			PRINT(("view = %s\n", v->Name() ? v->Name() : "null"));
	}
}
/*------------------------------------------------------------*/
static void _print_all_views_(BWindow *w, bool forward)
{
	BTraverseViews	t(w);
	BView *v;

	if (forward) {
		while ((v = t.Next()) != 0)
			PRINT(("view = %s\n", v->Name() ? v->Name() : "null"));
	} else {
		while ((v = t.Previous()) != 0)
			PRINT(("view = %s\n", v->Name() ? v->Name() : "null"));
	}
}
#endif

/*------------------------------------------------------------*/

BView *BTraverseViews::LastViewInWindow(BWindow *w)
{
	BView	*last_child = NULL;
	if (long c = w->CountChildren())
		last_child = w->ChildAt(c - 1);

	return LastView(last_child);
}

/*------------------------------------------------------------*/

BView *BTraverseViews::LastView(BView *view)
{
	// Find the last view (descendent) in the sub-tree starting with
	// the given view. If there are no children then the starting view
	// is the result.

	BView *test = view;
	while (test) {
		view = test;
		if (long c = test->CountChildren())
			test = test->ChildAt(c - 1);
		else
			test = NULL;
	}

	return view;
}

/*------------------------------------------------------------*/

BView *BTraverseViews::Next()
{
	if (!fInited) {
		fInited = true;
		return fCurrent;
	}

	if (!fCurrent)
		return NULL;

	// depth first
	BView *next = fCurrent->ChildAt(0);

	// if no kids then go to next sibling
	if (!next)
		next = fCurrent->NextSibling();

	// if no sibling then walk up parent chain looking for a parent
	// that has a next_sibling

	BView *parent = fCurrent;
	while (!next && parent) {
		parent = parent->Parent();
		if (parent)
			next = parent->NextSibling();
	}

	// if still haven't found a 'next' and wrapping is on, then wrap
	if (!next && fWrap)
		next = fFirstView;

	// make sure that we haven't wrapped all the way around, back to
	// the initial view
	if (next == fInitialView)
		next = NULL;

	// If we found a next view, then save it as the new current view
	if (next)
		fCurrent = next;

	return next;
}

/*------------------------------------------------------------*/

BView *BTraverseViews::Previous()
{
	if (!fInited) {
		fInited = true;
		fCurrent = fLastView;
		return fCurrent;
	}

	if (!fCurrent)
		return NULL;

	BView *prev = fCurrent->PreviousSibling();

	if (prev) {
		// find the last child (descendent) of 'prev'
		prev = LastView(prev);
	}

	// if no sibling then return the Parent
	if (!prev) {
		prev = fCurrent->Parent();
	}

	// if still haven't found a 'prev' and wrapping is on, then wrap
	if (!prev && fWrap)
		prev = fLastView;

	// make sure that we haven't wrapped all the way around, back to
	// the initial view
	if (prev == fInitialView)
		prev = NULL;

	// If we found a prev view, then save it as the new current view
	if (prev)
		fCurrent = prev;

	return prev;
}
