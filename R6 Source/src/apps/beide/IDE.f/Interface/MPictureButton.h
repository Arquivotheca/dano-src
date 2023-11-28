//========================================================================
//	MPictureButton.h
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#pragma once

#include <PictureButton.h>


class MPictureButton : public BPictureButton
{
public:
								MPictureButton(
									BRect frame,
									const char* name, 
									BPicture *off, 			   
									BPicture *on,
									BMessage *message, 
									uint32 behavior = B_ONE_STATE_BUTTON,
									uint32 resizeMask =
										B_FOLLOW_LEFT | B_FOLLOW_TOP, 
									uint32 flags = B_WILL_DRAW | B_NAVIGABLE); 

virtual							~MPictureButton();

virtual		void				Draw(
									BRect inArea);
virtual		void				DrawNavigationHilite(
									BRect inArea);
//virtual	void					KeyDown(
//									ulong key);

};
