// IMenuBar.h
#ifndef _IMENUBAR_H_
#define _IMENUBAR_H_

#include <MenuBar.h>
#include <Rect.h>

#include "IMenuBar.h"

class IMenuBar : public BMenuBar
{
public:
	IMenuBar(BRect frame,
			const char *title,
			ulong resizeMask =
				B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
			menu_layout layout = B_ITEMS_IN_ROW,
			bool resizeToFit = TRUE);
	
virtual void 		FrameResized(float w, float h);
		void		SetMaxWidth(float w);
private:
	float		fMaxWidth;
};

#endif
