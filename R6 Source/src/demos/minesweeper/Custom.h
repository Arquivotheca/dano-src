//--------------------------------------------------------------------
//	
//	Custom.h
//
//	Written by: Robert Polic
//	
//	Copyright 1998 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef CUSTOM_H
#define CUSTOM_H

#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <Point.h>
#include <Rect.h>
#include <TextControl.h>
#include <View.h>
#include <Window.h>

#define CUSTOM_WIND_WIDTH	200
#define CUSTOM_WIND_HEIGHT	125

#define CUSTOM_BUTTON_WIDTH	 70
#define CUSTOM_BUTTON_HEIGHT 20

#define CUSTOM_OK_X1		(CUSTOM_WIND_WIDTH - CUSTOM_BUTTON_WIDTH - 6)
#define CUSTOM_OK_Y1		(CUSTOM_WIND_HEIGHT - (CUSTOM_BUTTON_HEIGHT + 10))
#define CUSTOM_OK_X2		(CUSTOM_OK_X1 + CUSTOM_BUTTON_WIDTH)
#define CUSTOM_OK_Y2		(CUSTOM_OK_Y1 + CUSTOM_BUTTON_HEIGHT)
#define CUSTOM_OK_TEXT		"OK"

#define CUSTOM_CANCEL_X1	(CUSTOM_OK_X1 - (CUSTOM_BUTTON_WIDTH + 10))
#define CUSTOM_CANCEL_Y1	CUSTOM_OK_Y1
#define CUSTOM_CANCEL_X2	(CUSTOM_CANCEL_X1 + CUSTOM_BUTTON_WIDTH)
#define CUSTOM_CANCEL_Y2	CUSTOM_OK_Y2
#define CUSTOM_CANCEL_TEXT	"Cancel"

#define WIDTH_X1			 10
#define WIDTH_Y1			 10
#define WIDTH_X2			(CUSTOM_WIND_WIDTH - WIDTH_X1)
#define WIDTH_Y2			(WIDTH_Y1 + 16)
#define WIDTH_TEXT			"Width"

#define HEIGHT_X1			WIDTH_X1
#define HEIGHT_Y1			(WIDTH_Y2 + 10)
#define HEIGHT_X2			WIDTH_X2
#define HEIGHT_Y2			(HEIGHT_Y1 + 16)
#define HEIGHT_TEXT			"Height"

#define BOMBS_X1			WIDTH_X1
#define BOMBS_Y1			(HEIGHT_Y2 + 10)
#define BOMBS_X2			WIDTH_X2
#define BOMBS_Y2			(BOMBS_Y1 + 16)
#define BOMBS_TEXT			"Bombs"

enum	CUSTOM_MESSAGES		{M_OK = 1000, M_CANCEL, M_WIDTH, M_HEIGHT,
							 M_BOMBS};


//====================================================================

class TCustomWindow : public BWindow {

private:

int32			fB;
int32			fX;
int32			fY;
BButton			*fOK;
BTextControl	*fWidth;
BTextControl	*fHeight;
BTextControl	*fBombs;

public:

				TCustomWindow(BRect, int32, int32, int32);
virtual void	MessageReceived(BMessage*);
};
#endif
