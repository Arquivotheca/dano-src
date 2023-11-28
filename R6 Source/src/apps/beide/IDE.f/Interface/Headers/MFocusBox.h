//==================================================================
//	MFocusBox.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//==================================================================

#ifndef _MFOCUSBOX_H
#define _MFOCUSBOX_H

#include <View.h>
#include "IDEConstants.h"

class MFocusBox : public BView
{
public:
								MFocusBox(
									BRect			inFrame,
									const char*		inName = NULL,
									uint32 			resizeMask = B_FOLLOW_NONE,
									uint32 			flags = B_WILL_DRAW);
								~MFocusBox();

virtual	void					Draw(
									BRect updateRect);
virtual	void					AttachedToWindow();
	rgb_color					ParentViewColor();
};

#endif
