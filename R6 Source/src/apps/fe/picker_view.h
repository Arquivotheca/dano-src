/*
	
	HelloView.h
	
	Copyright 1995 Be Incorporated, All Rights Reserved.
	
*/

#ifndef PICK_VIEW_H
#define PICK_VIEW_H

#ifndef _VIEW_H
#include <View.h>
#endif

#include <Bitmap.h>


class PickView : public BView {
public:
		long	cur;
				PickView(BRect frame, char *name); 
virtual	void	Draw(BRect updateRect);
virtual	void	MouseDown(BPoint where);

};

#endif



