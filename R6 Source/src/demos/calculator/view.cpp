//
// BeCalculator
// Copyright 1997, Be Incorporated
// by Eric Shepherd
//

#include <View.h>

#include "window.h"
#include "view.h"
#include "button.h"
#include "application.h"
#include "main.h"

#define LEFT_MARGIN		5
#define TOP_MARGIN		29
#define HORIZ_SPACING	5
#define VERT_SPACING	5

#define BUTTON_WIDTH	25
#define BUTTON_HEIGHT	25

CalcButton *buttonClr, *buttonEq1, *buttonDiv, *buttonMul;
CalcButton *button7, *button8, *button9, *buttonMin;
CalcButton *button4, *button5, *button6, *buttonPlus;
CalcButton *button1, *button2, *button3, *buttonEq2;
CalcButton *button0, *buttonDec;


//
// CalcView::CalcView
//
// Construct the view.
//
CalcView::CalcView(BRect frame)
			: BView(frame, "BeCalculator View", B_FOLLOW_ALL_SIDES, 0) {
	BRect r;
	
	SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	
	// Attach the buttons to the view

	AddButton(0,0,"pow",OP_POW);
	AddButton(1,0,"sin",OP_SIN);
	AddButton(2,0,"cos",OP_COS);
	AddButton(3,0,"tan",OP_TAN);


	buttonClr = AddButton(0, 1, "CE", 'X');		// Clear
	buttonEq1 = AddButton(1, 1, "PI", OP_PI);
	buttonDiv = AddButton(2, 1, "/", OP_DIVIDE);
	buttonMul = AddButton(3, 1, "*", OP_MULTIPLY);

	button7 = AddButton(0, 2, "7", '7');			// 7
	button8 = AddButton(1, 2, "8", '8');			// 8
	button9 = AddButton(2, 2, "9", '9');			// 9
	buttonMin = AddButton(3, 2, "-", OP_MINUS);
	
	button4 = AddButton(0, 3, "4", '4');			// 4
	button5 = AddButton(1, 3, "5", '5');			// 5
	button6 = AddButton(2, 3, "6", '6');			// 6
	buttonPlus = AddButton(3, 3, "+", OP_PLUS);
	
	button1 = AddButton(0, 4, "1", '1');			// 1
	button2 = AddButton(1, 4, "2", '2');			// 2
	button3 = AddButton(2, 4, "3", '3');			// 3
	buttonEq2 = AddTallButton(3, 4, "=", OP_EQUALS);
	
	button0 = AddWideButton(0, 5, "0", '0');		// 0
	buttonDec = AddButton(2, 5, ".", '.');		// .
}


//
// CalcView::MessageReceived
//
// Handle incoming messages.
//
void CalcView::MessageReceived(BMessage *message) {
	
	// Handle the message
	
	switch(message->what) {
		case B_COPY:
			dispView->Copy();
			break;
		
		case B_PASTE:
			dispView->Paste();
			break;
		
		case B_CUT:
			dispView->Copy();
			break;
		
		default:
			BView::MessageReceived(message);
			break;
	}
}


//
// CalcView::KeyDown
//
// Handle keypresses by routing them to the appropriate
// buttons.
//
void CalcView::KeyDown(const char *bytes, int32 numBytes) {
	if (numBytes != 1) {
		KeyBeep();
	}
	else {
		switch(*bytes) {
			case '0':
				button0->Flash();
				button0->Invoke();
				break;
				
			case '1':
				button1->Flash();
				button1->Invoke();
				break;
				
			case '2':
				button2->Flash();
				button2->Invoke();
				break;
			
			case '3':
				button3->Flash();
				button3->Invoke();
				break;
			
			case '4':
				button4->Flash();
				button4->Invoke();
				break;
			
			case '5':
				button5->Flash();
				button5->Invoke();
				break;
			
			case '6':
				button6->Flash();
				button6->Invoke();
				break;
			
			case '7':
				button7->Flash();
				button7->Invoke();
				break;
			
			case '8':
				button8->Flash();
				button8->Invoke();
				break;
			
			case '9':
				button9->Flash();
				button9->Invoke();
				break;
			
			case '.':
				buttonDec->Flash();
				buttonDec->Invoke();
				break;
			
			case '/':
				buttonDiv->Flash();
				buttonDiv->Invoke();
				break;
			
			case '*':
				buttonMul->Flash();
				buttonMul->Invoke();
				break;
			
			case '-':
				buttonMin->Flash();
				buttonMin->Invoke();
				break;
			
			case '+':
				buttonPlus->Flash();
				buttonPlus->Invoke();
				break;
			
			case B_BACKSPACE:
			case B_DELETE:
			case 'c':
			case 'C':
				buttonClr->Flash();
				buttonClr->Invoke();
				break;
				
			case '=':
			case B_ENTER:
				buttonEq2->Flash();
				buttonEq2->Invoke();
				break;
				
			default:
				KeyBeep();
				break;
		}
	}
}


//
// CalcView::Draw
//
// Draw the contents of the view.
//
void CalcView::Draw(BRect updateRect) {
}


//
// CalcView::AddButton
//
// Add a button to the view.
//
CalcButton *CalcView::AddButton(int32 x, int32 y, char *name, int32 c) {
	BRect r;
	CalcButton *b;
	
	r.left = LEFT_MARGIN+(x*(HORIZ_SPACING+BUTTON_WIDTH));
	r.top = TOP_MARGIN+(y*(VERT_SPACING+BUTTON_HEIGHT));
	r.right = r.left + BUTTON_WIDTH;
	r.bottom = r.top + BUTTON_HEIGHT;
	AddChild(b=new CalcButton(r, name, new BMessage(c)));
	return b;
}


//
// CalcView::AddWideButton
//
// Add a a double-wide button to the view.
//
CalcButton *CalcView::AddWideButton(int32 x, int32 y, char *name, int32 c) {
	BRect r;
	CalcButton *b;
	
	r.left = LEFT_MARGIN+(x*(HORIZ_SPACING+BUTTON_WIDTH));
	r.top = TOP_MARGIN+(y*(VERT_SPACING+BUTTON_HEIGHT));
	r.right = r.left + BUTTON_WIDTH*2 + HORIZ_SPACING;
	r.bottom = r.top + BUTTON_HEIGHT;
	AddChild(b=new CalcButton(r, name, new BMessage(c)));
	return b;
}


//
// CalcView::AddTallButton
//
// Add a a double-tall button to the view.
//
CalcButton *CalcView::AddTallButton(int32 x, int32 y, char *name, int32 c) {
	BRect r;
	CalcButton *b;
	
	r.left = LEFT_MARGIN+(x*(HORIZ_SPACING+BUTTON_WIDTH));
	r.top = TOP_MARGIN+(y*(VERT_SPACING+BUTTON_HEIGHT));
	r.right = r.left + BUTTON_WIDTH;
	r.bottom = r.top + BUTTON_HEIGHT*2 + VERT_SPACING;
	AddChild(b=new CalcButton(r, name, new BMessage(c)));
	return b;
}
