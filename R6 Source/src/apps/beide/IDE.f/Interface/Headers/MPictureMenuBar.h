//========================================================================
//	MPictureMenuBar.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MPICTUREMENUBAR_H
#define _MPICTUREMENUBAR_H

#include "IDEConstants.h"

#include <MenuBar.h>

class BPicture;


class MPictureMenuBar : public BMenuBar
{
	friend class MPictureMenuField;
	
public:

					MPictureMenuBar(
								BRect 		frame,
								const char *title,
								BPicture*	inOnPicture,
								BPicture*	inOffPicture = nil,
								uint32 		resizeMask =
									B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
								menu_layout layout = B_ITEMS_IN_ROW,
								bool 		resizeToFit = false);
virtual				~MPictureMenuBar();

virtual	void		Draw(
						BRect updateRect);
virtual	void		MouseDown(
						BPoint inWhere);

	void			GetPictures(
						BPicture*& inOnPicture,
						BPicture*& inOffPicture);

	void			SetPicture(
						BPicture* inOnPicture,
						BPicture* inOffPicture = nil)
					{
						fOnPicture = inOnPicture;
						fOffPicture = inOffPicture;
					}

private:

		BPicture*	fOnPicture;
		BPicture*	fOffPicture;
};

#endif
