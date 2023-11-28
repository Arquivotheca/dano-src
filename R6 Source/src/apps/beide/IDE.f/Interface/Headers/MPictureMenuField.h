//========================================================================
//	MPictureMenuField.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MPICTUREMENUFIELD_H
#define _MPICTUREMENUFIELD_H

#include "IDEConstants.h"

#include <View.h>

class BPicture;
class BPopUpMenu;


class MPictureMenuField : public BView
{
public:

								MPictureMenuField(
									BRect 			frame,
									const char *	title,
									const char *	label,
									BPicture*		inPicture,
									BPopUpMenu *	menu,
									uint32 			resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP); 
virtual							~MPictureMenuField();

virtual	void					Draw(
									BRect update);
virtual	void					MouseDown(
									BPoint where);
virtual	void					KeyDown(
									const char *bytes, 
									int32 		numBytes);
virtual	void					MakeFocus(
									bool inBecomeFocus);

		BPopUpMenu*				Menu() const;

virtual	void					SetLabel(
									const char *label);
		const char*				Label() const;
		
virtual	void					SetDivider(
									float inDividingLine);
		float					Divider() const;

void							SetPictureFrame(
									BRect	inRect);
	
private:

		char*					fLabel;
		BPicture*				fPicture;
		BPopUpMenu*				fMenu;
		alignment				fAlign;
		float					fDivider;
		BRect					fRect;
		
		void					ShowPopup();
		void					DrawLabel(
									BRect 	inFrame);
		void					DrawBorder(
									bool inShowBorder);
};

inline const char * MPictureMenuField::Label() const
{
	if (fLabel != nil)
		return fLabel;
	else
		return B_EMPTY_STRING;
}
inline float MPictureMenuField::Divider() const
{
	return fDivider;
}
inline BPopUpMenu*	 MPictureMenuField::Menu() const
{
	return fMenu;
}

#endif
