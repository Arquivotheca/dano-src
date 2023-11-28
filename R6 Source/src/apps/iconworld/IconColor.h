//--------------------------------------------------------------------
//	
//	IconColor.h
//
//	Written by: Robert Polic
//	
//	Copyright 1994-97 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef ICON_COLOR_H
#define ICON_COLOR_H

#ifndef _FONT_H
#include <Font.h>
#endif
#ifndef _POINT_H
#include <Point.h>
#endif
#ifndef _RECT_H
#include <Rect.h>
#endif
#ifndef _VIEW_H
#include <View.h>
#endif

#define COLORWIDTH	32	// in cells
#define COLORHEIGHT	8	// in cells
#define COLORSIZE	8	// in pixels


//====================================================================

class TColorView : public BView {

private:
short			fHighColor;
short			fLowColor;

public:
				TColorView(BRect); 
virtual	void	AttachedToWindow(void);
virtual	void	Draw(BRect);
virtual void	MouseDown(BPoint);
short			GetTheHighColor(void);
short			GetTheLowColor(void);
void			SetTheHighColor(short);
void			SetTheLowColor(short);
};
#endif
