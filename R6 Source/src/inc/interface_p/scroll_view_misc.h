//******************************************************************************
//
//	File:		ScrollViewMisc.cpp
//
//	Written By:	Robert Polic
//
//	Copyright 2001, Be Incorporated
//
//******************************************************************************

#ifndef _SCROLL_VIEW_MISC_H
#define _SCROLL_VIEW_MISC_H

#include <ScrollBar.h>


#define kVALUE_MESSAGE	'valu'

class TScrollBar : public BScrollBar
{
	public:
							TScrollBar			(BRect frame,
												 const char* name,
												 BView* target,
												 float min,
												 float max,
												 orientation direction);
		virtual				~TScrollBar			();
		virtual void		ValueChanged		(float newValue);

		void				SetRulerView		(BView*);

	private:
		BView*				fRulerView;
};

#endif
