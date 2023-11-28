//
// BeCalculator
// Copyright 1997, Be Incorporated
// by Eric Shepherd
//

#include <Button.h>

#include "window.h"
#include "view.h"
#include "button.h"
#include "application.h"
#include "main.h"

#define FLASH_DELAY		50000

//
// CalcButton::CalcButton
//
// Construct a button on the calculator.
//
CalcButton::CalcButton(BRect frame, char *name, BMessage *message)
			: BButton(frame, name, name, message) {
}


//
// CalcButton::Flash
//
// Flash the button.
//
void CalcButton::Flash(void) {
	SetValue(1);
	Draw(Frame());				// Draw myself
	snooze(FLASH_DELAY);
	SetValue(0);
}
