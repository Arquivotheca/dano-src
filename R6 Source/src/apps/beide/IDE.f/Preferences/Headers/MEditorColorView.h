//========================================================================
//	MEditorColorView.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MEDITORCOLORVIEW_H
#define _MEDITORCOLORVIEW_H

#include "MBoxControlChild.h"


class MEditorColorView : public MBoxControlChild
{
public:
								MEditorColorView(
									BRect 		inArea,
									const char*	inName,
									rgb_color&	inColor);
								~MEditorColorView();

virtual	void					Draw(
									BRect area);
virtual	void					MouseMoved(	
									BPoint		where,
									uint32		code,
									const BMessage *	a_message);

virtual void					MouseDown(BPoint point);

virtual	void					MessageReceived(
									BMessage * message);

		void					SetValue(
									rgb_color	inColor);
		rgb_color				Value() const
								{
									return fColor;
								}
		
private:
		bool					MouseMovedWhileDown(BPoint point);
		void					DrawColorBox(const BRect& inBox, BView* inView);
		BBitmap*				CreateColorBitmap(const BPoint&);

		BRect					fColorBox;
		rgb_color&				fColor;
};

#endif
