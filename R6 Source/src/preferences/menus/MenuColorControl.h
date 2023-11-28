/* ++++++++++

   FILE:  MenuColor.h
   REVS:  $Revision: 1.3 $
   NAME:  peter
   DATE:  Mon May 13 10:50:26 PDT 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef MENU_COLOR_CONTROL_H
#define MENU_COLOR_CONTROL_H

#ifndef _COLOR_CONTROL_H
#include <ColorControl.h>
#endif

/*------------------------------------------------------------*/

class TSampleView : public BView {
public:

					TSampleView(BRect rect);

virtual void		Draw(BRect update);
		void		DrawColorRamp();

		rgb_color	fColor;
};

/*------------------------------------------------------------*/

class TMenuColorControl : public BColorControl {
public:
					TMenuColorControl(	BPoint start,
										color_control_layout layout,
										long cell_size,
										const char *name,
										BMessage *message = NULL,
										bool use_offscreen = FALSE,
										BWindow *w = NULL);

virtual	void	SetValue(long color);

private:
		BWindow		*fParent;
		TSampleView	*fSampleView;

};

#endif
