//==================================================================
//	MTextControl.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//==================================================================

#ifndef _MTEXTCONTROL_H
#define _MTEXTCONTROL_H

#include <TextControl.h>


class MTextControl : 	public BTextControl
{
public:
								MTextControl(
									BRect 			inFrame,
									const char *	inName,
									const char *	inLabel, 
									const char *	inInitialText, 
									BMessage *		inMessage,
									uint32 			resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
									uint32 			flags = B_WILL_DRAW | B_NAVIGABLE); 
								~MTextControl();

		int32					GetValue();
};

#endif
