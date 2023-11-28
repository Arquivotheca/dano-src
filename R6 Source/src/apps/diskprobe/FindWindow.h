//--------------------------------------------------------------------
//	
//	FindWindow.h
//
//	Written by: Robert Polic
//	
//	Copyright 1998 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef FIND_WINDOW_H
#define FIND_WINDOW_H

#include <Application.h>
#include <Button.h>
#include <CheckBox.h>
#include <Rect.h>
#include <TextView.h>
#include <View.h>
#include <Window.h>

#define FIND_WIDTH			280
#define FIND_HEIGHT			100

#define FIND_BUTTON_X1		(FIND_WIDTH - BUTTON_WIDTH - 11)
#define FIND_BUTTON_Y1		(FIND_HEIGHT - (BUTTON_HEIGHT + 15))
#define FIND_BUTTON_X2		(FIND_BUTTON_X1 + BUTTON_WIDTH)
#define FIND_BUTTON_Y2		(FIND_BUTTON_Y1 + BUTTON_HEIGHT)
#define FIND_BUTTON_TEXT	"Find"

#define CASE_BUTTON_X1		 10
#define CASE_BUTTON_Y1		(FIND_HEIGHT - 26)
#define CASE_BUTTON_X2		(CASE_BUTTON_X1 + 100)
#define CASE_BUTTON_Y2		(CASE_BUTTON_Y1 + 16)
#define CASE_TEXT			"Case-Sensitive"

#define M_FIND_IT			'Find'
#define M_CASE				'Case'

class	TFindView;


//====================================================================

class TFindWindow : public BWindow {

private:

TFindView		*fFindView;

public:

static TFindWindow	*fFindWindow;
static BRect	fLastPosition;

				TFindWindow(prefs*);
virtual			~TFindWindow(void);
void			FindAgain(void);
void			WindowGuess(BWindow*);
};


//====================================================================

class TFindView : public BView {

private:

BButton			*fFindButton;
BCheckBox		*fCaseSensitive;
BTextView		*fTextView;
BWindow			*fWindowGuess;
prefs			*fPrefs;

public:
				TFindView(BRect, prefs*);
virtual	void	AttachedToWindow(void);
virtual	void	MessageReceived(BMessage*);
virtual	void	Draw(BRect);
void			DoFind(void);
void			WindowGuess(BWindow*);
};
#endif
