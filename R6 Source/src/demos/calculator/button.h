//
// BeCalculator
// Copyright 1997, Be Incorporated
// by Eric Shepherd
//

#ifndef _CBUTTON_H_
#define _CBUTTON_H_

#include <interface/Button.h>

class CalcButton : public BButton {
	public:
							CalcButton(BRect frame, char *name,
										BMessage *message);
	
		void				Flash(void);
};

#endif
