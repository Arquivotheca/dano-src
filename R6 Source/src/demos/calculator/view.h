//
// BeCalculator
// Copyright 1997, Be Incorporated
// by Eric Shepherd
//

#ifndef _CVIEW_H_
#define _CVIEW_H_

#include <interface/View.h>
#include "button.h"

class CalcView : public BView {
	public:
							CalcView(BRect frame);
	virtual void			Draw(BRect updateRect);
	virtual void			KeyDown(const char *bytes, int32 numBytes);
	virtual void			MessageReceived(BMessage *message);

	CalcButton				*AddButton(int32 x, int32 y, char *name,
									   int32 c);
	CalcButton				*AddWideButton(int32 x, int32 y, char *name,
										   int32 c);
	CalcButton				*AddTallButton(int32 x, int32 y, char *name,
										   int32 c);
};

#endif
