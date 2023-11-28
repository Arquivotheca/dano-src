//
// BeCalculator
// Copyright 1997, Be Incorporated
// by Eric Shepherd
//

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <Roster.h>
#include <View.h>

#include "window.h"
#include "displayview.h"
#include "button.h"
#include "application.h"
#include "main.h"


//
// DisplayView::DisplayView
//
// Construct the view.
//
DisplayView::DisplayView(BRect frame)
			: BView(frame, "Display View", B_FOLLOW_LEFT_RIGHT,
					B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE) {
	BFont font(be_fixed_font);
	
	displayHeight = int32(frame.bottom - frame.top);
	displayWidth = int32(frame.right - frame.left) - 4;
	
	SetViewUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
	SetLowUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
	SetHighUIColor(B_UI_DOCUMENT_TEXT_COLOR);
	
	// Set the size to the one closest to the pixel
	// height we want.
	
	float size;
	font_height height;
	
	size = 1.0;
	do {
		font.SetSize(size);
		font.GetHeight(&height);
		if (height.ascent+height.descent > displayHeight) {
			size--;
			break;
		}
		size++;
	} while (1);
	font.SetSize(size);
	font.GetHeight(&height);
	displayBaseline = int32(2 + height.ascent);
	SetFont(&font);
	SetValue(initValue);
	
	Clear();
}


//
// DisplayView::Draw
//
// Draw the contents of the view.
//
void DisplayView::Draw(BRect updateRect) {
	const BRect r = Bounds();
	const float x = ((r.right-r.left)-4) - StringWidth(displayString);
	DrawString(displayString, BPoint(x, displayBaseline));
	DrawString(displayOp, BPoint(r.left+2, displayBaseline));
}


//
// DisplayView::Clear
//
// Clear the calculator.
//
void DisplayView::Clear(void) {
	operation = OP_EQUALS;
	prevKeyWasOp = false;
	prevOpWasEquals = true;
	prevValue = 0.0;
	displayValue = 0;
	displayOp = "";
	SetValue(0.0);
}


//
// DisplayView::SetString
//
// Set the string contained by the display area.
// If it's numeric, the float is updated too.
//
void DisplayView::SetString(char *s) {
	strcpy(displayString, s);
	sscanf(s, "%lf", &displayValue);
	Invalidate();
}


//
// DisplayView::AddDigit
//
// Append a digit to the string and update
// the float and the display.
//
void DisplayView::AddDigit(char d) {
	int i;
	int maxWidth;
	BRect r;
	
#if ALLOW_ZOOM
	r = Frame();
	if (r.right-r.left >= 120) {
		maxWidth = 30;
	}
	else {
		maxWidth = 13;
	}
#else
		maxWidth = 13;
#endif

	if (prevKeyWasOp) {
		displayValue = 0;
		displayString[0] = '0';
		displayString[1] = '\0';
		displayOp = "";
		if (prevOpWasEquals) {
			operation = OP_EQUALS;
		}
	}

	prevKeyWasOp = false;
	i = strlen(displayString);
	if (i < maxWidth) {
		if ((i==1) && (displayString[0] == '0') && (d != '.')) {
			i--;		// Don't allow leading 0 unless followed by .
		}
		else if (d == '.') {
			if (strchr(displayString, '.')) {
				KeyBeep();
				return;
			}
		}
		displayString[i] = d;
		displayString[i+1] = '\0';
		sscanf(displayString, "%lf", &displayValue);
		Invalidate();
	}
}


//
// DisplayView::SetValue
//
// Set the value contained by the display area.
//
void DisplayView::SetValue(double f) {
	sprintf(displayString, "%g", f);
	Invalidate();
}


//
// DisplayView::GetValue
//
// Returns the current value of the calculator.
//
double DisplayView::GetValue(void) {
	return (prevKeyWasOp) ? (prevValue) : (displayValue);
}


//
// DisplayView::Operation
//
// Set the operation and shift the displayed
// value into the prevValue.
//
void DisplayView::Operation(int32 op) {
	switch (op) {
		case OP_POW:		displayOp = "pow";	break;
		case OP_PLUS:		displayOp = "+";	break;
		case OP_MINUS:		displayOp = "-";	break;
		case OP_MULTIPLY:	displayOp = "*";	break;
		case OP_DIVIDE:		displayOp = "/";	break;
		case OP_EQUALS:		displayOp = "=";	break;
		case OP_SIN:
		case OP_COS:
		case OP_TAN:
		case OP_PI:
			break;
		default: return; // should not happen
	}

	if (!prevKeyWasOp || (op == OP_EQUALS)) {
		switch (operation) {
			case OP_POW:		prevValue = pow(prevValue,displayValue); break;
			case OP_PLUS:		prevValue += displayValue;	break;
			case OP_MINUS:		prevValue -= displayValue;	break;
			case OP_MULTIPLY:	prevValue *= displayValue;	break;
			case OP_DIVIDE:		prevValue /= displayValue;	break;
			case OP_EQUALS:		prevValue = displayValue;	break;
		}
	}

	if(op == OP_SIN) {
		displayValue = prevValue = sin(prevValue);
		op == OP_EQUALS;
	}
	else if(op == OP_COS) {
		displayValue = prevValue = cos(prevValue);
		op == OP_EQUALS;
	} else if(op == OP_TAN) {
		displayValue = prevValue = tan(prevValue);
		op == OP_EQUALS;
	} else if(op == OP_PI) {
		displayValue = prevValue = M_PI;
		op == OP_EQUALS;
	} else if (op != OP_EQUALS) {
		operation = op;
		displayValue = prevValue;
	}

	prevOpWasEquals = (op == OP_EQUALS);
	SetValue(prevValue);		// Save the next operation
	prevKeyWasOp = true;
}


//
// DisplayView::Copy
//
// Copys the current display into the clipboard.
//
void DisplayView::Copy(void) {
	if (be_clipboard->Lock()) {
		char s[16]; s[15]=0;
		if (prevKeyWasOp) {
			snprintf(s, 15, "%g", prevValue);
		} else {
			strncpy(s, displayString, 15);
		}
		be_clipboard->Clear();
		BMessage *clipper = be_clipboard->Data();
		clipper->AddData("text/plain", B_MIME_TYPE, s, strlen(s)+1);
		be_clipboard->Commit();
		be_clipboard->Unlock();
	}
}


//
// DisplayView::Paste
//
// Pastes the clipboard into the display, if it's
// a valid number.
//
void DisplayView::Paste(void) {
	if (be_clipboard->Lock()) {
		char *s;
		ssize_t numBytes;
		
		BMessage *clipper = be_clipboard->Data();
		clipper->FindData("text/plain", B_MIME_TYPE, 0, (void const**)&s, &numBytes);
		
		if (s) {
			int i;
			i = 0;
			do {
				if (!isdigit(s[i]) && (s[i] != '.')) {
					KeyBeep();
					be_clipboard->Unlock();
					return;
				}
			} while (++i < strlen(s));
			SetString(s);
		}
		be_clipboard->Unlock();
	}
}
