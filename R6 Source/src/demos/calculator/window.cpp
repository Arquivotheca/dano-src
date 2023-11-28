//
// BeCalculator
// Copyright 1997, Be Incorporated
// by Eric Shepherd
//

#include <Window.h>
#include <ctype.h>

#include "window.h"
#include "view.h"
#include "displayview.h"
#include "application.h"
#include "main.h"

#if ALLOW_ZOOM
	#define WINDOW_FLAGS B_NOT_RESIZABLE
#else
	#define WINDOW_FLAGS B_NOT_RESIZABLE|B_NOT_ZOOMABLE
#endif


//
// CalcWindow::CalcWindow
//
// Construct the main calculator window.
//
CalcWindow::CalcWindow(BRect frame)
			: BWindow(frame, "Calculator", B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
						WINDOW_FLAGS) {
	float width, height;
	
	width = frame.right - frame.left;
	height = frame.bottom - frame.top;
	#if ALLOW_ZOOM
		SetZoomLimits(width+110, height);
	#endif
}


//
// CalcWindow::MessageReceived
//
// Handle incoming messages for the buttons.
//
void CalcWindow::MessageReceived(BMessage *message) {
	char c = (char) message->what;			// Convert to a character
	
	// Handle the message
	
	switch(message->what) {
		case OP_POW:
		case OP_SIN:
		case OP_COS:
		case OP_TAN:
		case OP_PLUS:
		case OP_MINUS:
		case OP_MULTIPLY:
		case OP_DIVIDE:
		case OP_EQUALS:
		case OP_PI:
			dispView->Operation(message->what);
			break;
		
		case 'X':
			dispView->Clear();
			break;
		
		default:
			if (isdigit(c) || (c=='.')) {
				dispView->AddDigit(c);
			}
			break;
	}
}


//
// CalcWindow::QuitRequested
//
// Called when the close box has been clicked.
//
bool CalcWindow::QuitRequested() {
	theApp->SavePrefs();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
