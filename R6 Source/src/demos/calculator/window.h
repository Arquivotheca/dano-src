//
// BeCalculator
// Copyright 1997, Be Incorporated
// by Eric Shepherd
//

#ifndef _CWINDOW_H_
#define _CWINDOW_H_

#include <interface/Window.h>

class CalcWindow : public BWindow {
	public:
							CalcWindow(BRect frame);
		virtual bool		QuitRequested();
		virtual void		MessageReceived(BMessage *message);
		//void				SetZoomLimits(float width, float height);
};

#endif
