//--------------------------------------------------------------------
//	
//	IconPat.h
//
//	Written by: Robert Polic
//	
//	Copyright 1994-97 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef ICON_PAT_H
#define ICON_PAT_H

#ifndef _POINT_H
#include <Point.h>
#endif
#ifndef _RECT_H
#include <Rect.h>
#endif
#ifndef _VIEW_H
#include <View.h>
#endif

#define PATWIDTH	12	// in cells
#define	PATHEIGHT	2	// in cells
#define	PATSIZE		20	// in pixels


//====================================================================

class TPatView : public BView {

private:
pattern			*fPats[PATWIDTH * PATHEIGHT];
short			fPat;

public:
				TPatView(BRect);
virtual	void	AttachedToWindow(void);
virtual	void	Draw(BRect);
virtual void	MouseDown(BPoint);
short			GetPat(void);
pattern*		GetPatMap(short);
void			SetPat(short);
};
#endif
