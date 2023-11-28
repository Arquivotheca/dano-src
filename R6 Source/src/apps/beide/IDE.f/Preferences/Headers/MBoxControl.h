//========================================================================
//	MBoxControl.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MBOXCONTROL_H
#define _MBOXCONTROL_H

#include <View.h>


class MBoxControl : public BView
{
public:
								MBoxControl(
									BRect 		inArea,
									const char*	inName);
								~MBoxControl();

		void					Draw(
									BRect inArea);

virtual	void					MouseDown(
									BPoint inWhere);
virtual	void					KeyDown(
									const char *bytes, 
									int32 		numBytes);
virtual	void					MakeFocus(
									bool inBecomeFocus);

private:
		void					DrawBorder(
									bool inShowBorder);
		void					SelectNewChildView(
									uint32 inKey);

};

#endif
